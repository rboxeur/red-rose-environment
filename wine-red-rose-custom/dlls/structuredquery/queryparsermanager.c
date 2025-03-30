/*
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

#define COBJMACROS
#include "private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL( structquery );

struct queryparsermanager
{
    IQueryParserManager iface;
    LONG ref;
};

static inline struct queryparsermanager *impl_from_IQueryParserManager( IQueryParserManager *iface )
{
    return CONTAINING_RECORD( iface, struct queryparsermanager, iface );
}

static HRESULT WINAPI queryparsermanager_QueryInterface( IQueryParserManager *iface, REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    *out = NULL;
    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IQueryParserManager, iid ))
    {
        *out = iface;
        IUnknown_AddRef( iface );
        return S_OK;
    }

    FIXME( "interface not implemented, returning E_NOINTERFACE\n" );
    return E_NOINTERFACE;
}

static ULONG WINAPI queryparsermanager_AddRef( IQueryParserManager *iface )
{
    struct queryparsermanager *impl;
    TRACE( "(%p)\n", iface );

    impl = impl_from_IQueryParserManager( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI queryparsermanager_Release( IQueryParserManager *iface )
{
    struct queryparsermanager *impl;
    ULONG ref;

    TRACE( "(%p)\n", iface );

    impl = impl_from_IQueryParserManager( iface );
    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
        free( impl );
    return ref;
}


static HRESULT WINAPI queryparsermanager_CreateLoadedParser( IQueryParserManager *iface,
                                                             LPCWSTR catalog, LANGID langid,
                                                             REFIID iid, void **out )
{
    FIXME( "(%p, %s, %d, %s, %p) stub!\n", iface, debugstr_w( catalog ), langid,
           debugstr_guid( iid ), out );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparsermanager_InitializeOptions( IQueryParserManager *iface, BOOL nqs,
                                                            BOOL auto_wildcard,
                                                            IQueryParser *parser )
{
    FIXME( "(%p, %d, %d, %p) stub!\n", iface, nqs, auto_wildcard, parser );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparsermanager_SetOption( IQueryParserManager *iface,
                                                    QUERY_PARSER_MANAGER_OPTION opt,
                                                    const PROPVARIANT *val )
{
    FIXME("(%p, %d, %p) stub!\n", iface, opt, val);
    return E_NOTIMPL;
}

const static IQueryParserManagerVtbl queryparsermanager_vtbl =
{
    /* IUnknown */
    queryparsermanager_QueryInterface,
    queryparsermanager_AddRef,
    queryparsermanager_Release,
    /* IQueryParserManager */
    queryparsermanager_CreateLoadedParser,
    queryparsermanager_InitializeOptions,
    queryparsermanager_SetOption,
};

HRESULT queryparsermanager_create( REFIID iid, void **out )
{
    struct queryparsermanager *impl;

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return E_OUTOFMEMORY;
    impl->iface.lpVtbl = &queryparsermanager_vtbl;
    impl->ref = 1;
    *out = &impl->iface;
    return S_OK;
}
