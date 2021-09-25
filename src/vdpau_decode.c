/*
 *  vdpau_decode.c - VDPAU backend for VA-API (decoder)
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
#include "vdpau_decode.h"
#include "vdpau_driver.h"
#include "vdpau_buffer.h"
#include "vdpau_video.h"
#include "vdpau_dump.h"
#include "utils.h"
#include "put_bits.h"
#include "map.h" // https://github.com/rxi/map

#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#include "debug.h"

// Translates VdpDecoderProfile to VdpCodec
VdpCodec get_VdpCodec(VdpDecoderProfile profile)
{
    switch (profile) {
    case VDP_DECODER_PROFILE_MPEG1:
        return VDP_CODEC_MPEG1;
    case VDP_DECODER_PROFILE_MPEG2_SIMPLE:
    case VDP_DECODER_PROFILE_MPEG2_MAIN:
        return VDP_CODEC_MPEG2;
#if USE_VDPAU_MPEG4
    case VDP_DECODER_PROFILE_MPEG4_PART2_SP:
    case VDP_DECODER_PROFILE_MPEG4_PART2_ASP:
    case VDP_DECODER_PROFILE_DIVX4_QMOBILE:
    case VDP_DECODER_PROFILE_DIVX4_MOBILE:
    case VDP_DECODER_PROFILE_DIVX4_HOME_THEATER:
    case VDP_DECODER_PROFILE_DIVX4_HD_1080P:
    case VDP_DECODER_PROFILE_DIVX5_QMOBILE:
    case VDP_DECODER_PROFILE_DIVX5_MOBILE:
    case VDP_DECODER_PROFILE_DIVX5_HOME_THEATER:
    case VDP_DECODER_PROFILE_DIVX5_HD_1080P:
        return VDP_CODEC_MPEG4;
#endif
    case VDP_DECODER_PROFILE_H264_BASELINE:
    case VDP_DECODER_PROFILE_H264_MAIN:
    case VDP_DECODER_PROFILE_H264_HIGH:
        return VDP_CODEC_H264;
    case VDP_DECODER_PROFILE_VC1_SIMPLE:
    case VDP_DECODER_PROFILE_VC1_MAIN:
    case VDP_DECODER_PROFILE_VC1_ADVANCED:
        return VDP_CODEC_VC1;
    case VDP_DECODER_PROFILE_VP9_PROFILE_0:
        return VDP_CODEC_VP9;
    }
    return 0;
}

// Translates VAProfile to VdpDecoderProfile
VdpDecoderProfile get_VdpDecoderProfile(VAProfile profile)
{
    switch (profile) {
    case VAProfileMPEG2Simple:  return VDP_DECODER_PROFILE_MPEG2_SIMPLE;
    case VAProfileMPEG2Main:    return VDP_DECODER_PROFILE_MPEG2_MAIN;
#if USE_VDPAU_MPEG4
    case VAProfileMPEG4Simple:  return VDP_DECODER_PROFILE_MPEG4_PART2_SP;
    case VAProfileMPEG4AdvancedSimple: return VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
#endif
    case VAProfileH264Baseline: return VDP_DECODER_PROFILE_H264_BASELINE;
    case VAProfileH264Main:     return VDP_DECODER_PROFILE_H264_MAIN;
    case VAProfileH264High:     return VDP_DECODER_PROFILE_H264_HIGH;
    case VAProfileVC1Simple:    return VDP_DECODER_PROFILE_VC1_SIMPLE;
    case VAProfileVC1Main:      return VDP_DECODER_PROFILE_VC1_MAIN;
    case VAProfileVC1Advanced:  return VDP_DECODER_PROFILE_VC1_ADVANCED;
    case VAProfileVP9Profile0:  return VDP_DECODER_PROFILE_VP9_PROFILE_0;
    default:                    break;
    }
    return (VdpDecoderProfile)-1;
}

// Checks whether the VDPAU implementation supports the specified profile
static inline VdpBool
is_supported_profile(
    vdpau_driver_data_t *driver_data,
    VdpDecoderProfile    profile
)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus vdp_status;
    uint32_t max_level, max_references, max_width, max_height;

    if (profile == (VdpDecoderProfile)-1)
        return VDP_FALSE;

    vdp_status = vdpau_decoder_query_capabilities(
        driver_data,
        driver_data->vdp_device,
        profile,
        &is_supported,
        &max_level,
        &max_references,
        &max_width,
        &max_height
    );
    return (VDPAU_CHECK_STATUS(vdp_status, "VdpDecoderQueryCapabilities()") &&
            is_supported);
}

// Checks decoder for profile/entrypoint is available
VAStatus
check_decoder(
    vdpau_driver_data_t *driver_data,
    VAProfile            profile,
    VAEntrypoint         entrypoint
)
{
    if (!is_supported_profile(driver_data, get_VdpDecoderProfile(profile)))
        return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

    /* VDPAU only supports VLD */
    if (entrypoint != VAEntrypointVLD)
        return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

    return VA_STATUS_SUCCESS;
}

// Computes value for VdpDecoderCreate()::max_references parameter
static int
get_max_ref_frames(
    VdpDecoderProfile profile,
    unsigned int      width,
    unsigned int      height
)
{
    int max_ref_frames = 2;

    switch (profile) {
        case VDP_DECODER_PROFILE_H264_MAIN:
        case VDP_DECODER_PROFILE_H264_HIGH:
        {
            /* level 4.1 limits */
            unsigned int aligned_width  = (width + 15) & -16;
            unsigned int aligned_height = (height + 15) & -16;
            unsigned int surface_size   = (aligned_width * aligned_height * 3) / 2;
            if ((max_ref_frames = (12 * 1024 * 1024) / surface_size) > 16)
                max_ref_frames = 16;
            break;
        }
        case VDP_DECODER_PROFILE_VP9_PROFILE_0:
        {
            max_ref_frames = 3; // XXX: is this correct?
            break;
        }
    }
    return max_ref_frames;
}

// Returns the maximum number of reference frames of a decode session
static inline int get_num_ref_frames(object_context_p obj_context)
{
    if (obj_context->vdp_codec == VDP_CODEC_H264)
        return obj_context->vdp_picture_info.h264.num_ref_frames;
    else if (obj_context->vdp_codec == VDP_CODEC_VP9)
        return 3; // XXX: "any particular frame can make use of at most 3 reference frames"
    return 2;
}

// Ensure VDPAU decoder is created for the specified number of reference frames
static VdpStatus
ensure_decoder_with_max_refs(
    vdpau_driver_data_t *driver_data,
    object_context_p     obj_context,
    int                  max_ref_frames
)
{
    VdpStatus vdp_status;

    if (max_ref_frames < 0)
        max_ref_frames =
            get_max_ref_frames(obj_context->vdp_profile,
                               obj_context->picture_width,
                               obj_context->picture_height);

    if (obj_context->vdp_decoder == VDP_INVALID_HANDLE ||
        obj_context->max_ref_frames < max_ref_frames) {
        obj_context->max_ref_frames = max_ref_frames;

        if (obj_context->vdp_decoder != VDP_INVALID_HANDLE) {
            vdpau_decoder_destroy(driver_data, obj_context->vdp_decoder);
            obj_context->vdp_decoder = VDP_INVALID_HANDLE;
        }

        vdp_status = vdpau_decoder_create(
            driver_data,
            driver_data->vdp_device,
            obj_context->vdp_profile,
            obj_context->picture_width,
            obj_context->picture_height,
            max_ref_frames,
            &obj_context->vdp_decoder
        );
        if (!VDPAU_CHECK_STATUS(vdp_status, "VdpDecoderCreate()"))
            return vdp_status;
    }
    return VDP_STATUS_OK;
}

// Lazy allocate (generated) slice data buffer. Buffer lives until vaDestroyContext()
static uint8_t *
alloc_gen_slice_data(object_context_p obj_context, unsigned int size)
{
    uint8_t *gen_slice_data = obj_context->gen_slice_data;

    if (obj_context->gen_slice_data_size + size > obj_context->gen_slice_data_size_max) {
        obj_context->gen_slice_data_size_max += size;
        gen_slice_data = realloc(obj_context->gen_slice_data,
                                 obj_context->gen_slice_data_size_max);
        if (!gen_slice_data)
            return NULL;
        obj_context->gen_slice_data = gen_slice_data;
    }
    gen_slice_data += obj_context->gen_slice_data_size;
    obj_context->gen_slice_data_size += size;
    return gen_slice_data;
}

// Lazy allocate VdpBitstreamBuffer. Buffer lives until vaDestroyContext()
static VdpBitstreamBuffer *
alloc_VdpBitstreamBuffer(object_context_p obj_context)
{
    VdpBitstreamBuffer *vdp_bitstream_buffers;

    vdp_bitstream_buffers = realloc_buffer(
        (void **)&obj_context->vdp_bitstream_buffers,
        &obj_context->vdp_bitstream_buffers_count_max,
        1 + obj_context->vdp_bitstream_buffers_count,
        sizeof(*obj_context->vdp_bitstream_buffers)
    );
    if (!vdp_bitstream_buffers)
        return NULL;

    return &vdp_bitstream_buffers[obj_context->vdp_bitstream_buffers_count++];
}

