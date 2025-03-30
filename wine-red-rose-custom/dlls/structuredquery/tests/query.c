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
#include <initguid.h>
#include <structuredquery.h>

#include <wine/test.h>

void test_IQueryParser( void )
{
    HRESULT hr;
    IQueryParser *parser = NULL;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    ok( SUCCEEDED( hr ), "got %#lx\n", hr );

    hr = CoCreateInstance( &CLSID_QueryParser, NULL, CLSCTX_INPROC, &IID_IQueryParser,
                           (void **)&parser );
    ok( SUCCEEDED( hr ), "got %#lx\n", hr );

    if (!parser)
    {
        skip( "Could not create IQueryParser instance.\n" );
        CoUninitialize();
        return;
    }

    IQueryParser_Release( parser );
    CoUninitialize();
}

void test_IQueryParserManager( void )
{
    HRESULT hr;
    IQueryParserManager *manager = NULL;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    ok( SUCCEEDED( hr ), "got %#lx\n", hr );

    hr = CoCreateInstance( &CLSID_QueryParserManager, NULL, CLSCTX_INPROC, &IID_IQueryParserManager,
                           (void **)&manager );
    ok( SUCCEEDED( hr ), "got %#lx\n", hr );
    if (!manager)
    {
        skip( "Could not create IQueryParserManager instance.\n" );
        CoUninitialize();
        return;
    }

    IQueryParserManager_Release( manager );
    CoUninitialize();
}

START_TEST(query)
{
    test_IQueryParser();
    test_IQueryParserManager();
}
