/*
 * iyuv Video "Decoder"
 * Copyright 2026 Brendan McGrath for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "vfw.h"
#include "initguid.h"
#include "wmcodecdsp.h"
#include "iyuv_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(iyuv_32);

static HINSTANCE IYUV_32_hModule;

static LRESULT IYUV_Open( const ICINFO *icinfo )
{
    FIXME("DRV_OPEN %p\n", icinfo);

    return 0;
}

static LRESULT IYUV_DecompressQuery( const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out )
{
    FIXME("ICM_DECOMPRESS_QUERY %p %p\n", in, out);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_DecompressGetFormat( BITMAPINFOHEADER *in, BITMAPINFOHEADER *out )
{
    FIXME("ICM_DECOMPRESS_GETFORMAT %p %p\n", in, out);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_DecompressBegin( IMFTransform *transform, const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out )
{
    FIXME("ICM_DECOMPRESS_BEGIN %p %p %p\n", transform, in, out);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_Decompress( IMFTransform *transform, const ICDECOMPRESS *params )
{
    FIXME("ICM_DECOMPRESS %p %p\n", transform, params);

    return ICERR_UNSUPPORTED;
}

static LRESULT IYUV_GetInfo( ICINFO *icinfo, DWORD dwSize )
{
    FIXME("ICM_GETINFO %p %lu\n", icinfo, dwSize);

    return ICERR_UNSUPPORTED;
}

/***********************************************************************
 *              DriverProc (IYUV_32.@)
 */
LRESULT WINAPI IYUV_DriverProc( DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
                                LPARAM lParam1, LPARAM lParam2 )
{
    IMFTransform *transform = (IMFTransform *) dwDriverId;
    LRESULT r = ICERR_UNSUPPORTED;

    TRACE("%Id %p %04x %08Ix %08Ix\n", dwDriverId, hdrvr, msg, lParam1, lParam2);

    switch( msg )
    {
    case DRV_LOAD:
        TRACE("DRV_LOAD\n");
        r = TRUE;
        break;

    case DRV_OPEN:
        r = IYUV_Open((ICINFO *)lParam2);
        break;

    case DRV_CLOSE:
        TRACE("DRV_CLOSE\n");
        if ( transform )
            IMFTransform_Release( transform );
        r = TRUE;
        break;

    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
        break;

    case ICM_GETINFO:
        r = IYUV_GetInfo( (ICINFO *) lParam1, (DWORD) lParam2 );
        break;

    case ICM_DECOMPRESS_QUERY:
        r = IYUV_DecompressQuery( (BITMAPINFOHEADER *)lParam1, (BITMAPINFOHEADER *)lParam2 );
        break;

    case ICM_DECOMPRESS_GET_FORMAT:
        r = IYUV_DecompressGetFormat( (BITMAPINFOHEADER*) lParam1, (BITMAPINFOHEADER*) lParam2 );
        break;

    case ICM_DECOMPRESS_GET_PALETTE:
        FIXME("ICM_DECOMPRESS_GET_PALETTE\n");
        break;

    case ICM_DECOMPRESS:
        r = IYUV_Decompress( transform, (ICDECOMPRESS *)lParam1 );
        break;

    case ICM_DECOMPRESS_BEGIN:
        r = IYUV_DecompressBegin( transform, (BITMAPINFOHEADER *)lParam1, (BITMAPINFOHEADER *)lParam2 );
        break;

    case ICM_DECOMPRESS_END:
        r = ICERR_OK;
        break;

    case ICM_DECOMPRESSEX_QUERY:
    case ICM_DECOMPRESSEX_BEGIN:
    case ICM_DECOMPRESSEX:
    case ICM_DECOMPRESSEX_END:
        /* unsupported */
        break;

    case ICM_COMPRESS_QUERY:
        r = ICERR_BADFORMAT;
        /* fall through */
    case ICM_COMPRESS_GET_FORMAT:
    case ICM_COMPRESS_END:
    case ICM_COMPRESS:
        FIXME("compression not implemented\n");
        break;

    case ICM_CONFIGURE:
        break;

    default:
        FIXME("Unknown message: %04x %Id %Id\n", msg, lParam1, lParam2);
    }

    return r;
}

/***********************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    TRACE("(%p,%lu,%p)\n", hModule, dwReason, lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        IYUV_32_hModule = hModule;
        break;
    }
    return TRUE;
}
