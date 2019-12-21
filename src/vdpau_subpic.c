/*
 *  vdpau_subpic.c - VDPAU backend for VA-API (VA subpictures)
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
#include "vdpau_subpic.h"
#include "vdpau_video.h"
#include "vdpau_image.h"
#include "vdpau_buffer.h"
#include "utils.h"

#define DEBUG 1
#include "debug.h"


// List of supported subpicture formats
typedef struct {
    VdpImageFormatType  vdp_format_type;
    uint32_t            vdp_format;
    VAImageFormat       va_format;
    unsigned int        va_flags;
} vdpau_subpic_format_map_t;

static const vdpau_subpic_format_map_t
vdpau_subpic_formats_map[VDPAU_MAX_SUBPICTURE_FORMATS + 1] = {
    { VDP_IMAGE_FORMAT_TYPE_INDEXED, VDP_INDEXED_FORMAT_A4I4,
      { VA_FOURCC('A','I','4','4'), VA_MSB_FIRST, 8, },
      0 },
    { VDP_IMAGE_FORMAT_TYPE_INDEXED, VDP_INDEXED_FORMAT_I4A4,
      { VA_FOURCC('I','A','4','4'), VA_MSB_FIRST, 8, },
      0 },
    { VDP_IMAGE_FORMAT_TYPE_INDEXED, VDP_INDEXED_FORMAT_A8I8,
      { VA_FOURCC('A','I','8','8'), VA_MSB_FIRST, 16, },
      0 },
    { VDP_IMAGE_FORMAT_TYPE_INDEXED, VDP_INDEXED_FORMAT_I8A8,
      { VA_FOURCC('I','A','8','8'), VA_MSB_FIRST, 16, },
      0 },
#ifdef WORDS_BIGENDIAN
    { VDP_IMAGE_FORMAT_TYPE_RGBA, VDP_RGBA_FORMAT_B8G8R8A8,
      { VA_FOURCC('A','R','G','B'), VA_MSB_FIRST, 32,
        32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
      VA_SUBPICTURE_GLOBAL_ALPHA },
    { VDP_IMAGE_FORMAT_TYPE_RGBA, VDP_RGBA_FORMAT_R8G8B8A8,
      { VA_FOURCC('A','B','G','R'), VA_MSB_FIRST, 32,
        32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
      VA_SUBPICTURE_GLOBAL_ALPHA },
#else
    { VDP_IMAGE_FORMAT_TYPE_RGBA, VDP_RGBA_FORMAT_B8G8R8A8,
      { VA_FOURCC('B','G','R','A'), VA_LSB_FIRST, 32,
        32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
      VA_SUBPICTURE_GLOBAL_ALPHA },
    { VDP_IMAGE_FORMAT_TYPE_RGBA, VDP_RGBA_FORMAT_R8G8B8A8,
      { VA_FOURCC('R','G','B','A'), VA_LSB_FIRST, 32,
        32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
      VA_SUBPICTURE_GLOBAL_ALPHA },
#endif
    { 0, VDP_INVALID_HANDLE, }
};

// Returns a suitable VDPAU subpicture format for the specified VA image format
static const vdpau_subpic_format_map_t *get_format(const VAImageFormat *format)
{
    unsigned int i;
    for (i = 0; vdpau_subpic_formats_map[i].va_format.fourcc != 0; i++) {
        const vdpau_subpic_format_map_t * const m = &vdpau_subpic_formats_map[i];
        if (m->va_format.fourcc == format->fourcc &&
            (m->vdp_format_type == VDP_IMAGE_FORMAT_TYPE_RGBA ?
             (m->va_format.byte_order == format->byte_order &&
              m->va_format.red_mask   == format->red_mask   &&
              m->va_format.green_mask == format->green_mask &&
              m->va_format.blue_mask  == format->blue_mask  &&
              m->va_format.alpha_mask == format->alpha_mask) : 1))
            return m;
    }
    return NULL;
}

// Checks whether the VDPAU implementation supports the specified image format
static inline VdpBool
is_supported_format(
    vdpau_driver_data_t             *driver_data,
    const vdpau_subpic_format_map_t *format)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus vdp_status;
    uint32_t max_width, max_height;

    switch (format->vdp_format_type) {
    case VDP_IMAGE_FORMAT_TYPE_RGBA:
        vdp_status = vdpau_bitmap_surface_query_capabilities(
            driver_data,
            driver_data->vdp_device,
            format->vdp_format,
            &is_supported,
            &max_width,
            &max_height
        );
        break;
    case VDP_IMAGE_FORMAT_TYPE_INDEXED:
        vdp_status = vdpau_output_surface_query_put_bits_indexed_capabilities(
            driver_data,
            driver_data->vdp_device,
            VDP_RGBA_FORMAT_B8G8R8A8,
            format->vdp_format,
            VDP_COLOR_TABLE_FORMAT_B8G8R8X8,
            &is_supported
        );
        break;
    default:
        vdp_status = VDP_STATUS_ERROR;
        break;
    }
    return vdp_status == VDP_STATUS_OK && is_supported;
}

// Append association to the subpicture
static int
subpicture_add_association(
    object_subpicture_p    obj_subpicture,
    SubpictureAssociationP assoc
)
{
    SubpictureAssociationP *assocs;
    assocs = realloc_buffer(
        (void **)&obj_subpicture->assocs,
        &obj_subpicture->assocs_count_max,
        1 + obj_subpicture->assocs_count,
        sizeof(obj_subpicture->assocs[0])
    );
    if (!assocs)
        return -1;

    assocs[obj_subpicture->assocs_count++] = assoc;
    return 0;
}

// Remove association at the specified index from the subpicture
static inline int
subpicture_remove_association_at(object_subpicture_p obj_subpicture, int index)
{
    ASSERT(obj_subpicture->assocs && obj_subpicture->assocs_count > 0);
    if (!obj_subpicture->assocs || obj_subpicture->assocs_count == 0)
        return -1;

    /* Replace with the last association */
    const unsigned int last = obj_subpicture->assocs_count - 1;
    obj_subpicture->assocs[index] = obj_subpicture->assocs[last];
    obj_subpicture->assocs[last] = NULL;
    obj_subpicture->assocs_count--;
    return 0;
}

