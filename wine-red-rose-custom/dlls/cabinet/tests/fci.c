/*
 * Unit tests for the File Compression Interface
 *
 * Copyright 2026 Nigel Banks
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <windows.h>

#include "fci.h"
#include "fdi.h"
#include "wine/test.h"

/* make the media size large enough that everything ends up in one cabinet, and
 * the folder threshold large enough that FCIAddFile() never starts a new folder
 * on its own; the only folder boundary is then the explicit FCIFlushFolder() */
#define MEDIA_SIZE       999999999
#define FOLDER_THRESHOLD 900000
#define TEST_SET_ID      0xbeef

/* on-disk cabinet structures, from the cabinet file format specification */
#pragma pack(push,1)

struct cfheader
{
    char   signature[4];
    ULONG  reserved1;
    ULONG  cbCabinet;
    ULONG  reserved2;
    ULONG  coffFiles;
    ULONG  reserved3;
    UCHAR  versionMinor;
    UCHAR  versionMajor;
    USHORT cFolders;
    USHORT cFiles;
    USHORT flags;
    USHORT setID;
    USHORT iCabinet;
};

struct cffolder
{
    ULONG  coffCabStart;
    USHORT cCFData;
    USHORT typeCompress;
};

struct cffile
{
    ULONG  cbFile;
    ULONG  uoffFolderStart;
    USHORT iFolder;
    USHORT date;
    USHORT time;
    USHORT attribs;
    char   szName[1];
};

#pragma pack(pop)

/* the working directory, with a trailing backslash: this is where the test
 * files are created, and both FCI and FDI want it as a path prefix */
static char curr_dir[MAX_PATH];

/* set if FCI passed an invalid handle to the seek callback */
static BOOL invalid_seek;

/* shared by the FCI and FDI contexts below */

static void * CDECL mem_alloc(ULONG cb)
{
    return malloc(cb);
}

static void CDECL mem_free(void *memory)
{
    free(memory);
}

/* FCI callbacks */

static BOOL CDECL fci_get_next_cabinet(PCCAB pccab, ULONG cbPrevCab, void *pv)
{
    /* the media size is large enough that a second cabinet is never needed */
    ok(0, "unexpected call to get_next_cabinet\n");
    return FALSE;
}

static LONG CDECL fci_progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int CDECL fci_file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                                 BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    DWORD disposition;
    HANDLE handle;

    if (oflag & _O_EXCL) disposition = CREATE_NEW;
    else if (oflag & _O_CREAT) disposition = (oflag & _O_TRUNC) ? CREATE_ALWAYS : OPEN_ALWAYS;
    else disposition = (oflag & _O_TRUNC) ? TRUNCATE_EXISTING : OPEN_EXISTING;

    handle = CreateFileA(pszFile, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, disposition, 0, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "failed to open %s, error %lu\n", pszFile, GetLastError());
    if (handle == INVALID_HANDLE_VALUE) *err = GetLastError();
    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    DWORD read;

    /* a failed read is indistinguishable from end of file here, and would
     * silently truncate the file being added, so make sure it is reported */
    if (!ReadFile((HANDLE)hf, memory, cb, &read, NULL))
    {
        ok(0, "failed to read, error %lu\n", GetLastError());
        *err = GetLastError();
        return 0;
    }
    return read;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    DWORD written;

    if (!WriteFile((HANDLE)hf, memory, cb, &written, NULL))
    {
        ok(0, "failed to write, error %lu\n", GetLastError());
        *err = GetLastError();
        return 0;
    }
    return written;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *pv)
{
    if (!CloseHandle((HANDLE)hf))
    {
        *err = GetLastError();
        return -1;
    }
    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)
{
    DWORD ret;

    /* FCI must never hand an invalid handle to the callbacks.  Record it rather
     * than assert here: the callback is also called with valid handles. */
    if (hf == -1)
    {
        invalid_seek = TRUE;
        *err = ERROR_INVALID_HANDLE;
        return -1;
    }

    /* SEEK_SET/CUR/END have the same values as FILE_BEGIN/CURRENT/END */
    ret = SetFilePointer((HANDLE)hf, dist, NULL, seektype);
    if (ret == INVALID_SET_FILE_POINTER)
    {
        *err = GetLastError();
        return -1;
    }
    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *pv)
{
    if (!DeleteFileA(pszFile))
    {
        *err = GetLastError();
        return -1;
    }
    return 0;
}