// Append VASliceDataBuffer hunk into VDPAU buffer
static int
append_VdpBitstreamBuffer(
    object_context_p obj_context,
    const uint8_t   *buffer,
    uint32_t         buffer_size
)
{
    VdpBitstreamBuffer *bitstream_buffer;

    bitstream_buffer = alloc_VdpBitstreamBuffer(obj_context);
    if (!bitstream_buffer)
        return -1;

    bitstream_buffer->struct_version  = VDP_BITSTREAM_BUFFER_VERSION;
    bitstream_buffer->bitstream       = buffer;
    bitstream_buffer->bitstream_bytes = buffer_size;
    return 0;
}

// Initialize VdpReferenceFrameH264 to default values
static void init_VdpReferenceFrameH264(VdpReferenceFrameH264 *rf)
{
    rf->surface             = VDP_INVALID_HANDLE;
    rf->is_long_term        = VDP_FALSE;
    rf->top_is_reference    = VDP_FALSE;
    rf->bottom_is_reference = VDP_FALSE;
    rf->field_order_cnt[0]  = 0;
    rf->field_order_cnt[1]  = 0;
    rf->frame_idx           = 0;
}

static const uint8_t ff_identity[64] = {
    0,   1,  2,  3,  4,  5,  6,  7,
    8,   9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};