#if 0
// Remove association from the subpicture
static int
subpicture_remove_association(
    object_subpicture_p    obj_subpicture,
    SubpictureAssociationP assoc
)
{
    ASSERT(obj_subpicture->assocs && obj_subpicture->assocs_count > 0);
    if (!obj_subpicture->assocs || obj_subpicture->assocs_count == 0)
        return -1;

    unsigned int i;
    for (i = 0; i < obj_subpicture->assocs_count; i++) {
        if (obj_subpicture->assocs[i] == assoc)
            return subpicture_remove_association_at(obj_subpicture, i);
    }
    return -1;
}
#endif

// Associate one surface to the subpicture
VAStatus
subpicture_associate_1(
    object_subpicture_p obj_subpicture,
    object_surface_p    obj_surface,
    const VARectangle  *src_rect,
    const VARectangle  *dst_rect,
    unsigned int        flags
)
{
    /* we only support the VA_SUBPICTURE_GLOBAL_ALPHA flag */
    if (flags & ~VA_SUBPICTURE_GLOBAL_ALPHA)
        return VA_STATUS_ERROR_FLAG_NOT_SUPPORTED;

    SubpictureAssociationP assoc = malloc(sizeof(*assoc));
    if (!assoc)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    assoc->subpicture = obj_subpicture->base.id;
    assoc->surface    = obj_surface->base.id;
    assoc->src_rect   = *src_rect;
    assoc->dst_rect   = *dst_rect;
    assoc->flags      = flags;

    VAStatus status = surface_add_association(obj_surface, assoc);
    if (status !=  VA_STATUS_SUCCESS) {
        free(assoc);
        return status;
    }

    if (subpicture_add_association(obj_subpicture, assoc) < 0) {
        surface_remove_association(obj_surface, assoc);
        free(assoc);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    return VA_STATUS_SUCCESS;
}

// Associate surfaces to the subpicture
static VAStatus
associate_subpicture(
    vdpau_driver_data_t *driver_data,
    object_subpicture_p obj_subpicture,
    VASurfaceID        *surfaces,
    unsigned int        num_surfaces,
    const VARectangle  *src_rect,
    const VARectangle  *dst_rect,
    unsigned int        flags
)
{
    VAStatus status;
    unsigned int i;

    for (i = 0; i < num_surfaces; i++) {
        object_surface_p const obj_surface = VDPAU_SURFACE(surfaces[i]);
        if (!obj_surface)
            return VA_STATUS_ERROR_INVALID_SURFACE;
        status = subpicture_associate_1(obj_subpicture, obj_surface,
                                        src_rect, dst_rect, flags);
        if (status != VA_STATUS_SUCCESS)
            return status;
    }
    return VA_STATUS_SUCCESS;
}

// Deassociate one surface from the subpicture
VAStatus
subpicture_deassociate_1(
    object_subpicture_p obj_subpicture,
    object_surface_p    obj_surface
)
{
    ASSERT(obj_subpicture->assocs && obj_subpicture->assocs_count > 0);
    if (!obj_subpicture->assocs || obj_subpicture->assocs_count == 0)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    unsigned int i;
    for (i = 0; i < obj_subpicture->assocs_count; i++) {
        SubpictureAssociationP const assoc = obj_subpicture->assocs[i];
        ASSERT(assoc);
        if (assoc && assoc->surface == obj_surface->base.id) {
            surface_remove_association(obj_surface, assoc);
            subpicture_remove_association_at(obj_subpicture, i);
            free(assoc);
            return VA_STATUS_SUCCESS;
        }
    }
    return VA_STATUS_ERROR_OPERATION_FAILED;
}

// Deassociate surfaces from the subpicture
static VAStatus
deassociate_subpicture(
    vdpau_driver_data_t *driver_data,
    object_subpicture_p obj_subpicture,
    VASurfaceID        *surfaces,
    unsigned int        num_surfaces
)
{
    VAStatus status, error = VA_STATUS_SUCCESS;
    unsigned int i;

    for (i = 0; i < num_surfaces; i++) {
        object_surface_p const obj_surface = VDPAU_SURFACE(surfaces[i]);
        if (!obj_surface)
            return VA_STATUS_ERROR_INVALID_SURFACE;
        status = subpicture_deassociate_1(obj_subpicture, obj_surface);
        if (status != VA_STATUS_SUCCESS) {
            /* Simply report the first error to the user */
            if (error == VA_STATUS_SUCCESS)
                error = status;
        }
    }
    return error;
}

// Commit subpicture to VDPAU surface
VAStatus
commit_subpicture(
    vdpau_driver_data_p driver_data,
    object_subpicture_p obj_subpicture
)
{
    object_image_p obj_image = VDPAU_IMAGE(obj_subpicture->image_id);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    ASSERT(obj_subpicture->width == obj_image->image.width);
    if (obj_subpicture->width != obj_image->image.width)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    ASSERT(obj_subpicture->height == obj_image->image.height);
    if (obj_subpicture->height != obj_image->image.height)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_buffer_p obj_buffer = VDPAU_BUFFER(obj_image->image.buf);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    /* Update video surface only if the image (hence its buffer) was
       updated since our last synchronisation.

       NOTE: this assumes the user really unmaps the buffer when he is
       done with it, as it is actually required */
    if (obj_subpicture->last_commit >= obj_buffer->mtime)
        return VA_STATUS_SUCCESS;

    VdpRect dirty_rect;
    dirty_rect.x0 = obj_subpicture->width;
    dirty_rect.y0 = obj_subpicture->height;
    dirty_rect.x1 = 0;
    dirty_rect.y1 = 0;

    unsigned int i;
    for (i = 0; i < obj_subpicture->assocs_count; i++) {
        const VARectangle * const rect = &obj_subpicture->assocs[i]->src_rect;
        dirty_rect.x0 = MIN(dirty_rect.x0, rect->x);
        dirty_rect.y0 = MIN(dirty_rect.y0, rect->y);
        dirty_rect.x1 = MAX(dirty_rect.x1, rect->x + rect->width);
        dirty_rect.y1 = MAX(dirty_rect.y1, rect->y + rect->height);
    }

    const uint8_t *src;
    uint32_t src_stride;
    src_stride = obj_image->image.pitches[0];
    src = ((uint8_t *)obj_buffer->buffer_data + obj_image->image.offsets[0] +
           dirty_rect.y0 * obj_image->image.pitches[0] +
           dirty_rect.x0 * ((obj_image->image.format.bits_per_pixel + 7) / 8));

    VdpStatus vdp_status;
    switch (obj_subpicture->vdp_format_type) {
    case VDP_IMAGE_FORMAT_TYPE_RGBA:
        vdp_status = vdpau_bitmap_surface_put_bits_native(
            driver_data,
            obj_subpicture->vdp_bitmap_surface,
            &src, &src_stride,
            &dirty_rect
        );
        break;
    case VDP_IMAGE_FORMAT_TYPE_INDEXED:
        vdp_status = vdpau_output_surface_put_bits_indexed(
            driver_data,
            obj_subpicture->vdp_output_surface,
            obj_subpicture->vdp_format,
            &src, &src_stride,
            &dirty_rect,
            VDP_COLOR_TABLE_FORMAT_B8G8R8X8,
            obj_image->vdp_palette
        );
        break;
    default:
        vdp_status = VDP_STATUS_ERROR;
        break;
    }
    if (vdp_status != VDP_STATUS_OK)
        return vdpau_get_VAStatus(vdp_status);

    obj_subpicture->last_commit = obj_buffer->mtime;
    return VA_STATUS_SUCCESS;
}

// Create subpicture with image
static VAStatus
create_subpicture(
    vdpau_driver_data_t *driver_data,
    object_image_p       obj_image,
    VASubpictureID      *subpicture
)
{
    *subpicture = object_heap_allocate(&driver_data->subpicture_heap);
    if (*subpicture == VA_INVALID_ID)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(*subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    const vdpau_subpic_format_map_t *m = get_format(&obj_image->image.format);
    if (!is_supported_format(driver_data, m))
        return VA_STATUS_ERROR_UNKNOWN; /* VA_STATUS_ERROR_UNSUPPORTED_FORMAT */

    obj_subpicture->image_id           = obj_image->base.id;
    obj_subpicture->assocs             = NULL;
    obj_subpicture->assocs_count       = 0;
    obj_subpicture->assocs_count_max   = 0;
    obj_subpicture->width              = obj_image->image.width;
    obj_subpicture->height             = obj_image->image.height;
    obj_subpicture->vdp_bitmap_surface = VDP_INVALID_HANDLE;
    obj_subpicture->vdp_output_surface = VDP_INVALID_HANDLE;
    obj_subpicture->last_commit        = 0;
    obj_subpicture->vdp_format_type    = m->vdp_format_type;
    obj_subpicture->vdp_format         = m->vdp_format;
    obj_subpicture->alpha              = 1.0;

    VdpStatus vdp_status;
    switch (obj_subpicture->vdp_format_type) {
    case VDP_IMAGE_FORMAT_TYPE_RGBA:
        vdp_status = vdpau_bitmap_surface_create(
            driver_data,
            driver_data->vdp_device,
            obj_subpicture->vdp_format,
            obj_subpicture->width,
            obj_subpicture->height,
            VDP_FALSE,
            &obj_subpicture->vdp_bitmap_surface
        );
        break;
    case VDP_IMAGE_FORMAT_TYPE_INDEXED:
        vdp_status = vdpau_output_surface_create(
            driver_data,
            driver_data->vdp_device,
            VDP_RGBA_FORMAT_B8G8R8A8,
            obj_subpicture->width,
            obj_subpicture->height,
            &obj_subpicture->vdp_output_surface
        );
        break;
    default:
        vdp_status = VDP_STATUS_ERROR;
        break;
    }
    return vdpau_get_VAStatus(vdp_status);
}

// Destroy subpicture
static void
destroy_subpicture(
    vdpau_driver_data_t *driver_data,
    object_subpicture_p obj_subpicture
)
{
    object_surface_p obj_surface;
    VAStatus status;
    unsigned int i, n;

    if (obj_subpicture->assocs) {
        const unsigned int n_assocs = obj_subpicture->assocs_count;
        for (i = 0, n = 0; i < n_assocs; i++) {
            SubpictureAssociationP const assoc = obj_subpicture->assocs[0];
            if (!assoc)
                continue;
            obj_surface = VDPAU_SURFACE(assoc->surface);
            if (!obj_surface)
                continue;
            status = subpicture_deassociate_1(obj_subpicture, obj_surface);
            if (status == VA_STATUS_SUCCESS)
                ++n;
        }
        if (n != n_assocs)
            vdpau_error_message("vaDestroySubpicture(): subpicture 0x%08x still "
                               "has %d surfaces associated to it\n",
                               obj_subpicture->base.id, n_assocs - n);
        free(obj_subpicture->assocs);
        obj_subpicture->assocs = NULL;
    }
    obj_subpicture->assocs_count = 0;
    obj_subpicture->assocs_count_max = 0;

    if (obj_subpicture->vdp_bitmap_surface != VDP_INVALID_HANDLE) {
        vdpau_bitmap_surface_destroy(
            driver_data,
            obj_subpicture->vdp_bitmap_surface
        );
        obj_subpicture->vdp_bitmap_surface = VDP_INVALID_HANDLE;
    }

    if (obj_subpicture->vdp_output_surface != VDP_INVALID_HANDLE) {
        vdpau_output_surface_destroy(
            driver_data,
            obj_subpicture->vdp_output_surface
        );
        obj_subpicture->vdp_output_surface = VDP_INVALID_HANDLE;
    }

    obj_subpicture->image_id = VA_INVALID_ID;
    object_heap_free(&driver_data->subpicture_heap,
                     (object_base_p)obj_subpicture);
}

// vaQuerySubpictureFormats
VAStatus
vdpau_QuerySubpictureFormats(
    VADriverContextP    ctx,
    VAImageFormat      *format_list,
    unsigned int       *flags,
    unsigned int       *num_formats
)
{
    VDPAU_DRIVER_DATA_INIT;

    int n;
    for (n = 0; vdpau_subpic_formats_map[n].va_format.fourcc != 0; n++) {
        const vdpau_subpic_format_map_t * const m = &vdpau_subpic_formats_map[n];
        if (is_supported_format(driver_data, m)) {
            if (format_list)
                format_list[n] = m->va_format;
            if (flags)
                flags[n] = m->va_flags;
        }
    }

    if (num_formats)
        *num_formats = n;

    return VA_STATUS_SUCCESS;
}

// vaCreateSubpicture
VAStatus
vdpau_CreateSubpicture(
    VADriverContextP    ctx,
    VAImageID           image,
    VASubpictureID     *subpicture
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (!subpicture)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    return create_subpicture(driver_data, obj_image, subpicture);
}

// vaDestroySubpicture
VAStatus
vdpau_DestroySubpicture(
    VADriverContextP    ctx,
    VASubpictureID      subpicture
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    destroy_subpicture(driver_data, obj_subpicture);
    return VA_STATUS_SUCCESS;
}

// vaSetSubpictureImage
VAStatus
vdpau_SetSubpictureImage(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VAImageID           image
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    obj_subpicture->image_id = obj_image->base.id;
    return VA_STATUS_SUCCESS;
}

// vaSetSubpicturePalette (not a PUBLIC interface)
VAStatus
vdpau_SetSubpicturePalette(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    unsigned char      *palette
)
{
    /* TODO */
    return VA_STATUS_ERROR_OPERATION_FAILED;
}

// vaSetSubpictureChromaKey
VAStatus
vdpau_SetSubpictureChromakey(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    unsigned int        chromakey_min,
    unsigned int        chromakey_max,
    unsigned int        chromakey_mask
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    obj_subpicture->chromakey_min  = chromakey_min;
    obj_subpicture->chromakey_max  = chromakey_max;
    obj_subpicture->chromakey_mask = chromakey_mask;
    return VA_STATUS_SUCCESS;
}

// vaSetSubpictureGlobalAlpha
VAStatus
vdpau_SetSubpictureGlobalAlpha(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    float               global_alpha
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    obj_subpicture->alpha = global_alpha;
    return VA_STATUS_SUCCESS;
}

// vaAssociateSubpicture
VAStatus
vdpau_AssociateSubpicture(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VASurfaceID        *target_surfaces,
    int                 num_surfaces,
    short               src_x,
    short               src_y,
    short               dest_x,
    short               dest_y,
    unsigned short      width,
    unsigned short      height,
    unsigned int        flags
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (!target_surfaces || num_surfaces == 0)
        return VA_STATUS_SUCCESS;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    VARectangle src_rect, dst_rect;
    src_rect.x      = src_x;
    src_rect.y      = src_y;
    src_rect.width  = width;
    src_rect.height = height;
    dst_rect.x      = dest_x;
    dst_rect.y      = dest_y;
    dst_rect.width  = width;
    dst_rect.height = height;
    return associate_subpicture(driver_data, obj_subpicture,
                                target_surfaces, num_surfaces,
                                &src_rect, &dst_rect, flags);
}

// vaAssociateSubpicture2
VAStatus
vdpau_AssociateSubpicture_full(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VASurfaceID        *target_surfaces,
    int                 num_surfaces,
    short               src_x,
    short               src_y,
    unsigned short      src_width,
    unsigned short      src_height,
    short               dest_x,
    short               dest_y,
    unsigned short      dest_width,
    unsigned short      dest_height,
    unsigned int        flags
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (!target_surfaces || num_surfaces == 0)
        return VA_STATUS_SUCCESS;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    VARectangle src_rect, dst_rect;
    src_rect.x      = src_x;
    src_rect.y      = src_y;
    src_rect.width  = src_width;
    src_rect.height = src_height;
    dst_rect.x      = dest_x;
    dst_rect.y      = dest_y;
    dst_rect.width  = dest_width;
    dst_rect.height = dest_height;
    return associate_subpicture(driver_data, obj_subpicture,
                                target_surfaces, num_surfaces,
                                &src_rect, &dst_rect, flags);
}

// vaDeassociateSubpicture
VAStatus
vdpau_DeassociateSubpicture(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VASurfaceID        *target_surfaces,
    int                 num_surfaces
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (!target_surfaces || num_surfaces == 0)
        return VA_STATUS_SUCCESS;

    object_subpicture_p obj_subpicture = VDPAU_SUBPICTURE(subpicture);
    if (!obj_subpicture)
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;

    return deassociate_subpicture(driver_data, obj_subpicture,
                                  target_surfaces, num_surfaces);
}
