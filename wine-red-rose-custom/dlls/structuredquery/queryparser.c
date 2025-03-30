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

struct queryparser
{
    IQueryParser iface;
    PROPVARIANT options[SQSO_CONNECTOR_CASE];
    LONG ref;
};

static inline struct queryparser *impl_from_IQueryParser( IQueryParser *iface )
{
    return CONTAINING_RECORD( iface, struct queryparser, iface );
}

static HRESULT WINAPI queryparser_QueryInterface( IQueryParser *iface, REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    *out = NULL;
    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IQueryParser, iid ))
    {
        *out = iface;
        IUnknown_AddRef( iface );
        return S_OK;
    }

    FIXME( "interface not implemented, returning E_NOINTERFACE\n" );
    return E_NOINTERFACE;
}

static ULONG WINAPI queryparser_AddRef( IQueryParser *iface )
{
    struct queryparser *impl;
    TRACE( "(%p)\n", iface );

    impl = impl_from_IQueryParser( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI queryparser_Release( IQueryParser *iface )
{
    struct queryparser *impl;
    ULONG ref;

    TRACE( "(%p)\n", iface );

    impl = impl_from_IQueryParser( iface );
    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
        free( impl );
    return ref;
}

static HRESULT WINAPI queryparser_Parse( IQueryParser *iface, LPCWSTR input,
                                         IEnumUnknown *custom_props, IQuerySolution **solution )
{
    FIXME( "(%p, %s, %p, %p) stub!\n", iface, debugstr_w( input ), custom_props, solution );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_SetOption( IQueryParser *iface,
                                             STRUCTURED_QUERY_SINGLE_OPTION option,
                                             const PROPVARIANT *val )
{
    FIXME( "(%p, %d, %p) stub!\n", iface, option, val );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_GetOption( IQueryParser *iface,
                                             STRUCTURED_QUERY_SINGLE_OPTION option,
                                             PROPVARIANT *val )
{
    FIXME( "(%p, %d, %p) stub!\n", iface, option, val );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_SetMultiOption( IQueryParser *iface,
                                                  STRUCTURED_QUERY_MULTIOPTION opt, LPCWSTR key,
                                                  const PROPVARIANT *val )
{
    FIXME( "(%p, %d, %s, %p) stub!\n", iface, opt, debugstr_w( key ), val );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_GetSchemaProvider( IQueryParser *iface,
                                                     ISchemaProvider **provider )
{
    FIXME( "(%p, %p) stub!\n", iface, provider );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_RestateToString( IQueryParser *iface, ICondition *cond,
                                                   BOOL english, LPWSTR *query )
{
    FIXME( "(%p, %p, %d, %p) stub!\n", iface, cond, english, query );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_ParsePropertyValue( IQueryParser *iface, LPCWSTR property,
                                                      LPCWSTR input, IQuerySolution **solution )
{
    FIXME( "(%p, %s, %s, %p) stub!\n", iface, debugstr_w( property ), debugstr_w( input ),
           solution );
    return E_NOTIMPL;
}

static HRESULT WINAPI queryparser_RestatePropertyValueToString( IQueryParser *iface,
                                                                ICondition *cond, BOOL english,
                                                                LPWSTR *name, LPWSTR *query )
{
    FIXME( "(%p, %p, %d, %p, %p) stub!\n", iface, cond, english, name, query );
    return E_NOTIMPL;
}

const static IQueryParserVtbl queryparser_vtbl =
{
    /* IUnknown */
    queryparser_QueryInterface,
    queryparser_AddRef,
    queryparser_Release,
    /* IQueryParser */
    queryparser_Parse,
    queryparser_SetOption,
    queryparser_GetOption,
    queryparser_SetMultiOption,
    queryparser_GetSchemaProvider,
    queryparser_RestateToString,
    queryparser_ParsePropertyValue,
    queryparser_RestatePropertyValueToString,
};

HRESULT queryparser_create( REFIID iid, void **out )
{
    struct queryparser *impl;

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return E_OUTOFMEMORY;
    impl->iface.lpVtbl = &queryparser_vtbl;
    impl->ref = 1;
    *out = &impl->iface;
    return S_OK;
}
