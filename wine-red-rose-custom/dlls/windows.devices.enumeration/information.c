/* DeviceInformation implementation.
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

#include "private.h"

#include <roapi.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include <wine/debug.h>
#include <wine/rbtree.h>

WINE_DEFAULT_DEBUG_CHANNEL(enumeration);

/* DeviceInformation implementation for objects with kind DeviceInformationKind_DeviceInterface. */
struct devinfo_DeviceInterface
{
    IDeviceInformation IDeviceInformation_iface;

    WCHAR instance_id[MAX_DEVICE_ID_LEN];

    HSTRING path;
    LONG ref;
};

static inline struct devinfo_DeviceInterface *impl_DeviceInterface_from_IDeviceInformation( IDeviceInformation *iface )
{
    return CONTAINING_RECORD( iface, struct devinfo_DeviceInterface, IDeviceInformation_iface );
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_QueryInterface( IDeviceInformation *iface, REFIID iid,
                                                                         void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDeviceInformation ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE devinfo_DeviceInterface_AddRef( IDeviceInformation *iface )
{
    struct devinfo_DeviceInterface *impl;

    TRACE( "(%p)\n", iface );

    impl = impl_DeviceInterface_from_IDeviceInformation( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE devinfo_DeviceInterface_Release( IDeviceInformation *iface )
{
    struct devinfo_DeviceInterface *impl;
    ULONG ref;

    TRACE( "(%p)\n", iface );

    impl = impl_DeviceInterface_from_IDeviceInformation( iface );
    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
    {
        WindowsDeleteString( impl->path );
        free( impl );
    }

    return ref;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_GetIids( IDeviceInformation *iface,
                                                                  ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI devinfo_DeviceInterface_GetRuntimeClassName( IDeviceInformation *iface, HSTRING *class_name )
{
    const static WCHAR *name = RuntimeClass_Windows_Devices_Enumeration_DeviceInformation;

    TRACE( "(%p, %p)\n", iface, class_name );

    return WindowsCreateString( name, wcslen( name ), class_name );
}

static HRESULT STDMETHODCALLTYPE device_information_GetTrustLevel(
        IDeviceInformation *iface, TrustLevel *trust_level)
{
    FIXME("(%p, %p) stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_get_Id( IDeviceInformation *iface, HSTRING *id )
{
    struct devinfo_DeviceInterface *impl;

    TRACE( "(%p, %p)\n", iface, id );

    impl = impl_DeviceInterface_from_IDeviceInformation( iface );
    return WindowsDuplicateString( impl->path, id );
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_get_Name( IDeviceInformation *iface, HSTRING *name )
{
    FIXME( "(%p, %p) stub!\n", iface, name );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_get_IsEnabled( IDeviceInformation *iface, boolean *value )
{
    FIXME( "(%p, %p) stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_IsDefault( IDeviceInformation *iface, boolean *value )
{
    FIXME( "(%p, %p) stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_get_EnclosureLocation( IDeviceInformation *iface,
                                                                                IEnclosureLocation **location )
{
    FIXME( "(%p, %p) stub!\n", iface, location );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_get_Properties( IDeviceInformation *iface,
                                                                         IMapView_HSTRING_IInspectable **properties )
{
    FIXME( "(%p, %p) stub!\n", iface, properties );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_Update( IDeviceInformation *iface,
                                                                 IDeviceInformationUpdate *update )
{
    FIXME( "(%p, %p) stub!\n", iface, update );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE devinfo_DeviceInterface_GetThumbnailAsync( IDeviceInformation *iface,
                                                                            IAsyncOperation_DeviceThumbnail **async )
{
    FIXME( "(%p, %p) stub!\n", iface, async );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
devinfo_DeviceInterface_GetGlyphThumbnailAsync( IDeviceInformation *iface, IAsyncOperation_DeviceThumbnail **async )
{
    FIXME( "(%p, %p) stub!\n", iface, async );
    return E_NOTIMPL;
}

static const struct IDeviceInformationVtbl devinfo_DeviceInterface_vtbl = {
    /* IUnknown */
    devinfo_DeviceInterface_QueryInterface,
    devinfo_DeviceInterface_AddRef,
    devinfo_DeviceInterface_Release,
    /* IInspectable */
    devinfo_DeviceInterface_GetIids,
    devinfo_DeviceInterface_GetRuntimeClassName,
    device_information_GetTrustLevel,
    /* IDeviceInformation */
    devinfo_DeviceInterface_get_Id,
    devinfo_DeviceInterface_get_Name,
    devinfo_DeviceInterface_get_IsEnabled,
    devinfo_DeviceInterface_IsDefault,
    devinfo_DeviceInterface_get_EnclosureLocation,
    devinfo_DeviceInterface_get_Properties,
    devinfo_DeviceInterface_Update,
    devinfo_DeviceInterface_GetThumbnailAsync,
    devinfo_DeviceInterface_GetGlyphThumbnailAsync,
};

HRESULT deviceinformation_iface_create( const SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail,
                                        IDeviceInformation **info )
{
    struct devinfo_DeviceInterface *impl;
    HRESULT res;

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return E_OUTOFMEMORY;

    impl->IDeviceInformation_iface.lpVtbl = &devinfo_DeviceInterface_vtbl;
    res = WindowsCreateString( iface_detail->DevicePath, wcslen( iface_detail->DevicePath ),
                               &impl->path );
    if (FAILED( res ))
    {
        free( impl );
        return res;
    }
    impl->ref = 1;
    *info = &impl->IDeviceInformation_iface;
    return S_OK;
}