static BOOL CDECL fci_get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    char name[MAX_PATH];

    if (!GetTempFileNameA(".", "fci", 0, name)) return FALSE;
    if (strlen(name) >= (unsigned int)cbTempName) return FALSE;
    /* GetTempFileName() creates the file, but FCI opens it with _O_EXCL and so
     * wants to create it itself */
    DeleteFileA(name);
    strcpy(pszTempName, name);
    return TRUE;
}

static INT_PTR CDECL fci_get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                                       USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION info = { 0 };
    FILETIME filetime;
    HANDLE handle;

    handle = CreateFileA(pszName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "failed to open %s, error %lu\n", pszName, GetLastError());
    if (handle == INVALID_HANDLE_VALUE)
    {
        *err = GetLastError();
        return -1;
    }

    ok(GetFileInformationByHandle(handle, &info),
       "failed to get file information for %s, error %lu\n", pszName, GetLastError());
    FileTimeToLocalFileTime(&info.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);
    *pattribs = 0;
    return (INT_PTR)handle;
}

/* FDI callbacks, used to check that the cabinets FCI produces can be read back */

struct extracted_file
{
    char  name[CB_MAX_FILENAME];
    ULONG size;      /* size reported by FDI from the CFFILE entry */
    ULONG written;   /* bytes actually decompressed */
};

static struct extracted_file extracted[8];
static unsigned int extracted_count;
static unsigned int extracting;

static INT_PTR CDECL fdi_open(char *pszFile, int oflag, int pmode)
{
    HANDLE handle;

    /* the cabinet only needs to be read, and requesting write access to it
     * makes native FDI fail with a sharing violation */
    if (oflag & _O_CREAT)
        handle = CreateFileA(pszFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, CREATE_ALWAYS, 0, NULL);
    else
        handle = CreateFileA(pszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, NULL);
    return (INT_PTR)handle;
}

static UINT CDECL fdi_read(INT_PTR hf, void *memory, UINT cb)
{
    DWORD read;

    if (!ReadFile((HANDLE)hf, memory, cb, &read, NULL)) return 0;
    return read;
}

static UINT CDECL fdi_write(INT_PTR hf, void *memory, UINT cb)
{
    DWORD written;

    if (!WriteFile((HANDLE)hf, memory, cb, &written, NULL)) return 0;
    /* only the file currently being extracted is ever written to */
    if (extracting < ARRAY_SIZE(extracted)) extracted[extracting].written += written;
    return written;
}

static int CDECL fdi_close(INT_PTR hf)
{
    CloseHandle((HANDLE)hf);
    return 0;
}

static LONG CDECL fdi_seek(INT_PTR hf, LONG dist, int seektype)
{
    return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
}

static INT_PTR CDECL fdi_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION info)
{
    char path[sizeof("out_") + CB_MAX_FILENAME];

    switch (fdint)
    {
    case fdintCOPY_FILE:
        ok(extracted_count < ARRAY_SIZE(extracted), "unexpected file %s\n", info->psz1);
        if (extracted_count >= ARRAY_SIZE(extracted)) return 0;
        extracting = extracted_count++;
        lstrcpynA(extracted[extracting].name, info->psz1, CB_MAX_FILENAME);
        extracted[extracting].size = info->cb;
        extracted[extracting].written = 0;
        /* extract under a distinct name so the sources are left alone */
        sprintf(path, "out_%s", extracted[extracting].name);
        return fdi_open(path, _O_CREAT, 0);

    case fdintCLOSE_FILE_INFO:
        fdi_close(info->hf);
        extracting = ARRAY_SIZE(extracted);
        return TRUE;

    default:
        return 0;
    }
}

/* test helpers */

