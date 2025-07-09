/*
 * Kernel initialization code
 *
 * Copyright 2000 Alexandre Julliard
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

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wincon.h"
#include "winternl.h"

#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static STARTUPINFOA startup_infoA;

/***********************************************************************
 *           set_entry_point
 */
#ifdef __i386__
static void set_entry_point( HMODULE module, const char *name, DWORD rva )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;

    if ((exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        DWORD *functions = (DWORD *)((char *)module + exports->AddressOfFunctions);
        const WORD *ordinals = (const WORD *)((const char *)module + exports->AddressOfNameOrdinals);
        const DWORD *names = (const DWORD *)((const char *)module +  exports->AddressOfNames);
        int min = 0, max = exports->NumberOfNames - 1;

        while (min <= max)
        {
            int res, pos = (min + max) / 2;
            const char *ename = (const char *)module + names[pos];
            if (!(res = strcmp( ename, name )))
            {
                WORD ordinal = ordinals[pos];
                DWORD oldprot;

                TRACE( "setting %s at %p to %08lx\n", name, &functions[ordinal], rva );
                VirtualProtect( functions + ordinal, sizeof(*functions), PAGE_READWRITE, &oldprot );
                functions[ordinal] = rva;
                VirtualProtect( functions + ordinal, sizeof(*functions), oldprot, &oldprot );
                return;
            }
            if (res > 0) max = pos - 1;
            else min = pos + 1;
        }
    }
}
#endif


/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = startup_infoA;
}

static void copy_startup_info(void)
{
    RTL_USER_PROCESS_PARAMETERS* rupp;
    ANSI_STRING         ansi;

    RtlAcquirePebLock();

    rupp = NtCurrentTeb()->Peb->ProcessParameters;

    startup_infoA.cb                   = sizeof(startup_infoA);
    startup_infoA.lpReserved           = NULL;
    startup_infoA.lpDesktop = !RtlUnicodeStringToAnsiString( &ansi, &rupp->Desktop, TRUE ) ? ansi.Buffer : NULL;
    startup_infoA.lpTitle = !RtlUnicodeStringToAnsiString( &ansi, &rupp->WindowTitle, TRUE ) ? ansi.Buffer : NULL;
    startup_infoA.dwX                  = rupp->dwX;
    startup_infoA.dwY                  = rupp->dwY;
    startup_infoA.dwXSize              = rupp->dwXSize;
    startup_infoA.dwYSize              = rupp->dwYSize;
    startup_infoA.dwXCountChars        = rupp->dwXCountChars;
    startup_infoA.dwYCountChars        = rupp->dwYCountChars;
    startup_infoA.dwFillAttribute      = rupp->dwFillAttribute;
    startup_infoA.dwFlags              = rupp->dwFlags;
    startup_infoA.wShowWindow          = rupp->wShowWindow;
    startup_infoA.cbReserved2          = rupp->RuntimeInfo.MaximumLength;
    startup_infoA.lpReserved2          = rupp->RuntimeInfo.MaximumLength ? (void*)rupp->RuntimeInfo.Buffer : NULL;
    if (rupp->dwFlags & STARTF_USESTDHANDLES)
    {
        startup_infoA.hStdInput        = rupp->hStdInput;
        startup_infoA.hStdOutput       = rupp->hStdOutput;
        startup_infoA.hStdError        = rupp->hStdError;
    }
    else
    {
        startup_infoA.hStdInput        = INVALID_HANDLE_VALUE;
        startup_infoA.hStdOutput       = INVALID_HANDLE_VALUE;
        startup_infoA.hStdError        = INVALID_HANDLE_VALUE;
    }
    RtlReleasePebLock();
}

/***********************************************************************
 *           KERNEL process initialisation routine
 */
static BOOL process_attach( HMODULE module )
{
    RtlSetUnhandledExceptionFilter( UnhandledExceptionFilter );

    NtQuerySystemInformation( SystemBasicInformation, &system_info, sizeof(system_info), NULL );
    kernelbase_global_data = KernelBaseGetGlobalData();

    copy_startup_info();

#ifdef __i386__
    if (!(GetVersion() & 0x80000000))
    {
        /* Securom checks for this one when version is NT */
        set_entry_point( module, "FT_Thunk", 0 );
    }
    else
    {
        LDR_DATA_TABLE_ENTRY *ldr;

        if (LdrFindEntryForAddress( GetModuleHandleW( 0 ), &ldr ) || !(ldr->Flags & LDR_WINE_INTERNAL))
            LoadLibraryA( "krnl386.exe16" );
    }
#endif
    return TRUE;
}

/***********************************************************************
 *           KERNEL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        return process_attach( hinst );
    case DLL_PROCESS_DETACH:
        WritePrivateProfileSectionW( NULL, NULL, NULL );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           MulDiv   (KERNEL32.@, KERNELBASE.@)
 * RETURNS
 *	Result of multiplication and division
 *	-1: Overflow occurred or Divisor was 0
 */
