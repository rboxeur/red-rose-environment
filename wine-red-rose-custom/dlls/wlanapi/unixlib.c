/*
 * wlanapi unixlib implementation
 *
 * Copyright 2024 Vibhav Pant
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

#include <stdlib.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winternl.h>
#include <wlanapi.h>

#include <wine/list.h>
#include <wine/unixlib.h>

#include "unixlib.h"
#include "unixlib_priv.h"

static BOOL initialized;

NTSTATUS wlan_init( void *params )
{
    initialized = load_dbus_functions();
    return STATUS_SUCCESS;
}

NTSTATUS wlan_open_handle( void *params )
{
    struct wlan_open_handle_params *args = params;
    NTSTATUS status;
    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    status = init_dbus_connection( &args->handle );
    return status;
}

NTSTATUS wlan_close_handle( void *params )
{
    struct wlan_close_handle_params *args = params;
    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    close_dbus_connection( (void *)args->handle );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_get_interfaces( void *params )
{
    struct wlan_get_interfaces_params *args = params;
    struct list *ifaces;
    NTSTATUS status;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    ifaces = malloc( sizeof( *ifaces ) );
    if (!ifaces) return STATUS_NO_MEMORY;

    list_init( ifaces );
    status = networkmanager_get_wifi_devices( (void *)args->handle, ifaces );
    if (status != STATUS_SUCCESS)
    {
        free( ifaces );
        return status;
    }

    args->interfaces = (UINT_PTR)ifaces;
    args->len = list_count( ifaces );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_copy_and_free_interfaces( void *params )
{
    struct wlan_copy_and_free_interfaces_params *args = params;
    struct wlan_interface *ifaces = (struct wlan_interface *)args->interfaces;
    struct wlan_interface *cur, *next;
    SIZE_T i = 0;

    LIST_FOR_EACH_ENTRY_SAFE(cur, next, &ifaces->entry, struct wlan_interface, entry)
    {
        args->info[i++] = cur->info;
        list_remove( &cur->entry );
        free( cur );
    }

    free( ifaces );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_free_interfaces( void *params )
{
    struct wlan_free_interfaces_params *args = params;
    struct wlan_interface *ifaces = (struct wlan_interface *)args->interfaces;
    struct wlan_interface *cur, *next;

    LIST_FOR_EACH_ENTRY_SAFE(cur, next, &ifaces->entry, struct wlan_interface, entry)
    {
        list_remove( &cur->entry );
        free( cur );
    }

    free( ifaces );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_network_list_get( void *params )
{
    NTSTATUS status;
    struct wlan_network_list_get_params *args = params;
    struct list *networks;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    networks = malloc( sizeof( *networks ));
    if (!networks) return STATUS_NO_MEMORY;

    list_init( networks );
    status = networkmanager_get_access_points( (void *)args->handle, args->interface,
                                               args->ssid_filter, args->security, networks );
    if (status != STATUS_SUCCESS)
    {
        free( networks );
        return status;
    }

    args->networks = (UINT_PTR)networks;
    args->len = list_count( networks );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_network_list_move_to_avail_network( void *params )
{
    struct wlan_network_list_move_to_avail_network_params *args = params;
    struct wlan_network *networks = (struct wlan_network *)args->networks;
    struct wlan_network *cur, *next;
    SIZE_T i = 0;

    LIST_FOR_EACH_ENTRY_SAFE( cur, next, &networks->entry, struct wlan_network, entry)
    {
        wlan_bss_info_to_WLAN_AVAILABLE_NETWORK( &cur->info, &args->dest[i++] );
        list_remove( &cur->entry );
        free( cur );
    }

    free( networks );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_network_list_move_to_bss_entry( void *params )
{
    struct wlan_network_list_move_to_bss_entry_params *args = params;
    struct wlan_network *networks = (struct wlan_network *)args->networks;
    struct wlan_network *cur, *next;
    SIZE_T i = 0;

    LIST_FOR_EACH_ENTRY_SAFE( cur, next, &networks->entry, struct wlan_network, entry)
    {
        wlan_bss_info_to_WLAN_BSS_ENTRY( &cur->info, &args->dest[i++] );
        list_remove( &cur->entry );
        free( cur );
    }

    free( networks );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_network_list_free( void *params )
{
    struct wlan_network_list_free_params *args = params;
    struct wlan_network *networks = (struct wlan_network *)args->networks;
    struct wlan_network *cur, *next;

    LIST_FOR_EACH_ENTRY_SAFE(cur, next, &networks->entry, struct wlan_network, entry)
    {
        list_remove( &cur->entry );
        free( cur );
    }

    free( networks );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_start_scan( void *params )
{
    struct wlan_start_scan *args = params;

    return networkmanager_start_scan( (void *) args->handle, args->interface, args->ssid );
}

NTSTATUS wlan_get_profile_list( void *params )
{
    NTSTATUS status;
    struct wlan_get_profile_list_params *args = params;
    struct list *profiles;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    profiles = malloc( sizeof( *profiles ));
    if (!profiles) return STATUS_NO_MEMORY;

    list_init( profiles );
    status = networkmanager_wifi_device_get_setting_ids( (void *)args->handle, args->interface,
                                                         profiles );
    if (status)
    {
        free( profiles );
        return status;
    }

    args->list = (UINT_PTR)profiles;
    args->len = list_count( profiles );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_profile_list_move_to_profile_info( void *params )
{
    struct wlan_profile_list_move_to_profile_info_params *args = params;
    struct list *profiles = (struct list *)args->profiles;
    struct wlan_profile *cur, *next;
    SIZE_T i = 0;

    LIST_FOR_EACH_ENTRY_SAFE( cur, next, profiles, struct wlan_profile, entry)
    {
        WCHAR *dst = args->dest[i].strProfileName;

        args->dest[i++].dwFlags = WLAN_PROFILE_USER;
        ntdll_umbstowcs( cur->name, strlen( cur->name ) + 1, dst,
                        ARRAY_SIZE( args->dest[0].strProfileName ) );
        list_remove( &cur->entry );
        free( cur );
    }

    free( profiles );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_profile_list_free( void *params )
{
    struct wlan_profile_list_free_params *args = params;
    struct list *profiles = (struct list *)args->profiles;
    struct wlan_profile *cur, *next;

    LIST_FOR_EACH_ENTRY_SAFE( cur, next, profiles, struct wlan_profile, entry)
    {
        list_remove( &cur->entry );
        free( cur );
    }

    free( profiles );
    return STATUS_SUCCESS;
}

NTSTATUS wlan_connect_with_profile_name( void *params )
{
    struct wlan_connect_with_profile_name_params *args = params;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    return networkmanager_connect_with_setting_id( (void *)args->handle, args->device,
                                                   args->profile_name );
}

NTSTATUS wlan_profile_set( void *params )
{
    struct wlan_profile_set_params *args = params;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    return networkmanager_set_connection_settings(
        (void *)args->handle, args->device, args->profile, args->override, &args->already_exists );
}

NTSTATUS wlan_disconnect( void *params )
{
    struct wlan_disconnect_params *args = params;

    if (!initialized)
        return STATUS_NOT_SUPPORTED;

    return networkmanager_device_disconnect( (void *)args->handle, args->device );
}

const unixlib_entry_t __wine_unix_call_funcs[] = {
    wlan_init,
    wlan_open_handle,
    wlan_close_handle,

    wlan_get_interfaces,
    wlan_copy_and_free_interfaces,
    wlan_free_interfaces,

    wlan_network_list_get,
    wlan_network_list_move_to_avail_network,
    wlan_network_list_move_to_bss_entry,
    wlan_network_list_free,

    wlan_start_scan,

    wlan_get_profile_list,
    wlan_profile_list_move_to_profile_info,
    wlan_profile_list_free,

    wlan_connect_with_profile_name,
    wlan_disconnect,

    wlan_profile_set
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_funcs ) == unix_funcs_count );
