/*
 * Copyright (C) 2025 Mohamad Al-Jaf
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

#include <stdarg.h>

#define COBJMACROS
#include "initguid.h"
#include "dxcore.h"

#include "wine/test.h"

static HRESULT (WINAPI *pDXCoreCreateAdapterFactory)(REFIID riid, void **ppv);

#define check_interface( obj, iid, supported ) check_interface_( __LINE__, obj, iid, supported )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL supported )
{
    IUnknown *iface = obj;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == expected_hr, "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr)) IUnknown_Release( unk );
}

static void test_DXCoreCreateAdapterFactory(void)
{
    IDXCoreAdapterFactory *adapter_factory2 = (void *)0xdeadbeef;
    IDXCoreAdapterFactory *adapter_factory = (void *)0xdeadbeef;
    IDXCoreAdapterList *adapter_list2 = (void *)0xdeadbeef;
    IDXCoreAdapterList *adapter_list = (void *)0xdeadbeef;
    uint32_t adapter_count = 0;
    HRESULT hr;
    LONG ref;

    if (0) /* Crashes exclusively on w1064v1909 */
    {
        hr = pDXCoreCreateAdapterFactory( NULL, NULL );
        ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    }
    hr = pDXCoreCreateAdapterFactory( &IID_IDXCoreAdapterFactory, NULL );
    ok( hr == E_POINTER || broken(hr == E_NOINTERFACE) /* w1064v1909 */, "got hr %#lx.\n", hr );
    hr = pDXCoreCreateAdapterFactory( &DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS, (void **)&adapter_factory );
    ok( hr == E_NOINTERFACE, "got hr %#lx.\n", hr );
    ok( adapter_factory == NULL || broken(adapter_factory == (void *)0xdeadbeef) /* w1064v1909 */, "got adapter_factory %p.\n", adapter_factory );

    hr = pDXCoreCreateAdapterFactory( &IID_IDXCoreAdapterFactory, (void **)&adapter_factory );
    ok( hr == S_OK || broken(hr == E_NOINTERFACE) /* w1064v1909 */, "got hr %#lx.\n", hr );
    if (FAILED(hr)) return;

    hr = pDXCoreCreateAdapterFactory( &IID_IDXCoreAdapterFactory, (void **)&adapter_factory2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( adapter_factory == adapter_factory2, "got adapter_factory %p, adapter_factory2 %p.\n", adapter_factory, adapter_factory2 );
    ref = IDXCoreAdapterFactory_Release( adapter_factory2 );
    ok( ref == 1, "got ref %ld.\n", ref );

    check_interface( adapter_factory, &IID_IAgileObject, FALSE );
    check_interface( adapter_factory, &IID_IDXCoreAdapter, FALSE );
    check_interface( adapter_factory, &IID_IDXCoreAdapterList, FALSE );

    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 0, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&adapter_list );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( adapter_list == NULL, "got adapter_list %p.\n", adapter_list );
    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterFactory, (void **)&adapter_list );
    ok( hr == E_NOINTERFACE, "got hr %#lx.\n", hr );
    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 1, NULL, &IID_IDXCoreAdapterFactory, (void **)&adapter_list );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterFactory, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );

    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&adapter_list );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IDXCoreAdapterFactory_CreateAdapterList( adapter_factory, 1, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, &IID_IDXCoreAdapterList, (void **)&adapter_list2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( adapter_list != adapter_list2, "got same adapter_list %p, adapter_list2 %p.\n", adapter_list, adapter_list );
    ref = IDXCoreAdapterList_Release( adapter_list2 );
    ok( ref == 0, "got ref %ld.\n", ref );

    check_interface( adapter_list, &IID_IAgileObject, FALSE );
    check_interface( adapter_list, &IID_IDXCoreAdapter, FALSE );
    check_interface( adapter_list, &IID_IDXCoreAdapterFactory, FALSE );

    adapter_count = IDXCoreAdapterList_GetAdapterCount( adapter_list );
    ok( adapter_count != 0, "IDXCoreAdapterList_GetAdapterCount returned 0.\n" );

    ref = IDXCoreAdapterList_Release( adapter_list );
    ok( ref == 0, "got ref %ld.\n", ref );
    ref = IDXCoreAdapterFactory_Release( adapter_factory );
    ok( ref == 0, "got ref %ld.\n", ref );
}

START_TEST(dxcore)
{
    HMODULE dxcore_handle = LoadLibraryA( "dxcore.dll" );
    if (!dxcore_handle)
    {
        win_skip( "Could not load dxcore.dll\n" );
        return;
    }
    pDXCoreCreateAdapterFactory = (void *)GetProcAddress( dxcore_handle, "DXCoreCreateAdapterFactory" );
    if (!pDXCoreCreateAdapterFactory)
    {
        win_skip( "Failed to get DXCoreCreateAdapterFactory address, skipping dxcore tests\n" );
        FreeLibrary( dxcore_handle );
        return;
    }

    test_DXCoreCreateAdapterFactory();

    FreeLibrary( dxcore_handle );
}