static void create_test_file(const char *name, ULONG size)
{
    static const char data[] = "The quick brown fox jumps over the lazy dog\n";
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", name, GetLastError());
    while (size)
    {
        DWORD chunk = min(size, sizeof(data) - 1);
        ok(WriteFile(file, data, chunk, &written, NULL) && written == chunk,
           "failed to write %s, error %lu\n", name, GetLastError());
        size -= chunk;
    }
    CloseHandle(file);
}

static void *load_file(const char *name, DWORD *size)
{
    HANDLE file;
    void *data;
    DWORD read;

    file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;
    *size = GetFileSize(file, NULL);
    if ((data = malloc(*size)))
        ok(ReadFile(file, data, *size, &read, NULL) && read == *size,
           "failed to read %s, error %lu\n", name, GetLastError());
    CloseHandle(file);
    return data;
}

/* Check the cabinet header and return the CFFOLDER array, or NULL if the
 * cabinet is too malformed to walk safely.  ok() is not fatal, so everything
 * the walk relies on has to be validated and not merely asserted. */
static const struct cffolder *check_cab_header(const void *cab, DWORD size,
                                               unsigned int folders, unsigned int files)
{
    const struct cfheader *header = cab;

    if (size < sizeof(*header) + folders * sizeof(struct cffolder))
    {
        ok(0, "cabinet is only %lu bytes\n", size);
        return NULL;
    }

    ok(!memcmp(header->signature, "MSCF", 4), "got signature %.4s\n", header->signature);
    ok(header->cbCabinet == size, "got cbCabinet %lu, file size %lu\n", header->cbCabinet, size);
    /* reserve areas and previous/next cabinet names are all stored between the
     * header and the folder entries, so no flags means the folders follow it */
    ok(!header->flags, "got flags %#x\n", header->flags);
    ok(header->cFolders == folders, "got %u folders\n", header->cFolders);
    ok(header->cFiles == files, "got %u files\n", header->cFiles);
    ok(header->setID == TEST_SET_ID, "got setID %#x\n", header->setID);

    if (memcmp(header->signature, "MSCF", 4) || header->flags || header->cFolders != folders)
        return NULL;
    return (const struct cffolder *)(header + 1);
}

/* Return the first CFFILE entry, or NULL if it does not lie within the cabinet.
 * An entry needs its fixed part plus at least one byte of name. */
static const struct cffile *first_cab_file(const void *cab, DWORD size)
{
    const struct cfheader *header = cab;

    if (header->coffFiles > size ||
        size - header->coffFiles <= FIELD_OFFSET(struct cffile, szName))
    {
        ok(0, "got coffFiles %lu in a %lu byte cabinet\n", header->coffFiles, size);
        return NULL;
    }
    return (const struct cffile *)((const char *)cab + header->coffFiles);
}

/* CFFILE entries are variable-length: the name is stored inline, NUL-terminated.
 * Returns NULL unless the following entry lies entirely within the cabinet. */
static const struct cffile *next_cab_file(const struct cffile *file, const void *cab, DWORD size)
{
    const char *end = (const char *)cab + size, *name = file->szName;

    while (name < end && *name) name++;
    if (name == end) return NULL;   /* the name is not terminated */
    name++;                         /* step over the NUL */
    if (end - name <= FIELD_OFFSET(struct cffile, szName)) return NULL;
    return (const struct cffile *)name;
}

static HFCI create_fci(CCAB *params, ERF *erf, const char *cab_name)
{
    HFCI hfci;

    memset(params, 0, sizeof(*params));
    params->cb = MEDIA_SIZE;
    params->cbFolderThresh = FOLDER_THRESHOLD;
    params->setID = TEST_SET_ID;
    strcpy(params->szCabPath, curr_dir);
    strcpy(params->szCab, cab_name);

    memset(erf, 0, sizeof(*erf));
    hfci = FCICreate(erf, fci_file_placed, mem_alloc, mem_free, fci_open, fci_read, fci_write,
                     fci_close, fci_seek, fci_delete, fci_get_temp_file, params, NULL);
    ok(hfci != NULL, "failed to create an FCI context\n");
    return hfci;
}

