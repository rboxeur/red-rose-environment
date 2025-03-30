/* DeviceInformationCollection implementation
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
#include "windows.foundation.collections.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(enumeration);

struct iterator_DeviceInformation
{
    IIterator_DeviceInformation Iterator_DeviceInformation_iface;

    IVectorView_DeviceInformation *IVectorView_DeviceInformation_iface;
    UINT32 idx;
    UINT32 len;
    LONG ref;
};

static inline struct iterator_DeviceInformation *
impl_from_Iterator_DeviceInformation( IIterator_DeviceInformation *iface )
{
    return CONTAINING_RECORD( iface, struct iterator_DeviceInformation, Iterator_DeviceInformation_iface );
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_QueryInterface( IIterator_DeviceInformation *iface,
                                                                            REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IIterator_DeviceInformation ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE iterator_DeviceInformation_AddRef( IIterator_DeviceInformation *iface )
{
    struct iterator_DeviceInformation *impl;

    TRACE( "(%p)\n", iface );

    impl = impl_from_Iterator_DeviceInformation( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE iterator_DeviceInformation_Release( IIterator_DeviceInformation *iface )
{
    struct iterator_DeviceInformation *impl;
    ULONG ref;

    TRACE( "(%p)\n", iface );

    impl = impl_from_Iterator_DeviceInformation( iface );
    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
    {
        IVectorView_DeviceInformation_Release( impl->IVectorView_DeviceInformation_iface );
        free( impl );
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_GetIids( IIterator_DeviceInformation *iface,
                                                                     ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI iterator_DeviceInformation_GetRuntimeClassName( IIterator_DeviceInformation *iface,
                                                                      HSTRING *class_name )
{
    const static WCHAR name[] = L"Windows.Foundation.Collections.IIterator`1<Windows.Devices.Enumeration.DeviceInformation>";
    TRACE( "(%p, %p)\n", iface, class_name );
    return WindowsCreateString( name, ARRAY_SIZE( name ), class_name );
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_GetTrustLevel( IIterator_DeviceInformation *iface,
                                                                           TrustLevel *trust_level )
{
    FIXME( "(%p, %p) stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_get_Current( IIterator_DeviceInformation *iface,
                                                                         IDeviceInformation **info )
{
    struct iterator_DeviceInformation *impl;

    TRACE( "(%p, %p)\n", iface, info );

    impl = impl_from_Iterator_DeviceInformation( iface );
    return IVectorView_DeviceInformation_GetAt( impl->IVectorView_DeviceInformation_iface,
                                                impl->idx, info );
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_get_HasCurrent( IIterator_DeviceInformation *iface,
                                                                            boolean *exists )
{
    struct iterator_DeviceInformation *impl;

    TRACE( "(%p, %p)\n", iface, exists );

    impl = impl_from_Iterator_DeviceInformation( iface );
    *exists = impl->idx < impl->len;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_MoveNext( IIterator_DeviceInformation *iface,
                                                                      boolean *exists )
{
    struct iterator_DeviceInformation *impl;

    TRACE( "(%p, %p)\n", iface, exists );

    impl = impl_from_Iterator_DeviceInformation( iface );
    if (impl->idx < impl->len) impl->idx++;
    return IIterator_DeviceInformation_get_HasCurrent( iface, exists );
}

static HRESULT STDMETHODCALLTYPE iterator_DeviceInformation_GetMany( IIterator_DeviceInformation *iface, UINT32 size,
                                                                     IDeviceInformation **info, UINT32 *copied )
{
    struct iterator_DeviceInformation *impl;

    TRACE( "(%p, %u, %p, %p)\n", iface, size, info, copied );

    impl = impl_from_Iterator_DeviceInformation( iface );
    return IVectorView_DeviceInformation_GetMany( impl->IVectorView_DeviceInformation_iface,
                                                  impl->idx, size, info, copied );
}

const static IIterator_DeviceInformationVtbl iterator_DeviceInformation_vtbl =
{
    /* IUnknown */
    iterator_DeviceInformation_QueryInterface,
    iterator_DeviceInformation_AddRef,
    iterator_DeviceInformation_Release,
    /* IInspectable */
    iterator_DeviceInformation_GetIids,
    iterator_DeviceInformation_GetRuntimeClassName,
    iterator_DeviceInformation_GetTrustLevel,
    /* IIterator<DeviceInformation> */
    iterator_DeviceInformation_get_Current,
    iterator_DeviceInformation_get_HasCurrent,
    iterator_DeviceInformation_MoveNext,
    iterator_DeviceInformation_GetMany,
};

