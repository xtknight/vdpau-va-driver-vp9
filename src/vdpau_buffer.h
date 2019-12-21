/*
 *  vdpau_buffer.h - VDPAU backend for VA-API (VA buffers)
 *
 *  libva-vdpau-driver (C) 2009-2011 Splitted-Desktop Systems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef VDPAU_BUFFER_H
#define VDPAU_BUFFER_H

#include "vdpau_driver.h"

typedef struct object_buffer object_buffer_t;
struct object_buffer {
    struct object_base  base;
    VAContextID         va_context;
    VABufferType        type;
    void               *buffer_data;
    unsigned int        buffer_size;
    unsigned int        max_num_elements;
    unsigned int        num_elements;
    uint64_t            mtime;
    unsigned int        delayed_destroy : 1;
};

// Destroy dead VA buffers
void
destroy_dead_va_buffers(
    vdpau_driver_data_t *driver_data,
    object_context_p     obj_context
) attribute_hidden;

// Create VA buffer object
object_buffer_p
create_va_buffer(
    vdpau_driver_data_p driver_data,
    VAContextID         context,
    VABufferType        buffer_type,
    unsigned int        num_elements,
    unsigned int        size
) attribute_hidden;

// Destroy VA buffer object
void
destroy_va_buffer(
    vdpau_driver_data_p driver_data,
    object_buffer_p     obj_buffer
) attribute_hidden;

// Schedule VA buffer object for destruction
void
schedule_destroy_va_buffer(
    vdpau_driver_data_p driver_data,
    object_buffer_p     obj_buffer
) attribute_hidden;

// vaCreateBuffer
VAStatus
vdpau_CreateBuffer(
    VADriverContextP    ctx,
    VAContextID         context,
    VABufferType        type,
    unsigned int        size,
    unsigned int        num_elements,
    void               *data,
    VABufferID         *buf_id
) attribute_hidden;

// vaDestroyBuffer
VAStatus
vdpau_DestroyBuffer(
    VADriverContextP    ctx,
    VABufferID          buffer_id
) attribute_hidden;

// vaBufferSetNumElements
VAStatus
vdpau_BufferSetNumElements(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    unsigned int        num_elements
) attribute_hidden;

// vaMapBuffer
VAStatus
vdpau_MapBuffer(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    void              **pbuf
) attribute_hidden;

// vaUnmapBuffer
VAStatus
vdpau_UnmapBuffer(
    VADriverContextP    ctx,
    VABufferID          buf_id
) attribute_hidden;

// vaBufferInfo
VAStatus
vdpau_BufferInfo_0_31_1(
    VADriverContextP    ctx,
    VAContextID         context,
    VABufferID          buf_id,
    VABufferType       *type,
    unsigned int       *size,
    unsigned int       *num_elements
) attribute_hidden;

VAStatus
vdpau_BufferInfo(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    VABufferType       *type,
    unsigned int       *size,
    unsigned int       *num_elements
) attribute_hidden;

#endif /* VDPAU_BUFFER_H */
