/*
 *  vdpau_subpic.h - VDPAU backend for VA-API (VA subpictures)
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

#ifndef VDPAU_SUBPIC_H
#define VDPAU_SUBPIC_H

#include "vdpau_video.h"
#include "vdpau_image.h"

typedef struct object_subpicture  object_subpicture_t;
typedef struct object_subpicture *object_subpicture_p;

struct object_subpicture {
    struct object_base  base;
    VAImageID           image_id;
    SubpictureAssociationP *assocs;
    unsigned int        assocs_count;
    unsigned int        assocs_count_max;
    unsigned int        chromakey_min;
    unsigned int        chromakey_max;
    unsigned int        chromakey_mask;
    float               alpha;
    unsigned int        width;
    unsigned int        height;
    VdpImageFormatType  vdp_format_type;
    uint32_t            vdp_format;
    VdpBitmapSurface    vdp_bitmap_surface;
    VdpOutputSurface    vdp_output_surface;
    uint64_t            last_commit;
};

// Associate one surface to the subpicture
VAStatus
subpicture_associate_1(
    object_subpicture_p obj_subpicture,
    object_surface_p    obj_surface,
    const VARectangle  *src_rect,
    const VARectangle  *dst_rect,
    unsigned int        flags
) attribute_hidden;

// Deassociate one surface from the subpicture
VAStatus
subpicture_deassociate_1(
    object_subpicture_p obj_subpicture,
    object_surface_p    obj_surface
) attribute_hidden;

// Commit subpicture to VDPAU surface
VAStatus
commit_subpicture(
    vdpau_driver_data_p driver_data,
    object_subpicture_p obj_subpicture
) attribute_hidden;

// vaQuerySubpictureFormats
VAStatus
vdpau_QuerySubpictureFormats(
    VADriverContextP    ctx,
    VAImageFormat      *format_list,
    unsigned int       *flags,
    unsigned int       *num_formats
) attribute_hidden;

// vaCreateSubpicture
VAStatus
vdpau_CreateSubpicture(
    VADriverContextP    ctx,
    VAImageID           image,
    VASubpictureID     *subpicture
) attribute_hidden;

// vaDestroySubpicture
VAStatus
vdpau_DestroySubpicture(
    VADriverContextP    ctx,
    VASubpictureID      subpicture
) attribute_hidden;

// vaSetSubpictureImage
VAStatus
vdpau_SetSubpictureImage(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VAImageID           image
) attribute_hidden;

// vaSetSubpicturePalette (not a PUBLIC interface)
VAStatus
vdpau_SetSubpicturePalette(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    unsigned char      *palette
) attribute_hidden;

// vaSetSubpictureChromaKey
VAStatus
vdpau_SetSubpictureChromakey(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    unsigned int        chromakey_min,
    unsigned int        chromakey_max,
    unsigned int        chromakey_mask
) attribute_hidden;

// vaSetSubpictureGlobalAlpha
VAStatus
vdpau_SetSubpictureGlobalAlpha(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    float               global_alpha
) attribute_hidden;

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
) attribute_hidden;

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
) attribute_hidden;

// vaDeassociateSubpicture
VAStatus
vdpau_DeassociateSubpicture(
    VADriverContextP    ctx,
    VASubpictureID      subpicture,
    VASurfaceID        *target_surfaces,
    int                 num_surfaces
) attribute_hidden;

#endif /* VDPAU_SUBPIC_H */
