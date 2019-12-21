/*
 *  vdpau_image.c - VDPAU backend for VA-API (VA images)
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
#include "vdpau_image.h"
#include "vdpau_video.h"
#include "vdpau_buffer.h"
#include "vdpau_mixer.h"

#define DEBUG 1
#include "debug.h"


// List of supported image formats
typedef struct {
    VdpImageFormatType  vdp_format_type;
    uint32_t            vdp_format;
    VAImageFormat       va_format;
    unsigned int        num_palette_entries;
    unsigned int        entry_bytes;
    char                component_order[4];
} vdpau_image_format_map_t;

static const vdpau_image_format_map_t vdpau_image_formats_map[] = {
#define DEF(TYPE, FORMAT) \
    VDP_IMAGE_FORMAT_TYPE_##TYPE, VDP_##TYPE##_FORMAT_##FORMAT
#define DEF_YUV(TYPE, FORMAT, FOURCC, ENDIAN, BPP) \
    { DEF(TYPE, FORMAT), { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, }, }
#define DEF_RGB(TYPE, FORMAT, FOURCC, ENDIAN, BPP, DEPTH, R,G,B,A) \
    { DEF(TYPE, FORMAT), { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, DEPTH, R,G,B,A }, }
#define DEF_IDX(TYPE, FORMAT, FOURCC, ENDIAN, BPP, NPE, EB, C0,C1,C2,C3) \
    { DEF(TYPE, FORMAT), { VA_FOURCC FOURCC, VA_##ENDIAN##_FIRST, BPP, }, \
      NPE, EB, { C0, C1, C2, C3 } }
    DEF_YUV(YCBCR, NV12,        ('N','V','1','2'), LSB, 12),
    DEF_YUV(YCBCR, YV12,        ('Y','V','1','2'), LSB, 12),
    DEF_YUV(YCBCR, YV12,        ('I','4','2','0'), LSB, 12), // swap U/V planes
    DEF_YUV(YCBCR, UYVY,        ('U','Y','V','Y'), LSB, 16),
    DEF_YUV(YCBCR, YUYV,        ('Y','U','Y','V'), LSB, 16),
    DEF_YUV(YCBCR, V8U8Y8A8,    ('A','Y','U','V'), LSB, 32),
#ifdef WORDS_BIGENDIAN
    DEF_RGB(RGBA, B8G8R8A8,     ('A','R','G','B'), MSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(RGBA, R8G8B8A8,     ('A','B','G','R'), MSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
#else
    DEF_RGB(RGBA, B8G8R8A8,     ('B','G','R','A'), LSB, 32,
            32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000),
    DEF_RGB(RGBA, R8G8B8A8,     ('R','G','B','A'), LSB, 32,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000),
#endif
    DEF_IDX(INDEXED, A4I4,      ('A','I','4','4'), MSB, 8,
            16, 3, 'R','G','B',0),
    DEF_IDX(INDEXED, I4A4,      ('I','A','4','4'), MSB, 8,
            16, 3, 'R','G','B',0),
    DEF_IDX(INDEXED, A8I8,      ('A','I','8','8'), MSB, 16,
            256, 3, 'R','G','B',0),
    DEF_IDX(INDEXED, I8A8,      ('I','A','8','8'), MSB, 16,
            256, 3, 'R','G','B',0),
#undef DEF_IDX
#undef DEF_RGB
#undef DEF_YUV
#undef DEF
};

// Returns a suitable VDPAU image format for the specified VA image format
static const vdpau_image_format_map_t *get_format(const VAImageFormat *format)
{
    unsigned int i;
    for (i = 0; i < ARRAY_ELEMS(vdpau_image_formats_map); i++) {
        const vdpau_image_format_map_t * const m = &vdpau_image_formats_map[i];
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
    vdpau_driver_data_t *driver_data,
    VdpImageFormatType   type,
    uint32_t             format
)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus vdp_status;

    switch (type) {
    case VDP_IMAGE_FORMAT_TYPE_YCBCR:
        vdp_status =
            vdpau_video_surface_query_ycbcr_caps(driver_data,
                                                 driver_data->vdp_device,
                                                 VDP_CHROMA_TYPE_420,
                                                 format,
                                                 &is_supported);
        break;
    case VDP_IMAGE_FORMAT_TYPE_RGBA:
        vdp_status =
            vdpau_output_surface_query_rgba_caps(driver_data,
                                                 driver_data->vdp_device,
                                                 format,
                                                 &is_supported);
        break;
    default:
        vdp_status = VDP_STATUS_INVALID_VALUE;
        break;
    }
    return vdp_status == VDP_STATUS_OK && is_supported;
}

// vaQueryImageFormats
VAStatus
vdpau_QueryImageFormats(
    VADriverContextP    ctx,
    VAImageFormat      *format_list,
    int                *num_formats
)
{
    VDPAU_DRIVER_DATA_INIT;

    if (num_formats)
        *num_formats = 0;

    if (format_list == NULL)
        return VA_STATUS_SUCCESS;

    int i, n = 0;
    for (i = 0; i < ARRAY_ELEMS(vdpau_image_formats_map); i++) {
        const vdpau_image_format_map_t * const f = &vdpau_image_formats_map[i];
        if (is_supported_format(driver_data, f->vdp_format_type, f->vdp_format))
            format_list[n++] = f->va_format;
    }

    /* If the assert fails then VDPAU_MAX_IMAGE_FORMATS needs to be bigger */
    ASSERT(n <= VDPAU_MAX_IMAGE_FORMATS);
    if (num_formats)
        *num_formats = n;

    return VA_STATUS_SUCCESS;
}

