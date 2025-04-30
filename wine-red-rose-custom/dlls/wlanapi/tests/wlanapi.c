/*
 * Unit test suite for wlanapi functions
 *
 * Copyright 2017 Bruno Jesus
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wlanapi.h>

#include "wine/test.h"

static void test_WlanOpenHandle(void)
{
    HANDLE bad_handle = (HANDLE) 0xdeadcafe, handle = bad_handle, handle2;
    DWORD ret, neg_version = 0xdeadbeef, reserved = 0xdead;
    BOOL is_xp;

    /* invalid version requested */
    ret = WlanOpenHandle(0, NULL, &neg_version, &handle);
    is_xp = ret == ERROR_SUCCESS;
    if (!is_xp) /* the results in XP differ completely from all other versions */
    {
        ok(ret == ERROR_NOT_SUPPORTED, "Expected 50, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");
        ret = WlanOpenHandle(10, NULL, &neg_version, &handle);
        ok(ret == ERROR_NOT_SUPPORTED, "Expected 50, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");

        /* reserved parameter must not be used */
        ret = WlanOpenHandle(1, &reserved, &neg_version, &handle);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");

        /* invalid parameters */
        ret = WlanOpenHandle(1, NULL, NULL, &handle);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(handle == bad_handle, "bad handle\n");
        ret = WlanOpenHandle(1, NULL, &neg_version, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
    }
    else
    {
        ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
        ok(handle != bad_handle && handle, "handle changed\n");
        ret = WlanCloseHandle(handle, NULL);
        ok(ret == 0, "Expected 0, got %ld\n", ret);
    }

    /* good tests */
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
    ok(handle != bad_handle && handle, "handle changed\n");
    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanOpenHandle(2, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!is_xp) /* XP does not support client version 2 */
      ok(neg_version == 2, "Expected 2, got %ld\n", neg_version);
    else
      ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
    ok(handle != bad_handle && handle, "bad handle\n");
    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    /* open twice */
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle2);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);

    ret = WlanCloseHandle(handle, &reserved);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);

    ret = WlanCloseHandle(handle, NULL);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ret = WlanCloseHandle(handle2, NULL);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);

    ret = WlanCloseHandle(bad_handle, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Expected 6, got %ld\n", ret);

    ret = WlanCloseHandle(NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
}

static void test_WlanAllocateFreeMemory(void)
{
    void *ptr;

    SetLastError(0xdeadbeef);
    ptr = WlanAllocateMemory(0);
    ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", GetLastError());

    ptr = WlanAllocateMemory(1024);
    ok(ptr != NULL, "Expected non-NULL\n");

    WlanFreeMemory(ptr);

    WlanFreeMemory(NULL); /* return is void, proves that won't crash */
}

static void test_WlanEnumInterfaces(void)
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *bad_list = (WLAN_INTERFACE_INFO_LIST *)0xdeadcafe,
                             *list = bad_list;
    WLAN_INTERFACE_INFO *info;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    /* invalid parameters */
    ret = WlanEnumInterfaces(NULL, NULL, &list);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");
    ret = WlanEnumInterfaces(handle, &reserved, &list);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");
    ret = WlanEnumInterfaces(handle, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");

    /* good tests */
    list = NULL;
    ret = WlanEnumInterfaces(handle, NULL, &list);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ok(list != NULL, "bad interface list\n");
    if (!list || !list->dwNumberOfItems)
    {
        skip("No wireless interfaces\n");
        WlanCloseHandle(handle, NULL);
        WlanFreeMemory(list);
        return;
    }

    trace("Wireless interfaces: %ld\n", list->dwNumberOfItems);
    for (i = 0; i < list->dwNumberOfItems;i ++)
    {
        info = &list->InterfaceInfo[i];
        trace("  Index[%ld] GUID: %s\n", i, wine_dbgstr_guid(&info->InterfaceGuid));
        switch (info->isState)
        {
            case wlan_interface_state_disconnected:
                trace("  Status: Disconnected\n");
                break;
            case wlan_interface_state_connected:
                trace("  Status: Connected\n");
                break;
            default:
                trace("  Status: Other\n");
                break;
        }
        trace("  Description: %s\n", wine_dbgstr_w(info->strInterfaceDescription));
    }

    WlanFreeMemory(list);

    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);
}