static void add_file(HFCI hfci, char *name)
{
    BOOL ret = FCIAddFile(hfci, name, name, FALSE, fci_get_next_cabinet, fci_progress,
                          fci_get_open_info, tcompTYPE_MSZIP);
    ok(ret, "failed to add %s\n", name);
}

/* Extract the cabinet again with FDI: a cabinet containing a folder without
 * data must still be readable, and its files must come back intact. */
static void verify_cab_contents(char *cab_name, char * const *names,
                                const ULONG *sizes, unsigned int count)
{
    char path[sizeof("out_") + CB_MAX_FILENAME];
    unsigned int i;
    HFDI hfdi;
    ERF erf;
    BOOL ret;

    extracted_count = 0;
    extracting = ARRAY_SIZE(extracted);

    memset(&erf, 0, sizeof(erf));
    hfdi = FDICreate(mem_alloc, mem_free, fdi_open, fdi_read, fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "failed to create an FDI context\n");
    if (!hfdi) return;

    ret = FDICopy(hfdi, cab_name, curr_dir, 0, fdi_notify, NULL, NULL);
    ok(ret, "failed to extract %s, erfOper %d, erfType %d\n", cab_name, erf.erfOper, erf.erfType);
    FDIDestroy(hfdi);

    ok(extracted_count == count, "extracted %u files, expected %u\n", extracted_count, count);
    for (i = 0; i < min(extracted_count, count); i++)
    {
        winetest_push_context("file %u", i);
        ok(!strcmp(extracted[i].name, names[i]), "got name %s\n", extracted[i].name);
        ok(extracted[i].size == sizes[i], "got size %lu\n", extracted[i].size);
        ok(extracted[i].written == sizes[i], "decompressed %lu bytes\n", extracted[i].written);
        winetest_pop_context();
    }

    for (i = 0; i < extracted_count; i++)
    {
        sprintf(path, "out_%s", extracted[i].name);
        DeleteFileA(path);
    }
}

/* Build a cabinet of two folders, one of which holds only zero-length files and
 * so gets no data blocks at all; its CFFOLDER entry simply carries cCFData == 0.
 *
 * The empty folder is placed either last or first.  The first case is what WiX's
 * light.exe produces; the second additionally catches a fix that stopped at the
 * first folder without data instead of skipping over it, which would lose the
 * following folder's data blocks. */
