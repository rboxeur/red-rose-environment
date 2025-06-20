#include "urlmon_main.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#define PZI_CURRENT_FILE_VERSION 0x0001

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct
{
    USHORT product_version;
    USHORT file_version;
    URLZONE zone;
} FIXDLEN_DATA;

typedef struct {
    IUnknown        IUnknown_inner;
    IPersistFile    IPersistFile_iface;
    IZoneIdentifier IZoneIdentifier_iface;

    BOOL is_dirty;
    LPWSTR file_name;

    URLZONE zone;

    IUnknown        *outer;

    LONG ref;
} PersistentZoneIdentifier;

static inline PersistentZoneIdentifier *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IUnknown_inner);
}

static inline PersistentZoneIdentifier *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IPersistFile_iface);
}

static inline PersistentZoneIdentifier *impl_from_IZoneIdentifier(IZoneIdentifier *iface)
{
    return CONTAINING_RECORD(iface, PersistentZoneIdentifier, IZoneIdentifier_iface);
}

static HRESULT WINAPI PZIUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);

    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    } else if (IsEqualGUID(&IID_IPersist, riid))
    {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistFile_iface;
    } else if (IsEqualGUID(&IID_IPersistFile, riid))
    {
        TRACE("(%p)->(IID_IPersistFile %p)\n", This, ppv);
        *ppv = &This->IPersistFile_iface;
    } else if (IsEqualGUID(&IID_IZoneIdentifier, riid))
    {
        TRACE("(%p)->(IID_IZoneIdentifier %p)\n", This, ppv);
        *ppv = &This->IZoneIdentifier_iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PZIUnk_AddRef(IUnknown *iface)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI PZIUnk_Release(IUnknown *iface)
{
    PersistentZoneIdentifier *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        URLMON_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl PZIUnkVtbl = {
    PZIUnk_QueryInterface,
    PZIUnk_AddRef,
    PZIUnk_Release
};

static HRESULT WINAPI PZIPersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p, %s %p)\n", This, debugstr_guid(riid), ppv);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI PZIPersistFile_AddRef(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI PZIPersistFile_Release(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI PZIPersistFile_GetClassID(IPersistFile *iface, CLSID *clsid)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p, %p)\n", This, clsid);

    *clsid = CLSID_PersistentZoneIdentifier;

    return S_OK;
}

static HRESULT WINAPI PZIPersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *file_name)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p, %p)\n", This, file_name);

    *file_name = CoTaskMemAlloc((lstrlenW(This->file_name) + 1) * sizeof(WCHAR));
    if (!*file_name)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpyW(*file_name, This->file_name);

    return S_OK;
}

static HRESULT WINAPI PZIPersistFile_IsDirty(IPersistFile *iface)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return This->is_dirty ? S_OK : S_FALSE;
}

static HRESULT load_zone_id_data(PersistentZoneIdentifier *This, BYTE *data, DWORD size)
{
    const FIXDLEN_DATA *fixed;

    if (size < sizeof(*fixed))
    {
        TRACE("no space for FIXDLEN_DATA\n");
        return E_OUTOFMEMORY;
    }

    fixed = (const FIXDLEN_DATA*)data;

    TRACE("product_version %04x\n", fixed->product_version);
    TRACE("file_version %04x\n", fixed->file_version);

    TRACE("zone %08x\n", fixed->zone);
    This->zone = fixed->zone;

    return S_OK;
}

