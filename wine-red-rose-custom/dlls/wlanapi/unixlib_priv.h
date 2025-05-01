/*
 * Unix private definitions
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

#ifndef __WINE_WLANAPI_UNIXLIB_PRIV_H
#define __WINE_WLANAPI_UNIXLIB_PRIV_H

struct wlan_bss_info
{
    UINT32 flags;
    UINT32 wpa_flags;
    UINT32 rsn_flags;

    USHORT ssid_len;
    BYTE ssid[32];

    UINT32 frequency;
    BYTE hw_address[6];
    UINT32 mode;
    UINT32 max_bitrate;
    UINT32 bandwidth;
    UINT8 strength;
    INT32 last_seen;

    BOOL connected;
};

struct wlan_interface
{
    struct list entry;
    struct unix_wlan_interface_info info;
};

struct wlan_network
{
    struct list entry;
    struct wlan_bss_info info;
};

struct wlan_profile
{
    struct list entry;
    CHAR name[WLAN_MAX_NAME_LENGTH];
};

extern BOOL load_dbus_functions( void );
extern NTSTATUS init_dbus_connection( UINT_PTR *handle );
extern void close_dbus_connection( void *c );
extern NTSTATUS networkmanager_get_wifi_devices( void *connection, struct list *devices );
extern NTSTATUS networkmanager_get_access_points( void *connection, const GUID *device,
                                                  const DOT11_SSID *ssid, BOOL security,
                                                  struct list *access_points );
extern void wlan_bss_info_to_WLAN_AVAILABLE_NETWORK( const struct wlan_bss_info *info,
                                                     WLAN_AVAILABLE_NETWORK *dest );
extern void wlan_bss_info_to_WLAN_BSS_ENTRY( const struct wlan_bss_info *info,
                                             WLAN_BSS_ENTRY *dest );
extern NTSTATUS networkmanager_start_scan( void *connection, const GUID *interface,
                                           const DOT11_SSID *ssid );
extern NTSTATUS networkmanager_wifi_device_get_setting_ids( void *connection, const GUID *device,
                                                            struct list *ids );
extern NTSTATUS networkmanager_connect_with_setting_id( void *connection, const GUID *device,
                                                        const char *id );
extern NTSTATUS networkmanager_set_connection_settings( void *connection, const GUID *device,
                                                        const struct wlan_profile_data *profile,
                                                        BOOL override, BOOL *already_exists );
extern NTSTATUS networkmanager_device_disconnect( void *connection, const GUID *device );
#endif /* __WINE_WLANAPI_UNIXLIB_PRIV_H */