INT WINAPI MulDiv( INT nNumber, INT nNumerator, INT nDenominator )
{
#ifdef _WIN64
    unsigned __int64 abs_denominator, intermediate64, unsigned_quotient;
    int abs_numerator, abs_number;

    abs_denominator = (unsigned int)-nDenominator;

    if (nDenominator > 0)
    {
        abs_denominator = (unsigned int)nDenominator;
    }

    abs_numerator = -nNumerator;

    if (nNumerator >= 0)
    {
        abs_numerator = nNumerator;
    }

    abs_number = -nNumber;

    if (nNumber >= 0)
    {
        abs_number = nNumber;
    }

    intermediate64 = ((unsigned __int64)(unsigned int)abs_denominator >> 1) + abs_number * (__int64)abs_numerator;

    if ((unsigned int)abs_denominator <= ((DWORD)((intermediate64 >> 32) & 0xFFFFFFFFU)))
    {
        return -1;
    }

    unsigned_quotient = intermediate64 / abs_denominator;

    if ((unsigned_quotient & INT_MIN) != 0LL)
    {
        return -1;
    }

    // sign check
    if ((nDenominator ^ nNumerator ^ nNumber) >= 0)
    {
        return unsigned_quotient;
    }

    return -(int)unsigned_quotient;
#else
#define UINT32_ABS(x) \
    ((x) == INT_MIN ? INT_MIN : \
     (x) < 0 ? (unsigned int)(-(x)) : \
     (unsigned int)(x))

    unsigned int abs_number, abs_numerator, abs_denominator, rounding_term, unsigned_quotient;
    unsigned __int64 product, rounded_product;

    abs_number = UINT32_ABS(nNumber);
    abs_numerator = UINT32_ABS(nNumerator);
    abs_denominator = UINT32_ABS(nDenominator);
    product = (unsigned __int64)abs_numerator * abs_number;

    // Assume nDenominator == INT_MIN
    // In 32-bit MSVC, this results in it INT_MIN itself due to how two's complement arithmetic works,
    // and the lack of a positive equivalent value within the valid range for int32. So.. undefined behavior.
    // That makes `-nDenominator` effectively evaluate to `0x80000000` (`INT_MIN`).
    // The original code performs `-nDenominator >> 1`. At that point, MSVC thinks it's a signed integer!
    // Therefore, it compiles to `sar` instead of `shr`.
    // Disassembly from kernel32.dll reads:
    // neg     ecx             ; Two's Complement Negation
    // push    ecx
    // sar     ecx, 1          ; Shift Arithmetic Right
    //
    // Note - sar and not shr!
    //
    // 0x80000000 is `1000 0000 0000 0000 0000 0000 0000 0000`
    // In that case, `sar ecx, 1` results in `1100 0000 0000 0000 0000 0000 0000 0000`, which is.. as you guessed - 0xC0000000.
    if (nDenominator == INT_MIN)
    {
        rounding_term = 0xC0000000U;
    }

    else
    {
        rounding_term = abs_denominator >> 1;
    }

    rounded_product = product + rounding_term;

    if (rounded_product < product ||
        (rounded_product >> 32) >= abs_denominator)
    {
        return -1;
    }

    unsigned_quotient = (unsigned int)(rounded_product / abs_denominator);

    if ((nDenominator ^ nNumerator ^ nNumber) >= 0)
    {
        if (unsigned_quotient > INT_MIN)
        {
            return -1;
        }

        return (int)unsigned_quotient;
    }

    else
    {
        if (unsigned_quotient > INT_MIN)
        {
            return -1;
        }

        else if (unsigned_quotient == INT_MIN)
        {
            return INT_MIN;
        }
    }

    return -(int)unsigned_quotient;
#endif
}

/******************************************************************************
 *           GetSystemRegistryQuota       (KERNEL32.@)
 */
BOOL WINAPI GetSystemRegistryQuota(PDWORD pdwQuotaAllowed, PDWORD pdwQuotaUsed)
{
    FIXME("(%p, %p) faking reported quota values\n", pdwQuotaAllowed, pdwQuotaUsed);

    if (pdwQuotaAllowed)
        *pdwQuotaAllowed = 2 * 1000 * 1000 * 1000; /* 2 GB */

    if (pdwQuotaUsed)
        *pdwQuotaUsed = 100 * 1000 * 1000; /* 100 MB */

    return TRUE;
}

/******************************************************************************
 *          CreateBoundaryDescriptorA    (KERNEL32.@)
 */
HANDLE WINAPI CreateBoundaryDescriptorA(LPCSTR name, ULONG flags)
{
    FIXME("%s, %#lx - stub\n", debugstr_a(name), flags);
    return NULL;
}