static const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const uint8_t ff_mpeg1_default_intra_matrix[64] = {
     8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

static const uint8_t ff_mpeg1_default_non_intra_matrix[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};

static const uint8_t ff_mpeg4_default_intra_matrix[64] = {
     8, 17, 18, 19, 21, 23, 25, 27,
    17, 18, 19, 21, 23, 25, 27, 28,
    20, 21, 22, 23, 24, 26, 28, 30,
    21, 22, 23, 24, 26, 28, 30, 32,
    22, 23, 24, 26, 28, 30, 32, 35,
    23, 24, 26, 28, 30, 32, 35, 38,
    25, 26, 28, 30, 32, 35, 38, 41,
    27, 28, 30, 32, 35, 38, 41, 45,
};

static const uint8_t ff_mpeg4_default_non_intra_matrix[64] = {
    16, 17, 18, 19, 20, 21, 22, 23,
    17, 18, 19, 20, 21, 22, 23, 24,
    18, 19, 20, 21, 22, 23, 24, 25,
    19, 20, 21, 22, 23, 24, 26, 27,
    20, 21, 22, 23, 25, 26, 27, 28,
    21, 22, 23, 24, 26, 27, 28, 30,
    22, 23, 24, 26, 27, 28, 30, 31,
    23, 24, 25, 27, 28, 30, 31, 33,
};

/* VP9 8-Bit Quantization Coefficients, reverse lookup
 * From <https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_parser.cc>
 * XXX: Is there a better way to do this?
 * VA-API provides only the coefficients after looking them up in an array, but
 * VDPAU wants the raw indices (qpYAc, qpYDc, qpChDc, qpChAc).
 * 
 * So we have to derive the original indices using reverse lookup below.
 * Duplicate coefficients are matched to the first index in which they occur.
 * That's why there is less than 256 entries, for DC.
 * This is *not* a perfect reverse lookup, but seems close enough.
 * These are fed into a makeshift hash table for faster lookup.
 */
static const int16_t vp9_dcq_8bit_coeff[239] = {4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 87, 88, 90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 108, 110, 111, 113, 114, 116, 117, 118, 120, 121, 123, 125, 127, 129, 131, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 161, 164, 166, 169, 172, 174, 177, 180, 182, 185, 187, 190, 192, 195, 199, 202, 205, 208, 211, 214, 217, 220, 223, 226, 230, 233, 237, 240, 243, 247, 250, 253, 257, 261, 265, 269, 272, 276, 280, 284, 288, 292, 296, 300, 304, 309, 313, 317, 322, 326, 330, 335, 340, 344, 349, 354, 359, 364, 369, 374, 379, 384, 389, 395, 400, 406, 411, 417, 423, 429, 435, 441, 447, 454, 461, 467, 475, 482, 489, 497, 505, 513, 522, 530, 539, 549, 559, 569, 579, 590, 602, 614, 626, 640, 654, 668, 684, 700, 717, 736, 755, 775, 796, 819, 843, 869, 896, 925, 955, 988, 1022, 1058, 1098, 1139, 1184, 1232, 1282, 1336};

static const int16_t vp9_dcq_8bit_idx[239] = {0, 1, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22,   24, 25, 26, 27, 28, 29, 31, 32, 33, 34, 35, 36, 38, 39, 40, 41, 42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58, 59, 61, 62, 63, 64, 65, 67, 68, 69, 70, 72, 73, 74, 75, 77, 78, 79, 80, 82, 83, 84, 85, 87, 88, 89, 91, 92, 93, 94, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

static const int16_t vp9_acq_8bit_coeff[256] = {4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 155, 158, 161, 164, 167, 170, 173, 176, 179, 182, 185, 188, 191, 194, 197, 200, 203, 207, 211, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255, 260, 265, 270, 275, 280, 285, 290, 295, 300, 305, 311, 317, 323, 329, 335, 341, 347, 353, 359, 366, 373, 380, 387, 394, 401, 408, 416, 424, 432, 440, 448, 456, 465, 474, 483, 492, 501, 510, 520, 530, 540, 550, 560, 571, 582, 593, 604, 615, 627, 639, 651, 663, 676, 689, 702, 715, 729, 743, 757, 771, 786, 801, 816, 832, 848, 864, 881, 898, 915, 933, 951, 969, 988, 1007, 1026, 1046, 1066, 1087, 1108, 1129, 1151, 1173, 1196, 1219, 1243, 1267, 1292, 1317, 1343, 1369, 1396, 1423, 1451, 1479, 1508, 1537, 1567, 1597, 1628, 1660, 1692, 1725, 1759, 1793, 1828};

static const int16_t vp9_acq_8bit_idx[256] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

// Hash tables
static map_int_t g_vp9DcQReverseLookup;
static map_int_t g_vp9AcQReverseLookup;
static bool g_vp9QuantMapsSetup = false;

// Compute integer log2
static inline int ilog2(uint32_t v)
{
    /* From <http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog> */
    uint32_t r, shift;
    r     = (v > 0xffff) << 4; v >>= r;
    shift = (v > 0xff  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xf   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    return r | (v >> 1);
}

// Translate VASurfaceID
static int
translate_VASurfaceID(
    vdpau_driver_data_t *driver_data,
    VASurfaceID          va_surface,
    VdpVideoSurface     *vdp_surface
)
{
    object_surface_p obj_surface;

    if (va_surface == VA_INVALID_SURFACE) {
        *vdp_surface = VDP_INVALID_HANDLE;
        return 1;
    }

    obj_surface = VDPAU_SURFACE(va_surface);
    if (!obj_surface)
        return 0;

    *vdp_surface = obj_surface->vdp_surface;
    return 1;
}

// Translate VAPictureH264
static int
translate_VAPictureH264(
    vdpau_driver_data_t   *driver_data,
    const VAPictureH264   *va_pic,
    VdpReferenceFrameH264 *rf
)
{
    // Handle invalid surfaces specifically
    if (va_pic->picture_id == VA_INVALID_SURFACE) {
        init_VdpReferenceFrameH264(rf);
        return 1;
    }

    if (!translate_VASurfaceID(driver_data, va_pic->picture_id, &rf->surface))
        return 0;
    rf->is_long_term            = (va_pic->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE) != 0;
    if ((va_pic->flags & (VA_PICTURE_H264_TOP_FIELD|VA_PICTURE_H264_BOTTOM_FIELD)) == 0) {
        rf->top_is_reference    = VDP_TRUE;
        rf->bottom_is_reference = VDP_TRUE;
    }
    else {
        rf->top_is_reference    = (va_pic->flags & VA_PICTURE_H264_TOP_FIELD) != 0;
        rf->bottom_is_reference = (va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD) != 0;
    }
    rf->field_order_cnt[0]      = va_pic->TopFieldOrderCnt;
    rf->field_order_cnt[1]      = va_pic->BottomFieldOrderCnt;
    rf->frame_idx               = va_pic->frame_idx;
    return 1;
}

// Translate no buffer
static int
translate_nothing(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    return 1;
}

static int
translate_VASliceDataBuffer_MPEG2(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    static const uint8_t start_code_prefix[3] = { 0x00, 0x00, 0x01 };
    VASliceParameterBufferMPEG2 * const slice_params = obj_context->last_slice_params;
    uint8_t *slice_header;
    unsigned int i;

    /* Check we have the start code */
    for (i = 0; i < obj_context->last_slice_params_count; i++) {
        VASliceParameterBufferMPEG2 * const slice_param = &slice_params[i];
        uint8_t * const buf = (uint8_t *)obj_buffer->buffer_data + slice_param->slice_data_offset;
        if (memcmp(buf, start_code_prefix, sizeof(start_code_prefix)) != 0) {
            if (append_VdpBitstreamBuffer(obj_context,
                                          start_code_prefix,
                                          sizeof(start_code_prefix)) < 0)
                return 0;
            slice_header = alloc_gen_slice_data(obj_context, 1);
            if (!slice_header)
                return 0;
            slice_header[0] = slice_param->slice_vertical_position + 1;
            if (append_VdpBitstreamBuffer(obj_context, slice_header, 1) < 0)
                return 0;
        }
        if (append_VdpBitstreamBuffer(obj_context,
                                      buf,
                                      slice_param->slice_data_size) < 0)
            return 0;
    }
    return 1;
}

// Translate VASliceDataBuffer
static int
translate_VASliceDataBuffer(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    if (obj_context->vdp_codec == VDP_CODEC_H264) {
        /* Check we have the start code */
        /* XXX: check for other codecs too? */
        /* XXX: this assumes we get SliceParams before SliceData */
        static const uint8_t start_code_prefix[3] = { 0x00, 0x00, 0x01 };
        VASliceParameterBufferH264 * const slice_params = obj_context->last_slice_params;
        unsigned int i;
        for (i = 0; i < obj_context->last_slice_params_count; i++) {
            VASliceParameterBufferH264 * const slice_param = &slice_params[i];
            uint8_t *buf = (uint8_t *)obj_buffer->buffer_data + slice_param->slice_data_offset;
            if (memcmp(buf, start_code_prefix, sizeof(start_code_prefix)) != 0) {
                if (append_VdpBitstreamBuffer(obj_context,
                                              start_code_prefix,
                                              sizeof(start_code_prefix)) < 0)
                    return 0;
            }
            if (append_VdpBitstreamBuffer(obj_context,
                                          buf,
                                          slice_param->slice_data_size) < 0)
                return 0;
        }
        return 1;
    }

    if (obj_context->vdp_codec == VDP_CODEC_VP9) {
        /* Check we have the start code */
        /* XXX: check for other codecs too? */
        /* XXX: this assumes we get SliceParams before SliceData */
        static const uint8_t start_code_prefix[3] = { 0x00, 0x00, 0x01 };
        VASliceParameterBufferVP9 * const slice_params = obj_context->last_slice_params;
        unsigned int i;
        for (i = 0; i < obj_context->last_slice_params_count; i++) {
            VASliceParameterBufferVP9 * const slice_param = &slice_params[i];
            uint8_t *buf = (uint8_t *)obj_buffer->buffer_data + slice_param->slice_data_offset;
            if (trace_enabled())
                trace_print("translate_VASliceDataBuffer: VP9: process slice param #%d\n", i);

            if (memcmp(buf, start_code_prefix, sizeof(start_code_prefix)) != 0) {
                if (append_VdpBitstreamBuffer(obj_context,
                                              start_code_prefix,
                                              sizeof(start_code_prefix)) < 0)
                    return 0;
            }
            if (append_VdpBitstreamBuffer(obj_context,
                                          buf,
                                          slice_param->slice_data_size) < 0) {
                D(bug("ERROR: append_VdpBitstreamBuffer\n"));
                return 0;
            }
        }
        return 1;
    }

    if (obj_context->vdp_codec == VDP_CODEC_MPEG2)
        return translate_VASliceDataBuffer_MPEG2(driver_data,
                                                 obj_context, obj_buffer);

#if USE_VDPAU_MPEG4
    if (obj_context->vdp_codec == VDP_CODEC_MPEG4 &&
        obj_context->vdp_bitstream_buffers_count == 0) {
        PutBitContext pb;
        uint8_t slice_header_buffer[32];
        uint8_t *slice_header;
        int slice_header_size;
        const uint8_t *slice_data = obj_buffer->buffer_data;
        uint32_t slice_data_size = obj_buffer->buffer_size;
        VAPictureParameterBufferMPEG4 * const pic_param = obj_context->last_pic_param;
        VASliceParameterBufferMPEG4 * const slice_param = obj_context->last_slice_params;

        int time_incr = 1 + ilog2(pic_param->vop_time_increment_resolution - 1);
        if (time_incr < 1)
            time_incr = 1;

        static const uint16_t VOP_STARTCODE = 0x01b6;
        enum {
            VOP_I_TYPE = 0,
            VOP_P_TYPE,
            VOP_B_TYPE,
            VOP_S_TYPE
        };

        /* XXX: this is a hack to compute the length of
           modulo_time_base "1" sequence. We probably should be
           reconstructing through an extra VOP field in VA-API? */
        int nbits = (32 +                               /* VOP start code       */
                     2 +                                /* vop_coding_type      */
                     1 +                                /* modulo_time_base "0" */
                     1 +                                /* marker_bit           */
                     time_incr +                        /* vop_time_increment   */
                     1 +                                /* marker_bit           */
                     1 +                                /* vop_coded            */
                     (pic_param->vop_fields.bits.vop_coding_type == VOP_P_TYPE ? 1 : 0) +
                     3 +                                /* intra_dc_vlc_thr     */
                     (pic_param->vol_fields.bits.interlaced ? 2 : 0) +
                     5 +                                /* vop_quant            */
                     (pic_param->vop_fields.bits.vop_coding_type != VOP_I_TYPE ? 3 : 0) +
                     (pic_param->vop_fields.bits.vop_coding_type == VOP_B_TYPE ? 3 : 0));
        if ((nbits = slice_param->macroblock_offset - (nbits % 8)) < 0)
            nbits += 8;

        /* Reconstruct the VOP header */
        init_put_bits(&pb, slice_header_buffer, sizeof(slice_header_buffer));
        put_bits(&pb, 16, 0);                   /* vop header */
        put_bits(&pb, 16, VOP_STARTCODE);       /* vop header */
        put_bits(&pb, 2, pic_param->vop_fields.bits.vop_coding_type);
        while (nbits-- > 0)
            put_bits(&pb, 1, 1);                /* modulo_time_base "1" */
        put_bits(&pb, 1, 0);                    /* modulo_time_base "0" */
        put_bits(&pb, 1, 1);                    /* marker */
        put_bits(&pb, time_incr, 0);            /* time increment */
        put_bits(&pb, 1, 1);                    /* marker */
        put_bits(&pb, 1, 1);                    /* vop coded */
        if (pic_param->vop_fields.bits.vop_coding_type == VOP_P_TYPE)
            put_bits(&pb, 1, pic_param->vop_fields.bits.vop_rounding_type);
        put_bits(&pb, 3, pic_param->vop_fields.bits.intra_dc_vlc_thr);
        if (pic_param->vol_fields.bits.interlaced) {
            put_bits(&pb, 1, pic_param->vop_fields.bits.top_field_first);
            put_bits(&pb, 1, pic_param->vop_fields.bits.alternate_vertical_scan_flag);
        }
        put_bits(&pb, 5, slice_param->quant_scale);
        if (pic_param->vop_fields.bits.vop_coding_type != VOP_I_TYPE)
            put_bits(&pb, 3, pic_param->vop_fcode_forward);
        if (pic_param->vop_fields.bits.vop_coding_type == VOP_B_TYPE)
            put_bits(&pb, 3, pic_param->vop_fcode_backward);

        /* Merge in bits from the first byte of the slice */
        ASSERT((put_bits_count(&pb) % 8) == slice_param->macroblock_offset);
        if ((put_bits_count(&pb) % 8) != slice_param->macroblock_offset)
            return 0;
        const int r = 8 - (put_bits_count(&pb) % 8);
        if (r > 0)
            put_bits(&pb, r, slice_data[0] & ((1U << r) - 1));
        flush_put_bits(&pb);

        ASSERT((put_bits_count(&pb) % 8) == 0);
        slice_header_size = put_bits_count(&pb) / 8;
        ASSERT(slice_header_size <= sizeof(slice_header_buffer));
        slice_header = alloc_gen_slice_data(obj_context, slice_header_size);
        if (!slice_header)
            return 0;
        memcpy(slice_header, slice_header_buffer, slice_header_size);
        if (append_VdpBitstreamBuffer(obj_context, slice_header, slice_header_size) < 0)
            return 0;
        if (append_VdpBitstreamBuffer(obj_context, slice_data + 1, slice_data_size - 1) < 0)
            return 0;
        return 1;
    }
#endif

    if (append_VdpBitstreamBuffer(obj_context,
                                  obj_buffer->buffer_data,
                                  obj_buffer->buffer_size) < 0)
        return 0;
    return 1;
}

// Translate VAPictureParameterBufferMPEG2
static int
translate_VAPictureParameterBufferMPEG2(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoMPEG1Or2 * const pic_info = &obj_context->vdp_picture_info.mpeg2;
    VAPictureParameterBufferMPEG2 * const pic_param = obj_buffer->buffer_data;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->forward_reference_picture,
                               &pic_info->forward_reference))
        return 0;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->backward_reference_picture,
                               &pic_info->backward_reference))
        return 0;

    pic_info->picture_structure          = pic_param->picture_coding_extension.bits.picture_structure;
    pic_info->picture_coding_type        = pic_param->picture_coding_type;
    pic_info->intra_dc_precision         = pic_param->picture_coding_extension.bits.intra_dc_precision;
    pic_info->frame_pred_frame_dct       = pic_param->picture_coding_extension.bits.frame_pred_frame_dct;
    pic_info->concealment_motion_vectors = pic_param->picture_coding_extension.bits.concealment_motion_vectors;
    pic_info->intra_vlc_format           = pic_param->picture_coding_extension.bits.intra_vlc_format;
    pic_info->alternate_scan             = pic_param->picture_coding_extension.bits.alternate_scan;
    pic_info->q_scale_type               = pic_param->picture_coding_extension.bits.q_scale_type;
    pic_info->top_field_first            = pic_param->picture_coding_extension.bits.top_field_first;
    pic_info->full_pel_forward_vector    = 0;
    pic_info->full_pel_backward_vector   = 0;
    pic_info->f_code[0][0]               = (pic_param->f_code >> 12) & 0xf;
    pic_info->f_code[0][1]               = (pic_param->f_code >>  8) & 0xf;
    pic_info->f_code[1][0]               = (pic_param->f_code >>  4) & 0xf;
    pic_info->f_code[1][1]               = pic_param->f_code & 0xf;
    return 1;
}

