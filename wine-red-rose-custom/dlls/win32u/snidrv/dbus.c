/*
 * DBus tray support
 *
 * Copyright 2023 Sergei Chernyadyev
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#ifdef SONAME_LIBDBUS_1
#include "snidrv.h"

#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <dlfcn.h>
#include <poll.h>

#include <dbus/dbus.h>

#include "wine/list.h"
#include "wine/unixlib.h"
#include "wine/gdi_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winesni);

#define DBUS_FUNCS               \
    DO_FUNC(dbus_bus_add_match); \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_bus_get_private); \
    DO_FUNC(dbus_bus_add_match); \
    DO_FUNC(dbus_bus_remove_match); \
    DO_FUNC(dbus_bus_get_unique_name);   \
    DO_FUNC(dbus_connection_add_filter); \
    DO_FUNC(dbus_connection_read_write); \
    DO_FUNC(dbus_connection_dispatch); \
    DO_FUNC(dbus_connection_get_dispatch_status); \
    DO_FUNC(dbus_connection_read_write_dispatch); \
    DO_FUNC(dbus_connection_remove_filter); \
    DO_FUNC(dbus_connection_send); \
    DO_FUNC(dbus_connection_send_with_reply); \
    DO_FUNC(dbus_connection_send_with_reply_and_block); \
    DO_FUNC(dbus_connection_flush); \
    DO_FUNC(dbus_connection_try_register_object_path);  \
    DO_FUNC(dbus_connection_unregister_object_path);  \
    DO_FUNC(dbus_connection_list_registered);  \
    DO_FUNC(dbus_connection_close);  \
    DO_FUNC(dbus_connection_ref);   \
    DO_FUNC(dbus_connection_unref);   \
    DO_FUNC(dbus_connection_get_object_path_data);  \
    DO_FUNC(dbus_connection_set_watch_functions);   \
    DO_FUNC(dbus_watch_get_unix_fd); \
    DO_FUNC(dbus_watch_handle); \
    DO_FUNC(dbus_watch_get_flags); \
    DO_FUNC(dbus_watch_get_enabled); \
    DO_FUNC(dbus_error_free); \
    DO_FUNC(dbus_error_init); \
    DO_FUNC(dbus_error_is_set); \
    DO_FUNC(dbus_set_error_from_message); \
    DO_FUNC(dbus_free_string_array); \
    DO_FUNC(dbus_message_get_args); \
    DO_FUNC(dbus_message_get_interface); \
    DO_FUNC(dbus_message_get_member); \
    DO_FUNC(dbus_message_get_path); \
    DO_FUNC(dbus_message_get_type); \
    DO_FUNC(dbus_message_is_signal); \
    DO_FUNC(dbus_message_iter_append_basic); \
    DO_FUNC(dbus_message_iter_get_arg_type); \
    DO_FUNC(dbus_message_iter_get_basic); \
    DO_FUNC(dbus_message_iter_append_fixed_array); \
    DO_FUNC(dbus_message_iter_get_fixed_array); \
    DO_FUNC(dbus_message_iter_init); \
    DO_FUNC(dbus_message_iter_init_append); \
    DO_FUNC(dbus_message_iter_next); \
    DO_FUNC(dbus_message_iter_recurse); \
    DO_FUNC(dbus_message_iter_open_container);  \
    DO_FUNC(dbus_message_iter_close_container);  \
    DO_FUNC(dbus_message_iter_abandon_container_if_open);  \
    DO_FUNC(dbus_message_new_method_return);  \
    DO_FUNC(dbus_message_new_method_call); \
    DO_FUNC(dbus_message_new_signal); \
    DO_FUNC(dbus_message_is_method_call);  \
    DO_FUNC(dbus_message_new_error);  \
    DO_FUNC(dbus_pending_call_block); \
    DO_FUNC(dbus_pending_call_unref); \
    DO_FUNC(dbus_pending_call_steal_reply); \
    DO_FUNC(dbus_threads_init_default); \
    DO_FUNC(dbus_message_unref)

#define DO_FUNC(f) static typeof(f) * p_##f
DBUS_FUNCS;
#undef DO_FUNC

static pthread_once_t init_control = PTHREAD_ONCE_INIT;

struct standalone_notification {
    struct list entry;

    HWND owner;
    UINT id;
    unsigned int notification_id;
};

static struct list standalone_notification_list = LIST_INIT( standalone_notification_list );

static pthread_mutex_t standalone_notifications_mutex = PTHREAD_MUTEX_INITIALIZER;

#define BALLOON_SHOW_MIN_TIMEOUT 10000
#define BALLOON_SHOW_MAX_TIMEOUT 30000


static const char* notifications_interface_name = "org.freedesktop.Notifications";

static void* dbus_module = NULL;

static DBusConnection *global_connection;
static DBusWatch *global_connection_watch;
static int global_connection_watch_fd;
static UINT global_connection_watch_flags;
static BOOL sni_initialized = FALSE;
static BOOL notifications_initialized = FALSE;

static char* notifications_dst_path = NULL;

static BOOL load_dbus_functions(void)
{
    if (!(dbus_module = dlopen( SONAME_LIBDBUS_1, RTLD_NOW )))
        goto failed;

#define DO_FUNC(f) if (!(p_##f = dlsym( dbus_module, #f ))) goto failed
    DBUS_FUNCS;
#undef DO_FUNC
    return TRUE;

failed:
    WARN( "failed to load DBUS support: %s\n", dlerror() );
    return FALSE;
}

static void notifications_finalize(void)
{
    free(notifications_dst_path);
}

static void dbus_finalize(void)
{
    if (global_connection != NULL)
    {
        p_dbus_connection_flush(global_connection);
        p_dbus_connection_close(global_connection);
        p_dbus_connection_unref(global_connection);
    }
    if (dbus_module != NULL)
    {
        dlclose(dbus_module);
    }
}

static dbus_bool_t add_watch(DBusWatch *w, void *data);
static void remove_watch(DBusWatch *w, void *data);
static void toggle_watch(DBusWatch *w, void *data);

static BOOL notifications_initialize(void);

static BOOL dbus_initialize(void)
{
    DBusError error;
    p_dbus_error_init( &error );
    if (!p_dbus_threads_init_default()) return FALSE;
    if (!(global_connection = p_dbus_bus_get_private( DBUS_BUS_SESSION, &error )))
    {
        WARN("failed to get system dbus connection: %s\n", error.message );
        p_dbus_error_free( &error );
        return FALSE;
    }

    if (!p_dbus_connection_set_watch_functions(global_connection, add_watch, remove_watch,
                                               toggle_watch, NULL, NULL))
    {
        WARN("dbus_set_watch_functions() failed\n");
        return FALSE;
    }
    return TRUE;
}

static void snidrv_once_initialize(void)
{
    if (!load_dbus_functions()) goto err;
    if (!dbus_initialize()) goto err;

    if (notifications_initialize())
        notifications_initialized = TRUE;

    sni_initialized = TRUE;
err:
    if (!notifications_initialized)
        notifications_finalize();
    if (!sni_initialized && !notifications_initialized)
        dbus_finalize();
}

BOOL snidrv_notification_init(void)
{
    pthread_once(&init_control, snidrv_once_initialize);
    return notifications_initialized;
}

static dbus_bool_t add_watch(DBusWatch *w, void *data)
{
    int fd;
    unsigned int flags, poll_flags;
    if (!p_dbus_watch_get_enabled(w))
        return TRUE;

    fd = p_dbus_watch_get_unix_fd(w);
    flags = p_dbus_watch_get_flags(w);
    poll_flags = 0;

    if (flags & DBUS_WATCH_READABLE)
        poll_flags |= POLLIN;
    if (flags & DBUS_WATCH_WRITABLE)
        poll_flags |= POLLOUT;

    /* global connection */
    global_connection_watch_fd = fd;
    global_connection_watch_flags = poll_flags;
    global_connection_watch = w;

    return TRUE;
}