struct vectorview_DeviceInformation
{
    IVectorView_DeviceInformation IVectorView_DeviceInformation_iface;
    IIterable_DeviceInformation IIterable_DeviceInformation_iface;

    IDeviceInformation **devices;
    SIZE_T len;

    LONG ref;
};

static inline struct vectorview_DeviceInformation *
impl_from_IVectorView_DeviceInformation( IVectorView_DeviceInformation *iface )
{
    return CONTAINING_RECORD( iface, struct vectorview_DeviceInformation, IVectorView_DeviceInformation_iface );
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_QueryInterface(
    IVectorView_DeviceInformation *iface, REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IVectorView_DeviceInformation ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IIterable_DeviceInformation ))
    {
        struct vectorview_DeviceInformation *impl = impl_from_IVectorView_DeviceInformation( iface );
        *out = &impl->IIterable_DeviceInformation_iface;
        IUnknown_AddRef( iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vectorview_DeviceInformation_AddRef( IVectorView_DeviceInformation *iface )
{
    struct vectorview_DeviceInformation *impl;

    TRACE( "(%p)\n", iface );

    impl = impl_from_IVectorView_DeviceInformation( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE vectorview_DeviceInformation_Release( IVectorView_DeviceInformation *iface )
{
    struct vectorview_DeviceInformation *impl;
    ULONG ref;

    TRACE( "(%p)\n", iface );

    impl = impl_from_IVectorView_DeviceInformation( iface );
    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
    {
        while (impl->len--)
            IDeviceInformation_Release( impl->devices[impl->len] );
        free( impl->devices );
        free( impl );
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_GetIids( IVectorView_DeviceInformation *iface,
                                                                       ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_GetRuntimeClassName( IVectorView_DeviceInformation *iface,
                                                                                   HSTRING *class_name )
{
    const static WCHAR name[] = L"Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Enumeration.DeviceInformation>";
    TRACE( "(%p, %p)\n", iface, class_name );
    return WindowsCreateString( name, ARRAY_SIZE( name ), class_name );
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_GetTrustLevel( IVectorView_DeviceInformation *iface,
                                                                             TrustLevel *trust_level )
{
    FIXME( "(%p, %p) stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_GetAt( IVectorView_DeviceInformation *iface, UINT32 index,
                                                                     IDeviceInformation **value )
{
    struct vectorview_DeviceInformation *impl;

    TRACE( "(%p, %u, %p)\n", iface, index, value );

    impl = impl_from_IVectorView_DeviceInformation( iface );
    *value = NULL;
    if (index >= impl->len)
        return E_BOUNDS;
    *value = impl->devices[index];
    IDeviceInformation_AddRef( *value );
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_get_Size( IVectorView_DeviceInformation *iface,
                                                                        UINT32 *value )
{
    struct vectorview_DeviceInformation *impl;

    TRACE( "(%p, %p)\n", iface, value );

    impl = impl_from_IVectorView_DeviceInformation( iface );
    *value = impl->len;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_IndexOf( IVectorView_DeviceInformation *iface,
                                                                       IDeviceInformation *elem, UINT32 *index,
                                                                       boolean *found )
{
    struct vectorview_DeviceInformation *impl;
    UINT32 i;

    TRACE( "(%p, %p, %p, %p)\n", iface, elem, index, found );

    impl = impl_from_IVectorView_DeviceInformation( iface );
    for (i = 0; i < impl->len; i++)
        if (elem == impl->devices[i])
            break;

    if (i < impl->len)
    {
        *found = TRUE;
        *index = i;
    }
    else
    {
        *found = FALSE;
        *index = 0;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vectorview_DeviceInformation_GetMany( IVectorView_DeviceInformation *iface,
                                                                       UINT32 start, UINT32 size,
                                                                       IDeviceInformation **items, UINT32 *copied )
{
    struct vectorview_DeviceInformation *impl;
    UINT32 i;

    TRACE( "(%p, %u, %u, %p, %p)\n", iface, start, size, items, copied );
    impl = impl_from_IVectorView_DeviceInformation( iface );
    memset( items, 0, size * sizeof( *items ) );

    for (i = start; i < impl->len && i < start + size; i++)
    {
        items[i] = impl->devices[i - start];
        IUnknown_AddRef( items[i] );
    }
    *copied = i - start;
    return S_OK;
}

const static IVectorView_DeviceInformationVtbl vectorview_DeviceInformation_vtbl =
{
    /* IUnknown */
    vectorview_DeviceInformation_QueryInterface,
    vectorview_DeviceInformation_AddRef,
    vectorview_DeviceInformation_Release,
    /* IInspectable */
    vectorview_DeviceInformation_GetIids,
    vectorview_DeviceInformation_GetRuntimeClassName,
    vectorview_DeviceInformation_GetTrustLevel,
    /* IVectorView<DeviceInformation> */
    vectorview_DeviceInformation_GetAt,
    vectorview_DeviceInformation_get_Size,
    vectorview_DeviceInformation_IndexOf,
    vectorview_DeviceInformation_GetMany
};

DEFINE_IINSPECTABLE( iterable_view_DeviceInformation, IIterable_DeviceInformation, struct vectorview_DeviceInformation,
                     IVectorView_DeviceInformation_iface );

static HRESULT STDMETHODCALLTYPE iterable_view_DeviceInformation_First( IIterable_DeviceInformation *iface,
                                                                        IIterator_DeviceInformation **iter )

{
    struct vectorview_DeviceInformation *impl;
    struct iterator_DeviceInformation *impl_iter;

    TRACE( "(%p, %p)\n", iface, iter );

    impl = impl_from_IIterable_DeviceInformation( iface );
    impl_iter = calloc( 1, sizeof( *impl_iter ) );
    if (!impl_iter)
        return E_OUTOFMEMORY;
    impl_iter->Iterator_DeviceInformation_iface.lpVtbl = &iterator_DeviceInformation_vtbl;
    impl_iter->IVectorView_DeviceInformation_iface = &impl->IVectorView_DeviceInformation_iface;
    IUnknown_AddRef( iface );
    impl_iter->len = impl->len;
    impl_iter->ref = 1;
    *iter = &impl_iter->Iterator_DeviceInformation_iface;
    return S_OK;
}

const static IIterable_DeviceInformationVtbl iterable_view_DeviceInformation_vtbl =
{
    /* IUnknown */
    iterable_view_DeviceInformation_QueryInterface,
    iterable_view_DeviceInformation_AddRef,
    iterable_view_DeviceInformation_Release,
    /* IInspectable */
    iterable_view_DeviceInformation_GetIids,
    iterable_view_DeviceInformation_GetRuntimeClassName,
    iterable_view_DeviceInformation_GetTrustLevel,
    /* IIterable<DeviceInformation> */
    iterable_view_DeviceInformation_First,
};

HRESULT vectorview_deviceinformation_create( IDeviceInformation **devices, SIZE_T len,
                                             IVectorView_DeviceInformation **view )
{
    struct vectorview_DeviceInformation *impl;

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return E_OUTOFMEMORY;

    impl->IVectorView_DeviceInformation_iface.lpVtbl = &vectorview_DeviceInformation_vtbl;
    impl->IIterable_DeviceInformation_iface.lpVtbl = &iterable_view_DeviceInformation_vtbl;
    impl->devices = devices;
    impl->len = len;
    impl->ref = 1;
    *view = &impl->IVectorView_DeviceInformation_iface;
    return S_OK;
}