// Translate VAIQMatrixBufferMPEG2
static int
translate_VAIQMatrixBufferMPEG2(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoMPEG1Or2 * const pic_info = &obj_context->vdp_picture_info.mpeg2;
    VAIQMatrixBufferMPEG2 * const iq_matrix = obj_buffer->buffer_data;
    const uint8_t *intra_matrix;
    const uint8_t *intra_matrix_lookup;
    const uint8_t *inter_matrix;
    const uint8_t *inter_matrix_lookup;
    int i;

    if (iq_matrix->load_intra_quantiser_matrix) {
        intra_matrix = iq_matrix->intra_quantiser_matrix;
        intra_matrix_lookup = ff_zigzag_direct;
    }
    else {
        intra_matrix = ff_mpeg1_default_intra_matrix;
        intra_matrix_lookup = ff_identity;
    }

    if (iq_matrix->load_non_intra_quantiser_matrix) {
        inter_matrix = iq_matrix->non_intra_quantiser_matrix;
        inter_matrix_lookup = ff_zigzag_direct;
    }
    else {
        inter_matrix = ff_mpeg1_default_non_intra_matrix;
        inter_matrix_lookup = ff_identity;
    }

    for (i = 0; i < 64; i++) {
        pic_info->intra_quantizer_matrix[intra_matrix_lookup[i]] =
            intra_matrix[i];
        pic_info->non_intra_quantizer_matrix[inter_matrix_lookup[i]] =
            inter_matrix[i];
    }
    return 1;
}

// Translate VASliceParameterBufferMPEG2
static int
translate_VASliceParameterBufferMPEG2(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
    )
{
    VdpPictureInfoMPEG1Or2 * const pic_info = &obj_context->vdp_picture_info.mpeg2;

    pic_info->slice_count               += obj_buffer->num_elements;
    obj_context->last_slice_params       = obj_buffer->buffer_data;
    obj_context->last_slice_params_count = obj_buffer->num_elements;
    return 1;
}

#if USE_VDPAU_MPEG4
// Translate VAPictureParameterBufferMPEG4
static int
translate_VAPictureParameterBufferMPEG4(
    vdpau_driver_data_p driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoMPEG4Part2 * const pic_info = &obj_context->vdp_picture_info.mpeg4;
    VAPictureParameterBufferMPEG4 * const pic_param = obj_buffer->buffer_data;

    /* XXX: we don't support short-video-header formats */
    if (pic_param->vol_fields.bits.short_video_header)
        return 0;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->forward_reference_picture,
                               &pic_info->forward_reference))
        return 0;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->backward_reference_picture,
                               &pic_info->backward_reference))
        return 0;

    if (pic_param->vol_fields.bits.interlaced) {
        vdpau_information_message("unsupported MPEG-4 video with interlaced "
                                  "content, please report this video\n");
        pic_info->trd[0] = 2*pic_param->TRD; /* XXX: + d(0) */
        pic_info->trb[0] = 2*pic_param->TRB; /* XXX: + d(0) */
        pic_info->trd[1] = 2*pic_param->TRD; /* XXX: + d(1) */
        pic_info->trb[1] = 2*pic_param->TRB; /* XXX: + d(1) */
    }
    else {
        pic_info->trd[0] = pic_param->TRD;
        pic_info->trb[0] = pic_param->TRB;
        pic_info->trd[1] = 0;
        pic_info->trb[1] = 0;
    }

    pic_info->vop_time_increment_resolution     = pic_param->vop_time_increment_resolution;
    pic_info->vop_coding_type                   = pic_param->vop_fields.bits.vop_coding_type;
    pic_info->vop_fcode_forward                 = pic_param->vop_fcode_forward;
    pic_info->vop_fcode_backward                = pic_param->vop_fcode_backward;
    pic_info->resync_marker_disable             = pic_param->vol_fields.bits.resync_marker_disable;
    pic_info->interlaced                        = pic_param->vol_fields.bits.interlaced;
    pic_info->quant_type                        = pic_param->vol_fields.bits.quant_type;
    pic_info->quarter_sample                    = pic_param->vol_fields.bits.quarter_sample;
    pic_info->short_video_header                = pic_param->vol_fields.bits.short_video_header;
    pic_info->rounding_control                  = pic_param->vop_fields.bits.vop_rounding_type;
    pic_info->alternate_vertical_scan_flag      = pic_param->vop_fields.bits.alternate_vertical_scan_flag;
    pic_info->top_field_first                   = pic_param->vop_fields.bits.top_field_first;

    obj_context->last_pic_param                 = obj_buffer->buffer_data;
    return 1;
}

// Translate VAIQMatrixBufferMPEG4
static int
translate_VAIQMatrixBufferMPEG4(
    vdpau_driver_data_p driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoMPEG4Part2 * const pic_info = &obj_context->vdp_picture_info.mpeg4;
    VAIQMatrixBufferMPEG4 * const iq_matrix = obj_buffer->buffer_data;
    const uint8_t *intra_matrix;
    const uint8_t *intra_matrix_lookup;
    const uint8_t *inter_matrix;
    const uint8_t *inter_matrix_lookup;
    int i;

    if (iq_matrix->load_intra_quant_mat) {
        intra_matrix = iq_matrix->intra_quant_mat;
        intra_matrix_lookup = ff_zigzag_direct;
    }
    else {
        intra_matrix = ff_mpeg4_default_intra_matrix;
        intra_matrix_lookup = ff_identity;
    }

    if (iq_matrix->load_non_intra_quant_mat) {
        inter_matrix = iq_matrix->non_intra_quant_mat;
        inter_matrix_lookup = ff_zigzag_direct;
    }
    else {
        inter_matrix = ff_mpeg4_default_non_intra_matrix;
        inter_matrix_lookup = ff_identity;
    }

    for (i = 0; i < 64; i++) {
        pic_info->intra_quantizer_matrix[intra_matrix_lookup[i]] =
            intra_matrix[i];
        pic_info->non_intra_quantizer_matrix[inter_matrix_lookup[i]] =
            inter_matrix[i];
    }
    return 1;
}

// Translate VASliceParameterBufferMPEG4
static int
translate_VASliceParameterBufferMPEG4(
    vdpau_driver_data_p driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
    )
{
    obj_context->last_slice_params       = obj_buffer->buffer_data;
    obj_context->last_slice_params_count = obj_buffer->num_elements;
    return 1;
}
#endif