static void remove_watch(DBusWatch *w, void *data)
{
    /* global connection */
    global_connection_watch_fd = 0;
    global_connection_watch_flags = 0;
    global_connection_watch = NULL;
}


static void toggle_watch(DBusWatch *w, void *data)
{
    if (p_dbus_watch_get_enabled(w))
        add_watch(w, data);
    else
        remove_watch(w, data);
}

static const char* dbus_name_owning_match = "type='signal',"
    "interface='org.freedesktop.DBus',"
    "sender='org.freedesktop.DBus',"
    "member='NameOwnerChanged'";

static const char* dbus_notification_close_signal = "type='signal',"
    "interface='org.freedesktop.Notifications',"
    "member='NotificationClosed'";


static DBusHandlerResult name_owner_filter( DBusConnection *ctx, DBusMessage *msg, void *user_data )
{
    char *interface_name, *old_path, *new_path;
    DBusError error;

    p_dbus_error_init( &error );

    if (p_dbus_message_is_signal( msg, "org.freedesktop.DBus", "NameOwnerChanged" ) &&
        p_dbus_message_get_args( msg, &error, DBUS_TYPE_STRING, &interface_name, DBUS_TYPE_STRING, &old_path,
                                 DBUS_TYPE_STRING, &new_path, DBUS_TYPE_INVALID ))
    {
        if (strcmp(interface_name, notifications_interface_name) == 0)
        {
            struct standalone_notification *this, *next;
            pthread_mutex_lock(&standalone_notifications_mutex);
            old_path = notifications_dst_path;
            notifications_dst_path = strdup(new_path);
            free(old_path);

            LIST_FOR_EACH_ENTRY_SAFE( this, next, &standalone_notification_list, struct standalone_notification, entry )
            {
                list_remove(&this->entry);
                free(this);
            }
            pthread_mutex_unlock(&standalone_notifications_mutex);
        }
    }
    else if (p_dbus_message_is_signal( msg, notifications_interface_name, "NotificationClosed" ))
    {
        unsigned int id, reason;
        struct standalone_notification *this, *next;
        if (!p_dbus_message_get_args( msg, &error, DBUS_TYPE_UINT32, &id, DBUS_TYPE_UINT32, &reason ))
            goto cleanup;
        pthread_mutex_lock(&standalone_notifications_mutex);
        /* TODO: clear the list */
        LIST_FOR_EACH_ENTRY_SAFE( this, next, &standalone_notification_list, struct standalone_notification, entry )
        {
            if (this->notification_id == id)
            {
                list_remove(&this->entry);
                free(this);
            }
        }
        pthread_mutex_unlock(&standalone_notifications_mutex);
    }

cleanup:
    p_dbus_error_free( &error );
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static BOOL get_owner_for_interface(DBusConnection* connection, const char* interface_name, char** owner_path)
{
    DBusMessage* msg = NULL;
    DBusMessageIter args;
    DBusPendingCall* pending;
    DBusError error;
    char* status_notifier_dest = NULL;
    p_dbus_error_init( &error );
    msg = p_dbus_message_new_method_call("org.freedesktop.DBus",
                                         "/org/freedesktop/DBus",
                                         "org.freedesktop.DBus",
                                         "GetNameOwner");
    if (!msg) goto err;

    p_dbus_message_iter_init_append(msg, &args);
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface_name )) goto err;
    if (!p_dbus_connection_send_with_reply (connection, msg, &pending, -1)) goto err;
    if (!pending) goto err;

    p_dbus_message_unref(msg);

    p_dbus_pending_call_block(pending);

    msg = p_dbus_pending_call_steal_reply(pending);
    p_dbus_pending_call_unref(pending);
    if (!msg) goto err;

    if (p_dbus_set_error_from_message (&error, msg))
    {
        WARN("failed to query an owner - %s: %s\n", error.name, error.message);
        p_dbus_error_free( &error);
        goto err;
    }
    else if (!p_dbus_message_get_args( msg, &error, DBUS_TYPE_STRING, &status_notifier_dest,
                                         DBUS_TYPE_INVALID ))
    {
        WARN("failed to get a response - %s: %s\n", error.name, error.message);
        p_dbus_error_free( &error );
        goto err;
    }
    *owner_path = strdup(status_notifier_dest);
    p_dbus_message_unref(msg);
    return TRUE;
