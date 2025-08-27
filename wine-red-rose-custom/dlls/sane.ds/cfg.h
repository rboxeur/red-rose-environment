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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"

#include "sane_i.h"

#define SINGLE 1
#define MULTY  2

typedef struct
{
    int type;
    CHAR name[64];
    int optno;
    union
    {
        int int_val;
        BOOL bool_val;
        CHAR str_val[255];
        int fixed_val;
    } value;
    BOOL is_enabled;
} ScannerOption;

BOOL save_to_file(WCHAR* path, ScannerOption *option);
BOOL load_from_file(WCHAR* path, int type, CHAR* name, void* value);

BOOL is_exist_file(WCHAR* path);