// Translate VAPictureParameterBufferH264
static int
translate_VAPictureParameterBufferH264(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoH264 * const pic_info = &obj_context->vdp_picture_info.h264;
    VAPictureParameterBufferH264 * const pic_param = obj_buffer->buffer_data;
    VAPictureH264 * const CurrPic = &pic_param->CurrPic;
    unsigned int i;

    pic_info->field_order_cnt[0]                = CurrPic->TopFieldOrderCnt;
    pic_info->field_order_cnt[1]                = CurrPic->BottomFieldOrderCnt;
    pic_info->is_reference                      = pic_param->pic_fields.bits.reference_pic_flag;

    pic_info->frame_num                         = pic_param->frame_num;
    pic_info->field_pic_flag                    = pic_param->pic_fields.bits.field_pic_flag;
    pic_info->bottom_field_flag                 = pic_param->pic_fields.bits.field_pic_flag && (CurrPic->flags & VA_PICTURE_H264_BOTTOM_FIELD) != 0;
    pic_info->num_ref_frames                    = pic_param->num_ref_frames;
    pic_info->mb_adaptive_frame_field_flag      = pic_param->seq_fields.bits.mb_adaptive_frame_field_flag && !pic_info->field_pic_flag;
    pic_info->constrained_intra_pred_flag       = pic_param->pic_fields.bits.constrained_intra_pred_flag;
    pic_info->weighted_pred_flag                = pic_param->pic_fields.bits.weighted_pred_flag;
    pic_info->weighted_bipred_idc               = pic_param->pic_fields.bits.weighted_bipred_idc;
    pic_info->frame_mbs_only_flag               = pic_param->seq_fields.bits.frame_mbs_only_flag;
    pic_info->transform_8x8_mode_flag           = pic_param->pic_fields.bits.transform_8x8_mode_flag;
    pic_info->chroma_qp_index_offset            = pic_param->chroma_qp_index_offset;
    pic_info->second_chroma_qp_index_offset     = pic_param->second_chroma_qp_index_offset;
    pic_info->pic_init_qp_minus26               = pic_param->pic_init_qp_minus26;
    pic_info->log2_max_frame_num_minus4         = pic_param->seq_fields.bits.log2_max_frame_num_minus4;
    pic_info->pic_order_cnt_type                = pic_param->seq_fields.bits.pic_order_cnt_type;
    pic_info->log2_max_pic_order_cnt_lsb_minus4 = pic_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4;
    pic_info->delta_pic_order_always_zero_flag  = pic_param->seq_fields.bits.delta_pic_order_always_zero_flag;
    pic_info->direct_8x8_inference_flag         = pic_param->seq_fields.bits.direct_8x8_inference_flag;
    pic_info->entropy_coding_mode_flag          = pic_param->pic_fields.bits.entropy_coding_mode_flag;
    pic_info->pic_order_present_flag            = pic_param->pic_fields.bits.pic_order_present_flag;
    pic_info->deblocking_filter_control_present_flag = pic_param->pic_fields.bits.deblocking_filter_control_present_flag;
    pic_info->redundant_pic_cnt_present_flag    = pic_param->pic_fields.bits.redundant_pic_cnt_present_flag;

    for (i = 0; i < 16; i++) {
        if (!translate_VAPictureH264(driver_data,
                                     &pic_param->ReferenceFrames[i],
                                     &pic_info->referenceFrames[i]))
                return 0;
    }
    return 1;
}

// Translate VAIQMatrixBufferH264
static int
translate_VAIQMatrixBufferH264(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoH264 * const pic_info = &obj_context->vdp_picture_info.h264;
    VAIQMatrixBufferH264 * const iq_matrix = obj_buffer->buffer_data;
    int i, j;

    if (sizeof(pic_info->scaling_lists_4x4) == sizeof(iq_matrix->ScalingList4x4))
        memcpy(pic_info->scaling_lists_4x4, iq_matrix->ScalingList4x4,
               sizeof(pic_info->scaling_lists_4x4));
    else {
        for (j = 0; j < 6; j++) {
            for (i = 0; i < 16; i++)
                pic_info->scaling_lists_4x4[j][i] = iq_matrix->ScalingList4x4[j][i];
        }
    }

    if (sizeof(pic_info->scaling_lists_8x8) == sizeof(iq_matrix->ScalingList8x8))
        memcpy(pic_info->scaling_lists_8x8, iq_matrix->ScalingList8x8,
               sizeof(pic_info->scaling_lists_8x8));
    else {
        for (j = 0; j < 2; j++) {
            for (i = 0; i < 64; i++)
                pic_info->scaling_lists_8x8[j][i] = iq_matrix->ScalingList8x8[j][i];
        }
    }
    return 1;
}

// Translate VASliceParameterBufferH264
static int
translate_VASliceParameterBufferH264(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoH264 * const pic_info = &obj_context->vdp_picture_info.h264;
    VASliceParameterBufferH264 * const slice_params = obj_buffer->buffer_data;
    VASliceParameterBufferH264 * const slice_param = &slice_params[obj_buffer->num_elements - 1];

    pic_info->slice_count                 += obj_buffer->num_elements;
    pic_info->num_ref_idx_l0_active_minus1 = slice_param->num_ref_idx_l0_active_minus1;
    pic_info->num_ref_idx_l1_active_minus1 = slice_param->num_ref_idx_l1_active_minus1;
    obj_context->last_slice_params         = obj_buffer->buffer_data;
    obj_context->last_slice_params_count   = obj_buffer->num_elements;
    return 1;
}

// Translate VAPictureParameterBufferVC1
static int
translate_VAPictureParameterBufferVC1(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoVC1 * const pic_info = &obj_context->vdp_picture_info.vc1;
    VAPictureParameterBufferVC1 * const pic_param = obj_buffer->buffer_data;
    int picture_type, major_version, minor_version;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->forward_reference_picture,
                               &pic_info->forward_reference))
        return 0;

    if (!translate_VASurfaceID(driver_data,
                               pic_param->backward_reference_picture,
                               &pic_info->backward_reference))
        return 0;

    switch (pic_param->picture_fields.bits.picture_type) {
    case 0: picture_type = 0; break; /* I */
    case 1: picture_type = 1; break; /* P */
    case 2: picture_type = 3; break; /* B */
    case 3: picture_type = 4; break; /* BI */
    case 4: picture_type = 1; break; /* P "skipped" */
    default: return 0;
    }

    pic_info->picture_type      = picture_type;
    pic_info->frame_coding_mode = pic_param->picture_fields.bits.frame_coding_mode;
    pic_info->postprocflag      = pic_param->post_processing != 0;
    pic_info->pulldown          = pic_param->sequence_fields.bits.pulldown;
    pic_info->interlace         = pic_param->sequence_fields.bits.interlace;
    pic_info->tfcntrflag        = pic_param->sequence_fields.bits.tfcntrflag;
    pic_info->finterpflag       = pic_param->sequence_fields.bits.finterpflag;
    pic_info->psf               = pic_param->sequence_fields.bits.psf;
    pic_info->dquant            = pic_param->pic_quantizer_fields.bits.dquant;
    pic_info->panscan_flag      = pic_param->entrypoint_fields.bits.panscan_flag;
    pic_info->refdist_flag      = pic_param->reference_fields.bits.reference_distance_flag;
    pic_info->quantizer         = pic_param->pic_quantizer_fields.bits.quantizer;
    pic_info->extended_mv       = pic_param->mv_fields.bits.extended_mv_flag;
    pic_info->extended_dmv      = pic_param->mv_fields.bits.extended_dmv_flag;
    pic_info->overlap           = pic_param->sequence_fields.bits.overlap;
    pic_info->vstransform       = pic_param->transform_fields.bits.variable_sized_transform_flag;
    pic_info->loopfilter        = pic_param->entrypoint_fields.bits.loopfilter;
    pic_info->fastuvmc          = pic_param->fast_uvmc_flag;
    pic_info->range_mapy_flag   = pic_param->range_mapping_fields.bits.luma_flag;
    pic_info->range_mapy        = pic_param->range_mapping_fields.bits.luma;
    pic_info->range_mapuv_flag  = pic_param->range_mapping_fields.bits.chroma_flag;
    pic_info->range_mapuv       = pic_param->range_mapping_fields.bits.chroma;
    pic_info->multires          = pic_param->sequence_fields.bits.multires;
    pic_info->syncmarker        = pic_param->sequence_fields.bits.syncmarker;
    pic_info->rangered          = pic_param->sequence_fields.bits.rangered;
    if (!vdpau_is_nvidia(driver_data, &major_version, &minor_version) ||
        (major_version > 180 || minor_version >= 35))
        pic_info->rangered     |= pic_param->range_reduction_frame << 1;
    pic_info->maxbframes        = pic_param->sequence_fields.bits.max_b_frames;
    pic_info->deblockEnable     = pic_param->post_processing != 0; /* XXX: this is NVIDIA's vdpau.c semantics (postprocflag & 1) */
    pic_info->pquant            = pic_param->pic_quantizer_fields.bits.pic_quantizer_scale;
    return 1;
}

// Translate VASliceParameterBufferVC1
static int
translate_VASliceParameterBufferVC1(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoVC1 * const pic_info = &obj_context->vdp_picture_info.vc1;

    pic_info->slice_count               += obj_buffer->num_elements;
    obj_context->last_slice_params       = obj_buffer->buffer_data;
    obj_context->last_slice_params_count = obj_buffer->num_elements;
    return 1;
}

