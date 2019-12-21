/*
 *  vdpau_decode.h - VDPAU backend for VA-API (decoder)
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

#ifndef VDPAU_DECODE_H
#define VDPAU_DECODE_H

#include "vdpau_driver.h"

typedef enum {
    VDP_CODEC_MPEG1 = 1,
    VDP_CODEC_MPEG2,
    VDP_CODEC_MPEG4,
    VDP_CODEC_H264,
    VDP_CODEC_VC1
} VdpCodec;

// Translates VdpDecoderProfile to VdpCodec
VdpCodec get_VdpCodec(VdpDecoderProfile profile)
    attribute_hidden;

// Translates VAProfile to VdpDecoderProfile
VdpDecoderProfile get_VdpDecoderProfile(VAProfile profile)
    attribute_hidden;

// Checks decoder for profile/entrypoint is available
VAStatus
check_decoder(
    vdpau_driver_data_t *driver_data,
    VAProfile            profile,
    VAEntrypoint         entrypoint
) attribute_hidden;

// vaQueryConfigProfiles
VAStatus
vdpau_QueryConfigProfiles(
    VADriverContextP    ctx,
    VAProfile          *profile_list,
    int                *num_profiles
) attribute_hidden;

// vaQueryConfigEntrypoints
VAStatus
vdpau_QueryConfigEntrypoints(
    VADriverContextP    ctx,
    VAProfile           profile,
    VAEntrypoint       *entrypoint_list,
    int                *num_entrypoints
) attribute_hidden;

// vaBeginPicture
VAStatus
vdpau_BeginPicture(
    VADriverContextP    ctx,
    VAContextID         context,
    VASurfaceID         render_target
) attribute_hidden;

// vaRenderPicture
VAStatus
vdpau_RenderPicture(
    VADriverContextP    ctx,
    VAContextID         context,
    VABufferID         *buffers,
    int                 num_buffers
) attribute_hidden;

// vaEndPicture
VAStatus
vdpau_EndPicture(
    VADriverContextP    ctx,
    VAContextID         context
) attribute_hidden;

#endif /* VDPAU_DECODE_H */
