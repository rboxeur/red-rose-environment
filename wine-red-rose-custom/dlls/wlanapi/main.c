/*
 * Copyright 2010 Ričardas Barkauskas
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

/* How does this DLL work?
 * This DLL is used to probe and configure wireless access points using the
 * available wireless interfaces. Most functions are tied to a handle that is
 * first obtained by calling WlanOpenHandle. Usually it is followed by a call
 * to WlanEnumInterfaces and then for each interface a WlanScan call is made.
 * WlanScan starts a parallel access point discovery that delivers the ready
 * response through the callback registered by WlanRegisterNotification. After
 * that the program calls WlanGetAvailableNetworkList or WlanGetNetworkBssList.
 */

#include <stdarg.h>
#include <stdlib.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "wlanapi.h"

#include "unixlib.h"
#include "profile.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlanapi);

#define WLAN_MAGIC 0x574c414e /* WLAN */

static struct wine_wlan
{
    DWORD magic, cli_version;
    UINT_PTR unix_handle;
} handle_table[16];

static struct wine_wlan* handle_index(HANDLE handle)
{
    ULONG_PTR i = (ULONG_PTR)handle - 1;

    if (i < ARRAY_SIZE(handle_table) && handle_table[i].magic == WLAN_MAGIC)
        return &handle_table[i];

    return NULL;
}

static HANDLE handle_new(struct wine_wlan **entry)
{
    ULONG_PTR i;

    for (i = 0; i < ARRAY_SIZE(handle_table); i++)
    {
        if (handle_table[i].magic == 0)
        {
            *entry = &handle_table[i];
            return (HANDLE)(i + 1);
        }
    }

    return NULL;
}