// Translate VADecPictureParameterBufferVP9
static int
translate_VAPictureParameterBufferVP9(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    D(bug("translate_VAPictureParameterBufferVP9 [driver_data: %p, obj_context: %p, obj_buffer: %p]\n", driver_data, &obj_context, &obj_buffer));

    // When translating parameters, look at these documents:
    // http://git.videolan.org/?p=ffmpeg.git;a=blob;f=libavcodec/vp9.c;h=0eb92f8c08764c425c08c57393787b5c5d1d3808;hb=HEAD
    // https://github.com/FFmpeg/FFmpeg/blob/1054752c563cbe978f16010ed57dfa23a41ee490/libavcodec/vdpau_vp9.c
    // https://intel.github.io/libva/va__dec__vp9_8h_source.html
    // https://vdpau.pages.freedesktop.org/libvdpau/vdpau_8h_source.html
    // https://storage.googleapis.com/downloads.webmproject.org/docs/vp9/vp9-bitstream-specification-v0.6-20160331-draft.pdf
    // https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_uncompressed_header_parser.cc
    // https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_compressed_header_parser.cc
    // https://download.nvidia.com/XFree86/Linux-x86_64/440.26/README/vdpausupport.html
    // https://github.com/Intel-Media-SDK/MediaSDK/blob/master/_studio/shared/umc/codec/vp9_dec/src/umc_vp9_bitstream.cpp
    // https://vdpau.pages.freedesktop.org/libvdpau/struct_vdp_picture_info_v_p9.html#a6338e4e40fa414bf7c1d7158087bc139

    VdpPictureInfoVP9 * const pic_info = &obj_context->vdp_picture_info.vp9;
    VADecPictureParameterBufferVP9 * const pic_param = obj_buffer->buffer_data;

    //if (trace_enabled())
    //    dump_VADecPictureParameterBufferVP9(pic_param);

    // empty out pic_info
    memset(pic_info, 0, sizeof(VdpPictureInfoVP9));

    const int last_ref_index = pic_param->pic_fields.bits.last_ref_frame;
    const int golden_ref_index = pic_param->pic_fields.bits.golden_ref_frame;
    const int alt_ref_index = pic_param->pic_fields.bits.alt_ref_frame;

    pic_info->width                                = pic_param->frame_width;
    pic_info->height                            = pic_param->frame_height;

    if (pic_param->reference_frames[last_ref_index] == VA_INVALID_ID) {
        pic_info->lastReference = VDP_INVALID_HANDLE;
    } else {
        if (!translate_VASurfaceID(driver_data,
                                   /*(VASurfaceID)*/pic_param->reference_frames[last_ref_index],
                                   /*(VdpVideoSurface*)*/&pic_info->lastReference))
        {
            D(bug("ERROR: translate_VASurfaceID lastReference\n"));
            return 0;
        }
    }

    if (pic_param->reference_frames[golden_ref_index] == VA_INVALID_ID) {
        pic_info->goldenReference = VDP_INVALID_HANDLE;
    } else {
        if (!translate_VASurfaceID(driver_data,
                                   /*(VASurfaceID)*/pic_param->reference_frames[golden_ref_index],
                                   /*(VdpVideoSurface*)*/&pic_info->goldenReference))
        {
            D(bug("ERROR: translate_VASurfaceID goldenReference\n"));
            return 0;
        }
    }

    if (pic_param->reference_frames[alt_ref_index] == VA_INVALID_ID) {
        pic_info->altReference = VDP_INVALID_HANDLE;
    } else {
        if (!translate_VASurfaceID(driver_data,
                               /*(VASurfaceID)*/pic_param->reference_frames[alt_ref_index],
                               /*(VdpVideoSurface*)*/&pic_info->altReference))
        {
            D(bug("ERROR: translate_VASurfaceID altReference\n"));
            return 0;
        }
    }

    pic_info->colorSpace = 2; // BT709 (seems to be most common on YouTube). TODO: this needs to be read from 3 bits of the frame...but is not available in all frames
    
    pic_info->profile                            = pic_param->profile; // VP9 Profile definition value range [0..3]
    // seems that even VDPAU needs [0..3], not VDP_DECODER_PROFILE...
    // see ffmpeg: https://ffmpeg.org/doxygen/3.2/vp9_8c_source.html#l00513
    
    pic_info->frameContextIdx = pic_param->pic_fields.bits.frame_context_idx;
    pic_info->keyFrame = (pic_param->pic_fields.bits.frame_type == 0); // frame_type: 0 KEY_FRAME / 1 NON_KEY_FRAME
    pic_info->showFrame = pic_param->pic_fields.bits.show_frame;
    pic_info->errorResilient = pic_param->pic_fields.bits.error_resilient_mode;
    
    // XXX: FIXME: ???? s->parallelmode = s->errorres ? 1 : get_bits1(&s->gb);
    // this is probably already decoded at VA-API level: honor VA-API itself
    pic_info->frameParallelDecoding = pic_param->pic_fields.bits.frame_parallel_decoding_mode;
    
    pic_info->subSamplingX = pic_param->pic_fields.bits.subsampling_x;
    pic_info->subSamplingY = pic_param->pic_fields.bits.subsampling_y;
    pic_info->intraOnly = pic_param->pic_fields.bits.intra_only;
    pic_info->allowHighPrecisionMv = pic_param->pic_fields.bits.allow_high_precision_mv;
    //pic_info->refreshEntropyProbs /*aka refreshctx */ = pic_param->error_resilient_mode ? 0 : refresh_frame_context
    // XXX: FIXME: verify that this is the same: honor VA-API as this is probably decoded properly there
    // check VA-API/Chromium code (seems to be already decoded in header as passed through properly)
    pic_info->refreshEntropyProbs = pic_param->pic_fields.bits.refresh_frame_context;
    
    // https://github.com/FFmpeg/FFmpeg/blob/1054752c563cbe978f16010ed57dfa23a41ee490/libavcodec/vdpau_vp9.c#L104
    pic_info->refFrameSignBias[0] = 0;
    pic_info->refFrameSignBias[1] = pic_param->pic_fields.bits.last_ref_frame_sign_bias;
    pic_info->refFrameSignBias[2] = pic_param->pic_fields.bits.golden_ref_frame_sign_bias;
    pic_info->refFrameSignBias[3] = pic_param->pic_fields.bits.alt_ref_frame_sign_bias;

    // https://chromium.googlesource.com/chromium/src/+/master/media/gpu/windows/d3d11_vp9_accelerator.cc
    pic_info->bitDepthMinus8Luma = pic_param->bit_depth - 8;
    pic_info->bitDepthMinus8Chroma = pic_param->bit_depth - 8;
    
    // /* loopfilter header data */   s->filter.level = get_bits(&s->gb, 6);
    // filter_level/sharpness?
    pic_info->loopFilterLevel = pic_param->filter_level;
    pic_info->loopFilterSharpness = pic_param->sharpness_level;
    
    // XXX: ?????? lf_delta.enabled? not supported in VA-API?
    pic_info->modeRefLfEnabled = 0;
    // check if all elements in filter_level are equal to loopFilterLevel
    // if true, delta should be disabled
    
    // probably encoded in filter_level in VASegmentParameterVP9
    
    // seems that delta enabled may be hard to ascertain from VA-API itself
    // https://chromium.googlesource.com/chromium/src/media/+/master/filters/vp9_parser.cc#990
    
    pic_info->log2TileColumns = pic_param->log2_tile_columns;
    pic_info->log2TileRows = pic_param->log2_tile_rows;
    
    pic_info->segmentEnabled = pic_param->pic_fields.bits.segmentation_enabled;
    pic_info->segmentMapUpdate = pic_param->pic_fields.bits.segmentation_update_map;
    pic_info->segmentMapTemporalUpdate = pic_param->pic_fields.bits.segmentation_temporal_update;
    
    // segmentFeatureMode, Enable, Data set in SliceParameter

    for (int i = 0; i < 7; i++) {
        pic_info->mbSegmentTreeProbs[i] = pic_param->mb_segment_tree_probs[i];
    }
    
    for (int i = 0; i < 3; i++) {
        pic_info->segmentPredProbs[i] = pic_param->segment_pred_probs[i];
    }

    // qpYAc, qpYDc, qpChDc, qpChAc set in SliceParameter

    pic_info->activeRefIdx[0] = pic_param->pic_fields.bits.last_ref_frame;
    pic_info->activeRefIdx[1] = pic_param->pic_fields.bits.golden_ref_frame;
    pic_info->activeRefIdx[2] = pic_param->pic_fields.bits.alt_ref_frame;
    
    pic_info->resetFrameContext = pic_param->pic_fields.bits.reset_frame_context;
    pic_info->mcompFilterType = pic_param->pic_fields.bits.mcomp_filter_type; // XXX needs more checking

    // mbRefLfDelta, mbModeLfDelta need to be set in SliceParameter, but it
    // may not be possible to perfectly derive them

    pic_info->uncompressedHeaderSize = pic_param->frame_header_length_in_bytes;
    pic_info->compressedHeaderSize = pic_param->first_partition_size;

    //if (trace_enabled())
    //    dump_VdpPictureInfoVP9(pic_info);

    return 1;
}

