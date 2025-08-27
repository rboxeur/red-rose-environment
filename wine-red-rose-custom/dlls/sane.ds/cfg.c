/*
* TWAIN32 Configuration Manager
*
* Copyright 2025 Ivan Lyugaev
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

#include "wine/debug.h"
#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

BOOL is_exists_folder(WCHAR* path)
{
    WCHAR *last_slash = wcsrchr(path, L'\\');
    DWORD err;

    if (!last_slash)
    {
        ERR("incorrect path %s\n", debugstr_w(path));
        return FALSE;
    }
    *last_slash = L'\0';

    if (CreateDirectoryW(path, NULL))
        return TRUE;

    err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
        return TRUE;

    ERR("CreateDirectoryW failed, error: %lu\n", err);
    return FALSE;
}

BOOL is_exist_file(WCHAR* path)
{
    HANDLE hFile;
    hFile = CreateFileW(
        path,
	GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    CloseHandle(hFile);
    return TRUE;
}

BOOL save_to_file(WCHAR* path, ScannerOption *option)
{
    WCHAR folder_path[MAX_PATH];
    CHAR buffer[1024] = {0};
    HANDLE hFile;
    DWORD size, read;
    CHAR* content;
    CHAR* new_content;
    BOOL found;
    CHAR search[256];
    DWORD written;
    size_t len;

    lstrcpynW(folder_path, path, MAX_PATH);
    folder_path[MAX_PATH-1] = L'\0';

    if (!is_exists_folder(folder_path))
        return FALSE;

    hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW(GENERIC_READ) returned error: %lu\n", GetLastError());
        return FALSE;
    }

    switch (option->type)
    {
    case TYPE_INT:
        sprintf(buffer, "%s=%d\n", (option->name), option->value.int_val);
        break;
    case TYPE_FIXED:
        sprintf(buffer, "%s=%d\n", (option->name), option->value.fixed_val);
        break;
    case TYPE_STRING:
        sprintf(buffer, "%s=%s\n", (option->name), option->value.str_val);
        break;
    case TYPE_BOOL:
        sprintf(buffer, "%s=%s\n", (option->name), option->value.bool_val ? "true" : "false");
        break;
    }

    size = GetFileSize(hFile, NULL);
    content = (CHAR*)malloc(size + sizeof(CHAR));
    if (!content) {
        ERR("malloc failed (size=%lu)\n", size);
        CloseHandle(hFile);
        return FALSE;
    }

    ReadFile(hFile, content, size, &read, NULL);
    content[size / sizeof(CHAR)] = '\0';
    CloseHandle(hFile);

    new_content = NULL;
    found = FALSE;

    sprintf(search, "%s=", option->name);

    if (size > 0 && content[0] != '\0')
    {
        CHAR* lines[1000] = {0};
        int count = 0;
        CHAR* context = NULL;
        size_t new_size = 0;
        CHAR* line;

        line = strtok_s(content, "\n", &context);

        while (line && count < ARRAY_SIZE(lines))
        {
            if (!found && strstr(line, search) == line)
            {
                lines[count++] = buffer;
                found = TRUE;
            }
            else
            {
                lines[count++] = line;
            }
            line = strtok_s(NULL, "\n", &context);
        }

        if (!found && count < ARRAY_SIZE(lines))
        {
            lines[count++] = buffer;
        }

        for (int i = 0; i < count; i++)
        {
            new_size += strlen(lines[i]) + 1;
        }

        new_content = (CHAR*)malloc(new_size + 1);
        if (!new_content)
        {
            ERR("malloc failed (size=%lu)\n", size);
            CloseHandle(hFile);
            free(content);
            return FALSE;
        }
        new_content[0] = '\0';

        for (int i = 0; i < count; i++)
        {
            strcat_s(new_content, new_size + 1, lines[i]);
            if (i < count - 1)
                strcat_s(new_content, new_size + 1, "\n");
        }
        free(content);
    }
    else
        new_content = strdup(buffer);

    hFile = CreateFileW(
        path,
	GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        free(new_content);
        ERR("CreateFileW(GENERIC_WRITE) returned error: %lu\n", err);
        return FALSE;
    }

    len = strlen(new_content);
    WriteFile(hFile, new_content, (DWORD)len, &written, NULL);

    CloseHandle(hFile);
    free(new_content);

    return TRUE;
}

BOOL load_from_file(WCHAR* path, int type, CHAR* name, void* value)
{
    DWORD size;
    CHAR* content;
    DWORD read;
    CHAR search[256];
    CHAR* context;
    CHAR* line;
    BOOL found;
    HANDLE hFile = CreateFileW(
        path,
	GENERIC_READ,
        0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW returned error: %lu\n", GetLastError());
        return FALSE;
    }

    sprintf(search, "%s=", name);

    size = GetFileSize(hFile, NULL);
    content = (CHAR*)malloc(size + sizeof(CHAR));
    if (!content) {
        ERR("malloc failed (size=%lu)\n", size);
        CloseHandle(hFile);
        return FALSE;
    }

    ReadFile(hFile, content, size, &read, NULL);
    content[size / sizeof(CHAR)] = '\0';
    CloseHandle(hFile);

    context = NULL;
    line = strtok_s(content, "\n", &context);
    found = FALSE;

    while (line)
    {
        if (strstr(line, search) == line)
        {
            CHAR* val_start = line + strlen(search);
            while(*val_start == ' ') val_start++;

            switch (type)
            {
                case TYPE_INT:
                    *(INT*)value = atoi(val_start);
                    break;
                case TYPE_FIXED:
                    *(INT*)value = atoi(val_start);
                    break;
                case TYPE_STRING:
                    strcpy((CHAR*)value, val_start);
                    break;
                case TYPE_BOOL:
                    if (!strcmp(val_start, "true"))
                    {
                        *(BOOL*)value = TRUE;
                    }
                    else if (!strcmp(val_start, "false"))
                    {
                        *(BOOL*)value = FALSE;
                    }
                    else
                    {
                        ERR("Incorrect bool value: %s\n", val_start);
                        free(content);
                        return FALSE;
                    }
                    break;
                default:
                    ERR("Unknown type: %d\n", type);
                    free(content);
                    return FALSE;
            }

            found = TRUE;
            break;
        }
        line = strtok_s(NULL, "\n", &context);
    }
   free(content);
   return found;
}
