/*
 * Copyright (C) 2023 Paul Gofman for CodeWeavers
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
#include <stdlib.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <ioringapi.h>

#include "wine/test.h"

static HRESULT (WINAPI *pQueryIoRingCapabilities)(IORING_CAPABILITIES *);
static BOOL (WINAPI *pSetFileInformationByHandle)(HANDLE, FILE_INFO_BY_HANDLE_CLASS, void *, DWORD);

static void test_ioring_caps(void)
{
    IORING_CAPABILITIES caps;
    HRESULT hr;

    if (!pQueryIoRingCapabilities)
    {
        win_skip("QueryIoRingCapabilities is not available, skipping tests.\n");
        return;
    }

    memset(&caps, 0xcc, sizeof(caps));
    hr = pQueryIoRingCapabilities(&caps);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);
}

static void test_SetFileInformationByHandle(void)
{
    HANDLE handle;
    static const WCHAR filename1[] = L"test1";
    static const WCHAR filename2[] = L"test2";
    static const WCHAR filename3[] = L"test3";
    BOOL success;
    FILE_RENAME_INFO* renameinfo;
    size_t renameinfo_sz;
    FILE_RENAME_INFO* renameinfoex;
    size_t renameinfoex_sz;

    if (!pQueryIoRingCapabilities)
    {
        win_skip("QueryIoRingCapabilities is not available, skipping tests.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    handle = CreateFileW(filename1, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileA error: %lu\n", GetLastError());

    renameinfo_sz = sizeof(FILE_RENAME_INFO) + sizeof(filename2);
    renameinfo = malloc(renameinfo_sz);
    ok(renameinfo != NULL, "Out of memory\n");
    memset(renameinfo, 0, renameinfo_sz);
    SetLastError(0xdeadbeef);
    renameinfo->FileNameLength = lstrlenW(filename2);
    memcpy(renameinfo->FileName, filename2, sizeof(filename2));
    success = SetFileInformationByHandle(handle, FileRenameInfo, renameinfo, renameinfo_sz);
    ok(success, "SetFileInformationByHandle failed to change filename, error: %lu\n", GetLastError());

    renameinfoex_sz = sizeof(FILE_RENAME_INFO) + sizeof(filename3);
    renameinfoex = malloc(renameinfoex_sz);
    ok(renameinfoex != NULL, "Out of memory\n");
    memset(renameinfoex, 0, renameinfoex_sz);
    SetLastError(0xdeadbeef);
    renameinfoex->FileNameLength = lstrlenW(filename3);
    memcpy(renameinfoex->FileName, filename3, sizeof(filename3));
    success = SetFileInformationByHandle(handle, FileRenameInfoEx, renameinfoex, renameinfoex_sz);
    ok(success, "SetFileInformationByHandle failed to change filename, error: %lu\n", GetLastError());

    free(renameinfo);
    free(renameinfoex);
    CloseHandle(handle);
}

START_TEST(file)
{
    HMODULE hmod;

    hmod = LoadLibraryA("kernelbase.dll");
    pQueryIoRingCapabilities = (void *)GetProcAddress(hmod, "QueryIoRingCapabilities");
    pSetFileInformationByHandle = (void *)GetProcAddress(hmod, "SetFileInformationByHandle");

    test_ioring_caps();
    test_SetFileInformationByHandle();
}
