/*
 * Copyright 2022 Julian Klemann for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Enumeration
#include "windows.devices.enumeration.h"

#include "wine/test.h"

#define IDeviceInformationStatics2_CreateWatcher IDeviceInformationStatics2_CreateWatcherWithKindAqsFilterAndAdditionalProperties
#define check_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp, FALSE )
#define check_optional_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp, TRUE )
static void check_interface_(unsigned int line, void *obj, const IID *iid, BOOL supported, BOOL optional)
{
    IUnknown *iface = obj;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr || broken(hr == E_NOINTERFACE && optional), "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

struct device_watcher_handler
{
    ITypedEventHandler_DeviceWatcher_IInspectable ITypedEventHandler_DeviceWatcher_IInspectable_iface;
    LONG ref;

    HANDLE event;
    BOOL invoked;
    IInspectable *args;
};

static inline struct device_watcher_handler *impl_from_ITypedEventHandler_DeviceWatcher_IInspectable(
        ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct device_watcher_handler, ITypedEventHandler_DeviceWatcher_IInspectable_iface );
}

static HRESULT WINAPI device_watcher_handler_QueryInterface(
        ITypedEventHandler_DeviceWatcher_IInspectable *iface, REFIID iid, void **out )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_ITypedEventHandler_DeviceWatcher_IInspectable ))
    {
        IUnknown_AddRef( &impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface );
        *out = &impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface;
        return S_OK;
    }

    trace( "%s not implemented, returning E_NO_INTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_watcher_handler_AddRef( ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    return ref;
}

static ULONG WINAPI device_watcher_handler_Release( ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    return ref;
}

static HRESULT WINAPI device_watcher_handler_Invoke( ITypedEventHandler_DeviceWatcher_IInspectable *iface,
                                                     IDeviceWatcher *sender, IInspectable *args )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );
    ULONG ref;
    trace( "iface %p, sender %p, args %p\n", iface, sender, args );

    impl->invoked = TRUE;
    impl->args = args;

    IDeviceWatcher_AddRef( sender );
    ref = IDeviceWatcher_Release( sender );
    ok( ref == 3, "got ref %lu\n", ref );

    SetEvent( impl->event );

    return S_OK;
}

static const ITypedEventHandler_DeviceWatcher_IInspectableVtbl device_watcher_handler_vtbl =
{
    device_watcher_handler_QueryInterface,
    device_watcher_handler_AddRef,
    device_watcher_handler_Release,
    /* ITypedEventHandler<DeviceWatcher*,IInspectable*> methods */
    device_watcher_handler_Invoke,
};

static void device_watcher_handler_create( struct device_watcher_handler *impl )
{
    impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface.lpVtbl = &device_watcher_handler_vtbl;
    impl->invoked = FALSE;
    impl->ref = 1;
}

struct deviceinformationcollection_async_handler
{
    IAsyncOperationCompletedHandler_DeviceInformationCollection iface;

    IAsyncOperation_DeviceInformationCollection *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
    LONG ref;
};

static inline struct deviceinformationcollection_async_handler *
impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection(
    IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    return CONTAINING_RECORD( iface, struct deviceinformationcollection_async_handler, iface );
}