err:
    p_dbus_message_unref(msg);
    return FALSE;
}

static BOOL notifications_initialize(void)
{
    DBusError error;
    p_dbus_error_init( &error );

    if (!get_owner_for_interface(global_connection, "org.freedesktop.Notifications", &notifications_dst_path))
    {
        goto err;
    }

    p_dbus_connection_add_filter( global_connection, name_owner_filter, NULL, NULL );
    p_dbus_bus_add_match( global_connection, dbus_name_owning_match, &error );
    p_dbus_bus_add_match( global_connection, dbus_notification_close_signal, &error );

    if (p_dbus_error_is_set(&error))
    {
        WARN("failed to register matcher %s: %s\n", error.name, error.message);
        p_dbus_error_free( &error);
        goto err;
    }

    return TRUE;
err:
    return FALSE;
}

static BOOL handle_notification_icon(DBusMessageIter *iter, const unsigned char* icon_bits, unsigned width, unsigned height)
{
    DBusMessageIter sIter,bIter;
    unsigned row_stride = width * 4;
    const unsigned channel_count = 4;
    const unsigned bits_per_sample = 8;
    const bool has_alpha = true;
    if (!p_dbus_message_iter_open_container(iter, 'r', NULL, &sIter))
    {
        WARN("Failed to open struct inside array!\n");
        goto fail;
    }

    p_dbus_message_iter_append_basic(&sIter, 'i', &width);
    p_dbus_message_iter_append_basic(&sIter, 'i', &height);
    p_dbus_message_iter_append_basic(&sIter, 'i', &row_stride);
    p_dbus_message_iter_append_basic(&sIter, 'b', &has_alpha);
    p_dbus_message_iter_append_basic(&sIter, 'i', &bits_per_sample);
    p_dbus_message_iter_append_basic(&sIter, 'i', &channel_count);

    if (p_dbus_message_iter_open_container(&sIter, 'a', DBUS_TYPE_BYTE_AS_STRING, &bIter))
    {
        p_dbus_message_iter_append_fixed_array(&bIter, DBUS_TYPE_BYTE, &icon_bits, width * height * 4);
        p_dbus_message_iter_close_container(&sIter, &bIter);
    }
    else
    {
        p_dbus_message_iter_abandon_container_if_open(iter, &sIter);
        goto fail;
    }
    p_dbus_message_iter_close_container(iter, &sIter);
    return TRUE;
fail:
    return FALSE;
}