static HRESULT WINAPI PZIPersistFile_Load(IPersistFile *iface, LPCOLESTR file_name, DWORD mode)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);
    HANDLE mapping;
    HANDLE hfile;
    DWORD sharing;
    DWORD access;
    DWORD size;
    DWORD try;
    void* data;
    HRESULT hres;

    TRACE("(%p, %s, 0x%08lx)\n", iface, debugstr_w(file_name), mode);

    switch (mode & 0x000f)
    {
    default:
    case STGM_READ:
        access = GENERIC_READ;
        break;
    case STGM_WRITE:
    case STGM_READWRITE:
        access = GENERIC_READ | GENERIC_WRITE;
        break;
    }

    switch (mode & 0x00f0)
    {
    default:
    case STGM_SHARE_DENY_NONE:
        sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
        break;
    case STGM_SHARE_DENY_READ:
        sharing = FILE_SHARE_WRITE;
        break;
    case STGM_SHARE_DENY_WRITE:
        sharing = FILE_SHARE_READ;
        break;
    case STGM_SHARE_EXCLUSIVE:
        sharing = 0;
        break;
    }

    try = 1;
    for (;;)
    {
        hfile = CreateFileW(file_name, access, sharing, NULL, OPEN_EXISTING, 0, NULL);
        if (hfile != INVALID_HANDLE_VALUE) { break; }

        if (GetLastError() != ERROR_SHARING_VIOLATION || try++ >= 3)
        {
            TRACE("Failed to open %s, error %lu\n", debugstr_w(file_name), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
        Sleep(100);
    }

    size = GetFileSize(hfile, NULL);

    mapping = CreateFileMappingW(hfile, NULL, PAGE_READONLY, 0, 0, 0);
    if (!mapping)
    {
        TRACE("Failed to create file mapping %s, error %lu\n", debugstr_w(file_name), GetLastError());
        CloseHandle(hfile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (data)
    {
        hres = load_zone_id_data(This, data, size);
        if (hres == S_OK) This->is_dirty = FALSE;
        UnmapViewOfFile(data);
    }
    else
    {
        hres = HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(mapping);
    CloseHandle(hfile);

    return hres;
}

static HRESULT WINAPI PZIPersistFile_Save(IPersistFile *iface, LPCOLESTR file_name, BOOL remember)
{
    PersistentZoneIdentifier *This = impl_from_IPersistFile(iface);
    FIXDLEN_DATA fixed;
    DWORD disposition;
    DWORD size;
    DWORD try;
    DWORD ver;
    HANDLE hfile;
    HRESULT hres;

    TRACE("(%p, %s, %d)\n", iface, debugstr_w(file_name), remember);

    disposition = file_name ? CREATE_NEW : OPEN_ALWAYS;

    if (!file_name)
    {
        file_name = This->file_name;
        remember = FALSE;
    }

    try = 1;
    for (;;)
    {
        hfile = CreateFileW(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, disposition, 0, NULL);
        if (hfile != INVALID_HANDLE_VALUE) { break; }

        if (try++ >= 3)
        {
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
        Sleep(100);
    }

    ver = GetVersion();
    fixed.product_version = MAKEWORD(ver >> 8, ver);
    fixed.file_version = PZI_CURRENT_FILE_VERSION;
    fixed.zone = This->zone;

    if (!WriteFile(hfile, &fixed, sizeof(fixed), &size, NULL))
    {
        hres = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    hres = S_OK;
    This->is_dirty = FALSE;

cleanup:
    if (hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        if (hres != S_OK)
        {
            DeleteFileW(file_name);
        }
        else if (remember)
        {
            if (This->file_name) { CoTaskMemFree(This->file_name); }
            This->file_name = wcsdup(file_name);
        }
    }

    return hres;
}

static HRESULT WINAPI PZIPersistFile_SaveCompleted(
        IPersistFile* iface,
        LPCOLESTR pszFileName)
{
    FIXME("(%p, %p) not implemented\n", iface, pszFileName);

    return E_NOTIMPL;
}

static const IPersistFileVtbl PZIPersistFileVtbl = {
    PZIPersistFile_QueryInterface,
    PZIPersistFile_AddRef,
    PZIPersistFile_Release,
    PZIPersistFile_GetClassID,
    PZIPersistFile_IsDirty,
    PZIPersistFile_Load,
    PZIPersistFile_Save,
    PZIPersistFile_SaveCompleted,
    PZIPersistFile_GetCurFile
};

static HRESULT WINAPI PZIZoneId_QueryInterface(IZoneIdentifier *iface, REFIID riid, void **ppv)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p, %s %p)\n", This, debugstr_guid(riid), ppv);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI PZIZoneId_AddRef(IZoneIdentifier *iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p)\n", This);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI PZIZoneId_Release(IZoneIdentifier *iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->outer);
}

static BOOL is_trusted_zone(URLZONE zone)
{
    switch (zone)
    {
    case URLZONE_INVALID:
    case URLZONE_LOCAL_MACHINE:
    case URLZONE_INTRANET:
    case URLZONE_TRUSTED:
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOL is_known_zone(URLZONE zone)
{
    switch (zone)
    {
    case URLZONE_INVALID:
    case URLZONE_LOCAL_MACHINE:
    case URLZONE_INTRANET:
    case URLZONE_TRUSTED:
    case URLZONE_INTERNET:
    case URLZONE_UNTRUSTED:
        return TRUE;
    default:
        return FALSE;
    }
}

static HRESULT WINAPI PZIZoneId_GetId(IZoneIdentifier* iface, DWORD* pdwZone)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p, %p)\n", This, pdwZone);

    *pdwZone = This->zone;

    return is_trusted_zone(*pdwZone) ? S_OK : E_ACCESSDENIED;
}

static HRESULT WINAPI PZIZoneId_Remove(IZoneIdentifier* iface)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p)\n", This);

    This->zone = URLZONE_LOCAL_MACHINE;

    return S_OK;
}

static HRESULT WINAPI PZIZoneId_SetId(IZoneIdentifier* iface, DWORD dwZone)
{
    PersistentZoneIdentifier *This = impl_from_IZoneIdentifier(iface);

    TRACE("(%p, 0x%08lx)\n", This, dwZone);

    This->zone = dwZone;
    This->is_dirty = TRUE;

    if (is_trusted_zone(This->zone))
    {
        return S_OK;
    } else if (!is_known_zone(This->zone))
    {
        FIXME("Unknown zone identifier: 0x%08x\n", This->zone);
    }

    return E_ACCESSDENIED;
}

static const IZoneIdentifierVtbl PZIZoneIdVtbl = {
    PZIZoneId_QueryInterface,
    PZIZoneId_AddRef,
    PZIZoneId_Release,
    PZIZoneId_GetId,
    PZIZoneId_SetId,
    PZIZoneId_Remove
};

HRESULT PersistentZoneIdentifier_Construct(IUnknown *outer, LPVOID *ppobj)
{

    PersistentZoneIdentifier *ret;

    TRACE("(%p %p)\n", outer, ppobj);

    URLMON_LockModule();

    ret = malloc(sizeof(PersistentZoneIdentifier));

    ret->IUnknown_inner.lpVtbl = &PZIUnkVtbl;
    ret->IPersistFile_iface.lpVtbl = &PZIPersistFileVtbl;
    ret->IZoneIdentifier_iface.lpVtbl = &PZIZoneIdVtbl;

    ret->file_name = NULL;
    ret->is_dirty = FALSE;

    ret->zone = URLZONE_INVALID;

    ret->ref = 1;
    ret->outer = outer ? outer : &ret->IUnknown_inner;

    *ppobj = &ret->IUnknown_inner;

    return S_OK;
}