// Translate VASliceParameterBufferVP9
static int
translate_VASliceParameterBufferVP9(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    VdpPictureInfoVP9 * const pic_info = &obj_context->vdp_picture_info.vp9;
    VASliceParameterBufferVP9 * const slice_params = obj_buffer->buffer_data;
    VASliceParameterBufferVP9 * const slice_param = &slice_params[obj_buffer->num_elements - 1];

    D(bug("translate_VASliceParameterBufferVP9 [driver_data: %p, obj_context: %p, obj_buffer: %p, obj_buffer->num_elements: %d]\n", driver_data, &obj_context, &obj_buffer, obj_buffer->num_elements));

    //if (trace_enabled())
    //    dump_VASliceParameterBufferVP9(slice_param);

    // segmentFeatureMode = segmentation_abs_or_delta_update
    
    // segmentation_abs_or_delta_update: 0 means delta. 1 means absolute.
    // XXX: force to absolute? not sure if this information is available from VA-API
    pic_info->segmentFeatureMode = 1;

    // segment_reference is data for segment_reference feature

    for (int i = 0; i < 8; i++) {
        // XXX: go through the segments and set the features we can
        // not sure if we can set more based on what VA-API gives us
        VASegmentParameterVP9 *seg = &slice_param->seg_param[i];
        pic_info->segmentFeatureEnable[i][0] = 0; //  = ???? q_enabled? [information not provided by VAAPI]
        pic_info->segmentFeatureEnable[i][1] = 0; //  = ???? lf_enabled loop filter [information not provided by VAAPI]
        pic_info->segmentFeatureEnable[i][2] = seg->segment_flags.fields.segment_reference_enabled; //  = ???? ref_enabled
        pic_info->segmentFeatureEnable[i][3] = seg->segment_flags.fields.segment_reference_skipped; //  = ???? skip_enabled
        
        // the only data saved is for ref frame. the rest of the data should be left blank
        pic_info->segmentFeatureData[i][0] = 0;
        pic_info->segmentFeatureData[i][1] = 0;
        pic_info->segmentFeatureData[i][2] = seg->segment_flags.fields.segment_reference; // reference enabled (unsigned 2-bit value according to 'segmentation_feature_(bits/signed)' structures)
        pic_info->segmentFeatureData[i][3] = 0;
    }

    const size_t bit_depth_index = ((pic_info->bitDepthMinus8Luma + 8) == 8) ? 0 : 1;
    if (bit_depth_index == 1) {
        D(bug("ERROR: Only bit depth 8 supported for now.\n"));
        // VDPAU only supports profile 0 to my knowledge,
        // which only supports 8-bit bit depth
        return 0;
    }

    // XXX: better way to do reverse lookup?
    // Reverse lookup example:
    // luma_ac_quant_scale=95:
    // bit_depth is 8, so bit_depth_index is 0
    // https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_parser.cc
    // so we look up where 95 is in kAcQLookup[0] (the 8-bit table)
    // it's at index 88, so base Q index is 88.

    // there are no duplicate values in 8-bit AC (very lucky!), and q_index is defined as an unsigned integer in ffmpeg(yac_qi) and spec as f(8) unsigned n-bit number
    // q_index is an unsigned 8-bit number, which is max 255 anyway.

    // note that deltas can be negative but we can detect that
    // based on q_index itself

    // XXX: for now, just look at the first segment
    // not sure how to handle this case for other segments
    VASegmentParameterVP9 *seg = &slice_param->seg_param[0];

    // Derive qpYAc, qpYDc, qpChDc, qpChAc very roughly.
    // Even though they are clamped to [0,255], after VDPAU
    // looks at these values hopefully they will end up the
    // same. [except for minor index lookup differences due to
    // duplications in the DC table].

    int *val = map_get(&g_vp9AcQReverseLookup, seg->luma_ac_quant_scale);
    int q_index = 0;
    if (val) {
        q_index = *val;
        if (trace_enabled())
            trace_print("luma_ac_quant_scale=%d ==> q_index=%d\n", seg->luma_ac_quant_scale, q_index);
    } else {
        D(bug("ERROR: no q_index found for luma_ac_quant_scale=%d\n", seg->luma_ac_quant_scale));
    }
    pic_info->qpYAc = q_index;
    
    val = map_get(&g_vp9DcQReverseLookup, seg->luma_dc_quant_scale);
    int delta_q_y_dc = 0;
    if (val) {
        delta_q_y_dc = *val - q_index;
        if (trace_enabled())
            trace_print("luma_dc_quant_scale=%d ==> delta_q_y_dc=%d\n", seg->luma_dc_quant_scale, delta_q_y_dc);
    } else {
        D(bug("ERROR: no delta_q_y_dc found for luma_dc_quant_scale=%d\n", seg->luma_dc_quant_scale));
    }
    pic_info->qpYDc = delta_q_y_dc;

    val = map_get(&g_vp9DcQReverseLookup, seg->chroma_dc_quant_scale);
    int delta_q_uv_dc = 0;
    if (val) {
        delta_q_uv_dc = *val - q_index;
        if (trace_enabled())
            trace_print("chroma_dc_quant_scale=%d ==> delta_q_uv_dc=%d\n", seg->chroma_dc_quant_scale, delta_q_uv_dc);
    } else {
        D(bug("ERROR: no delta_q_uv_dc found for chroma_dc_quant_scale=%d\n", seg->chroma_dc_quant_scale));
    }
    pic_info->qpChDc = delta_q_uv_dc;

    val = map_get(&g_vp9AcQReverseLookup, seg->chroma_ac_quant_scale);
    int delta_q_uv_ac = 0;
    if (val) {
        delta_q_uv_ac = *val - q_index;
        if (trace_enabled())
            trace_print("chroma_ac_quant_scale=%d ==> delta_q_uv_ac=%d\n", seg->chroma_ac_quant_scale, delta_q_uv_ac);
    } else {
        D(bug("ERROR: no delta_q_uv_ac found for chroma_ac_quant_scale=%d\n", seg->chroma_ac_quant_scale));
    }
    pic_info->qpChAc = delta_q_uv_ac;

    // XXX: TODO: loop filter
    // not sure if this is possible with the info VA-API gives us.

    // XXX: is it ok just to look at segment 0?
    // reset loop filter to 1,0,-1,-1
    pic_info->mbRefLfDelta[0] = 1;
    pic_info->mbRefLfDelta[1] = 0;
    pic_info->mbRefLfDelta[2] = -1;
    pic_info->mbRefLfDelta[3] = -1;
    pic_info->mbModeLfDelta[0] = 0;
    pic_info->mbModeLfDelta[1] = 0;

    //if (trace_enabled())
    //    dump_VdpPictureInfoVP9(pic_info); // dump updated pic_info

    obj_context->last_slice_params       = obj_buffer->buffer_data;
    obj_context->last_slice_params_count = obj_buffer->num_elements;

    return 1;
}

// Translate VA buffer
typedef int
(*translate_buffer_func_t)(vdpau_driver_data_t *driver_data,
                           object_context_p    obj_context,
                           object_buffer_p     obj_buffer);

typedef struct translate_buffer_info translate_buffer_info_t;
struct translate_buffer_info {
    VdpCodec codec;
    VABufferType type;
    translate_buffer_func_t func;
};

