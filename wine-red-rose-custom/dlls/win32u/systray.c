/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#ifdef SONAME_LIBDBUS_1
#include "snidrv/snidrv.h"
#endif
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

#ifdef SONAME_LIBDBUS_1
static volatile LONG dbus_notifications_initialized = (LONG)FALSE;
#endif

LRESULT system_tray_call( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, void *data )
{
#ifdef SONAME_LIBDBUS_1
    LONG l_dbus_notifications_initialized = InterlockedCompareExchange(&dbus_notifications_initialized, (LONG)FALSE, (LONG)FALSE);
    if (!l_dbus_notifications_initialized && snidrv_notification_init())
    {
        InterlockedCompareExchange(&dbus_notifications_initialized, TRUE, FALSE);
        l_dbus_notifications_initialized = TRUE;
    }
    if (l_dbus_notifications_initialized)
    {
        if (msg == WINE_SYSTRAY_RUN_LOOP)
            return snidrv_run_loop();
        if (msg == WINE_SYSTRAY_SHOW_BALLOON)
            return snidrv_show_balloon(hwnd, wparam, lparam, data);
    }

#endif

    switch (msg)
    {
    case WINE_SYSTRAY_NOTIFY_ICON:
        return user_driver->pNotifyIcon( hwnd, wparam, data );
    case WINE_SYSTRAY_CLEANUP_ICONS:
        user_driver->pCleanupIcons( hwnd );
        return 0;

    case WINE_SYSTRAY_DOCK_INIT:
        user_driver->pSystrayDockInit( hwnd );
        return 0;
    case WINE_SYSTRAY_DOCK_INSERT:
        return user_driver->pSystrayDockInsert( hwnd, wparam, lparam, data );
    case WINE_SYSTRAY_DOCK_CLEAR:
        user_driver->pSystrayDockClear( hwnd );
        return 0;
    case WINE_SYSTRAY_DOCK_REMOVE:
        return user_driver->pSystrayDockRemove( hwnd );

    case WINE_SYSTRAY_RUN_LOOP:
        return -1;

    case WINE_SYSTRAY_SHOW_BALLOON:
        return user_driver->pSystrayShowBalloon( hwnd, wparam, lparam, data );

    default:
        FIXME( "Unknown NtUserSystemTrayCall msg %#x\n", msg );
        break;
    }

    return -1;
}
