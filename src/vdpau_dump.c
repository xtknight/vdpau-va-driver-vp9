/*
 *  vdpau_dump.c - Dump utilities
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
#include "vdpau_dump.h"

#define DEBUG 1
#include "debug.h"


// Returns string representation of FOURCC
const char *string_of_FOURCC(uint32_t fourcc)
{
    static char str[5];
    str[0] = fourcc;
    str[1] = fourcc >> 8;
    str[2] = fourcc >> 16;
    str[3] = fourcc >> 24;
    str[4] = '\0';
    return str;
}

// Returns string representation of VABufferType
const char *string_of_VABufferType(VABufferType type)
{
    const char *str = NULL;
    switch (type) {
#define _(X) case X: str = #X; break
        _(VAPictureParameterBufferType);
        _(VAIQMatrixBufferType);
        _(VABitPlaneBufferType);
        _(VASliceGroupMapBufferType);
        _(VASliceParameterBufferType);
        _(VASliceDataBufferType);
        _(VAMacroblockParameterBufferType);
        _(VAResidualDataBufferType);
        _(VADeblockingParameterBufferType);
        _(VAImageBufferType);
#if VA_CHECK_VERSION(0,30,0)
        _(VAProtectedSliceDataBufferType);
        _(VAEncCodedBufferType);
        _(VAEncSequenceParameterBufferType);
        _(VAEncPictureParameterBufferType);
        _(VAEncSliceParameterBufferType);
#endif
#if VA_CHECK_VERSION(0,31,1)
        _(VAQMatrixBufferType);
#endif
#if VA_CHECK_VERSION(0,32,0)
        _(VAEncMiscParameterBufferType);
#endif
    default:
        break;
#undef _
    }
    return str;
}

// Returns string representation of VdpCodec
const char *string_of_VdpCodec(VdpCodec codec)
{
    const char *str = NULL;
    switch (codec) {
#define _(X) case VDP_CODEC_##X: str = #X; break
        _(MPEG1);
        _(MPEG2);
        _(MPEG4);
        _(H264);
        _(VC1);
#undef _
    }
    return str;
}

#if USE_TRACER
#define TRACE               trace_print
#define INDENT(INC)         trace_indent(INC)
#define DUMPi(S, M)         TRACE("." #M " = %d,\n", S->M)
#define DUMPx(S, M)         TRACE("." #M " = 0x%08x,\n", S->M)
#define DUMPp(S, M)         TRACE("." #M " = %p,\n", S->M)
#define DUMPm(S, M, I, J)   dump_matrix_NxM(#M, (uint8_t *)S->M, I, J, I * J)
#else
#define trace_enabled()     (0)
#define do_nothing()        do { } while (0)
#define TRACE(FORMAT,...)   do_nothing()
#define INDENT(INC)         do_nothing()
#define DUMPi(S, M)         do_nothing()
#define DUMPx(S, M)         do_nothing()
#define DUMPp(S, M)         do_nothing()
#define DUMPm(S, M, I, J)   do_nothing()
#endif

// Dumps matrix[N][M] = N rows x M columns (uint8_t)
static void
dump_matrix_NxM(const char *label, const uint8_t *matrix, int N, int M, int L)
{
    int i, j, n = 0;

    TRACE(".%s = {\n", label);
    INDENT(1);
    for (j = 0; j < N; j++) {
        for (i = 0; i < M; i++, n++) {
            if (n >= L)
                break;
            if (i > 0)
                TRACE(", ");
            TRACE("0x%02x", matrix[n]);
        }
        if (j < (N - 1))
            TRACE(",");
        TRACE("\n");
        if (n >= L)
            break;
    }
    INDENT(-1);
    TRACE("}\n");
}

// Dumps VdpPictureInfoMPEG1Or2
void dump_VdpPictureInfoMPEG1Or2(VdpPictureInfoMPEG1Or2 *pic_info)
{
    INDENT(1);
    TRACE("VdpPictureInfoMPEG1Or2 = {\n");
    INDENT(1);
    DUMPx(pic_info, forward_reference);
    DUMPx(pic_info, backward_reference);
    DUMPi(pic_info, slice_count);
    DUMPi(pic_info, picture_structure);
    DUMPi(pic_info, picture_coding_type);
    DUMPi(pic_info, intra_dc_precision);
    DUMPi(pic_info, frame_pred_frame_dct);
    DUMPi(pic_info, concealment_motion_vectors);
    DUMPi(pic_info, intra_vlc_format);
    DUMPi(pic_info, alternate_scan);
    DUMPi(pic_info, q_scale_type);
    DUMPi(pic_info, top_field_first);
    DUMPi(pic_info, full_pel_forward_vector);
    DUMPi(pic_info, full_pel_backward_vector);
    TRACE(".f_code = { { %d, %d }, { %d, %d } };\n",
          pic_info->f_code[0][0], pic_info->f_code[0][1],
          pic_info->f_code[1][0], pic_info->f_code[1][1]);
    DUMPm(pic_info, intra_quantizer_matrix, 8, 8);
    DUMPm(pic_info, non_intra_quantizer_matrix, 8, 8);
    INDENT(-1);
    TRACE("};\n");
    INDENT(-1);
}

// Dumps VdpPictureInfoMPEG4Part2
#if HAVE_VDPAU_MPEG4
void dump_VdpPictureInfoMPEG4Part2(VdpPictureInfoMPEG4Part2 *pic_info)
{
    INDENT(1);
    TRACE("VdpPictureInfoMPEG4Part2 = {\n");
    INDENT(1);
    DUMPx(pic_info, forward_reference);
    DUMPx(pic_info, backward_reference);
    DUMPi(pic_info, vop_time_increment_resolution);
    DUMPi(pic_info, vop_coding_type);
    DUMPi(pic_info, vop_fcode_forward);
    DUMPi(pic_info, vop_fcode_backward);
    DUMPi(pic_info, resync_marker_disable);
    DUMPi(pic_info, interlaced);
    DUMPi(pic_info, quant_type);
    DUMPi(pic_info, quarter_sample);
    DUMPi(pic_info, short_video_header);
    DUMPi(pic_info, rounding_control);
    DUMPi(pic_info, alternate_vertical_scan_flag);
    DUMPi(pic_info, top_field_first);
    DUMPm(pic_info, intra_quantizer_matrix, 8, 8);
    DUMPm(pic_info, non_intra_quantizer_matrix, 8, 8);
    INDENT(-1);
    TRACE("};\n");
    INDENT(-1);
}
#endif

// Dumps VdpReferenceFrameH264
static void
dump_VdpReferenceFrameH264(VdpReferenceFrameH264 *rf, const char *label)
{
    TRACE(".%s = {\n", label);
    INDENT(1);
    DUMPx(rf, surface);
    DUMPi(rf, is_long_term);
    DUMPi(rf, top_is_reference);
    DUMPi(rf, bottom_is_reference);
    DUMPi(rf, field_order_cnt[0]);
    DUMPi(rf, field_order_cnt[1]);
    DUMPi(rf, frame_idx);
    INDENT(-1);
    TRACE("}\n");
}

// Dumps VdpPictureInfoH264
void dump_VdpPictureInfoH264(VdpPictureInfoH264 *pic_info)
{
    int i;

    INDENT(1);
    TRACE("VdpPictureInfoH264 = {\n");
    INDENT(1);
    DUMPi(pic_info, slice_count);
    DUMPi(pic_info, field_order_cnt[0]);
    DUMPi(pic_info, field_order_cnt[1]);
    DUMPi(pic_info, is_reference);
    DUMPi(pic_info, frame_num);
    DUMPi(pic_info, field_pic_flag);
    DUMPi(pic_info, bottom_field_flag);
    DUMPi(pic_info, num_ref_frames);
    DUMPi(pic_info, mb_adaptive_frame_field_flag);
    DUMPi(pic_info, constrained_intra_pred_flag);
    DUMPi(pic_info, weighted_pred_flag);
    DUMPi(pic_info, weighted_bipred_idc);
    DUMPi(pic_info, frame_mbs_only_flag);
    DUMPi(pic_info, transform_8x8_mode_flag);
    DUMPi(pic_info, chroma_qp_index_offset);
    DUMPi(pic_info, second_chroma_qp_index_offset);
    DUMPi(pic_info, pic_init_qp_minus26);
    DUMPi(pic_info, num_ref_idx_l0_active_minus1);
    DUMPi(pic_info, num_ref_idx_l1_active_minus1);
    DUMPi(pic_info, log2_max_frame_num_minus4);
    DUMPi(pic_info, pic_order_cnt_type);
    DUMPi(pic_info, log2_max_pic_order_cnt_lsb_minus4);
    DUMPi(pic_info, delta_pic_order_always_zero_flag);
    DUMPi(pic_info, direct_8x8_inference_flag);
    DUMPi(pic_info, entropy_coding_mode_flag);
    DUMPi(pic_info, pic_order_present_flag);
    DUMPi(pic_info, deblocking_filter_control_present_flag);
    DUMPi(pic_info, redundant_pic_cnt_present_flag);
    DUMPm(pic_info, scaling_lists_4x4, 6, 16);
    DUMPm(pic_info, scaling_lists_8x8[0], 8, 8);
    DUMPm(pic_info, scaling_lists_8x8[1], 8, 8);
    for (i = 0; i < 16; i++) {
        char label[100];
        sprintf(label, "referenceFrames[%d]", i);
        dump_VdpReferenceFrameH264(&pic_info->referenceFrames[i], label);
    }
    INDENT(-1);
    TRACE("};\n");
    INDENT(-1);
}

// Dumps VdpPictureInfoVC1
void dump_VdpPictureInfoVC1(VdpPictureInfoVC1 *pic_info)
{
    INDENT(1);
    TRACE("VdpPictureInfoVC1 = {\n");
    INDENT(1);
    DUMPx(pic_info, forward_reference);
    DUMPx(pic_info, backward_reference);
    DUMPi(pic_info, slice_count);
    DUMPi(pic_info, picture_type);
    DUMPi(pic_info, frame_coding_mode);
    DUMPi(pic_info, postprocflag);
    DUMPi(pic_info, pulldown);
    DUMPi(pic_info, interlace);
    DUMPi(pic_info, tfcntrflag);
    DUMPi(pic_info, finterpflag);
    DUMPi(pic_info, psf);
    DUMPi(pic_info, dquant);
    DUMPi(pic_info, panscan_flag);
    DUMPi(pic_info, refdist_flag);
    DUMPi(pic_info, quantizer);
    DUMPi(pic_info, extended_mv);
    DUMPi(pic_info, extended_dmv);
    DUMPi(pic_info, overlap);
    DUMPi(pic_info, vstransform);
    DUMPi(pic_info, loopfilter);
    DUMPi(pic_info, fastuvmc);
    DUMPi(pic_info, range_mapy_flag);
    DUMPi(pic_info, range_mapy);
    DUMPi(pic_info, range_mapuv_flag);
    DUMPi(pic_info, range_mapuv);
    DUMPi(pic_info, multires);
    DUMPi(pic_info, syncmarker);
    DUMPi(pic_info, rangered);
    DUMPi(pic_info, maxbframes);
    DUMPi(pic_info, deblockEnable);
    DUMPi(pic_info, pquant);
    INDENT(-1);
    TRACE("};\n");
    INDENT(-1);
}

// Dumps VdpBitstreamBuffer
void dump_VdpBitstreamBuffer(VdpBitstreamBuffer *bitstream_buffer)
{
    const uint8_t *buffer = bitstream_buffer->bitstream;
    const uint32_t size   = bitstream_buffer->bitstream_bytes;

    INDENT(1);
    TRACE("VdpBitstreamBuffer (%d bytes) = {\n", size);
    INDENT(1);
    dump_matrix_NxM("buffer", buffer, 10, 15, size);
    INDENT(-1);
    TRACE("};\n");
    INDENT(-1);
}