static int
translate_buffer(
    vdpau_driver_data_t *driver_data,
    object_context_p    obj_context,
    object_buffer_p     obj_buffer
)
{
    static const translate_buffer_info_t translate_info[] = {
#define _(CODEC, TYPE)                                  \
        { VDP_CODEC_##CODEC, VA##TYPE##BufferType,      \
          translate_VA##TYPE##Buffer##CODEC }
        _(MPEG2, PictureParameter),
        _(MPEG2, IQMatrix),
        _(MPEG2, SliceParameter),
#if USE_VDPAU_MPEG4
        _(MPEG4, PictureParameter),
        _(MPEG4, IQMatrix),
        _(MPEG4, SliceParameter),
#endif
        _(H264, PictureParameter),
        _(H264, IQMatrix),
        _(H264, SliceParameter),
        _(VC1, PictureParameter),
        _(VC1, SliceParameter),
        _(VP9, PictureParameter),
        _(VP9, SliceParameter),
#undef _
        { VDP_CODEC_VC1, VABitPlaneBufferType, translate_nothing },
        { 0, VASliceDataBufferType, translate_VASliceDataBuffer },
        { 0, 0, NULL }
    };
    const translate_buffer_info_t *tbip;
    for (tbip = translate_info; tbip->func != NULL; tbip++) {
        if (tbip->codec && tbip->codec != obj_context->vdp_codec)
            continue;
        if (tbip->type != obj_buffer->type)
            continue;
        return tbip->func(driver_data, obj_context, obj_buffer);
    }
    D(bug("ERROR: no translate function found for %s%s\n",
          string_of_VABufferType(obj_buffer->type),
          obj_context->vdp_codec ? string_of_VdpCodec(obj_context->vdp_codec) : NULL));
    return 0;
}

// vaQueryConfigProfiles
VAStatus
vdpau_QueryConfigProfiles(
    VADriverContextP    ctx,
    VAProfile          *profile_list,
    int                *num_profiles
)
{
    VDPAU_DRIVER_DATA_INIT;

    static const VAProfile va_profiles[] = {
        VAProfileMPEG2Simple,
        VAProfileMPEG2Main,
        VAProfileMPEG4Simple,
        VAProfileMPEG4AdvancedSimple,
        VAProfileMPEG4Main,
        VAProfileH264Baseline,
        VAProfileH264Main,
        VAProfileH264High,
        VAProfileVC1Simple,
        VAProfileVC1Main,
        VAProfileVC1Advanced,
        VAProfileVP9Profile0
    };

    int i, n = 0;
    for (i = 0; i < ARRAY_ELEMS(va_profiles); i++) {
        VAProfile profile = va_profiles[i];
        VdpDecoderProfile vdp_profile = get_VdpDecoderProfile(profile);
        if (is_supported_profile(driver_data, vdp_profile))
            profile_list[n++] = profile;
    }

    /* If the assert fails then VDPAU_MAX_PROFILES needs to be bigger */
    ASSERT(n <= VDPAU_MAX_PROFILES);
    if (num_profiles)
        *num_profiles = n;

    return VA_STATUS_SUCCESS;
}

// vaQueryConfigEntrypoints
VAStatus
vdpau_QueryConfigEntrypoints(
    VADriverContextP    ctx,
    VAProfile           profile,
    VAEntrypoint       *entrypoint_list,
    int                *num_entrypoints
)
{
    VDPAU_DRIVER_DATA_INIT;

    VdpDecoderProfile vdp_profile = get_VdpDecoderProfile(profile);
    if (!is_supported_profile(driver_data, vdp_profile))
        return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

    VAEntrypoint entrypoint;
    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        entrypoint = VAEntrypointVLD;
        break;
    case VAProfileMPEG4Simple:
    case VAProfileMPEG4AdvancedSimple:
    case VAProfileMPEG4Main:
        entrypoint = VAEntrypointVLD;
        break;
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        entrypoint = VAEntrypointVLD;
        break;
    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
        entrypoint = VAEntrypointVLD;
        break;
    case VAProfileVP9Profile0:
        entrypoint = VAEntrypointVLD;
        break;
    default:
        entrypoint = 0;
        break;
    }

    if (entrypoint_list)
        *entrypoint_list = entrypoint;

    if (num_entrypoints)
        *num_entrypoints = entrypoint != 0;

    return VA_STATUS_SUCCESS;
}

// vaBeginPicture
VAStatus
vdpau_BeginPicture(
    VADriverContextP    ctx,
    VAContextID         context,
    VASurfaceID         render_target
)
{
    VDPAU_DRIVER_DATA_INIT;

    D(bug("vdpau_BeginPicture [ctx: %p, context: %d, render_target: %d]\n", ctx, context, render_target));

    object_context_p obj_context = VDPAU_CONTEXT(context);
    if (!obj_context)
        return VA_STATUS_ERROR_INVALID_CONTEXT;

    object_surface_p obj_surface = VDPAU_SURFACE(render_target);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    obj_surface->va_surface_status           = VASurfaceRendering;
    obj_context->last_pic_param              = NULL;
    obj_context->last_slice_params           = NULL;
    obj_context->last_slice_params_count     = 0;
    obj_context->current_render_target       = obj_surface->base.id;
    obj_context->gen_slice_data_size         = 0;
    obj_context->vdp_bitstream_buffers_count = 0;

    switch (obj_context->vdp_codec) {
    case VDP_CODEC_MPEG1:
    case VDP_CODEC_MPEG2:
        obj_context->vdp_picture_info.mpeg2.slice_count = 0;
        break;
    case VDP_CODEC_MPEG4:
        break;
    case VDP_CODEC_H264:
        obj_context->vdp_picture_info.h264.slice_count = 0;
        break;
    case VDP_CODEC_VC1:
        obj_context->vdp_picture_info.vc1.slice_count = 0;
        break;
    case VDP_CODEC_VP9:
        // XXX: setting up these maps elsewhere would be more proper
        if (!g_vp9QuantMapsSetup) {
            map_init(&g_vp9DcQReverseLookup);
            for (int i = 0; i < sizeof(vp9_dcq_8bit_coeff) / sizeof(int16_t); i++)
            {
                map_set(&g_vp9DcQReverseLookup, vp9_dcq_8bit_coeff[i], vp9_dcq_8bit_idx[i]);
            }
            map_init(&g_vp9AcQReverseLookup);
            for (int i = 0; i < sizeof(vp9_acq_8bit_coeff) / sizeof(int16_t); i++)
            {
                map_set(&g_vp9AcQReverseLookup, vp9_acq_8bit_coeff[i], vp9_acq_8bit_idx[i]);
            }
            g_vp9QuantMapsSetup = true;
        }
        break;
    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }

    return VA_STATUS_SUCCESS;
}

// vaRenderPicture
VAStatus
vdpau_RenderPicture(
    VADriverContextP    ctx,
    VAContextID         context,
    VABufferID         *buffers,
    int                 num_buffers
)
{
    VDPAU_DRIVER_DATA_INIT;
    int i;

    D(bug("vdpau_RenderPicture [ctx: %p, context: %d, buffers: %p, num_buffers: %d]\n", ctx, context, buffers, num_buffers));

    object_context_p obj_context = VDPAU_CONTEXT(context);
    if (!obj_context)
        return VA_STATUS_ERROR_INVALID_CONTEXT;

    object_surface_p obj_surface = VDPAU_SURFACE(obj_context->current_render_target);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    /* Verify that we got valid buffer references */
    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = VDPAU_BUFFER(buffers[i]);

        D(bug("... buffers[%d]->type: %s (%d)\n", i, string_of_VABufferType(obj_buffer->type), obj_buffer->type));

        if (!obj_buffer)
            return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    /* Translate buffers */
    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = VDPAU_BUFFER(buffers[i]);

        if (!translate_buffer(driver_data, obj_context, obj_buffer))
            return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    return VA_STATUS_SUCCESS;
}

// vaEndPicture
VAStatus
vdpau_EndPicture(
    VADriverContextP    ctx,
    VAContextID         context
)
{
    VDPAU_DRIVER_DATA_INIT;
    unsigned int i;

    D(bug("vdpau_EndPicture [ctx: %p, context: %d]\n", ctx, context));
    
    object_context_p obj_context = VDPAU_CONTEXT(context);
    if (!obj_context)
        return VA_STATUS_ERROR_INVALID_CONTEXT;

    object_surface_p obj_surface = VDPAU_SURFACE(obj_context->current_render_target);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    if (trace_enabled()) {
        switch (obj_context->vdp_codec) {
        case VDP_CODEC_MPEG1:
        case VDP_CODEC_MPEG2:
            dump_VdpPictureInfoMPEG1Or2(&obj_context->vdp_picture_info.mpeg2);
            break;
#if HAVE_VDPAU_MPEG4
        case VDP_CODEC_MPEG4:
            dump_VdpPictureInfoMPEG4Part2(&obj_context->vdp_picture_info.mpeg4);
            break;
#endif
        case VDP_CODEC_H264:
            dump_VdpPictureInfoH264(&obj_context->vdp_picture_info.h264);
            break;
        case VDP_CODEC_VC1:
            dump_VdpPictureInfoVC1(&obj_context->vdp_picture_info.vc1);
            break;
        case VDP_CODEC_VP9:
            dump_VdpPictureInfoVP9(&obj_context->vdp_picture_info.vp9);
            break;
        default:
            break;
        }
        for (i = 0; i < obj_context->vdp_bitstream_buffers_count; i++)
            dump_VdpBitstreamBuffer(&obj_context->vdp_bitstream_buffers[i]);
    }

    D(bug("rendering to surface %x\n", obj_context->current_render_target));
    VAStatus va_status;
    VdpStatus vdp_status;
    vdp_status = ensure_decoder_with_max_refs(
        driver_data,
        obj_context,
        get_num_ref_frames(obj_context)
    );

    D(bug("vdp_status after ensure = %d\n", vdp_status));
    if (vdp_status == VDP_STATUS_OK)
        vdp_status = vdpau_decoder_render(
            driver_data,
            obj_context->vdp_decoder,
            obj_surface->vdp_surface,
            (VdpPictureInfo*)&obj_context->vdp_picture_info,
            obj_context->vdp_bitstream_buffers_count,
            obj_context->vdp_bitstream_buffers
        );
    va_status = vdpau_get_VAStatus(vdp_status);
    D(bug("vdp_status after render = %d\n", vdp_status));

    /* XXX: assume we are done with rendering right away */
    obj_context->current_render_target = VA_INVALID_SURFACE;

    return va_status;
}