static HRESULT WINAPI deviceinformationcollection_async_handler_QueryInterface(
    IAsyncOperationCompletedHandler_DeviceInformationCollection *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_DeviceInformationCollection ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI deviceinformationcollection_async_handler_AddRef(
    IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    struct deviceinformationcollection_async_handler *impl;
    impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI deviceinformationcollection_async_handler_Release(
    IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    struct deviceinformationcollection_async_handler *impl;
    ULONG ref;

    impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );
    ref =  InterlockedDecrement( &impl->ref );
    if (!ref)
        free( impl );
    return ref;
}

static HRESULT WINAPI deviceinformationcollection_async_handler_Invoke(
    IAsyncOperationCompletedHandler_DeviceInformationCollection *iface,
    IAsyncOperation_DeviceInformationCollection *async, AsyncStatus status )
{
    struct deviceinformationcollection_async_handler *impl;

    impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );
    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl-> event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_DeviceInformationCollectionVtbl deviceinformationcollection_async_handler_vtbl =
{
    /* IUnknown */
    deviceinformationcollection_async_handler_QueryInterface,
    deviceinformationcollection_async_handler_AddRef,
    deviceinformationcollection_async_handler_Release,
    /* IAsyncOperationCompletedHandler<DeviceInformationCollection> */
    deviceinformationcollection_async_handler_Invoke,
};

static IAsyncOperationCompletedHandler_DeviceInformationCollection *
deviceinformationcollection_async_handler_create( HANDLE event )
{
    struct deviceinformationcollection_async_handler *impl;

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return NULL;
    impl->iface.lpVtbl = &deviceinformationcollection_async_handler_vtbl;
    impl->event = event;
    impl->ref = 1;

    return &impl->iface;
}

#define await_deviceinformationcollection( a ) await_deviceinformationcollection_( __LINE__, ( a ) )
static void await_deviceinformationcollection_( int line, IAsyncOperation_DeviceInformationCollection *async )
{
    IAsyncOperationCompletedHandler_DeviceInformationCollection *handler;
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(__FILE__, line)( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    handler = deviceinformationcollection_async_handler_create( event );
    ok_(__FILE__, line)( !!handler, "deviceinformationcollection_async_handler_create failed\n" );
    hr = IAsyncOperation_DeviceInformationCollection_put_Completed( async, handler );
    ok_(__FILE__, line)( hr == S_OK, "put_Completed returned %#lx\n", hr );
    IAsyncOperationCompletedHandler_DeviceInformationCollection_Release( handler );

    ret = WaitForSingleObject( event, 5000 );
    ok_(__FILE__, line)( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( event );
    ok_(__FILE__, line)( ret, "CloseHandle failed, error %lu\n", GetLastError() );
}

#define check_deviceinformationcollection_async( a, b, c, d, e ) check_deviceinformationcollection_async_( __LINE__, a, b, c, d, e )
static void check_deviceinformationcollection_async_(
    int line, IAsyncOperation_DeviceInformationCollection *async, UINT32 expect_id,
    AsyncStatus expect_status, HRESULT expect_hr, IVectorView_DeviceInformation **result )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_DeviceInformationCollection_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( async_id == expect_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) todo_wine_if(FAILED(expect_hr)) ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );

    hr = IAsyncOperation_DeviceInformationCollection_GetResults( async, result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr, "GetResults returned %#lx\n", hr );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

static void test_DeviceInformation_obj( int line, IDeviceInformation *info )
{
    HRESULT hr;
    HSTRING str;
    boolean bool_val;

    hr = IDeviceInformation_get_Id( info, &str );
    ok_(__FILE__, line)( SUCCEEDED( hr ), "got hr %#lx\n", hr );
    trace_(__FILE__, line)( "id: %s\n", debugstr_hstring( str ) );
    WindowsDeleteString( str );
    str = NULL;
    hr = IDeviceInformation_get_Name( info, &str );
    todo_wine ok_(__FILE__, line)( SUCCEEDED( hr ), "got hr %#lx\n", hr );
    trace_(__FILE__, line)( "  name: %s\n", debugstr_hstring( str ) );
    WindowsDeleteString( str );
    hr = IDeviceInformation_get_IsEnabled( info, &bool_val );
    todo_wine ok_(__FILE__, line)( SUCCEEDED( hr ), "got hr %#lx\n", hr );
    trace_(__FILE__, line)( "  enabled: %d\n", bool_val );
    hr = IDeviceInformation_get_IsDefault( info, &bool_val );
    todo_wine ok_(__FILE__, line)( SUCCEEDED( hr ), "got hr %#lx\n", hr );
    trace_(__FILE__, line)( "  default: %d\n", bool_val );
}

static void test_DeviceInformation( void )
{
    static const WCHAR *device_info_name = L"Windows.Devices.Enumeration.DeviceInformation";

    static struct device_watcher_handler stopped_handler, added_handler;
    EventRegistrationToken stopped_token, added_token;
    IInspectable *inspectable, *inspectable2;
    IActivationFactory *factory;
    IDeviceInformationStatics2 *device_info_statics2;
    IDeviceInformationStatics *device_info_statics;
    IDeviceWatcher *device_watcher;
    DeviceWatcherStatus status = 0xdeadbeef;
    IAsyncOperation_DeviceInformationCollection *infocollection_async = NULL;
    IVectorView_DeviceInformation *info_collection = NULL;
    IIterable_DeviceInformation *info_iterable = NULL;
    IIterator_DeviceInformation *info_iterator = NULL;
    ULONG ref;
    HSTRING str;
    HRESULT hr;

    device_watcher_handler_create( &added_handler );
    device_watcher_handler_create( &stopped_handler );
    stopped_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stopped_handler.event, "failed to create event, got error %lu\n", GetLastError() );

    hr = WindowsCreateString( device_info_name, wcslen( device_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    if ( hr == REGDB_E_CLASSNOTREG )
    {
        win_skip( "%s runtimeclass, not registered.\n", wine_dbgstr_w( device_info_name ) );
        goto done;
    }

    hr = IActivationFactory_QueryInterface( factory, &IID_IInspectable, (void **)&inspectable );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    check_interface( factory, &IID_IAgileObject, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceInformationStatics2, (void **)&device_info_statics2 );
    ok( hr == S_OK || broken( hr == E_NOINTERFACE ), "got hr %#lx\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "IDeviceInformationStatics2 not supported.\n" );
        goto skip_device_statics;
    }

    hr = IDeviceInformationStatics2_QueryInterface( device_info_statics2, &IID_IInspectable, (void **)&inspectable2 );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( inspectable == inspectable2, "got inspectable %p, inspectable2 %p\n", inspectable, inspectable2 );

    hr = IDeviceInformationStatics2_CreateWatcher( device_info_statics2, NULL, NULL, DeviceInformationKind_AssociationEndpoint, &device_watcher );
    check_interface( device_watcher, &IID_IUnknown, TRUE );
    check_interface( device_watcher, &IID_IInspectable, TRUE );
    check_interface( device_watcher, &IID_IAgileObject, TRUE );
    check_interface( device_watcher, &IID_IDeviceWatcher, TRUE );

    hr = IDeviceWatcher_add_Added(
            device_watcher,
            (ITypedEventHandler_DeviceWatcher_DeviceInformation *)&added_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface,
            &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped(
            device_watcher, &stopped_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface,
            &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_handler.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_handler.invoked, "stopped_handler not invoked\n" );
    ok( stopped_handler.args == NULL, "stopped_handler not invoked\n" );

    IDeviceWatcher_Release( device_watcher );
    IInspectable_Release( inspectable2 );
    IDeviceInformationStatics2_Release( device_info_statics2 );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceInformationStatics, (void **)&device_info_statics );
    ok( hr == S_OK || broken( hr == E_NOINTERFACE ), "got hr %#lx\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "IDeviceInformationStatics not supported.\n" );
        goto skip_device_statics;
    }

    IDeviceInformationStatics_CreateWatcherAqsFilter( device_info_statics, NULL, &device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    check_interface( device_watcher, &IID_IUnknown, TRUE );
    check_interface( device_watcher, &IID_IInspectable, TRUE );
    check_interface( device_watcher, &IID_IAgileObject, TRUE );
    check_interface( device_watcher, &IID_IDeviceWatcher, TRUE );

    hr = IDeviceWatcher_add_Added(
            device_watcher,
            (ITypedEventHandler_DeviceWatcher_DeviceInformation *)&added_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface,
            &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped(
            device_watcher, &stopped_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface,
            &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_handler.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_handler.invoked, "stopped_handler not invoked\n" );
    ok( stopped_handler.args == NULL, "stopped_handler not invoked\n" );

    IDeviceWatcher_Release( device_watcher );

    hr = IDeviceInformationStatics_FindAllAsync( device_info_statics, &infocollection_async );
    ok( SUCCEEDED( hr ), "got %#lx\n", hr );
    if (infocollection_async)
    {
        await_deviceinformationcollection( infocollection_async );
        check_deviceinformationcollection_async( infocollection_async, 1, Completed, S_OK, &info_collection );
        IAsyncOperation_DeviceInformationCollection_Release( infocollection_async );
    }
    if (info_collection)
    {
        UINT32 idx = 0, size = 0;
        IDeviceInformation **devices;

        hr = IVectorView_DeviceInformation_get_Size( info_collection, &size );
        ok( SUCCEEDED( hr ), "got %#lx\n", hr );
        for (idx = 0; idx < size ;idx++)
        {
            IDeviceInformation *info;
            winetest_push_context("info_collection %u", idx);
            hr = IVectorView_DeviceInformation_GetAt( info_collection, idx, &info );
            ok( SUCCEEDED( hr ), "got %#lx\n", hr);
            if (SUCCEEDED( hr ))
            {
                UINT32 idx2 = 0;
                boolean found = FALSE;

                test_DeviceInformation_obj(__LINE__, info);
                IDeviceInformation_Release( info );
                hr = IVectorView_DeviceInformation_IndexOf( info_collection, info, &idx2, &found );
                ok( SUCCEEDED( hr ), "got %#lx\n", hr );
                if (SUCCEEDED( hr ))
                {
                    ok( found, "Expected IndexOf to return true\n" );
                    ok( idx == idx2, "%u != %u\n", idx, idx2);
                }
            }
            winetest_pop_context();
        }

        devices = calloc( 1, sizeof( *devices ) * size );
        ok( !!devices, "Unable to allocate array\n" );
        if (devices)
        {
            UINT32 copied = 0;

            hr = IVectorView_DeviceInformation_GetMany( info_collection, 0, size, devices, &copied );
            ok( SUCCEEDED( hr ), "got %#lx\n", hr );
            if (SUCCEEDED( hr ))
                ok( copied == size, "%u != %u\n", copied, size );
            for(idx = 0; idx < copied; idx++)
            {
                IDeviceInformation *info = NULL;
                HSTRING id1 = NULL, id2 = NULL;

                winetest_push_context("devices %u", idx);
                hr = IDeviceInformation_get_Id( devices[idx], &id1 );
                ok( SUCCEEDED( hr ), "got %#lx\n", hr );
                hr = IVectorView_DeviceInformation_GetAt( info_collection, idx, &info);
                ok( SUCCEEDED( hr ), "got %#lx\n", hr );
                if (SUCCEEDED( hr ))
                {
                    hr = IDeviceInformation_get_Id( info, &id2 );
                    ok( SUCCEEDED( hr ), "got %#lx\n", hr );
                }
                if (id1 && id2)
                {
                    INT32 order = 1;
                    WindowsCompareStringOrdinal( id1, id2, &order );
                    ok( !order, "%s != %s\n", debugstr_hstring( id1 ), debugstr_hstring( id2 ) );
                }
                WindowsDeleteString( id1 );
                WindowsDeleteString( id2 );
                if (info)
                    IDeviceInformation_Release( info );
                IDeviceInformation_Release( devices[idx] );
                winetest_pop_context();
            }
            free( devices );
        }

        hr = IVectorView_DeviceInformation_QueryInterface(
            info_collection, &IID_IIterable_DeviceInformation, (void **)&info_iterable );
        IVectorView_DeviceInformation_Release( info_collection );
        ok( SUCCEEDED( hr ), "got hr %#lx\n", hr );
    }
    if (info_iterable)
    {
        hr = IIterable_DeviceInformation_First( info_iterable, &info_iterator );
        ok( SUCCEEDED( hr ), "got hr %#lx\n", hr );
        IIterable_DeviceInformation_Release( info_iterable );
    }
    if (info_iterator)
    {
        boolean exists;

        hr = IIterator_DeviceInformation_get_HasCurrent( info_iterator, &exists );
        ok( SUCCEEDED( hr ), "got hr %#lx\n", hr );
        while (SUCCEEDED( hr ) && exists)
        {
            IDeviceInformation *info;

            hr = IIterator_DeviceInformation_get_Current( info_iterator, &info );
            ok( SUCCEEDED( hr ), "got hr %#lx\n", hr );
            if (FAILED( hr )) break;
            test_DeviceInformation_obj( __LINE__, info );
            IDeviceInformation_Release( info );
            hr = IIterator_DeviceInformation_MoveNext( info_iterator, &exists );
            ok( SUCCEEDED( hr ), "got hr %#lx\n", hr );
        }

        IIterator_DeviceInformation_Release( info_iterator );
    }

    IDeviceInformationStatics_Release( device_info_statics );
skip_device_statics:
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %lu\n", ref );

done:
    WindowsDeleteString( str );
    CloseHandle( stopped_handler.event );
}

static void test_DeviceAccessInformation( void )
{
    static const WCHAR *device_access_info_name = L"Windows.Devices.Enumeration.DeviceAccessInformation";
    static const WCHAR *device_info_name = L"Windows.Devices.Enumeration.DeviceInformation";
    IDeviceAccessInformationStatics *statics;
    IActivationFactory *factory, *factory2;
    IDeviceAccessInformation *access_info;
    enum DeviceAccessStatus access_status;
    HSTRING str;
    HRESULT hr;
    ULONG ref;

    hr = WindowsCreateString( device_access_info_name, wcslen( device_access_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    WindowsDeleteString( str );

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered.\n", wine_dbgstr_w(device_access_info_name) );
        return;
    }

    hr = WindowsCreateString( device_info_name, wcslen( device_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory2 );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    WindowsDeleteString( str );

    ok( factory != factory2, "Got the same factory.\n" );
    IActivationFactory_Release( factory2 );

    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IDeviceAccessInformation, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceAccessInformationStatics, (void **)&statics );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceAccessInformationStatics_CreateFromDeviceClass( statics, DeviceClass_AudioCapture, &access_info );
    ok( hr == S_OK || broken( hr == RPC_E_CALL_COMPLETE ) /* broken on some Testbot machines */, "got hr %#lx\n", hr );

    if (hr == S_OK)
    {
        hr = IDeviceAccessInformation_get_CurrentStatus( access_info, &access_status );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( access_status == DeviceAccessStatus_Allowed, "got %d.\n", access_status );
        ref = IDeviceAccessInformation_Release( access_info );
        ok( !ref, "got ref %lu\n", ref );
    }

    ref = IDeviceAccessInformationStatics_Release( statics );
    ok( ref == 2, "got ref %lu\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %lu\n", ref );
}

START_TEST( devices )
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    test_DeviceInformation();
    test_DeviceAccessInformation();

    RoUninitialize();
}
