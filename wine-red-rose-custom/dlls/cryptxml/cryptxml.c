/* CryptXML Implementation
 *
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

#include "windef.h"
#include "winbase.h"
#include "cryptxml.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptxml);

HRESULT WINAPI CryptXmlOpenToDecode( const CRYPT_XML_TRANSFORM_CHAIN_CONFIG *config, DWORD flags, const CRYPT_XML_PROPERTY *property,
                                     ULONG property_count, const CRYPT_XML_BLOB *blob, HCRYPTXML *handle )
{
    static CRYPT_XML_SIGNED_INFO signed_info = { 0 };
    static CRYPT_XML_KEY_INFO key_info = { 0 };
    static PCRYPT_XML_SIGNATURE signatures[1];
    static CRYPT_XML_SIGNATURE signature;
    static CRYPT_XML_DOC_CTXT *context;

    FIXME( "config %p, flags %lx, property %p, property_count %lu, blob %p, handle %p stub!\n",
            config, flags, property, property_count, blob, handle );

    if (!blob || !handle) return E_INVALIDARG;
    if (!(context = calloc( 1, sizeof( *context ) ))) return E_OUTOFMEMORY;

    signature.cbSize                = sizeof( CRYPT_XML_SIGNATURE );
    signature.hSignature            = (HCRYPTXML)&signature;
    signature.wszId                 = L"";
    signature.SignedInfo            = signed_info;
    signature.SignatureValue.cbData = blob->cbData;
    signature.SignatureValue.pbData = blob->pbData;
    signature.pKeyInfo              = &key_info;
    signature.cObject               = 0;
    signature.rgpObject             = NULL;

    signatures[0] = &signature;

    context->cbSize            = sizeof( *context );
    context->hDocCtxt          = (HCRYPTXML)context;
    context->pTransformsConfig = (CRYPT_XML_TRANSFORM_CHAIN_CONFIG *)config;
    context->cSignature        = 1;
    context->rgpSignature      = signatures;

    *handle = (HCRYPTXML)context;
    return S_OK;
}

HRESULT WINAPI CryptXmlClose( HCRYPTXML handle )
{
    FIXME( "handle %p stub!\n", handle );

    if (!handle) return E_INVALIDARG;

    free( handle );
    return S_OK;
}

HRESULT WINAPI CryptXmlGetDocContext( HCRYPTXML handle, const CRYPT_XML_DOC_CTXT **context )
{
    CRYPT_XML_DOC_CTXT *doc_context;

    FIXME( "handle %p, context %p stub!\n", handle, context );

    if (!handle || !context) return E_INVALIDARG;

    doc_context = (CRYPT_XML_DOC_CTXT *)handle;
    *context = doc_context;
    return S_OK;
}

static BOOL is_handle_signature( HCRYPTXML handle )
{
    CRYPT_XML_SIGNATURE *sig = (CRYPT_XML_SIGNATURE *)handle;
    if (sig->cbSize != sizeof( CRYPT_XML_SIGNATURE )) return FALSE;
    return TRUE;
}

HRESULT WINAPI CryptXmlGetSignature( HCRYPTXML handle, const CRYPT_XML_SIGNATURE **signature )
{
    CRYPT_XML_SIGNATURE *sig;

    FIXME( "handle %p, signature %p stub!\n", handle, signature );

    if (!handle || !signature) return E_INVALIDARG;
    if (!(is_handle_signature( handle )))
    {
        *signature = NULL;
        return CRYPT_XML_E_HANDLE;
    }

    sig = (CRYPT_XML_SIGNATURE *)handle;
    *signature = sig;
    return S_OK;
}

HRESULT WINAPI CryptXmlGetStatus( HCRYPTXML handle, CRYPT_XML_STATUS *status )
{
    CRYPT_XML_STATUS ret_status;

    FIXME( "handle %p, status %p stub!\n", handle, status );

    if (!handle || !status) return E_INVALIDARG;

    ret_status.cbSize        = sizeof( CRYPT_XML_STATUS );
    ret_status.dwErrorStatus = CRYPT_XML_STATUS_NO_ERROR;
    ret_status.dwInfoStatus  = CRYPT_XML_STATUS_SIGNATURE_VALID;

    *status = ret_status;
    return S_OK;
}

HRESULT WINAPI CryptXmlVerifySignature( HCRYPTXML handle, BCRYPT_KEY_HANDLE key, DWORD flags )
{
    FIXME( "handle %p, key %p, flags %lx, stub!\n", handle, key, flags );

    if (!handle) return E_INVALIDARG;
    if (!(is_handle_signature( handle ))) return CRYPT_XML_E_HANDLE;

    return S_OK;
}
