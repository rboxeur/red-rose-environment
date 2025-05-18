/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#include "dxgi1_6.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxcore);

struct dxcore_adapter
{
    IDXCoreAdapter IDXCoreAdapter_iface;
    LONG ref;

    DXGI_ADAPTER_DESC3 desc;
};

static inline struct dxcore_adapter *impl_from_IDXCoreAdapter( IDXCoreAdapter *iface )
{
    return CONTAINING_RECORD( iface, struct dxcore_adapter, IDXCoreAdapter_iface );
}

static HRESULT WINAPI dxcore_adapter_QueryInterface( IDXCoreAdapter *iface, REFIID iid, void **out )
{
    struct dxcore_adapter *impl = impl_from_IDXCoreAdapter( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IDXCoreAdapter ))
    {
        *out = &impl->IDXCoreAdapter_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dxcore_adapter_AddRef( IDXCoreAdapter *iface )
{
    struct dxcore_adapter *impl = impl_from_IDXCoreAdapter( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI dxcore_adapter_Release( IDXCoreAdapter *iface )
{
    struct dxcore_adapter *impl = impl_from_IDXCoreAdapter( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static BOOL WINAPI dxcore_adapter_IsValid( IDXCoreAdapter *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return FALSE;
}

static BOOL WINAPI dxcore_adapter_IsAttributeSupported( IDXCoreAdapter *iface, REFGUID attribute )
{
    FIXME( "iface %p, attribute %s stub!\n", iface, debugstr_guid( attribute ) );
    return FALSE;
}

static BOOL WINAPI dxcore_adapter_IsPropertySupported( IDXCoreAdapter *iface, DXCoreAdapterProperty property )
{
    FIXME( "iface %p, property %u stub!\n", iface, property );
    return FALSE;
}

static HRESULT WINAPI dxcore_adapter_GetProperty( IDXCoreAdapter *iface, DXCoreAdapterProperty property, size_t buffer_size, void *buffer )
{
    FIXME( "iface %p, property %u, buffer_size %Iu, buffer %p stub!\n", iface, property, buffer_size, buffer );
    return E_NOTIMPL;
}

static HRESULT WINAPI dxcore_adapter_GetPropertySize( IDXCoreAdapter *iface, DXCoreAdapterProperty property, size_t *buffer_size )
{
    FIXME( "iface %p, property %u, buffer_size %p stub!\n", iface, property, buffer_size );
    return E_NOTIMPL;
}

static BOOL WINAPI dxcore_adapter_IsQueryStateSupported( IDXCoreAdapter *iface, DXCoreAdapterState property )
{
    FIXME( "iface %p, property %u stub!\n", iface, property );
    return FALSE;
}

static HRESULT WINAPI dxcore_adapter_QueryState( IDXCoreAdapter *iface, DXCoreAdapterState state, size_t state_details_size,
                                                 const void *state_details, size_t buffer_size, void *buffer )
{
    FIXME( "iface %p, state %u, state_details_size %Iu, state_details %p, buffer_size %Iu, buffer %p stub!\n",
           iface, state, state_details_size, state_details, buffer_size, buffer );
    return E_NOTIMPL;
}

static BOOL WINAPI dxcore_adapter_IsSetStateSupported( IDXCoreAdapter *iface, DXCoreAdapterState property )
{
    FIXME( "iface %p, property %u stub!\n", iface, property );
    return FALSE;
}

static HRESULT WINAPI dxcore_adapter_SetState( IDXCoreAdapter *iface, DXCoreAdapterState state, size_t state_details_size,
                                               const void *state_details, size_t buffer_size, const void *buffer )
{
    FIXME( "iface %p, state %u, state_details_size %Iu, state_details %p, buffer_size %Iu, buffer %p stub!\n",
           iface, state, state_details_size, state_details, buffer_size, buffer );
    return E_NOTIMPL;
}

static HRESULT WINAPI dxcore_adapter_GetFactory( IDXCoreAdapter *iface, REFIID riid, void **ppv )
{
    FIXME( "iface %p, riid %s, ppv %p stub!\n", iface, debugstr_guid( riid ), ppv );
    return E_NOTIMPL;
}

static const struct IDXCoreAdapterVtbl dxcore_adapter_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_QueryInterface,
    dxcore_adapter_AddRef,
    dxcore_adapter_Release,
    /* IDXCoreAdapter methods */
    dxcore_adapter_IsValid,
    dxcore_adapter_IsAttributeSupported,
    dxcore_adapter_IsPropertySupported,
    dxcore_adapter_GetProperty,
    dxcore_adapter_GetPropertySize,
    dxcore_adapter_IsQueryStateSupported,
    dxcore_adapter_QueryState,
    dxcore_adapter_IsSetStateSupported,
    dxcore_adapter_SetState,
    dxcore_adapter_GetFactory,
};

struct dxcore_adapter_list
{
    IDXCoreAdapterList IDXCoreAdapterList_iface;
    LONG ref;

    struct dxcore_adapter **adapters;
    uint32_t adapter_count;
};

static inline struct dxcore_adapter_list *impl_from_IDXCoreAdapterList( IDXCoreAdapterList *iface )
{
    return CONTAINING_RECORD( iface, struct dxcore_adapter_list, IDXCoreAdapterList_iface );
}

static HRESULT WINAPI dxcore_adapter_list_QueryInterface( IDXCoreAdapterList *iface, REFIID iid, void **out )
{
    struct dxcore_adapter_list *impl = impl_from_IDXCoreAdapterList( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IDXCoreAdapterList ))
    {
        *out = &impl->IDXCoreAdapterList_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dxcore_adapter_list_AddRef( IDXCoreAdapterList *iface )
{
    struct dxcore_adapter_list *impl = impl_from_IDXCoreAdapterList( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI dxcore_adapter_list_Release( IDXCoreAdapterList *iface )
{
    struct dxcore_adapter_list *impl = impl_from_IDXCoreAdapterList( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        for (UINT i = 0; i < impl->adapter_count; i++)
            if (impl->adapters[i]) IDXCoreAdapter_Release( &impl->adapters[i]->IDXCoreAdapter_iface );
        free( impl->adapters );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI dxcore_adapter_list_GetAdapter( IDXCoreAdapterList *iface, uint32_t index, REFIID riid, void **ppv )
{
    FIXME( "iface %p, index %u, riid %s, ppv %p stub!\n", iface, index, debugstr_guid( riid ), ppv );
    return E_NOTIMPL;
}

static uint32_t WINAPI dxcore_adapter_list_GetAdapterCount( IDXCoreAdapterList *iface )
{
    struct dxcore_adapter_list *impl = impl_from_IDXCoreAdapterList( iface );
    TRACE( "iface %p\n", iface );
    return impl->adapter_count;
}

static BOOL WINAPI dxcore_adapter_list_IsStale( IDXCoreAdapterList *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return FALSE;
}

static HRESULT WINAPI dxcore_adapter_list_GetFactory( IDXCoreAdapterList *iface, REFIID riid, void **ppv )
{
    FIXME( "iface %p, riid %s, ppv %p stub!\n", iface, debugstr_guid( riid ), ppv );
    return E_NOTIMPL;
}

static HRESULT WINAPI dxcore_adapter_list_Sort( IDXCoreAdapterList *iface, uint32_t num_preferences, const DXCoreAdapterPreference *preferences )
{
    FIXME( "iface %p, num_preferences %u, preferences %p stub!\n", iface, num_preferences, preferences );
    return E_NOTIMPL;
}

static BOOL WINAPI dxcore_adapter_list_IsAdapterPreferenceSupported( IDXCoreAdapterList *iface, DXCoreAdapterPreference preference )
{
    FIXME( "iface %p, preference %u stub!\n", iface, preference );
    return FALSE;
}

static const struct IDXCoreAdapterListVtbl dxcore_adapter_list_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_list_QueryInterface,
    dxcore_adapter_list_AddRef,
    dxcore_adapter_list_Release,
    /* IDXCoreAdapterList methods */
    dxcore_adapter_list_GetAdapter,
    dxcore_adapter_list_GetAdapterCount,
    dxcore_adapter_list_IsStale,
    dxcore_adapter_list_GetFactory,
    dxcore_adapter_list_Sort,
    dxcore_adapter_list_IsAdapterPreferenceSupported,
};

struct dxcore_adapter_factory_statics
{
    IDXCoreAdapterFactory IDXCoreAdapterFactory_iface;
    LONG ref;
};

static inline struct dxcore_adapter_factory_statics *impl_from_IDXCoreAdapterFactory( IDXCoreAdapterFactory *iface )
{
    return CONTAINING_RECORD( iface, struct dxcore_adapter_factory_statics, IDXCoreAdapterFactory_iface );
}

static HRESULT WINAPI dxcore_adapter_factory_statics_QueryInterface( IDXCoreAdapterFactory *iface, REFIID iid, void **out )
{
    struct dxcore_adapter_factory_statics *impl = impl_from_IDXCoreAdapterFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IDXCoreAdapterFactory ))
    {
        *out = &impl->IDXCoreAdapterFactory_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dxcore_adapter_factory_statics_AddRef( IDXCoreAdapterFactory *iface )
{
    struct dxcore_adapter_factory_statics *impl = impl_from_IDXCoreAdapterFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI dxcore_adapter_factory_statics_Release( IDXCoreAdapterFactory *iface )
{
    struct dxcore_adapter_factory_statics *impl = impl_from_IDXCoreAdapterFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT get_adapters( struct dxcore_adapter_list *impl )
{
    IDXGIFactory6 *factory6 = NULL;
    IDXGIAdapter4 *adapter4 = NULL;
    HRESULT hr = CreateDXGIFactory2( 0, &IID_IDXGIFactory6, (void **)&factory6 );

    if (FAILED(hr)) return hr;

    while (SUCCEEDED(IDXGIFactory6_EnumAdapterByGpuPreference( factory6, impl->adapter_count, DXGI_GPU_PREFERENCE_UNSPECIFIED, &IID_IDXGIAdapter4, (void **)&adapter4 )))
    {
        impl->adapter_count++;
        IDXGIAdapter4_Release( adapter4 );
    }

    if (!impl->adapter_count)
    {
        IDXGIFactory6_Release( factory6 );
        return S_OK;
    }

    if (!(impl->adapters = calloc( impl->adapter_count, sizeof( *impl->adapters ) )))
    {
        IDXGIFactory6_Release( factory6 );
        return E_OUTOFMEMORY;
    }

    for (UINT i = 0; i < impl->adapter_count; i++)
    {
        struct dxcore_adapter *adapter = calloc( 1, sizeof( *adapter ) );
        DXGI_ADAPTER_DESC3 desc;

        if (!adapter)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        if (FAILED(hr = IDXGIFactory6_EnumAdapterByGpuPreference( factory6, i, DXGI_GPU_PREFERENCE_UNSPECIFIED, &IID_IDXGIAdapter4, (void **)&adapter4 )))
        {
            break;
        }
        if (FAILED(hr = IDXGIAdapter4_GetDesc3( adapter4, &desc )))
        {
            IDXGIAdapter4_Release( adapter4 );
            break;
        }

        adapter->IDXCoreAdapter_iface.lpVtbl = &dxcore_adapter_vtbl;
        adapter->ref = 1;
        adapter->desc = desc;

        impl->adapters[i] = adapter;
        IDXGIAdapter4_Release( adapter4 );
    }

    IDXGIFactory6_Release( factory6 );
    return hr;
}

static HRESULT WINAPI dxcore_adapter_factory_statics_CreateAdapterList( IDXCoreAdapterFactory *iface, uint32_t num_attributes,
                                                                        const GUID *filter_attributes, REFIID riid, void **ppv )
{
    struct dxcore_adapter_list *impl;
    HRESULT hr;

    FIXME( "iface %p, num_attributes %u, filter_attributes %p, riid %s, ppv %p semi-stub!\n", iface, num_attributes, filter_attributes, debugstr_guid( riid ), ppv );

    if (!ppv) return E_POINTER;
    if (!num_attributes || !filter_attributes)
    {
        *ppv = NULL;
        return E_INVALIDARG;
    }
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->IDXCoreAdapterList_iface.lpVtbl = &dxcore_adapter_list_vtbl;
    impl->ref = 1;
    if (FAILED(hr = get_adapters( impl )))
    {
        IDXCoreAdapterList_Release( &impl->IDXCoreAdapterList_iface );
        return hr;
    }

    hr = IDXCoreAdapterList_QueryInterface( &impl->IDXCoreAdapterList_iface, riid, ppv );
    IDXCoreAdapterList_Release( &impl->IDXCoreAdapterList_iface );
    TRACE( "created IDXCoreAdapterList %p.\n", *ppv );
    return hr;
}

static HRESULT WINAPI dxcore_adapter_factory_statics_GetAdapterByLuid( IDXCoreAdapterFactory *iface, REFLUID adapter_luid, REFIID riid, void **ppv )
{
    FIXME( "iface %p, adapter_luid %p, riid %s, ppv %p stub!\n", iface, adapter_luid, debugstr_guid( riid ), ppv );
    return E_NOTIMPL;
}

static BOOL WINAPI dxcore_adapter_factory_statics_IsNotificationTypeSupported( IDXCoreAdapterFactory *iface, DXCoreNotificationType type )
{
    FIXME( "iface %p, type %u stub!\n", iface, type );
    return FALSE;
}

static HRESULT WINAPI dxcore_adapter_factory_statics_RegisterEventNotification( IDXCoreAdapterFactory *iface, IUnknown *dxcore_object,
                                                                                DXCoreNotificationType type, PFN_DXCORE_NOTIFICATION_CALLBACK callback,
                                                                                void *callback_context, uint32_t *event_cookie )
{
    FIXME( "iface %p, dxcore_object %p, type %u, callback %p, callback_context %p, event_cookie %p stub!\n", iface, dxcore_object, type, callback, callback_context, event_cookie );
    return E_NOTIMPL;
}

static HRESULT WINAPI dxcore_adapter_factory_statics_UnregisterEventNotification( IDXCoreAdapterFactory *iface, uint32_t event_cookie )
{
    FIXME( "iface %p, event_cookie %u stub!\n", iface, event_cookie );
    return E_NOTIMPL;
}

static const struct IDXCoreAdapterFactoryVtbl dxcore_adapter_factory_statics_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_factory_statics_QueryInterface,
    dxcore_adapter_factory_statics_AddRef,
    dxcore_adapter_factory_statics_Release,
    /* IDXCoreAdapterFactory methods */
    dxcore_adapter_factory_statics_CreateAdapterList,
    dxcore_adapter_factory_statics_GetAdapterByLuid,
    dxcore_adapter_factory_statics_IsNotificationTypeSupported,
    dxcore_adapter_factory_statics_RegisterEventNotification,
    dxcore_adapter_factory_statics_UnregisterEventNotification,
};

HRESULT WINAPI DXCoreCreateAdapterFactory( REFIID riid, void **ppv )
{
    static struct dxcore_adapter_factory_statics *impl = NULL;

    TRACE( "riid %s, ppv %p\n", debugstr_guid( riid ), ppv );

    if (!ppv) return E_POINTER;
    if (!impl)
    {
        if (!(impl = calloc( 1, sizeof( *impl ) )))
        {
            *ppv = NULL;
            return E_OUTOFMEMORY;
        }

        impl->IDXCoreAdapterFactory_iface.lpVtbl = &dxcore_adapter_factory_statics_vtbl;
        impl->ref = 0;
    }

    TRACE( "created IDXCoreAdapterFactory %p.\n", *ppv );
    return IDXCoreAdapterFactory_QueryInterface( &impl->IDXCoreAdapterFactory_iface, riid, ppv );
}
