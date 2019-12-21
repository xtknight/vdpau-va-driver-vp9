/*
 *  vdpau_image.h - VDPAU backend for VA-API (VA images)
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

#ifndef VDPAU_IMAGE_H
#define VDPAU_IMAGE_H

#include "vdpau_driver.h"

typedef enum {
    VDP_IMAGE_FORMAT_TYPE_YCBCR = 1,
    VDP_IMAGE_FORMAT_TYPE_RGBA,
    VDP_IMAGE_FORMAT_TYPE_INDEXED
} VdpImageFormatType;

typedef struct object_image object_image_t;
struct object_image {
    struct object_base  base;
    VAImage             image;
    VdpImageFormatType  vdp_format_type;
    uint32_t            vdp_format;
    VdpOutputSurface    vdp_rgba_output_surface;
    uint32_t           *vdp_palette;
};

// vaQueryImageFormats
VAStatus
vdpau_QueryImageFormats(
    VADriverContextP    ctx,
    VAImageFormat      *format_list,
    int                *num_formats
) attribute_hidden;

// vaCreateImage
VAStatus
vdpau_CreateImage(
    VADriverContextP    ctx,
    VAImageFormat      *format,
    int                 width,
    int                 height,
    VAImage            *image
) attribute_hidden;

// vaDestroyImage
VAStatus
vdpau_DestroyImage(
    VADriverContextP    ctx,
    VAImageID           image_id
) attribute_hidden;

// vaDeriveImage
VAStatus
vdpau_DeriveImage(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    VAImage            *image
) attribute_hidden;

// vaSetImagePalette
VAStatus
vdpau_SetImagePalette(
    VADriverContextP    ctx,
    VAImageID           image,
    unsigned char      *palette
) attribute_hidden;

// vaGetImage
VAStatus
vdpau_GetImage(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    int                 x,
    int                 y,
    unsigned int        width,
    unsigned int        height,
    VAImageID           image_id
) attribute_hidden;

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
) attribute_hidden;

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
) attribute_hidden;

#endif /* VDPAU_IMAGE_H */