static void test_dataless_folder(BOOL empty_first)
{
    /* the sizes are small enough that a folder needs a single CFDATA block */
    static const ULONG data_first_sizes[] = { 187, 140, 0, 0 };
    static const ULONG empty_first_sizes[] = { 0, 0, 187, 140 };
    /* the first two files go into folder 0, the last two into folder 1 */
    static const USHORT folders[] = { 0, 0, 1, 1 };
    static char name0[] = "fcitest1.dat";
    static char name1[] = "fcitest2.dat";
    static char name2[] = "fcitest3.dat";
    static char name3[] = "fcitest4.dat";
    static char cab_name[] = "fcitest.cab";
    static char * const names[] = { name0, name1, name2, name3 };
    const ULONG *sizes = empty_first ? empty_first_sizes : data_first_sizes;
    const struct cffolder *folder;
    const struct cffile *file;
    unsigned int i;
    CCAB params;
    char *cab;
    HFCI hfci;
    DWORD size;
    ERF erf;
    BOOL ret;

    winetest_push_context(empty_first ? "empty folder first" : "empty folder last");

    invalid_seek = FALSE;
    if (!(hfci = create_fci(&params, &erf, cab_name)))
    {
        winetest_pop_context();
        return;
    }
    for (i = 0; i < ARRAY_SIZE(names); i++)
        create_test_file(names[i], sizes[i]);

    add_file(hfci, name0);
    add_file(hfci, name1);

    /* close the folder explicitly, so that the files added below start a new one */
    ret = FCIFlushFolder(hfci, fci_get_next_cabinet, fci_progress);
    ok(ret, "failed to flush the folder, erfOper %d, erfType %d\n", erf.erfOper, erf.erfType);

    add_file(hfci, name2);
    add_file(hfci, name3);

    ret = FCIFlushCabinet(hfci, FALSE, fci_get_next_cabinet, fci_progress);
    ok(ret, "failed to flush the cabinet, erfOper %d, erfType %d\n", erf.erfOper, erf.erfType);
    ok(!invalid_seek, "seek was called with an invalid handle\n");

    ret = FCIDestroy(hfci);
    ok(ret, "failed to destroy the FCI context\n");

    cab = load_file(cab_name, &size);
    ok(cab != NULL, "failed to load %s\n", cab_name);
    if (cab && (folder = check_cab_header(cab, size, 2, ARRAY_SIZE(names))))
    {
        if (empty_first)
        {
            ok(folder[0].cCFData == 0, "got %u data blocks in folder 0\n", folder[0].cCFData);
            ok(folder[1].cCFData != 0, "got %u data blocks in folder 1\n", folder[1].cCFData);
        }
        else
        {
            ok(folder[0].cCFData != 0, "got %u data blocks in folder 0\n", folder[0].cCFData);
            ok(folder[1].cCFData == 0, "got %u data blocks in folder 1\n", folder[1].cCFData);
        }

        /* the names and sizes are checked again below, by FDI */
        file = first_cab_file(cab, size);
        for (i = 0; i < ARRAY_SIZE(names) && file; i++)
        {
            winetest_push_context("file %u", i);
            ok(file->iFolder == folders[i], "got folder %u\n", file->iFolder);
            ok(file->cbFile == sizes[i], "got size %lu\n", file->cbFile);
            winetest_pop_context();
            file = next_cab_file(file, cab, size);
        }
        ok(i == ARRAY_SIZE(names), "only %u file entries fit in the cabinet\n", i);
    }
    free(cab);

    /* deliberately outside the checks above: reading the cabinet back does not
     * depend on any of them succeeding */
    verify_cab_contents(cab_name, names, sizes, ARRAY_SIZE(names));

    DeleteFileA(cab_name);
    for (i = 0; i < ARRAY_SIZE(names); i++)
        DeleteFileA(names[i]);

    winetest_pop_context();
}

/* The degenerate case: every file in the cabinet is empty, so its one and only
 * folder has no data. */
static void test_only_empty_files(void)
{
    static const ULONG sizes[] = { 0 };
    static char empty[] = "fciempty.dat";
    static char cab_name[] = "fciempty.cab";
    static char * const names[] = { empty };
    const struct cffolder *folder;
    const struct cffile *file;
    CCAB params;
    char *cab;
    HFCI hfci;
    DWORD size;
    ERF erf;
    BOOL ret;

    invalid_seek = FALSE;
    if (!(hfci = create_fci(&params, &erf, cab_name))) return;
    create_test_file(empty, sizes[0]);
    add_file(hfci, empty);

    ret = FCIFlushCabinet(hfci, FALSE, fci_get_next_cabinet, fci_progress);
    ok(ret, "failed to flush the cabinet, erfOper %d, erfType %d\n", erf.erfOper, erf.erfType);
    ok(!invalid_seek, "seek was called with an invalid handle\n");

    ret = FCIDestroy(hfci);
    ok(ret, "failed to destroy the FCI context\n");

    cab = load_file(cab_name, &size);
    ok(cab != NULL, "failed to load %s\n", cab_name);
    if (cab && (folder = check_cab_header(cab, size, 1, ARRAY_SIZE(names))))
    {
        ok(folder->cCFData == 0, "got %u data blocks\n", folder->cCFData);

        if ((file = first_cab_file(cab, size)))
        {
            ok(file->iFolder == 0, "got folder %u\n", file->iFolder);
            ok(file->cbFile == sizes[0], "got size %lu\n", file->cbFile);
        }
    }
    free(cab);

    verify_cab_contents(cab_name, names, sizes, ARRAY_SIZE(names));

    DeleteFileA(cab_name);
    DeleteFileA(empty);
}

START_TEST(fci)
{
    GetCurrentDirectoryA(ARRAY_SIZE(curr_dir) - 1, curr_dir);
    strcat(curr_dir, "\\");

    test_dataless_folder(FALSE);
    test_dataless_folder(TRUE);
    test_only_empty_files();
}