DWORD WINAPI WlanEnumInterfaces(HANDLE handle, void *reserved, WLAN_INTERFACE_INFO_LIST **interface_list)
{
    struct wine_wlan *wlan;
    struct wlan_get_interfaces_params args = {0};
    SIZE_T count = 0, list_size;
    WLAN_INTERFACE_INFO_LIST *ret_list;
    NTSTATUS status;

    TRACE( "(%p, %p, %p)\n", handle, reserved, interface_list );

    if (!handle || reserved || !interface_list)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index(handle);
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    args.handle = wlan->unix_handle;
    status = UNIX_WLAN_CALL( wlan_get_interfaces, &args );
    if (status != STATUS_SUCCESS)
    {
        ERR( "Could not get list of interfaces from host: %lx.\n", status );
        return RtlNtStatusToDosError( status );
    }
    else
        count = args.len;

    list_size = offsetof(WLAN_INTERFACE_INFO_LIST, InterfaceInfo[count ? count : 1]);
    ret_list = WlanAllocateMemory(list_size);
    if (!ret_list)
    {
        if (args.interfaces)
        {
            struct wlan_free_interfaces_params free_args = {0};

            free_args.interfaces = args.interfaces;
            UNIX_WLAN_CALL( wlan_free_interfaces, &free_args );
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset( ret_list, 0, list_size );
    if (args.interfaces)
    {
        struct wlan_copy_and_free_interfaces_params copy_args = {0};
        struct unix_wlan_interface_info *unix_ifaces = NULL;
        SIZE_T i;

        unix_ifaces = malloc( sizeof( *unix_ifaces ) * count );
        if (!unix_ifaces)
        {
            struct wlan_free_interfaces_params free_args = {0};

            free_args.interfaces = args.interfaces;
            UNIX_WLAN_CALL( wlan_free_interfaces, &free_args );
            WlanFreeMemory( ret_list );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        copy_args.info = unix_ifaces;
        copy_args.interfaces = args.interfaces;
        UNIX_WLAN_CALL( wlan_copy_and_free_interfaces, &copy_args );

        for (i = 0; i < count; i++)
        {
            const size_t desc_max =
                sizeof( ret_list->InterfaceInfo[i].strInterfaceDescription ) / sizeof( WCHAR );

            ret_list->InterfaceInfo[i].InterfaceGuid = unix_ifaces[i].guid;
            ret_list->InterfaceInfo[i].isState = unix_ifaces[i].state;

            mbstowcs( ret_list->InterfaceInfo[i].strInterfaceDescription,
                     unix_ifaces[i].description, desc_max );
            ret_list->InterfaceInfo[i].strInterfaceDescription[desc_max - 1] = '\0';
        }
        free( unix_ifaces );
    }
    ret_list->dwNumberOfItems = args.interfaces ? count : 0;
    ret_list->dwIndex = 0; /* unused in this function */
    *interface_list = ret_list;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanCloseHandle(HANDLE handle, void *reserved)
{
    struct wine_wlan *wlan;
    struct wlan_close_handle_params params = {0};
    NTSTATUS status;

    TRACE("(%p, %p)\n", handle, reserved);

    if (!handle || reserved)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index(handle);
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    status = UNIX_WLAN_CALL( wlan_close_handle, &params );
    if (status != STATUS_SUCCESS && status != STATUS_NOT_SUPPORTED)
        return RtlNtStatusToDosError( status );

    wlan->magic = 0;
    return ERROR_SUCCESS;
}

DWORD WINAPI WlanOpenHandle(DWORD client_version, void *reserved, DWORD *negotiated_version, HANDLE *handle)
{
    struct wine_wlan *wlan;
    struct wlan_open_handle_params params = {0};
    HANDLE ret_handle;
    NTSTATUS status;

    TRACE("(%lu, %p, %p, %p)\n", client_version, reserved, negotiated_version, handle);

    if (reserved || !negotiated_version || !handle)
        return ERROR_INVALID_PARAMETER;

    if (client_version != 1 && client_version != 2)
        return ERROR_NOT_SUPPORTED;

    ret_handle = handle_new(&wlan);
    if (!ret_handle)
        return ERROR_REMOTE_SESSION_LIMIT_EXCEEDED;

    status = UNIX_WLAN_CALL( wlan_open_handle, &params );
    if (status != STATUS_SUCCESS && status != STATUS_NOT_SUPPORTED)
        return RtlNtStatusToDosError( status );

    wlan->unix_handle = params.handle;
    wlan->magic = WLAN_MAGIC;
    wlan->cli_version = *negotiated_version = client_version;
    *handle = ret_handle;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanScan(HANDLE handle, const GUID *guid, const DOT11_SSID *ssid,
                      const WLAN_RAW_DATA *raw, void *reserved)
{
    struct wine_wlan *wlan;
    struct wlan_start_scan params = {0};
    NTSTATUS status;

    TRACE( "(%p, %s, %p, %p, %p)\n", handle, wine_dbgstr_guid( guid ), ssid, raw, reserved );

    if (!handle || !guid || reserved)
        return ERROR_INVALID_PARAMETER;

    if (ssid && ssid->uSSIDLength > sizeof( ssid->ucSSID ))
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    params.interface = guid;
    params.ssid = ssid;
    status = UNIX_WLAN_CALL( wlan_start_scan, &params );

    return RtlNtStatusToDosError( status );
}

DWORD WINAPI WlanRegisterNotification(HANDLE handle, DWORD notify_source, BOOL ignore_dup,
                                      WLAN_NOTIFICATION_CALLBACK callback, void *context,
                                      void *reserved, DWORD *notify_prev)
{
    FIXME("(%p, %ld, %d, %p, %p, %p, %p) stub\n",
          handle, notify_source, ignore_dup, callback, context, reserved, notify_prev);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanGetAvailableNetworkList(HANDLE handle, const GUID *guid, DWORD flags,
                                         void *reserved, WLAN_AVAILABLE_NETWORK_LIST **network_list)
{
    struct wine_wlan *wlan;
    struct wlan_network_list_get_params params = {0};
    struct wlan_network_list_move_to_avail_network_params move_params = {0};
    WLAN_AVAILABLE_NETWORK_LIST *ret_list;
    NTSTATUS status;

    TRACE( "(%p, %s, 0x%lx, %p, %p)\n", handle, wine_dbgstr_guid( guid ), flags, reserved,
           network_list );

    if (!handle || !guid || reserved || !network_list)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    params.interface = guid;
    status = UNIX_WLAN_CALL( wlan_network_list_get, &params );
    if (status != STATUS_SUCCESS)
        return RtlNtStatusToDosError( status );

    ret_list = WlanAllocateMemory( offsetof( WLAN_AVAILABLE_NETWORK_LIST, Network[params.len] ) );
    if (!ret_list)
    {
        struct wlan_network_list_free_params free_params = {0};
        free_params.networks = params.networks;
        UNIX_WLAN_CALL( wlan_network_list_free, &free_params );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    move_params.networks = params.networks;
    move_params.dest = ret_list->Network;
    UNIX_WLAN_CALL( wlan_network_list_move_to_avail_network, &move_params );

    ret_list->dwNumberOfItems = params.len;
    ret_list->dwIndex = 0;
    *network_list = ret_list;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanGetNetworkBssList( HANDLE handle, const GUID *guid, const DOT11_SSID *ssid,
                                    DOT11_BSS_TYPE bss_type, BOOL security, void *reserved,
                                    WLAN_BSS_LIST **bss_list )
{
    struct wine_wlan *wlan;
    struct wlan_network_list_get_params params = {0};
    struct wlan_network_list_move_to_bss_entry_params move_params = {0};
    WLAN_BSS_LIST *ret_list;
    NTSTATUS status;
    DWORD size;

    TRACE( "(%p, %s, %p, %d, %d, %p, %p)\n", handle, debugstr_guid( guid ), ssid, bss_type, security, reserved,
           bss_list );

    if (!handle || !guid || reserved || !bss_list)
        return ERROR_INVALID_PARAMETER;
    if (ssid)
    {
        if (ssid->uSSIDLength > sizeof(ssid->ucSSID))
            return ERROR_INVALID_PARAMETER;
        switch (bss_type)
        {
            case dot11_BSS_type_infrastructure:
            case dot11_BSS_type_independent:
            case dot11_BSS_type_any:
                break;
            default:
                return ERROR_INVALID_PARAMETER;
        }
    }

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    params.interface = guid;
    params.ssid_filter = ssid;
    params.security = ssid && security;
    status = UNIX_WLAN_CALL( wlan_network_list_get, &params );
    if (status != STATUS_SUCCESS)
        return RtlNtStatusToDosError( status );

    size = offsetof( WLAN_BSS_LIST, wlanBssEntries[params.len] );
    ret_list = WlanAllocateMemory( size );
    if (!ret_list)
    {
        struct wlan_network_list_free_params free_params = {0};
        free_params.networks = params.networks;
        UNIX_WLAN_CALL( wlan_network_list_free, &free_params );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    move_params.networks = params.networks;
    move_params.dest = ret_list->wlanBssEntries;
    UNIX_WLAN_CALL( wlan_network_list_move_to_bss_entry, &move_params );

    ret_list->dwTotalSize = size;
    ret_list->dwNumberOfItems = params.len;
    *bss_list = ret_list;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanGetProfileList( HANDLE handle, const GUID *guid, void *reserved,
                                 WLAN_PROFILE_INFO_LIST **list )
{
    NTSTATUS status;
    WLAN_PROFILE_INFO_LIST *ret_list;
    struct wine_wlan *wlan;
    struct wlan_get_profile_list_params params = {0};
    struct wlan_profile_list_move_to_profile_info_params move_params = {0};
    DWORD size;

    TRACE( "(%p, %s, %p, %p)\n", handle, debugstr_guid( guid ), reserved, list );

    if (!handle || !guid || reserved || !list)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    params.interface = guid;
    status = UNIX_WLAN_CALL( wlan_get_profile_list, &params );
    if (status)
        return RtlNtStatusToDosError( status );

    size = offsetof( WLAN_PROFILE_INFO_LIST, ProfileInfo[params.len] );
    ret_list = WlanAllocateMemory( size );
    if (!ret_list)
    {
        struct wlan_profile_list_free_params free_params = {0};
        free_params.profiles = params.list;
        UNIX_WLAN_CALL( wlan_profile_list_free, &free_params );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    move_params.dest = ret_list->ProfileInfo;
    move_params.profiles = params.list;
    UNIX_WLAN_CALL( wlan_profile_list_move_to_profile_info, &move_params );

    ret_list->dwIndex = 0;
    ret_list->dwNumberOfItems = params.len;
    *list = ret_list;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanSetProfile( HANDLE handle, const GUID *guid, DWORD flags,
                             const WCHAR *profile_xml_str, const WCHAR *sec_desc, BOOL overwrite,
                             void *reserved, DWORD *wlan_reason )
{
    NTSTATUS status;
    DWORD ret;
    struct wine_wlan *wlan;
    struct wlan_profile_data profile = {0};
    struct wlan_profile_set_params params = {0};

    TRACE( "(%p, %p, 0x%lx, %p, %p, %d, %p, %p)\n", handle, guid, flags, profile_xml_str, sec_desc,
           overwrite, reserved, wlan_reason );

    if (!handle || !guid || !profile_xml_str || reserved || !wlan_reason )
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    ret = wlan_profile_parse( profile_xml_str, &profile, wlan_reason );
    if (ret)
        return ret;

    params.handle = wlan->unix_handle;
    params.device = guid;
    params.profile = &profile;
    params.override = overwrite;

    status = UNIX_WLAN_CALL( wlan_profile_set, &params );
    if (status)
    {
        if (params.already_exists)
            return ERROR_ALREADY_EXISTS;
        return RtlNtStatusToDosError( status );
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanConnect( HANDLE handle, const GUID *guid, const WLAN_CONNECTION_PARAMETERS *params,
                          void *reserved )
{
    struct wine_wlan *wlan;

    TRACE( "(%p, %s, %p, %p)\n", handle, debugstr_guid( guid ), params, reserved );

    if (!handle || !guid || !params || reserved)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    switch (params->wlanConnectionMode)
    {
        case wlan_connection_mode_profile:
        {
            struct wlan_connect_with_profile_name_params connect_params = {0};
            char *name;
            SIZE_T len;
            NTSTATUS status;

            if (!params->strProfile || !wcslen( params->strProfile ) ||
                wcslen( params->strProfile ) > WLAN_MAX_NAME_LENGTH)
                return ERROR_INVALID_PARAMETER;

            connect_params.handle = wlan->unix_handle;
            connect_params.device = guid;
            len = wcstombs( NULL, params->strProfile, 0 ) + 1;
            name = malloc( len );
            if (!name)
                return ERROR_INVALID_PARAMETER;
            wcstombs( name, params->strProfile, len );
            connect_params.profile_name = name;
            status = UNIX_WLAN_CALL( wlan_connect_with_profile_name, &connect_params );
            if (status)
            {
                free( name );
                return RtlNtStatusToDosError( status );
            }
            return ERROR_SUCCESS;
        }

        case wlan_connection_mode_discovery_secure:
        case wlan_connection_mode_discovery_unsecure:
            if (params->strProfile || !params->pdot11Ssid ||
                params->dot11BssType == dot11_BSS_type_any)
                return ERROR_INVALID_PARAMETER;
        case wlan_connection_mode_temporary_profile:
            FIXME( "unsupported wlanConnectionMode value: %#x", params->wlanConnectionMode );
            return ERROR_CALL_NOT_IMPLEMENTED;

        case wlan_connection_mode_invalid:
        case wlan_connection_mode_auto:
        default:
            return ERROR_INVALID_PARAMETER;
    }
}

DWORD WINAPI WlanDisconnect( HANDLE handle, const GUID *guid, void *reserved )
{
    struct wine_wlan *wlan;
    struct wlan_disconnect_params params = {0};

    TRACE( "(%p, %s, %p)\n", handle, debugstr_guid( guid ), reserved );

    if (!handle || !guid || reserved)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index( handle );
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    params.handle = wlan->unix_handle;
    params.device = guid;

    return RtlNtStatusToDosError( UNIX_WLAN_CALL( wlan_disconnect, &params ));
}

DWORD WINAPI WlanQueryInterface(HANDLE handle, const GUID *guid, WLAN_INTF_OPCODE opcode,
                    void *reserved, DWORD *data_size, void **data, WLAN_OPCODE_VALUE_TYPE *opcode_type)
{
    FIXME("(%p, %s, 0x%x, %p, %p, %p, %p) stub\n",
          handle, wine_dbgstr_guid(guid), opcode, reserved, data_size, data, opcode_type);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQueryProperty(HANDLE handle, WLAN_HOSTED_NETWORK_OPCODE opcode,
                                            DWORD *data_size, void **data,
                                            WLAN_OPCODE_VALUE_TYPE *opcode_type, void *reserved)
{
    FIXME("(%p, 0x%x, %p, %p, %p, %p) stub\n",
          handle, opcode, data_size, data, opcode_type, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQuerySecondaryKey(HANDLE handle, DWORD *key_size, unsigned char *key,
                                                BOOL *passphrase, BOOL *persistent,
                                                WLAN_HOSTED_NETWORK_REASON *error, void *reserved)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p) stub\n",
          handle, key_size, key, passphrase, persistent, error, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQueryStatus(HANDLE handle, WLAN_HOSTED_NETWORK_STATUS *status, void *reserved)
{
    FIXME("(%p, %p, %p) stub\n", handle, status, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

void WINAPI WlanFreeMemory(void *ptr)
{
    TRACE("(%p)\n", ptr);

    HeapFree(GetProcessHeap(), 0, ptr);
}

void *WINAPI WlanAllocateMemory(DWORD size)
{
    void *ret;

    TRACE("(%ld)\n", size);

    if (!size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    ret = HeapAlloc(GetProcessHeap(), 0, size);
    if (!ret)
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return ret;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        if (__wine_init_unix_call()) return FALSE;
        UNIX_WLAN_CALL( wlan_init, NULL );
        break;
    }

    return TRUE;
}
