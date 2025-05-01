/*
 * WLAN settings profile
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

#define COBJMACROS

#include <windef.h>
#include <wlanapi.h>
#include <shlwapi.h>
#include <initguid.h>
#include <xmllite.h>
#include <msxml.h>
#include <msxml2.h>

#include <wine/debug.h>

#include "profile.h"

WINE_DEFAULT_DEBUG_CHANNEL( wlanapi );

static DWORD hresult_to_win32_code( HRESULT hr )
{
    if (HRESULT_SEVERITY( hr ) == SEVERITY_ERROR &&
        HRESULT_FACILITY( hr ) == FACILITY_WIN32)
    {
        return HRESULT_CODE( hr );
    }

    return ERROR_INTERNAL_ERROR;
}

#define FACILITY_WLANAPI 0xaef
#define MAKE_WLANERROR( code ) MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WLANAPI, (code) )

static BSTR read_element_text( IXMLDOMNode *elem )
{
    DOMNodeType node_type;
    BSTR text;
    IXMLDOMNode *child;
    HRESULT res;

    res = IXMLDOMNode_get_firstChild( elem, &child );
    if (res != S_OK)
        return NULL;

    IXMLDOMNode_get_nodeType( child, &node_type );
    if (node_type != NODE_TEXT)
    {
        IXMLDOMNode_Release( elem );
        return NULL;
    }

    if (IXMLDOMNode_get_text( child, &text ) != S_OK)
    {
        IXMLDOMNode_Release( elem );
        return NULL;
    }

    IXMLDOMNode_Release( elem );
    return text;
}

enum bool_val_type
{
    bool_val_type_true,
    bool_val_type_enabled,
};

static HRESULT read_element_bool( IXMLDOMNode *node, const char *elem_name, BOOL *val, enum bool_val_type type )
{
    BSTR str;
    BOOL invalid = FALSE;

    str = read_element_text( node );
    if (!str)
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );

    switch (type)
    {
        case bool_val_type_true:
            if (!wcsicmp( str, L"true" ))
                *val = TRUE;
            else if (!wcsicmp( str, L"false" ))
                *val = FALSE;
            else
                invalid = TRUE;
            break;
        case bool_val_type_enabled:
            if (!(wcsicmp( str, L"enabled" )))
                *val = TRUE;
            else if (!wcsicmp( str, L"disabled" ))
                *val = FALSE;
            else
                invalid = TRUE;
    }

    if (invalid)
    {
        ERR( "invalid %s value: %s", elem_name, debugstr_w( str ) );
        SysFreeString( str );
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
    }

    SysFreeString( str );
    return S_OK;
}

static HRESULT read_element_int( IXMLDOMNode *node, const char *elem_name, INT *val )
{
    BSTR str;

    str = read_element_text( node );
    if (!str)
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );

    if (!swscanf( str, L"%d", val ))
    {
        ERR( "invalid %s value: %s\n", elem_name, debugstr_w( str ) );
        SysFreeString( str );
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
    }

    SysFreeString( str );
    return S_OK;
}

static HRESULT read_element_dword( IXMLDOMNode *node, const char *elem_name, DWORD *val )
{
    BSTR str;

    str = read_element_text( node );
    if (!str)
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );

    if (!swscanf( str, L"%lu", val ))
    {
        ERR( "invalid %s value: %s\n", elem_name, debugstr_w( str ) );
        SysFreeString( str );
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
    }

    SysFreeString( str );
    return S_OK;
}

static HRESULT read_SSID( IXMLDOMNode *node, DOT11_SSID *ssid )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;
    BOOL have_ssid = FALSE;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
         IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );
        if (!wcsicmp( elem_name, L"name" ))
        {
            BSTR nameW = read_element_text( child );
            if (nameW)
            {
                SIZE_T len = wcslen( nameW );
                if (len > sizeof( ssid->ucSSID ))
                {
                    ERR("invalid SSID name: %s\n", debugstr_w( nameW ));
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_SSID_INVALID );
                }
                else
                {
                    wcstombs( (char *)ssid->ucSSID, nameW, sizeof( ssid->ucSSID ) );
                    ssid->uSSIDLength = len;
                    have_ssid = TRUE;
                }
                SysFreeString( nameW );
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        else if (!wcsicmp( elem_name, L"hex" ))
        {
            BSTR hex_str = read_element_text( child );
            if (hex_str)
            {
                SIZE_T len = wcslen( hex_str );
                if (len % 2 != 0 || len / 2 > sizeof( ssid->ucSSID ))
                {
                    ERR( "invalid SSID hex value: %s\n", debugstr_w( hex_str ) );
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_SSID_INVALID );
                }
                else
                {
                    SIZE_T i;
                    for (i = 0; i < len; i += 2)
                    {
                        int byte;
                        if (!swscanf( &hex_str[i], L"%2x", &byte ))
                        {
                            ERR( "invalid SSID hex value: %s\n", debugstr_w( hex_str ) );
                            res = MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_SSID_INVALID );
                            break;
                        }
                        ssid->ucSSID[i] = byte;
                    }
                    ssid->uSSIDLength = len/2;
                    have_ssid = SUCCEEDED( res );
                }
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
            SysFreeString( hex_str );
        }
        else
        {
            ERR( "Uknonwn SSID element: %s\n", debugstr_w( elem_name ) );
            res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }

        SysFreeString( elem_name );
        IXMLDOMNode_Release( node );
        break;
    }

    IXMLDOMNodeList_Release( children );
    if (FAILED ( res ))
        return res;
    return have_ssid ? S_OK : MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
}

static HRESULT read_SSIDConfig( IXMLDOMNode *node, struct wlan_profile_ssid_config *ssid_config )
{
    HRESULT res = S_OK;
    BOOL have_ssid = FALSE;
    IXMLDOMNodeList *children;
    LONG len = 0, i;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );
        if (!wcsicmp( elem_name, L"nonBroadcast" ))
        {
            res = read_element_bool( node, "nonBroadcast", &ssid_config->non_broadcast,
                                     bool_val_type_true );
            ssid_config->non_broadcast_set = TRUE;
        }
        else if (!wcsicmp( elem_name, L"SSID" ))
        {
            res = read_SSID( child, &ssid_config->ssid );
            have_ssid = TRUE;
        }
        else if (!wcsicmp( elem_name, L"SSIDPrefix" ))
        {
            res = read_SSID( child, &ssid_config->ssid_prefix );
            have_ssid = TRUE;
        }

        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    if (FAILED( res ))
        return res;
    return have_ssid ? S_OK : MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_SSID_INVALID );
}

const static struct
{
    const WCHAR *str;
    DOT11_AUTH_ALGORITHM alg;
} AUTH_ALGS[] = {
    {L"open", DOT11_AUTH_ALGO_80211_OPEN},
    {L"shared", DOT11_AUTH_ALGO_80211_SHARED_KEY},
    {L"WPA", DOT11_AUTH_ALGO_WPA},
    {L"WPAPSK", DOT11_AUTH_ALGO_WPA_PSK},
    {L"WPA2", DOT11_AUTH_ALGO_RSNA},
    {L"WPA2PSK", DOT11_AUTH_ALGO_RSNA_PSK},
    {L"WPA3", DOT11_AUTH_ALGO_WPA3},
    {L"WPA3ENT192", DOT11_AUTH_ALGO_WPA3_ENT_192},
    {L"WPA3ENT", DOT11_AUTH_ALGO_WPA3_ENT},
    {L"WPA3SAE", DOT11_AUTH_ALGO_WPA3_SAE},
    {L"OWE", DOT11_AUTH_ALGO_OWE}
};

const static struct
{
    const WCHAR *str;
    DOT11_CIPHER_ALGORITHM alg;
} CIPHER_ALGS[] = {
    {L"none", DOT11_CIPHER_ALGO_NONE},
    {L"WEP", DOT11_CIPHER_ALGO_WEP},
    {L"TKIP", DOT11_CIPHER_ALGO_TKIP},
    {L"AES", DOT11_CIPHER_ALGO_CCMP},
    {L"GCMP256", DOT11_CIPHER_ALGO_GCMP_256}
};

static HRESULT read_authEncryption( IXMLDOMNode *node, struct wlan_profile_security *security )
{
    BOOL have_auth = FALSE, have_cipher = FALSE;
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );

        if (!wcsicmp( elem_name, L"authentication" ))
        {
            BSTR alg;
            alg = read_element_text( child );
            if (alg)
            {
                SIZE_T i;
                for (i = 0; i < ARRAY_SIZE( AUTH_ALGS ); i++)
                {
                    if (!wcsicmp( alg, AUTH_ALGS[i].str ))
                    {
                        security->authentication = AUTH_ALGS[i].alg;
                        break;
                    }
                }
                if (i == ARRAY_SIZE( AUTH_ALGS ))
                {
                    ERR( "unknown authentication value: %s\n", debugstr_w( alg ));
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_MSMSEC_PROFILE_UNSUPPORTED_AUTH );
                }
                SysFreeString( alg );
                have_auth = TRUE;
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        else if (!wcsicmp( elem_name, L"encryption" ))
        {
            BSTR alg;
            alg = read_element_text( child );
            if (alg)
            {
                SIZE_T i;
                for (i = 0; i < ARRAY_SIZE( CIPHER_ALGS ); i++)
                {
                    if (!wcsicmp( alg, CIPHER_ALGS[i].str ))
                    {
                        security->encryption = CIPHER_ALGS[i].alg;
                        break;
                    }
                }
                if (i == ARRAY_SIZE( CIPHER_ALGS ))
                {
                    ERR( "unknown encryption value: %s\n", debugstr_w( alg ) );
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_MSMSEC_PROFILE_UNSUPPORTED_CIPHER );
                }
                SysFreeString( alg );
                have_cipher = TRUE;
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        else if (!wcsicmp( elem_name, L"useOneX" ))
            res = read_element_bool( node, "useOneX", &security->use_onex, bool_val_type_true );
        else if (!wcsicmp( elem_name, L"transitionMode" ))
            res = read_element_bool( node, "transitionMode", &security->transition_mode, bool_val_type_true );
        else
            ERR("unknown element: %s\n", debugstr_w( elem_name ));

        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    if (FAILED( res ))
        return res;

    return have_auth && have_cipher
               ? S_OK
               : MAKE_WLANERROR( WLAN_REASON_CODE_MSMSEC_PROFILE_NO_AUTH_CIPHER_SPECIFIED );
}

static HRESULT read_sharedKey( IXMLDOMNode *node, struct wlan_shared_key *shared_key )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;
    BOOL have_type = FALSE, have_protected = FALSE, have_material = FALSE;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );
    if (len < 3)
    {
        IXMLDOMNodeList_Release( children );
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
    }

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );

        if (!wcsicmp( elem_name, L"keyType" ))
        {
            BSTR type = read_element_text( child );
            if (child)
            {
                if (!wcsicmp( type, L"networkKey" ))
                    shared_key->key_type = WLANAPI_KEY_TYPE_NETWORK_KEY;
                else if (!wcsicmp( type, L"passPhrase" ))
                    shared_key->key_type = WLANAPI_KEY_TYPE_PASSPHRASE;
                else
                {
                    ERR( "unknown keyType value: %s\n", debugstr_w( type ) );
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
                }
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
            SysFreeString( type );
        }
        else if (!wcsicmp( elem_name, L"protected" ))
            res = read_element_bool( child, "protected", &shared_key->protected, bool_val_type_true );
        else if (!wcsicmp( elem_name, L"keyMaterial" ))
        {
            BSTR key = read_element_text( child );
            if (key)
            {
                if (len > sizeof( shared_key->key_material ) - 1)
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
                else
                    wcscpy( shared_key->key_material, key );
                SysFreeString( key );
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    if (FAILED( res ))
        return res;

    return have_type && have_protected && have_material
               ? S_OK
               : MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
}

static HRESULT read_msm_security( IXMLDOMNode *node, struct wlan_profile_security *security )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );

        if (!wcsicmp( elem_name, L"authEncryption" ))
            res = read_authEncryption( child, security );
        else if (!wcsicmp( elem_name, L"sharedKey" ))
        {
            if (security->keys_len == 4)
            {
                FIXME( "More than 4 shared keys are not supported.\n" );
                res = MAKE_WLANERROR( WLAN_REASON_CODE_MSMSEC_PROFILE_UNSUPPORTED_AUTH );
            }
            else
                res = read_sharedKey( child, &security->shared_key[security->keys_len++] );
        }
        else if (!wcsicmp( elem_name, L"keyIndex" ))
        {
            res = read_element_int( child, "keyIndex", &security->key_index );
            if (!(security->key_index >= 0 && security->key_index <= 3) ||
                security->key_index >= security->keys_len)
                res = MAKE_WLANERROR( WLAN_REASON_CODE_MSMSEC_PROFILE_INVALID_KEY_INDEX );
        }

        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    return res;
}

static HRESULT read_MSM( IXMLDOMNode *node, struct wlan_profile_msm *msm )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );

        if (!wcsicmp( elem_name, L"security" ))
        {
            msm->security_enabled = TRUE;
            res = read_msm_security( child, &msm->security );
        }

        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    return res;
}

static HRESULT read_MacRandomization( IXMLDOMNode *node, struct wlan_profile_mac_randomization *rand )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );

    for (i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        BSTR elem_name;
        DOMNodeType node_type;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );
        if (!wcsicmp( elem_name, L"enableRandomization" ))
            res = read_element_bool( child, "enableRandomization", &rand->enable_randomization, bool_val_type_true );
        else if (!wcsicmp( elem_name, L"randomizeEveryday" ))
            res = read_element_bool( child, "randomizeEveryday", &rand->enable_randomization, bool_val_type_true );
        else if (!wcsicmp( elem_name, L"randomizationSeed" ))
            res = read_element_dword( node, "randomizationSeed", &rand->randomization_seed );
        else
        {
            ERR( "unknown MacRandomization element: %s\n", debugstr_w( elem_name ) );
            res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

    IXMLDOMNodeList_Release( children );
    return res;
}

static HRESULT read_WLANProfile( IXMLDOMNode *node, struct wlan_profile_data *profile )
{
    HRESULT res = S_OK;
    IXMLDOMNodeList *children;
    LONG len = 0, i;
    BOOL have_name = FALSE, have_msm = FALSE, have_ssidconfig = FALSE;

    IXMLDOMNode_get_childNodes( node, &children );
    IXMLDOMNodeList_get_length( children, &len );
    if (!len)
    {
        res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        goto done;
    }

    for(i = 0; i < len && SUCCEEDED( res ); i++)
    {
        IXMLDOMNode *child;
        DOMNodeType node_type;
        BSTR elem_name;

        IXMLDOMNodeList_get_item( children, i, &child );
        IXMLDOMNode_get_nodeType( child, &node_type );
        if (node_type != NODE_ELEMENT)
        {
            IXMLDOMNode_Release( child );
            continue;
        }

        IXMLDOMNode_get_nodeName( child, &elem_name );
        if (!wcsicmp( elem_name, L"name" ))
        {
            BSTR name = read_element_text( child );
            if (name)
            {
                SIZE_T len = wcslen( name );
                if (len > sizeof( profile->name ) - 1)
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_NAME );
                else
                {
                    wcstombs( profile->name, name, sizeof( profile->name ) - 1);
                    profile->name[sizeof( profile->name ) - 1] = '\0';
                    have_name = TRUE;
                }
                SysFreeString( name );
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_NAME );
        }
        else if (!wcsicmp( elem_name, L"connectionType" ))
        {
            BSTR conn_type = read_element_text( child );
            if (conn_type)
            {
                if (!wcsicmp( conn_type, L"ESS" ))
                    profile->connection_type = dot11_BSS_type_infrastructure;
                else if (!wcsicmp( conn_type, L"IBSS" ))
                    profile->connection_type = dot11_BSS_type_independent;
                else
                {
                    ERR( "invalid connectionType value: %s", debugstr_w( conn_type ) );
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_BSS_TYPE );
                }
                SysFreeString( conn_type );
            }
            else
                res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_BSS_TYPE );
        }
        else if (!wcsicmp( elem_name, L"connectionMode" ))
        {
            BSTR conn_mode = read_element_text( child );
            if (conn_mode)
            {
                if (!wcsicmp( conn_mode, L"auto" ))
                    profile->connection_mode = WLANAPI_PROFILE_MODE_AUTO;
                else if (!wcsicmp( conn_mode, L"manual" ))
                    profile->connection_mode = WLANAPI_PROFILE_MODE_MANUAL;
                else
                {
                    ERR( "invalid connectionMode value: %s", debugstr_w( conn_mode ));
                    res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
                }
                SysFreeString( conn_mode );
            }
            else
                res =  MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        }
        else if (!wcsicmp( elem_name, L"autoSwitch" ))
            res = read_element_bool( child, "autoSwitch", &profile->auto_switch, bool_val_type_true );
        else if (!wcsicmp( elem_name, L"MacRandomization" ))
            res = read_MacRandomization( node, &profile->mac_randomization );
        else if (!wcsicmp( elem_name, L"MSM" ))
        {
            res = read_MSM( child, &profile->msm );
            have_msm = TRUE;
        }
        else if (!wcsicmp( elem_name, L"SSIDConfig" ))
        {
            if (have_ssidconfig)
            {
                FIXME( " Multiple SSIDConfig elements are not supported\n" );
                res = MAKE_WLANERROR( WLAN_REASON_CODE_TOO_MANY_SSID );
            }
            else
            {
                have_ssidconfig = TRUE;
                res = read_SSIDConfig( child, &profile->ssid_config );
            }
        }

        SysFreeString( elem_name );
        IXMLDOMNode_Release( child );
    }

done:
    IXMLDOMNodeList_Release( children );
    if (FAILED( res ))
        return res;
    if (!have_name)
        return MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
    if (!have_msm)
        return MAKE_WLANERROR( WLAN_REASON_CODE_MSM_SECURITY_MISSING );
    if (!have_ssidconfig)
        return MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_SSID_INVALID );

    return S_OK;
}


static DWORD wlan_profile_validate( const struct wlan_profile_data *profile )
{
    int i;
    if (profile->connection_type == dot11_BSS_type_independent)
    {
        if (profile->connection_mode != WLANAPI_PROFILE_MODE_MANUAL)
            return WLAN_REASON_CODE_INVALID_ADHOC_CONNECTION_MODE;
        if (profile->auto_switch)
            return WLAN_REASON_CODE_AUTO_SWITCH_SET_FOR_ADHOC;
        if (profile->ssid_config.non_broadcast)
            return WLAN_REASON_CODE_NON_BROADCAST_SET_FOR_ADHOC;
    }
    if (profile->connection_mode == WLANAPI_PROFILE_MODE_MANUAL && profile->auto_switch)
        return WLAN_REASON_CODE_AUTO_SWITCH_SET_FOR_MANUAL_CONNECTION;

    for (i = 0; i < profile->msm.security.keys_len; i++)
    {
        if (profile->msm.security.shared_key[i].protected)
        {
            FIXME( "Encrypted keys are not supported\n" );
            return WLAN_REASON_CODE_MSMSEC_PROFILE_UNSUPPORTED_AUTH;
        }
    }

    switch (profile->msm.security.authentication)
    {
        case DOT11_AUTH_ALGO_80211_SHARED_KEY:
        case DOT11_AUTH_ALGO_80211_OPEN:
        {
            int i;

            if (profile->msm.security.encryption != DOT11_CIPHER_ALGO_WEP)
                return WLAN_REASON_CODE_MSMSEC_PROFILE_INVALID_AUTH_CIPHER;

            if (!profile->msm.security.keys_len)
                return WLAN_REASON_CODE_SECURITY_MISSING;

            for (i = 0; i < profile->msm.security.keys_len; i++)
            {
                const WCHAR *key = profile->msm.security.shared_key[i].key_material;
                size_t len = wcslen( key );

                if (profile->msm.security.shared_key[i].key_type != WLANAPI_KEY_TYPE_NETWORK_KEY)
                    return WLAN_REASON_CODE_MSMSEC_PROFILE_WRONG_KEYTYPE;
                if (len == 5 || len == 13)
                {
                    for (i = 0; i < len; i++)
                        if (!iswascii( key[i] ))
                            return WLAN_REASON_CODE_MSMSEC_PROFILE_KEYMATERIAL_CHAR;
                }
                if (len == 10 || len == 26)
                {
                    for (i = 0; i < len; i++)
                        if (!iswxdigit( key[i] ))
                            return WLAN_REASON_CODE_MSMSEC_PROFILE_KEYMATERIAL_CHAR;
                }
                else if (len >= 8 && len <= 63)
                    return WLAN_REASON_CODE_MSMSEC_PROFILE_PSK_PRESENT;
                else
                    return WLAN_REASON_CODE_MSMSEC_PROFILE_KEY_LENGTH;
            }

            break;
        }
        case DOT11_AUTH_ALGO_WPA_PSK:
        case DOT11_AUTH_ALGO_RSNA_PSK:
        {
            int i;

            if (!profile->msm.security.keys_len)
                return WLAN_REASON_CODE_SECURITY_MISSING;
            switch (profile->msm.security.encryption)
            {
                case DOT11_CIPHER_ALGO_TKIP:
                case DOT11_CIPHER_ALGO_CCMP:
                    break;
                default:
                    return WLAN_REASON_CODE_MSMSEC_PROFILE_INVALID_AUTH_CIPHER;
            }

            for (i = 0; i < profile->msm.security.keys_len; i++)
            {
                const WCHAR *key = profile->msm.security.shared_key[i].key_material;
                size_t len = wcslen( key );

                switch (profile->msm.security.shared_key[i].key_type)
                {
                    case WLANAPI_KEY_TYPE_NETWORK_KEY:
                        if (len >= 8 && len <= 63)
                            return WLAN_REASON_CODE_MSMSEC_PROFILE_PSK_PRESENT;
                        else if (len != 64)
                            return WLAN_REASON_CODE_MSMSEC_PROFILE_KEY_LENGTH;
                        for (i = 0; i < len; i++)
                            if (!iswxdigit( key[i] ))
                                return WLAN_REASON_CODE_MSMSEC_PROFILE_KEYMATERIAL_CHAR;

                    case WLANAPI_KEY_TYPE_PASSPHRASE:
                        if (!(len >= 8 && len <= 63))
                            return WLAN_REASON_CODE_MSMSEC_PROFILE_PSK_LENGTH;
                        for (i = 0; i < len; i++)
                            if (!(key[i] >= 32 && key[i] <= 126))
                                return WLAN_REASON_CODE_MSMSEC_PROFILE_PASSPHRASE_CHAR;
                    default:
                        return WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA;
                }
            }
        }
        default:
            break;
    }

    return WLAN_REASON_CODE_SUCCESS;
}

DWORD wlan_profile_parse( const WCHAR *profile_xml_str, struct wlan_profile_data *profile,
                          DWORD *wlan_reason )
{
    IXMLDOMDocument *xml_doc;
    IXMLDOMNode *wlan_profile_node;
    VARIANT_BOOL success;
    DOMNodeType node_type;
    BSTR bs;
    HRESULT res;
    DWORD ret = ERROR_SUCCESS;

    bs = SysAllocStringLen( profile_xml_str, wcslen( profile_xml_str ) );
    if (!bs)
        return STATUS_NO_MEMORY;

    res = CoInitialize( NULL );
    if (FAILED( res ))
        return hresult_to_win32_code( res );

    res = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument,
                            (void **)&xml_doc );
    if (FAILED( res ))
        return hresult_to_win32_code( res );

    res = IXMLDOMDocument_loadXML( xml_doc, bs, &success);
    SysFreeString( bs );
    if (res != S_OK)
    {
        IXMLDOMDocument_Release( xml_doc );
        return ERROR_INVALID_PARAMETER;
    }

    IXMLDOMDocument_put_resolveExternals( xml_doc, VARIANT_FALSE );

    bs = SysAllocString( L"WLANProfile" );
    res = IXMLDOMDocument_selectSingleNode( xml_doc, bs, &wlan_profile_node );
    SysFreeString( bs );

    if (res != S_OK)
    {
        res = MAKE_WLANERROR( WLAN_REASON_CODE_PROFILE_MISSING );
        goto fail;
    }

    IXMLDOMNode_get_nodeType( wlan_profile_node, &node_type );
    if (node_type != NODE_ELEMENT)
    {
        res = MAKE_WLANERROR( WLAN_REASON_CODE_INVALID_PROFILE_SCHEMA );
        goto fail;
    }

    res = read_WLANProfile( wlan_profile_node, profile );
    goto done;

 fail:
    if (HRESULT_FACILITY( res ) == FACILITY_WLANAPI &&
        HRESULT_SEVERITY( res ) == SEVERITY_ERROR )
    {
        *wlan_reason = HRESULT_CODE( res );
        ret = ERROR_BAD_PROFILE;
    }
    else
        ret = hresult_to_win32_code( res );
 done:
    if (ret)
    {
        *wlan_reason = !wlan_profile_validate( profile );
        if (*wlan_reason)
            ret = ERROR_BAD_PROFILE;
    }
    IXMLDOMNode_Release( wlan_profile_node );
    IXMLDOMDocument_Release( xml_doc );
    return ret;
 }