static BOOL close_notification(DBusConnection* connection, UINT id)
{
    BOOL ret = FALSE;

    DBusMessage* msg = NULL;
    DBusMessageIter args;
    DBusPendingCall* pending;
    DBusError error;

    p_dbus_error_init( &error );
    msg = p_dbus_message_new_method_call(notifications_dst_path,
                                         "/org/freedesktop/Notifications",
                                         notifications_interface_name,
                                         "CloseNotification");
    if (!msg) goto err;
    p_dbus_message_iter_init_append(msg, &args);
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id )) goto err;

    if (!p_dbus_connection_send_with_reply (connection, msg, &pending, -1))
        goto err;

    if (!pending) goto err;

    p_dbus_message_unref(msg);

    p_dbus_pending_call_block(pending);

    msg = p_dbus_pending_call_steal_reply(pending);
    p_dbus_pending_call_unref(pending);
    if (!msg) goto err;
    if (p_dbus_set_error_from_message (&error, msg))
    {
        WARN("got an error - %s: %s\n", error.name, error.message);
        p_dbus_error_free( &error);
    }
    ret = TRUE;
err:
    p_dbus_message_unref(msg);

    return ret;
}

static BOOL send_notification(DBusConnection* connection, UINT id, const WCHAR* title, const WCHAR* text, HICON icon, UINT info_flags, UINT timeout, unsigned int *p_new_id)
{
    char info_text[256 * 3];
    char info_title[128 * 3];
    const char *info_text_ptr = info_text, *info_title_ptr = info_title;
    const char* empty_string = "";
    const char* icon_name = "";
    BOOL ret = FALSE;
    DBusMessage* msg = NULL;
    DBusMessageIter args, aIter, eIter, vIter;
    DBusPendingCall* pending;
    DBusError error;
    /* icon */
    void* icon_bits = NULL;
    unsigned width, height;
    HICON new_icon = NULL;
    int expire_timeout;
    /* no text for balloon, so no balloon  */
    if (!text || !text[0])
        return TRUE;

    info_title[0] = 0;
    info_text[0] = 0;
    if (title) ntdll_wcstoumbs(title, wcslen(title) + 1, info_title, ARRAY_SIZE(info_title), FALSE);
    if (text) ntdll_wcstoumbs(text, wcslen(text) + 1, info_text, ARRAY_SIZE(info_text), FALSE);
    /*icon*/
    if ((info_flags & NIIF_ICONMASK) == NIIF_USER && icon)
    {
        unsigned int *u_icon_bits;
        new_icon = CopyImage(icon, IMAGE_ICON, 0, 0, 0);
        if (!create_bitmap_from_icon(new_icon, &width, &height, &icon_bits))
        {
            WARN("failed to copy icon %p\n", new_icon);
            goto err;
        }
        u_icon_bits = icon_bits;
        /* convert to RGBA, turns out that unlike tray icons it needs RGBA */
        for (unsigned i = 0; i < width * height; i++)
        {
#ifdef WORDS_BIGENDIAN
            u_icon_bits[i] = (u_icon_bits[i] << 8) | (u_icon_bits[i] >> 24);
#else
            u_icon_bits[i] = (u_icon_bits[i] << 24) | (u_icon_bits[i] >> 8);
#endif
        }
    }
    else
    {
        /* show placeholder icons */
        switch (info_flags & NIIF_ICONMASK)
        {
        case NIIF_INFO:
            icon_name = "dialog-information";
            break;
        case NIIF_WARNING:
            icon_name = "dialog-warning";
            break;
        case NIIF_ERROR:
            icon_name = "dialog-error";
            break;
        default:
            break;
        }
    }
    p_dbus_error_init( &error );
    msg = p_dbus_message_new_method_call(notifications_dst_path,
                                         "/org/freedesktop/Notifications",
                                         notifications_interface_name,
                                         "Notify");
    if (!msg) goto err;

    p_dbus_message_iter_init_append(msg, &args);
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &empty_string ))
        goto err;
    /* override id */
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id ))
        goto err;
    /* icon name */
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &icon_name ))
        goto err;
    /* title */
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &info_title_ptr ))
        goto err;
    /* body */
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &info_text_ptr ))
        goto err;
    /* actions */
    /* TODO: add default action */
    if (p_dbus_message_iter_open_container(&args, 'a', DBUS_TYPE_STRING_AS_STRING, &aIter))
        p_dbus_message_iter_close_container(&args, &aIter);
    else
        goto err;

    /* hints */
    if (p_dbus_message_iter_open_container(&args, 'a', "{sv}", &aIter))
    {
        if ((info_flags & NIIF_ICONMASK) == NIIF_USER && icon)
        {
            const char* icon_data_field = "image-data";
            if (!p_dbus_message_iter_open_container(&aIter, 'e', NULL, &eIter))
            {
                p_dbus_message_iter_abandon_container_if_open(&args, &aIter);
                goto err;
            }

            p_dbus_message_iter_append_basic(&eIter, 's', &icon_data_field);

            if (!p_dbus_message_iter_open_container(&eIter, 'v', "(iiibiiay)", &vIter))
            {
                p_dbus_message_iter_abandon_container_if_open(&aIter, &eIter);
                p_dbus_message_iter_abandon_container_if_open(&args, &aIter);
                goto err;
            }

            if (!handle_notification_icon(&vIter, icon_bits, width, height))
            {
                p_dbus_message_iter_abandon_container_if_open(&eIter, &vIter);
                p_dbus_message_iter_abandon_container_if_open(&aIter, &eIter);
                p_dbus_message_iter_abandon_container_if_open(&args, &aIter);
                goto err;
            }

            p_dbus_message_iter_close_container(&eIter, &vIter);
            p_dbus_message_iter_close_container(&aIter, &eIter);
        }
        p_dbus_message_iter_close_container(&args, &aIter);
    }
    else
        goto err;
    if (timeout == 0)
        /* just set it to system default */
        expire_timeout = -1;
    else
        expire_timeout = max(min(timeout, BALLOON_SHOW_MAX_TIMEOUT), BALLOON_SHOW_MIN_TIMEOUT);

    /* timeout */
    if (!p_dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &expire_timeout ))
        goto err;
    if (!p_dbus_connection_send_with_reply (connection, msg, &pending, -1))
        goto err;
    if (!pending) goto err;

    p_dbus_message_unref(msg);

    p_dbus_pending_call_block(pending);

    msg = p_dbus_pending_call_steal_reply(pending);
    p_dbus_pending_call_unref(pending);
    if (!msg) goto err;

    if (p_dbus_set_error_from_message (&error, msg))
    {
        WARN("failed to create a notification - %s: %s\n", error.name, error.message);
        p_dbus_error_free( &error);
        goto err;
    }
    if (!p_dbus_message_iter_init(msg, &args))
        goto err;

    if (DBUS_TYPE_UINT32 != p_dbus_message_iter_get_arg_type(&args))
        goto err;
    else if (p_new_id)
        p_dbus_message_iter_get_basic(&args, p_new_id);
    ret = TRUE;
