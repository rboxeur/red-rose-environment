/*
 * MMSYSTEM time functions
 *
 * Copyright 1993 Martin Ayotte
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

#define _WINMM_
#include "mmsystem.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)

DWORD WINAPI timeGetTime(void)
{
    LARGE_INTEGER now, freq;

    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);

    return (now.QuadPart * 1000) / freq.QuadPart;
}

MMRESULT WINAPI timeGetSystemTime(MMTIME *time, UINT size)
{
    if (size >= sizeof(*time))
    {
        time->wType = TIME_MS;
        time->u.ms = timeGetTime();
    }

    return 0;
}

MMRESULT WINAPI timeGetDevCaps(TIMECAPS *caps, UINT size)
{
    TRACE("(%p, %u)\n", caps, size);

    if (!caps)
    {
        WARN("invalid caps\n");
        return TIMERR_NOCANDO;
    }

    if (size < sizeof(TIMECAPS))
    {
        WARN("invalid size\n");
        return TIMERR_NOCANDO;
    }

    caps->wPeriodMin = MMSYSTIME_MININTERVAL;
    caps->wPeriodMax = MMSYSTIME_MAXINTERVAL;

    return 0;
}

MMRESULT WINAPI timeBeginPeriod(UINT period)
{
    if (period < MMSYSTIME_MININTERVAL || period > MMSYSTIME_MAXINTERVAL)
        return TIMERR_NOCANDO;

    if (period > MMSYSTIME_MININTERVAL)
        WARN("Stub; we set our timer resolution at minimum\n");

    return 0;
}

MMRESULT WINAPI timeEndPeriod(UINT period)
{
    if (period < MMSYSTIME_MININTERVAL || period > MMSYSTIME_MAXINTERVAL)
        return TIMERR_NOCANDO;

    if (period > MMSYSTIME_MININTERVAL)
        WARN("Stub; we set our timer resolution at minimum\n");

    return 0;
}