// vaCreateImage
VAStatus
vdpau_CreateImage(
    VADriverContextP    ctx,
    VAImageFormat      *format,
    int                 width,
    int                 height,
    VAImage            *out_image
)
{
    VDPAU_DRIVER_DATA_INIT;

    VAStatus va_status = VA_STATUS_ERROR_OPERATION_FAILED;
    unsigned int i, width2, height2, size2, size;

    if (!format || !out_image)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    out_image->image_id = VA_INVALID_ID;
    out_image->buf      = VA_INVALID_ID;

    VAImageID image_id = object_heap_allocate(&driver_data->image_heap);
    if (image_id == VA_INVALID_ID) {
        va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
        goto error;
    }

    object_image_p obj_image = VDPAU_IMAGE(image_id);
    if (!obj_image) {
        va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
        goto error;
    }

    const vdpau_image_format_map_t *m = get_format(format);
    if (!m) {
        va_status = VA_STATUS_ERROR_UNKNOWN; /* VA_STATUS_ERROR_UNSUPPORTED_FORMAT */
        goto error;
    }

    VAImage * const image = &obj_image->image;
    image->image_id       = image_id;
    image->buf            = VA_INVALID_ID;

    size    = width * height;
    width2  = (width  + 1) / 2;
    height2 = (height + 1) / 2;
    size2   = width2 * height2;

    switch (format->fourcc) {
    case VA_FOURCC('N','V','1','2'):
        image->num_planes = 2;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->pitches[1] = width;
        image->offsets[1] = size;
        image->data_size  = size + 2 * size2;
        break;
    case VA_FOURCC('Y','V','1','2'):
    case VA_FOURCC('I','4','2','0'):
        image->num_planes = 3;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->pitches[1] = width2;
        image->offsets[1] = size;
        image->pitches[2] = width2;
        image->offsets[2] = size + size2;
        image->data_size  = size + 2 * size2;
        break;
    case VA_FOURCC('A','R','G','B'):
    case VA_FOURCC('A','B','G','R'):
    case VA_FOURCC('B','G','R','A'):
    case VA_FOURCC('R','G','B','A'):
    case VA_FOURCC('U','Y','V','Y'):
    case VA_FOURCC('Y','U','Y','V'):
        image->num_planes = 1;
        image->pitches[0] = width * 4;
        image->offsets[0] = 0;
        image->data_size  = image->offsets[0] + image->pitches[0] * height;
        break;
    case VA_FOURCC('I','A','4','4'):
    case VA_FOURCC('A','I','4','4'):
        image->num_planes = 1;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->data_size  = image->offsets[0] + image->pitches[0] * height;
        break;
    case VA_FOURCC('I','A','8','8'):
    case VA_FOURCC('A','I','8','8'):
        image->num_planes = 1;
        image->pitches[0] = width * 2;
        image->offsets[0] = 0;
        image->data_size  = image->offsets[0] + image->pitches[0] * height;
        break;
    default:
        goto error;
    }

    /* Allocate more bytes to align image data base on 16-byte boundaries */
    /* XXX: align other planes too? */
    static const int ALIGN = 16;

    va_status = vdpau_CreateBuffer(ctx, 0, VAImageBufferType,
                                   image->data_size + ALIGN, 1, NULL,
                                   &image->buf);
    if (va_status != VA_STATUS_SUCCESS)
        goto error;

    object_buffer_p obj_buffer = VDPAU_BUFFER(image->buf);
    if (!obj_buffer)
        goto error;

    int align = ((uintptr_t)obj_buffer->buffer_data) % ALIGN;
    if (align) {
        align = ALIGN - align;
        for (i = 0; i < image->num_planes; i++)
            image->offsets[i] += align;
    }

    obj_image->vdp_rgba_output_surface = VDP_INVALID_HANDLE;
    obj_image->vdp_format_type  = m->vdp_format_type;
    obj_image->vdp_format       = m->vdp_format;
    obj_image->vdp_palette      = NULL;

    image->image_id             = image_id;
    image->format               = *format;
    image->width                = width;
    image->height               = height;
    image->num_palette_entries  = m->num_palette_entries;
    image->entry_bytes          = m->entry_bytes;
    for (i = 0; i < image->entry_bytes; i++)
        image->component_order[i] = m->component_order[i];
    for (; i < 4; i++)
        image->component_order[i] = 0;

    *out_image                  = *image;
    return VA_STATUS_SUCCESS;

 error:
    vdpau_DestroyImage(ctx, image_id);
    return va_status;
}