err:
    p_dbus_message_unref(msg);
    if (new_icon) NtUserDestroyCursor(new_icon, 0);
    free(icon_bits);
    return ret;
}

BOOL snidrv_run_loop()
{
    while (true)
    {
        int poll_ret;
        struct pollfd fd_info;
        DBusConnection* conn;
        /* TODO: add condvar */
        if (!global_connection_watch_fd) continue;

        conn = p_dbus_connection_ref(global_connection);
        fd_info = (struct pollfd) {
            .fd = global_connection_watch_fd,
            .events = global_connection_watch_flags,
            .revents = 0,
        };

        poll_ret = poll(&fd_info, 1, 100);
        if (poll_ret == 0)
            goto cleanup;
        if (poll_ret == -1)
        {
            ERR("fd poll error\n");
            goto cleanup;
        }

        if (fd_info.revents & (POLLERR | POLLHUP | POLLNVAL)) continue;
        if (fd_info.revents & POLLIN)
        {
            p_dbus_watch_handle(global_connection_watch, DBUS_WATCH_READABLE);
            while ( p_dbus_connection_get_dispatch_status ( conn ) == DBUS_DISPATCH_DATA_REMAINS )
            {
                p_dbus_connection_dispatch ( conn ) ;
            }
        }

        if (fd_info.revents & POLLOUT)
            p_dbus_watch_handle(global_connection_watch, DBUS_WATCH_WRITABLE);
    cleanup:
        p_dbus_connection_unref(conn);
    }

    return 0;
}


