/*
 * Definitions for registering device notifications
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

#ifndef __WINE_WINE_DBT_H_
#define __WINE_WINE_DBT_H_

#include <wine/debug.h>

struct device_notification_details
{
    DWORD (CALLBACK *cb)(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header);
    HANDLE handle;
    DWORD devicetype;
    union
    {
        struct
        {
            /* Used to implement DEVICE_NOTIFY_ALL_INTERFACE_CLASSES. If true, the class field below
             * should be ignored. */
            BOOL all_classes;
            GUID class;
        } deviceinterface;
        struct
        {
            /* The path for the device file the notification is originating from. */
            OBJECT_NAME_INFORMATION *name_info;
            /* HANDLE to the device the event originates from. Is passed to the callback as-is, even
             * if the user has closed this handle. */
            HANDLE device;
        } device;
    } filter;
};

extern HDEVNOTIFY WINAPI I_ScRegisterDeviceNotification( struct device_notification_details *details,
        void *filter, DWORD flags );
extern BOOL WINAPI I_ScUnregisterDeviceNotification( HDEVNOTIFY handle );

#endif /* __WINE_WINE_DBT_H_ */
