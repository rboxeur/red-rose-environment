/*
 * WLAN settings profile
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

#ifndef __WINE_WLANAPI_PROFILE_H
#define __WINE_WLANAPI_PROFILE_H

struct wlan_profile_ssid_config
{
    DOT11_SSID ssid;
    DOT11_SSID ssid_prefix;
    BOOL non_broadcast_set;
    BOOL non_broadcast;
};

enum wlan_profile_mode
{
    WLANAPI_PROFILE_MODE_AUTO,
    WLANAPI_PROFILE_MODE_MANUAL,
};

struct wlan_profile_mac_randomization
{
    BOOL enable_randomization;
    BOOL randomize_everyday;
    DWORD randomization_seed;
};

enum wlan_key_type
{
    WLANAPI_KEY_TYPE_INVALID,
    WLANAPI_KEY_TYPE_NETWORK_KEY,
    WLANAPI_KEY_TYPE_PASSPHRASE
};

struct wlan_shared_key
{
    BOOL protected;
    enum wlan_key_type key_type;
    WCHAR key_material[65];
};

struct wlan_profile_security
{
    DOT11_AUTH_ALGORITHM authentication;
    DOT11_CIPHER_ALGORITHM encryption;
    BOOL use_onex;
    BOOL transition_mode;

    USHORT keys_len;
    struct wlan_shared_key shared_key[4];

    INT key_index;
    BOOL pmk_cache_mode;
    INT pmk_cache_ttl;
    INT pmk_cache_size;
    BOOL pre_auth_mode;
    INT pre_auth_throttle;
};

struct wlan_profile_msm
{
    BOOL security_enabled;
    struct wlan_profile_security security;
};

struct wlan_profile_data
{
    CHAR name[WLAN_MAX_NAME_LENGTH];
    DOT11_BSS_TYPE connection_type;
    enum wlan_profile_mode connection_mode;
    BOOL auto_switch;
    struct wlan_profile_ssid_config ssid_config;
    struct wlan_profile_msm msm;
    struct wlan_profile_mac_randomization mac_randomization;
};

extern DWORD wlan_profile_parse( const WCHAR *profile_xml_str, struct wlan_profile_data *profile,
                                 DWORD *wlan_reason );

#endif /* __WINE_WLANAPI_PROFILE_H */
