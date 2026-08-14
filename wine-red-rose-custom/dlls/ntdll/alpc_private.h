/*
 * ALPC helpers for PE and Unix side
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
#ifndef __NTDLL_ALPC_PRIVATE_H
#define __NTDLL_ALPC_PRIVATE_H

#include <stdarg.h>
#include <windef.h>
#include <winternl.h>
#include <ntstatus.h>

static inline SIZE_T alpc_get_header_size( ULONG attribute_flags )
{
    static const struct
    {
        ULONG attribute;
        SIZE_T size;
    }
    attribute_sizes[] =
    {
        /* Attribute with a higher bit is stored before that with a lower bit */
        {ALPC_MESSAGE_SECURITY_ATTRIBUTE, sizeof(ALPC_SECURITY_ATTR)},
        {ALPC_MESSAGE_VIEW_ATTRIBUTE, sizeof(ALPC_VIEW_ATTR)},
        {ALPC_MESSAGE_CONTEXT_ATTRIBUTE, sizeof(ALPC_CONTEXT_ATTR)},
        {ALPC_MESSAGE_HANDLE_ATTRIBUTE, sizeof(ALPC_HANDLE_ATTR)},
        {ALPC_MESSAGE_TOKEN_ATTRIBUTE, sizeof(ALPC_TOKEN_ATTR)},
        {ALPC_MESSAGE_DIRECT_ATTRIBUTE, sizeof(ALPC_DIRECT_ATTR)},
        {ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE, sizeof(ALPC_WORK_ON_BEHALF_ATTR)},
    };
    unsigned int i;
    SIZE_T size;

    size = sizeof(ALPC_MESSAGE_ATTRIBUTES);
    for (i = 0; i < ARRAY_SIZE(attribute_sizes); i++)
    {
        if (attribute_flags & attribute_sizes[i].attribute)
            size += attribute_sizes[i].size;
    }

    return size;
}

static inline void *alpc_get_message_attribute( ALPC_MESSAGE_ATTRIBUTES *attributes, ULONG attribute_flag )
{
    /* If no flag is specified */
    if (!attribute_flag)
        return NULL;

    /* If more than one flag is specified */
    if (attribute_flag & (attribute_flag - 1))
        return NULL;

    /* If the specified flag is not in allocated attributes */
    if ((attribute_flag & attributes->AllocatedAttributes & ALPC_MESSAGE_ATTRIBUTE_ALL) != attribute_flag)
        return NULL;

    return (unsigned char *)attributes + alpc_get_header_size(attributes->AllocatedAttributes & ~(attribute_flag | (attribute_flag - 1)));
}

static inline NTSTATUS alpc_initialize_message_attribute( ULONG attribute_flags,
                                                          ALPC_MESSAGE_ATTRIBUTES *buffer,
                                                          SIZE_T buffer_size,
                                                          SIZE_T *required_buffer_size )
{
    *required_buffer_size = alpc_get_header_size(attribute_flags);

    if (buffer_size < *required_buffer_size)
        return STATUS_BUFFER_TOO_SMALL;

    if (buffer)
    {
        buffer->AllocatedAttributes = attribute_flags;
        buffer->ValidAttributes = 0;
    }

    return STATUS_SUCCESS;
}

#endif /* __NTDLL_ALPC_PRIVATE_H */
