/*
 * Unit tests for shell32 dialogs
 *
 * Copyright 2025 Matthias Zorn
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

#include "wine/test.h"
#include "winbase.h"
#include "shlobj.h"

static void test_PickIconDlg(void)
{
    WCHAR icon_path_filename[30];
    int icon_index = 1;
    int return_value;

    lstrcpyW(icon_path_filename, L"shell32.dll");
    SetLastError(0);

    todo_wine {
        return_value = PickIconDlg(NULL, icon_path_filename, ARRAYSIZE(icon_path_filename), NULL);
        ok(return_value == 0, "PickIconDlg(NULL, ..., NULL) return_value failed\n");
    }
    
    todo_wine {
        return_value = PickIconDlg(NULL, NULL, ARRAYSIZE(icon_path_filename), &icon_index);
        ok(return_value == 0, "PickIconDlg(NULL, NULL, ...) return_value failed\n");
    }

    todo_wine {
        return_value = PickIconDlg((HWND) -1, icon_path_filename, ARRAYSIZE(icon_path_filename), &icon_index);
        ok(return_value == 1, "PickIconDlg((HWND) -1, ...) return_value failed\n");
    }
}

START_TEST(dialogs)
{
    test_PickIconDlg();
}
