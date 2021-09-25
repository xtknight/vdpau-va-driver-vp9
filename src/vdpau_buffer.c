/*
 *  vdpau_buffer.c - VDPAU backend for VA-API (VA buffers)
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

#include "sysdeps.h"
#include "vdpau_buffer.h"
#include "vdpau_driver.h"
#include "vdpau_video.h"
#include "vdpau_dump.h"
#include "utils.h"

#define DEBUG 1
#include "debug.h"

// Create VA buffer object
object_buffer_p
create_va_buffer(
    vdpau_driver_data_t *driver_data,
    VAContextID         context,
    VABufferType        buffer_type,
    unsigned int        num_elements,
    unsigned int        size
)
{
    VABufferID buffer_id;
    object_buffer_p obj_buffer;

    buffer_id = object_heap_allocate(&driver_data->buffer_heap);
    if (buffer_id == VA_INVALID_BUFFER)
        return NULL;

    obj_buffer = VDPAU_BUFFER(buffer_id);
    if (!obj_buffer)
        return NULL;

    obj_buffer->va_context       = context;
    obj_buffer->type             = buffer_type;
    obj_buffer->max_num_elements = num_elements;
    obj_buffer->num_elements     = num_elements;
    obj_buffer->buffer_size      = size * num_elements;
    obj_buffer->buffer_data      = malloc(obj_buffer->buffer_size);
    obj_buffer->mtime            = 0;
    obj_buffer->delayed_destroy  = 0;

    if (!obj_buffer->buffer_data) {
        destroy_va_buffer(driver_data, obj_buffer);
        return NULL;
    }
    return obj_buffer;
}

// Destroy VA buffer object
void
destroy_va_buffer(
    vdpau_driver_data_t *driver_data,
    object_buffer_p      obj_buffer
)
{
    if (!obj_buffer)
        return;

    if (obj_buffer->buffer_data) {
        free(obj_buffer->buffer_data);
        obj_buffer->buffer_data = NULL;
    }
    object_heap_free(&driver_data->buffer_heap, (object_base_p)obj_buffer);
}

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
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (buf_id)
        *buf_id = VA_INVALID_BUFFER;

    /* Validate type */
    switch (type) {
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VASliceParameterBufferType:
    case VASliceDataBufferType:
    case VABitPlaneBufferType:
    case VAImageBufferType:
        /* Ok */
        break;
    default:
        D(bug("ERROR: unsupported buffer type %d\n", type));
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    object_buffer_p obj_buffer;
    obj_buffer = create_va_buffer(driver_data, context, type, num_elements, size);
    if (!obj_buffer)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    if (data)
        memcpy(obj_buffer->buffer_data, data, obj_buffer->buffer_size);

    if (buf_id)
        *buf_id = obj_buffer->base.id;

    return VA_STATUS_SUCCESS;
}

// vaDestroyBuffer
VAStatus
vdpau_DestroyBuffer(
    VADriverContextP    ctx,
    VABufferID          buffer_id
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_buffer_p obj_buffer = VDPAU_BUFFER(buffer_id);

    if (obj_buffer && !obj_buffer->delayed_destroy)
        destroy_va_buffer(driver_data, obj_buffer);

    return VA_STATUS_SUCCESS;
}

// vaBufferSetNumElements
VAStatus
vdpau_BufferSetNumElements(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    unsigned int        num_elements
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_buffer_p obj_buffer = VDPAU_BUFFER(buf_id);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    if (num_elements < 0 || num_elements > obj_buffer->max_num_elements)
        return VA_STATUS_ERROR_UNKNOWN;

    obj_buffer->num_elements = num_elements;
    return VA_STATUS_SUCCESS;
}

// vaMapBuffer
VAStatus
vdpau_MapBuffer(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    void              **pbuf
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_buffer_p obj_buffer = VDPAU_BUFFER(buf_id);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    if (pbuf)
        *pbuf = obj_buffer->buffer_data;

    if (obj_buffer->buffer_data == NULL)
        return VA_STATUS_ERROR_UNKNOWN;

    ++obj_buffer->mtime;
    return VA_STATUS_SUCCESS;
}

// vaUnmapBuffer
VAStatus
vdpau_UnmapBuffer(
    VADriverContextP    ctx,
    VABufferID          buf_id
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_buffer_p obj_buffer = VDPAU_BUFFER(buf_id);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    ++obj_buffer->mtime;
    return VA_STATUS_SUCCESS;
}

// vaBufferInfo
VAStatus
vdpau_BufferInfo_0_31_1(
    VADriverContextP    ctx,
    VAContextID         context,
    VABufferID          buf_id,
    VABufferType       *type,
    unsigned int       *size,
    unsigned int       *num_elements
)
{
    return vdpau_BufferInfo(ctx, buf_id, type, size, num_elements);
}

VAStatus
vdpau_BufferInfo(
    VADriverContextP    ctx,
    VABufferID          buf_id,
    VABufferType       *type,
    unsigned int       *size,
    unsigned int       *num_elements
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_buffer_p obj_buffer = VDPAU_BUFFER(buf_id);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    if (type)
        *type = obj_buffer->type;
    if (size)
        *size = obj_buffer->buffer_size / obj_buffer->num_elements;
    if (num_elements)
        *num_elements = obj_buffer->num_elements;
    return VA_STATUS_SUCCESS;
}