// vaDestroyImage
VAStatus
vdpau_DestroyImage(
    VADriverContextP    ctx,
    VAImageID           image_id
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_image_p obj_image = VDPAU_IMAGE(image_id);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    if (obj_image->vdp_rgba_output_surface != VDP_INVALID_HANDLE)
        vdpau_output_surface_destroy(driver_data,
                                     obj_image->vdp_rgba_output_surface);

    if (obj_image->vdp_palette) {
        free(obj_image->vdp_palette);
        obj_image->vdp_palette = NULL;
    }

    VABufferID buf = obj_image->image.buf;
    object_heap_free(&driver_data->image_heap, (object_base_p)obj_image);
    return vdpau_DestroyBuffer(ctx, buf);
}

// vaDeriveImage
VAStatus
vdpau_DeriveImage(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    VAImage             *image
)
{
    /* TODO */
    return VA_STATUS_ERROR_OPERATION_FAILED;
}

// Set image palette
static VAStatus
set_image_palette(
    vdpau_driver_data_t *driver_data,
    object_image_p       obj_image,
    const unsigned char *palette
)
{
    if (obj_image->vdp_format_type != VDP_IMAGE_FORMAT_TYPE_INDEXED)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    if (!obj_image->vdp_palette) {
        obj_image->vdp_palette = malloc(4 * obj_image->image.num_palette_entries);
        if (!obj_image->vdp_palette)
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    unsigned int i;
    for (i = 0; i < obj_image->image.num_palette_entries; i++) {
        /* B8G8R8X8 format */
        obj_image->vdp_palette[i] = ((palette[3*i + 0] << 16) |
                                     (palette[3*i + 1] <<  8) |
                                      palette[3*i + 2]);
    }
    return VA_STATUS_SUCCESS;
}

// vaSetImagePalette
VAStatus
vdpau_SetImagePalette(
    VADriverContextP    ctx,
    VAImageID           image,
    unsigned char      *palette
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    return set_image_palette(driver_data, obj_image, palette);
}

// Get image from surface
static VAStatus
get_image(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface,
    object_image_p       obj_image,
    const VARectangle   *rect
)
{
    VAImage * const image = &obj_image->image;
    VdpStatus vdp_status;
    uint8_t *src[3];
    unsigned int src_stride[3];
    int i;

    object_buffer_p obj_buffer = VDPAU_BUFFER(image->buf);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    switch (image->format.fourcc) {
    case VA_FOURCC('I','4','2','0'):
        src[0] = (uint8_t *)obj_buffer->buffer_data + image->offsets[0];
        src_stride[0] = image->pitches[0];
        src[1] = (uint8_t *)obj_buffer->buffer_data + image->offsets[2];
        src_stride[1] = image->pitches[2];
        src[2] = (uint8_t *)obj_buffer->buffer_data + image->offsets[1];
        src_stride[2] = image->pitches[1];
        break;
    default:
        for (i = 0; i < image->num_planes; i++) {
            src[i] = (uint8_t *)obj_buffer->buffer_data + image->offsets[i];
            src_stride[i] = image->pitches[i];
        }
        break;
    }

    switch (obj_image->vdp_format_type) {
    case VDP_IMAGE_FORMAT_TYPE_YCBCR: {
        /* VDPAU only supports full video surface readback */
        if (rect->x != 0 ||
            rect->y != 0 ||
            obj_surface->width  != rect->width ||
            obj_surface->height != rect->height)
            return VA_STATUS_ERROR_OPERATION_FAILED;

        vdp_status = vdpau_video_surface_get_bits_ycbcr(
            driver_data,
            obj_surface->vdp_surface,
            obj_image->vdp_format,
            src, src_stride
        );
        break;
    }
    case VDP_IMAGE_FORMAT_TYPE_RGBA: {
        if (obj_image->vdp_rgba_output_surface == VA_INVALID_ID) {
            vdp_status = vdpau_output_surface_create(
                driver_data,
                driver_data->vdp_device,
                obj_image->vdp_format,
                obj_image->image.width,
                obj_image->image.height,
                &obj_image->vdp_rgba_output_surface
            );
            if (vdp_status != VDP_STATUS_OK)
                return vdpau_get_VAStatus(vdp_status);
        }

        VdpRect vdp_rect;
        vdp_rect.x0 = rect->x;
        vdp_rect.y0 = rect->y;
        vdp_rect.x1 = rect->x + rect->width;
        vdp_rect.y1 = rect->y + rect->height;
        vdp_status = video_mixer_render(
            driver_data,
            obj_surface->video_mixer,
            obj_surface,
            VDP_INVALID_HANDLE,
            obj_image->vdp_rgba_output_surface,
            &vdp_rect,
            &vdp_rect,
            0
        );
        if (vdp_status != VDP_STATUS_OK)
            return vdpau_get_VAStatus(vdp_status);

        vdp_status = vdpau_output_surface_get_bits_native(
            driver_data,
            obj_image->vdp_rgba_output_surface,
            &vdp_rect,
            src, src_stride
        );
        break;
    }
    default:
        return VA_STATUS_ERROR_OPERATION_FAILED;
    }
    return vdpau_get_VAStatus(vdp_status);
}

// vaGetImage
VAStatus
vdpau_GetImage(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    int                 x,
    int                 y,
    unsigned int        width,
    unsigned int        height,
    VAImageID           image
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_surface_p obj_surface = VDPAU_SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    VARectangle rect;
    rect.x      = x;
    rect.y      = y;
    rect.width  = width;
    rect.height = height;
    return get_image(driver_data, obj_surface, obj_image, &rect);
}

// Put image to surface
static VAStatus
put_image(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface,
    object_image_p       obj_image,
    const VARectangle   *src_rect,
    const VARectangle   *dst_rect
)
{
    VAImage * const image = &obj_image->image;
    VdpStatus vdp_status;
    uint8_t *src[3];
    unsigned int src_stride[3];
    int i;

#if 0
    /* Don't do anything if the surface is used for rendering for example */
    /* XXX: VDPAU has no API to inform when decoding is completed... */
    if (obj_surface->va_surface_status != VASurfaceReady)
        return VA_STATUS_ERROR_SURFACE_BUSY;
#endif

    /* RGBA to video surface requires color space conversion */
    if (obj_image->vdp_rgba_output_surface != VDP_INVALID_HANDLE)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    /* VDPAU does not support partial video surface updates */
    if (src_rect->x != 0 ||
        src_rect->y != 0 ||
        src_rect->width != image->width ||
        src_rect->height != image->height)
        return VA_STATUS_ERROR_OPERATION_FAILED;
    if (dst_rect->x != 0 ||
        dst_rect->y != 0 ||
        dst_rect->width != obj_surface->width ||
        dst_rect->height != obj_surface->height)
        return VA_STATUS_ERROR_OPERATION_FAILED;
    if (src_rect->width != dst_rect->width ||
        src_rect->height != dst_rect->height)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_buffer_p obj_buffer = VDPAU_BUFFER(image->buf);
    if (!obj_buffer)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    switch (image->format.fourcc) {
    case VA_FOURCC('I','4','2','0'):
        src[0] = (uint8_t *)obj_buffer->buffer_data + image->offsets[0];
        src_stride[0] = image->pitches[0];
        src[1] = (uint8_t *)obj_buffer->buffer_data + image->offsets[2];
        src_stride[1] = image->pitches[2];
        src[2] = (uint8_t *)obj_buffer->buffer_data + image->offsets[1];
        src_stride[2] = image->pitches[1];
        break;
    default:
        for (i = 0; i < image->num_planes; i++) {
            src[i] = (uint8_t *)obj_buffer->buffer_data + image->offsets[i];
            src_stride[i] = image->pitches[i];
        }
        break;
    }

    /* XXX: only support YCbCr surfaces for now */
    if (obj_image->vdp_format_type != VDP_IMAGE_FORMAT_TYPE_YCBCR)
        return VA_STATUS_ERROR_OPERATION_FAILED;

    vdp_status = vdpau_video_surface_put_bits_ycbcr(
        driver_data,
        obj_surface->vdp_surface,
        obj_image->vdp_format,
        src, src_stride
    );
    return vdpau_get_VAStatus(vdp_status);
}

// vaPutImage
VAStatus
vdpau_PutImage(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    VAImageID           image,
    int                 src_x,
    int                 src_y,
    unsigned int        width,
    unsigned int        height,
    int                 dest_x,
    int                 dest_y
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_surface_p obj_surface = VDPAU_SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    VARectangle src_rect, dst_rect;
    src_rect.x      = src_x;
    src_rect.y      = src_y;
    src_rect.width  = width;
    src_rect.height = height;
    dst_rect.x      = dest_x;
    dst_rect.y      = dest_y;
    dst_rect.width  = width;
    dst_rect.height = height;
    return put_image(driver_data, obj_surface, obj_image, &src_rect, &dst_rect);
}

// vaPutImage2
VAStatus
vdpau_PutImage_full(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    VAImageID           image,
    int                 src_x,
    int                 src_y,
    unsigned int        src_width,
    unsigned int        src_height,
    int                 dest_x,
    int                 dest_y,
    unsigned int        dest_width,
    unsigned int        dest_height
)
{
    VDPAU_DRIVER_DATA_INIT;

    object_surface_p obj_surface = VDPAU_SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    object_image_p obj_image = VDPAU_IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    VARectangle src_rect, dst_rect;
    src_rect.x      = src_x;
    src_rect.y      = src_y;
    src_rect.width  = src_width;
    src_rect.height = src_height;
    dst_rect.x      = dest_x;
    dst_rect.y      = dest_y;
    dst_rect.width  = dest_width;
    dst_rect.height = dest_height;
    return put_image(driver_data, obj_surface, obj_image, &src_rect, &dst_rect);
}