static void test_WlanGetAvailableNetworkList( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems;i ++)
    {
        WLAN_INTERFACE_INFO *info;
        WLAN_AVAILABLE_NETWORK_LIST *bad_list = (WLAN_AVAILABLE_NETWORK_LIST *)0xdeadbeef,
                                    *list = bad_list;
        DWORD j;

        info = &ifaces->InterfaceInfo[i];
        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        /* invalid parameters */
        ret = WlanGetAvailableNetworkList( NULL, NULL, 0, NULL, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret = WlanGetAvailableNetworkList( handle, &info->InterfaceGuid, 0, &reserved, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret = WlanGetAvailableNetworkList( handle, NULL, 0, NULL, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );

        /* valid parameters */
        ret = WlanGetAvailableNetworkList( handle, &info->InterfaceGuid, 0, NULL, &list );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
        if (!list || !list->dwNumberOfItems)
        {
            skip( "No wireless networks\n" );
            WlanFreeMemory( list );
            continue;
        }

        for (j = 0; j < list->dwNumberOfItems; j++)
        {
            WLAN_AVAILABLE_NETWORK *network = &list->Network[j];

            ok( network->dot11Ssid.uSSIDLength <= sizeof( network->dot11Ssid.ucSSID ),
                "Unexpected length for uSSID, should be <= 32: %ld\n",
                network->dot11Ssid.uSSIDLength );

            trace(
                "    Index[%ld] SSID: %s\n", j,
                  debugstr_an( (char *)network->dot11Ssid.ucSSID, network->dot11Ssid.uSSIDLength ) );
            if (network->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
            {
                trace("      connected\n");
            }
            else
            {
                trace("      not connected\n");
            }
            trace( "      Signal Quality: %ld\n", network->wlanSignalQuality );
        }

        WlanFreeMemory( list );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}

static void test_WlanGetNetworkBssList( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);

    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        WLAN_INTERFACE_INFO *info;
        WLAN_BSS_LIST *bad_list = (WLAN_BSS_LIST *)0xdeadbeef, *list = bad_list;
        DWORD j;

        info = &ifaces->InterfaceInfo[i];
        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        /* invalid parameters */
        ret = WlanGetNetworkBssList( NULL, NULL, NULL, 0, FALSE, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret = WlanGetNetworkBssList( handle, &info->InterfaceGuid, NULL, 0, FALSE, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret = WlanGetNetworkBssList( handle, &info->InterfaceGuid, NULL, 0, FALSE, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret =
            WlanGetNetworkBssList( handle, &info->InterfaceGuid, NULL, 0, FALSE, &reserved, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );

        /* valid paramters */
        ret = WlanGetNetworkBssList( handle, &info->InterfaceGuid, NULL, 0, FALSE, NULL, &list );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
        if (!list || !list->dwNumberOfItems)
        {
            skip( "No wireless networks\n" );
            WlanFreeMemory( list );
            continue;
        }

        for (j = 0; j < list->dwNumberOfItems; j++)
        {
            WLAN_BSS_ENTRY *entry = &list->wlanBssEntries[j];

            ok( entry->dot11Ssid.uSSIDLength <= sizeof( entry->dot11Ssid.ucSSID ),
                "Unexpected length for uSSID, should be <= 32: %ld\n",
                entry->dot11Ssid.uSSIDLength );

            trace(
                "    Index[%ld] SSID: %s\n", j,
                  debugstr_an( (char *)entry->dot11Ssid.ucSSID, entry->dot11Ssid.uSSIDLength ) );
        }

        WlanFreeMemory( list );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}

static void test_WlanStartScan( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        WLAN_INTERFACE_INFO *info;
        DOT11_SSID invalid_ssid = {.uSSIDLength = 60},
                   ssid = {.uSSIDLength = 4, .ucSSID = {'t', 'e', 's', 't'}};

        info = &ifaces->InterfaceInfo[i];
        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        /* invalid parameters */
        ret = WlanScan( NULL, NULL, NULL, NULL, NULL );
        ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ret = WlanScan( handle, NULL, NULL, NULL, NULL );
        ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ret = WlanScan( handle, &info->InterfaceGuid, &invalid_ssid, NULL, NULL );
        ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ret = WlanScan ( handle, &info->InterfaceGuid, &ssid, NULL, &reserved );
        ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);

        /* valid parameters */
        ret = WlanScan( handle, &info->InterfaceGuid, NULL, NULL, NULL );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret );
        ret = WlanScan( handle, &info->InterfaceGuid, &ssid, NULL, NULL );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}

/* If profile_name is not NULL, additionally test whether a WLAN profile with this name exists. */
static void test_WlanGetProfileList( const WCHAR *profile_name, int todo )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;
    BOOL profile_exists = FALSE;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        WLAN_PROFILE_INFO_LIST *bad_list = (WLAN_PROFILE_INFO_LIST *)0xdeadbeef, *list = bad_list;
        WLAN_INTERFACE_INFO *info;
        DWORD j;

        info = &ifaces->InterfaceInfo[i];
        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        ret = WlanGetProfileList( NULL, NULL, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanGetProfileList( handle, NULL, NULL, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );
        ret = WlanGetProfileList( handle, &info->InterfaceGuid, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanGetProfileList( handle, &info->InterfaceGuid, &reserved, &list );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ok( list == bad_list, "list changed\n" );

        ret = WlanGetProfileList( handle, &info->InterfaceGuid, NULL, &list );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
        if (!list || !list->dwNumberOfItems)
        {
            skip( "No WLAN profiles\n" );
            WlanFreeMemory( list );
            continue;
        }

        for (j = 0; j < list->dwNumberOfItems; j++)
        {
            const WCHAR *name = list->ProfileInfo[j].strProfileName;
            trace( "    Index[%ld] Name: %s Flags: %#lx\n", j, debugstr_w( name ),
                   list->ProfileInfo[j].dwFlags );

            if (profile_name && !profile_exists)
                profile_exists = !wcscmp( name, profile_name );
        }
        WlanFreeMemory( list );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );

    if (profile_name)
        todo_if( todo ) ok( profile_exists, "Could not find a WLAN profile with the name %s.\n",
                            debugstr_w( profile_name ) );
}

static void test_WlanConnect( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok( ret == 0, "Expected 0, got %ld\n", ret );

    ret = WlanConnect( NULL, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

    ret = WlanConnect( handle, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        WLAN_PROFILE_INFO_LIST *list;
        WLAN_INTERFACE_INFO *info = &ifaces->InterfaceInfo[i];
        WLAN_CONNECTION_PARAMETERS params = {0};
        DWORD j;

        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );
        ret = WlanGetProfileList( handle, &info->InterfaceGuid, NULL, &list );
        ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
        if (!list || !list->dwNumberOfItems)
        {
            skip( "No WLAN profiles\n" );
            WlanFreeMemory( list );
            continue;
        }

        ret = WlanConnect( handle, &info->InterfaceGuid, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

        params.wlanConnectionMode = wlan_connection_mode_auto;
        ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

        params.wlanConnectionMode = wlan_connection_mode_invalid;
        ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

        for (j = 0; j < list->dwNumberOfItems; j++)
        {
            DOT11_SSID dummy_ssid = {0};

            params.wlanConnectionMode = wlan_connection_mode_discovery_secure;
            params.pdot11Ssid = &dummy_ssid;
            params.dot11BssType = dot11_BSS_type_infrastructure;
            params.strProfile = list->ProfileInfo[j].strProfileName;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.wlanConnectionMode = wlan_connection_mode_discovery_unsecure;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.wlanConnectionMode = wlan_connection_mode_discovery_secure;
            params.strProfile = NULL;
            params.dot11BssType = dot11_BSS_type_any;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.wlanConnectionMode = wlan_connection_mode_discovery_unsecure;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.wlanConnectionMode = wlan_connection_mode_discovery_secure;
            params.dot11BssType = dot11_BSS_type_infrastructure;
            params.pdot11Ssid = NULL;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.wlanConnectionMode = wlan_connection_mode_discovery_unsecure;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.pdot11Ssid = &dummy_ssid;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, &reserved );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            memset( &params, 0, sizeof( params ) );

            params.wlanConnectionMode = wlan_connection_mode_profile;
            params.strProfile = NULL;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            params.strProfile = list->ProfileInfo[j].strProfileName;
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, &reserved );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            /* WlanConnect succeeds immediately, regardless of whether the connection was successful
             * or not. */
            ret = WlanConnect( handle, &info->InterfaceGuid, &params, NULL );
            ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret );
        }

        WlanFreeMemory( list );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}


static const WCHAR *load_res( const WCHAR *name )
{
    const char *data;
    WCHAR *dataW;
    DWORD lenW, size;
    HRSRC src;
    HMODULE module = GetModuleHandleW( NULL );

    src = FindResourceW( NULL, name, (const WCHAR *)RT_RCDATA );
    ok( src != NULL, "Could not find resource %s\n", debugstr_w( name ) );
    if (!src)
        return NULL;

    data = LockResource( LoadResource( module, src ) );
    size = SizeofResource( module, src );
    lenW = MultiByteToWideChar( CP_ACP, 0, data, size, NULL, 0 );
    dataW = malloc( lenW * sizeof( WCHAR ) );
    MultiByteToWideChar( CP_ACP, 0, data, -1, dataW, lenW );

    return dataW;
}

static struct
{
    const WCHAR *profile_name;
    const WCHAR *profile_res_name;
    const WCHAR *profile_xml_str;
} valid_profiles[] = {
    {L"Wine Testing WPA-PSK AES Profile", L"wpa-psk-aes.xml"},
    {L"Wine Testing WPA-PSK TKIP Profile", L"wpa-psk-tkip.xml"},
};

static void initialize_profiles( void )
{
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( valid_profiles ); i++)
    {
        const WCHAR *xml = load_res( valid_profiles[i].profile_res_name );
        ok( xml != NULL, "Could not load resource %s\n", debugstr_w( valid_profiles[i].profile_res_name ) );
        valid_profiles[i].profile_xml_str = xml;
    }
}

static void test_WlanSetProfile( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        DWORD wlan_reason = 0;
        SIZE_T j;
        WLAN_INTERFACE_INFO *info = &ifaces->InterfaceInfo[i];

        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        ret = WlanSetProfile( NULL, NULL, 0, NULL, NULL, FALSE, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanSetProfile( handle, NULL, 0, NULL, NULL, FALSE, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanSetProfile( handle, &ifaces->InterfaceInfo[i].InterfaceGuid, 0, NULL, NULL, FALSE,
                              NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

        for (j = 0; j < ARRAY_SIZE( valid_profiles ); j++)
        {
            const WCHAR *profile_xml = valid_profiles[j].profile_xml_str;

            winetest_push_context( "WLAN profile %s\n",
                                   debugstr_w( valid_profiles[j].profile_res_name ) );
            if (!profile_xml)
            {
                skip("profile not loaded, skipping\n");
                winetest_pop_context();
                continue;
            }

            ret = WlanSetProfile( handle, &info->InterfaceGuid, 0, profile_xml, NULL, FALSE, NULL,
                                  NULL );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
            ret = WlanSetProfile( handle, &info->InterfaceGuid, 0, profile_xml, NULL, FALSE,
                                  &reserved, &wlan_reason );
            ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

            ret = WlanSetProfile( handle, &info->InterfaceGuid, 0, profile_xml, NULL, TRUE, NULL,
                                  &wlan_reason );
            ok( !ret, "Expected 0, got %ld\n", ret );
            ok( !wlan_reason, "Expected 0, got %ld\n", wlan_reason );
            test_WlanGetProfileList( valid_profiles[j].profile_name, FALSE );

            ret = WlanSetProfile( handle, &info->InterfaceGuid, 0, profile_xml, NULL, FALSE, NULL,
                                  &wlan_reason );
            ok( ret == ERROR_ALREADY_EXISTS, "Expected 183, got %ld\n", ret );
            ok( !wlan_reason, "Expected 0, got %ld\n", wlan_reason );
            winetest_pop_context();
        }
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}

static void test_WlanDisconnect( void )
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *ifaces;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanEnumInterfaces( handle, NULL, &ifaces );
    ok( ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!ifaces || !ifaces->dwNumberOfItems)
    {
        skip( "No wireless interfaces\n" );
        WlanCloseHandle( handle, NULL );
        WlanFreeMemory( ifaces );
        return;
    }

    trace("Wireless interfaces: %ld\n", ifaces->dwNumberOfItems);
    for (i = 0; i < ifaces->dwNumberOfItems; i++)
    {
        WLAN_INTERFACE_INFO *info = &ifaces->InterfaceInfo[i];

        trace( "  Index[%ld] GUID: %s\n", i, debugstr_guid( &info->InterfaceGuid ) );

        ret = WlanDisconnect( NULL, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanDisconnect( handle, NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );
        ret = WlanDisconnect( handle, &info->InterfaceGuid, &reserved );
        ok( ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret );

        ret = WlanDisconnect( handle, &info->InterfaceGuid, NULL );
        ok( !ret, "Expected 0, got %ld\n", ret );
    }

    WlanFreeMemory( ifaces );
    ret = WlanCloseHandle( handle, NULL );
    ok( ret == 0, "Expected 0, got %ld\n", ret );
}

START_TEST(wlanapi)
{
  HANDLE handle;
  DWORD neg_version;

  /* Windows checks the service before validating the client version so this
   * call will always result in error, no need to free the handle. */
  if (WlanOpenHandle(0, NULL, &neg_version, &handle) == ERROR_SERVICE_NOT_ACTIVE)
  {
      win_skip("No wireless service running\n");
      return;
  }

  initialize_profiles();

  test_WlanOpenHandle();
  test_WlanAllocateFreeMemory();
  test_WlanEnumInterfaces();
  test_WlanStartScan();
  test_WlanGetAvailableNetworkList();
  test_WlanGetNetworkBssList();
  test_WlanGetProfileList( NULL, FALSE );
  test_WlanConnect();
  test_WlanSetProfile();
  test_WlanDisconnect();
}