BOOL snidrv_show_balloon( HWND owner, UINT id, BOOL hidden, const struct systray_balloon* balloon )
{
    BOOL ret = TRUE;
    struct standalone_notification *found_notification = NULL, *this;

    if (!notifications_dst_path || !notifications_dst_path[0])
        return -1;
    pthread_mutex_lock(&standalone_notifications_mutex);

    LIST_FOR_EACH_ENTRY(this, &standalone_notification_list, struct standalone_notification, entry)
    {
        if (this->owner == owner && this->id == id)
        {
            found_notification = this;
            break;
        }
    }
    /* close existing notification anyway */
    if (!hidden)
    {
        if (!found_notification)
        {
            found_notification = malloc(sizeof(struct standalone_notification));
            if (!found_notification)
            {
                ret = FALSE;
                goto cleanup;
            }
            found_notification->owner = owner;
            found_notification->id = id;
            found_notification->notification_id = 0;
            list_add_tail(&standalone_notification_list, &found_notification->entry);
        }
        else
            TRACE("found existing notification %p %d\n", owner, id);

        ret = send_notification(global_connection,
                                found_notification->notification_id,
                                balloon->info_title,
                                balloon->info_text,
                                balloon->info_icon,
                                balloon->info_flags,
                                balloon->info_timeout,
                                &found_notification->notification_id);
    }
    else if (found_notification)
    {
        ret = close_notification(global_connection, found_notification->notification_id);
    }
cleanup:
    pthread_mutex_unlock(&standalone_notifications_mutex);
    return ret;
}
#endif
