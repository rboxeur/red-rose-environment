/* CryptXML Tests
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

#include "wine/test.h"

static void test_validate_signature(void)
{
    static const BYTE signature[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Envelope xmlns=\"urn:envelope\">"
    "<Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\">"
    "<SignedInfo>"
    "<CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments\"/>"
    "<SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#dsa-sha1\"/>"
    "<Reference URI=\"\">"
    "<Transforms>"
    "<Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/>"
    "</Transforms>"
    "<DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"/>"
    "<DigestValue>uooqbWYa5VCqcJCbuymBKqm17vY=</DigestValue>"
    "</Reference>"
    "</SignedInfo>"
    "<SignatureValue>"
    "KedJuTob5gtvYx9qM3k3gm7kbLBwVbEQRl26S2tmXjqNND7MRGtoew=="
    "</SignatureValue>"
    "<KeyInfo>"
    "<KeyValue>"
    "<DSAKeyValue>"
    "<P>/KaCzo4Syrom78z3EQ5SbbB4sF7ey80etKII864WF64B81uRpH5t9jQTxeEu0ImbzRMqzVDZkVG9xD7nN1kuFw==</P>"
    "<Q>li7dzDacuo67Jg7mtqEm2TRuOMU=</Q>"
    "<G>Z4Rxsnqc9E7pGknFFH2xqaryRPBaQ01khpMdLRQnG541Awtx/XPaF5Bpsy4pNWMOHCBiNU0NogpsQW5QvnlMpA==</G>"
    "<Y>qV38IqrWJG0V/mZQvRVi1OHw9Zj84nDC4jO8P0axi1gb6d+475yhMjSc/BrIVC58W3ydbkK+Ri4OKbaRZlYeRA==</Y>"
    "</DSAKeyValue>"
    "</KeyValue>"
    "</KeyInfo>"
    "</Signature>"
    "</Envelope>";
    const CRYPT_XML_SIGNATURE *sig;
    const CRYPT_XML_DOC_CTXT *doc;
    HCRYPTXML handle = NULL;
    CRYPT_XML_STATUS status;
    CRYPT_XML_BLOB blob;
    HRESULT hr;

    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    handle = (HCRYPTXML)0xdeadbeef;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, NULL, &handle );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( handle == (HCRYPTXML)0xdeadbeef, "got handle %p.\n", handle );

    blob.dwCharset = CRYPT_XML_CHARSET_UTF8;
    blob.cbData = strlen((const char *)signature) - 1;
    blob.pbData = (BYTE *)signature;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, &blob, &handle );
    todo_wine
    ok( hr == WS_E_INVALID_FORMAT, "got hr %#lx.\n", hr );
    todo_wine
    ok( handle == NULL, "got handle %p.\n", handle );

    hr = CryptXmlClose( NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    blob.dwCharset = CRYPT_XML_CHARSET_UTF8;
    blob.cbData = strlen( (const char *)signature );
    blob.pbData = (BYTE *)signature;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, &blob, &handle );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = CryptXmlGetStatus( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetStatus( NULL, &status );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    hr = CryptXmlGetDocContext( NULL, &doc );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetDocContext( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetDocContext( handle, &doc );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( doc->cSignature == 1, "got signature count %lu\n", doc->cSignature );
    ok( doc->rgpSignature != NULL, "got NULL rgpSignature\n" );

    hr = CryptXmlGetSignature( NULL, &sig );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetSignature( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetSignature( handle, &sig );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );
    ok( sig == NULL, "got sig %p\n", sig );

    hr = CryptXmlGetSignature( doc->rgpSignature[0]->hSignature, &sig );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( sig != NULL, "failed to get signature\n" );

    status.cbSize = 0xdeadbeef;
    status.dwErrorStatus = 0xdeadbeef;
    status.dwInfoStatus = 0xdeadbeef;
    hr = CryptXmlGetStatus( handle, &status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( status.cbSize == sizeof( CRYPT_XML_STATUS ), "got status.cbSize %ld.\n", status.cbSize );
    todo_wine
    ok( status.dwErrorStatus == CRYPT_XML_STATUS_ERROR_NOT_RESOLVED, "got status.dwErrorStatus %ld.\n", status.dwErrorStatus );
    todo_wine
    ok( status.dwInfoStatus == 0, "got status.dwInfoStatus %ld.\n", status.dwInfoStatus );

    hr = CryptXmlVerifySignature( NULL, NULL, 0 );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlVerifySignature( handle, NULL, 0 );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );
    hr = CryptXmlVerifySignature( handle, sig->pKeyInfo->hVerifyKey, 0 );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );

    hr = CryptXmlVerifySignature( sig->hSignature, sig->pKeyInfo->hVerifyKey, 0 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    status.cbSize = 0xdeadbeef;
    status.dwErrorStatus = 0xdeadbeef;
    status.dwInfoStatus = 0xdeadbeef;
    hr = CryptXmlGetStatus( handle, &status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( status.cbSize == sizeof( CRYPT_XML_STATUS ), "got status.cbSize %ld.\n", status.cbSize );
    todo_wine
    ok( status.dwErrorStatus == CRYPT_XML_STATUS_ERROR_DIGEST_INVALID, "got status.dwErrorStatus %ld.\n", status.dwErrorStatus );
    ok( status.dwInfoStatus == CRYPT_XML_STATUS_SIGNATURE_VALID, "got status.dwInfoStatus %ld.\n", status.dwInfoStatus );

    CryptXmlClose( handle );
}

START_TEST(cryptxml)
{
    test_validate_signature();
}
