/*
 * Advanced Local Procedure Call
 *
 * Copyright 2026 Zhiyi Zhang for CodeWeavers
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
#include "alpc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(alpc);

SIZE_T WINAPI AlpcGetHeaderSize(ULONG attribute_flags)
{
    TRACE("%#lx.\n", attribute_flags);
    return alpc_get_header_size(attribute_flags);
}

void * WINAPI AlpcGetMessageAttribute(ALPC_MESSAGE_ATTRIBUTES *attributes, ULONG attribute_flag)
{
    TRACE("%p, %lx.\n", attributes, attribute_flag);
    return alpc_get_message_attribute(attributes, attribute_flag);
}

NTSTATUS WINAPI AlpcInitializeMessageAttribute(ULONG attribute_flags, ALPC_MESSAGE_ATTRIBUTES *buffer,
                                               SIZE_T buffer_size, SIZE_T *required_buffer_size)
{
    TRACE("%#lx, %p, %Ix, %p.\n", attribute_flags, buffer, buffer_size, required_buffer_size);
    return alpc_initialize_message_attribute(attribute_flags, buffer, buffer_size, required_buffer_size);
}
