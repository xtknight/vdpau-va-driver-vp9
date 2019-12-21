/*
 *  vdpau_gate.c - VDPAU hooks
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
#include "vdpau_gate.h"
#include "vdpau_video.h"

#define DEBUG 1
#include "debug.h"


// Initialize VDPAU hooks
int vdpau_gate_init(vdpau_driver_data_t *driver_data)
{
    VdpStatus vdp_status;

#define VDP_INIT_PROC(FUNC_ID, FUNC) do {                       \
        vdp_status = driver_data->vdp_get_proc_address          \
            (driver_data->vdp_device,                           \
             VDP_FUNC_ID_##FUNC_ID,                             \
             (void *)&driver_data->vdp_vtable.vdp_##FUNC);      \
        if (vdp_status != VDP_STATUS_OK)                        \
            return -1;                                          \
    } while (0)

    VDP_INIT_PROC(DEVICE_DESTROY,
                  device_destroy);
    VDP_INIT_PROC(GENERATE_CSC_MATRIX,
                  generate_csc_matrix);
    VDP_INIT_PROC(VIDEO_SURFACE_CREATE,
                  video_surface_create);
    VDP_INIT_PROC(VIDEO_SURFACE_DESTROY,
                  video_surface_destroy);
    VDP_INIT_PROC(VIDEO_SURFACE_GET_BITS_Y_CB_CR,
                  video_surface_get_bits_ycbcr);
    VDP_INIT_PROC(VIDEO_SURFACE_PUT_BITS_Y_CB_CR,
                  video_surface_put_bits_ycbcr);
    VDP_INIT_PROC(OUTPUT_SURFACE_CREATE,
                  output_surface_create);
    VDP_INIT_PROC(OUTPUT_SURFACE_DESTROY,
                  output_surface_destroy);
    VDP_INIT_PROC(OUTPUT_SURFACE_GET_BITS_NATIVE,
                  output_surface_get_bits_native);
    VDP_INIT_PROC(OUTPUT_SURFACE_PUT_BITS_NATIVE,
                  output_surface_put_bits_native);
    VDP_INIT_PROC(OUTPUT_SURFACE_RENDER_BITMAP_SURFACE,
                  output_surface_render_bitmap_surface);
    VDP_INIT_PROC(OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,
                  output_surface_render_output_surface);
    VDP_INIT_PROC(OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,
                  output_surface_query_put_bits_indexed_capabilities);
    VDP_INIT_PROC(OUTPUT_SURFACE_PUT_BITS_INDEXED,
                  output_surface_put_bits_indexed);
    VDP_INIT_PROC(BITMAP_SURFACE_QUERY_CAPABILITIES,
                  bitmap_surface_query_capabilities);
    VDP_INIT_PROC(BITMAP_SURFACE_CREATE,
                  bitmap_surface_create);
    VDP_INIT_PROC(BITMAP_SURFACE_DESTROY,
                  bitmap_surface_destroy);
    VDP_INIT_PROC(BITMAP_SURFACE_PUT_BITS_NATIVE,
                  bitmap_surface_put_bits_native);
    VDP_INIT_PROC(VIDEO_MIXER_CREATE,
                  video_mixer_create);
    VDP_INIT_PROC(VIDEO_MIXER_DESTROY,
                  video_mixer_destroy);
    VDP_INIT_PROC(VIDEO_MIXER_RENDER,
                  video_mixer_render);
    VDP_INIT_PROC(VIDEO_MIXER_QUERY_FEATURE_SUPPORT,
                  video_mixer_query_feature_support);
    VDP_INIT_PROC(VIDEO_MIXER_GET_FEATURE_ENABLES,
                  video_mixer_get_feature_enables);
    VDP_INIT_PROC(VIDEO_MIXER_SET_FEATURE_ENABLES,
                  video_mixer_set_feature_enables);
    VDP_INIT_PROC(VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT,
                  video_mixer_query_attribute_support);
    VDP_INIT_PROC(VIDEO_MIXER_GET_ATTRIBUTE_VALUES,
                  video_mixer_get_attribute_values);
    VDP_INIT_PROC(VIDEO_MIXER_SET_ATTRIBUTE_VALUES,
                  video_mixer_set_attribute_values);
    VDP_INIT_PROC(PRESENTATION_QUEUE_CREATE,
                  presentation_queue_create);
    VDP_INIT_PROC(PRESENTATION_QUEUE_DESTROY,
                  presentation_queue_destroy);
    VDP_INIT_PROC(PRESENTATION_QUEUE_SET_BACKGROUND_COLOR,
                  presentation_queue_set_background_color);
    VDP_INIT_PROC(PRESENTATION_QUEUE_GET_BACKGROUND_COLOR,
                  presentation_queue_get_background_color);
    VDP_INIT_PROC(PRESENTATION_QUEUE_DISPLAY,
                  presentation_queue_display);
    VDP_INIT_PROC(PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
                  presentation_queue_block_until_surface_idle);
    VDP_INIT_PROC(PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,
                  presentation_queue_query_surface_status);
    VDP_INIT_PROC(PRESENTATION_QUEUE_TARGET_CREATE_X11,
                  presentation_queue_target_create_x11);
    VDP_INIT_PROC(PRESENTATION_QUEUE_TARGET_DESTROY,
                  presentation_queue_target_destroy);
    VDP_INIT_PROC(DECODER_CREATE,
                  decoder_create);
    VDP_INIT_PROC(DECODER_DESTROY,
                  decoder_destroy);
    VDP_INIT_PROC(DECODER_RENDER,
                  decoder_render);
    VDP_INIT_PROC(DECODER_QUERY_CAPABILITIES,
                  decoder_query_capabilities);
    VDP_INIT_PROC(VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES,
                  video_surface_query_ycbcr_caps);
    VDP_INIT_PROC(OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES,
                  output_surface_query_rgba_caps);
    VDP_INIT_PROC(GET_API_VERSION,
                  get_api_version);
    VDP_INIT_PROC(GET_INFORMATION_STRING,
                  get_information_string);
    VDP_INIT_PROC(GET_ERROR_STRING,
                  get_error_string);

#undef VDP_INIT_PROC
    return 0;
}

// Deinitialize VDPAU hooks
void vdpau_gate_exit(vdpau_driver_data_t *driver_data)
{
}

// Check VdpStatus
int vdpau_check_status(
    vdpau_driver_data_p driver_data,
    VdpStatus           vdp_status,
    const char         *msg
)
{
    if (vdp_status != VDP_STATUS_OK) {
        const char *vdp_status_string;
        vdp_status_string = vdpau_get_error_string(driver_data, vdp_status);
        vdpau_information_message("%s: status %d: %s\n", msg, vdp_status,
            vdp_status_string ? vdp_status_string : "<unknown error>");
        return 0;
    }
    return 1;
}

#define VDPAU_INVOKE_(retval, func, ...)               \
    (driver_data && driver_data->vdp_vtable.vdp_##func \
     ? driver_data->vdp_vtable.vdp_##func(__VA_ARGS__) \
     : (retval))

#define VDPAU_INVOKE(func, ...)                        \
    VDPAU_INVOKE_(VDP_STATUS_INVALID_POINTER,          \
                  func, __VA_ARGS__)

// VdpGenerateCSCMatrix
VdpStatus
vdpau_generate_csc_matrix(
    vdpau_driver_data_p driver_data,
    VdpProcamp         *procamp,
    VdpColorStandard    standard,
    VdpCSCMatrix       *csc_matrix
)
{
    return VDPAU_INVOKE(generate_csc_matrix,
                        procamp,
                        standard,
                        csc_matrix);
}

// VdpDeviceDestroy
VdpStatus
vdpau_device_destroy(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device
)
{
    return VDPAU_INVOKE(device_destroy, device);
}

// VdpVideoSurfaceCreate
VdpStatus
vdpau_video_surface_create(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpChromaType        chroma_type,
    uint32_t             width,
    uint32_t             height,
    VdpVideoSurface     *surface
)
{
    return VDPAU_INVOKE(video_surface_create,
                        device,
                        chroma_type,
                        width,
                        height,
                        surface);
}

// VdpVideoSurfaceDestroy
VdpStatus
vdpau_video_surface_destroy(
    vdpau_driver_data_t *driver_data,
    VdpVideoSurface      surface
)
{
    return VDPAU_INVOKE(video_surface_destroy, surface);
}

// VdpVideoSurfaceGetBitsYCbCr
VdpStatus
vdpau_video_surface_get_bits_ycbcr(
    vdpau_driver_data_t *driver_data,
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **dest,
    uint32_t            *stride
)
{
    return VDPAU_INVOKE(video_surface_get_bits_ycbcr,
                        surface,
                        format,
                        (void * const *)dest,
                        stride);
}

// VdpVideoSurfacePutBitsYCbCr
VdpStatus
vdpau_video_surface_put_bits_ycbcr(
    vdpau_driver_data_t *driver_data,
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **src,
    uint32_t            *stride
)
{
    return VDPAU_INVOKE(video_surface_put_bits_ycbcr,
                        surface,
                        format,
                        (const void * const *)src,
                        stride);
}

// VdpOutputSurfaceCreate
VdpStatus
vdpau_output_surface_create(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpOutputSurface    *surface
)
{
    return VDPAU_INVOKE(output_surface_create,
                        device,
                        rgba_format,
                        width,
                        height,
                        surface);
}

// VdpOutputSurfaceDestroy
VdpStatus
vdpau_output_surface_destroy(
    vdpau_driver_data_t *driver_data,
    VdpOutputSurface     surface
)
{
    return VDPAU_INVOKE(output_surface_destroy, surface);
}

// VdpOutputSurfaceGetBitsNative
VdpStatus
vdpau_output_surface_get_bits_native(
    vdpau_driver_data_t *driver_data,
    VdpOutputSurface     surface,
    const VdpRect       *source_rect,
    uint8_t            **dst,
    uint32_t            *stride
)
{
    return VDPAU_INVOKE(output_surface_get_bits_native,
                        surface,
                        source_rect,
                        (void * const *)dst,
                        stride);
}

// VdpOutputSurfacePutBitsNative
VdpStatus
vdpau_output_surface_put_bits_native(
    vdpau_driver_data_t *driver_data,
    VdpOutputSurface     surface,
    const uint8_t      **src,
    uint32_t            *stride,
    const VdpRect       *dest_rect
)
{
    return VDPAU_INVOKE(output_surface_put_bits_native,
                        surface,
                        (const void * const *)src,
                        stride,
                        dest_rect);
}

// VdpOutputSurfaceRenderBitmapSurface
VdpStatus
vdpau_output_surface_render_bitmap_surface(
    vdpau_driver_data_t *driver_data,
    VdpOutputSurface     destination_surface,
    const VdpRect       *destination_rect,
    VdpBitmapSurface     source_surface,
    const VdpRect       *source_rect,
    const VdpColor      *colors,
    const VdpOutputSurfaceRenderBlendState *blend_state,
    uint32_t             flags
)
{
    return VDPAU_INVOKE(output_surface_render_bitmap_surface,
                        destination_surface,
                        destination_rect,
                        source_surface,
                        source_rect,
                        colors,
                        blend_state,
                        flags);
}

// VdpOutputSurfaceRenderOutputSurface
VdpStatus
vdpau_output_surface_render_output_surface(
    vdpau_driver_data_p driver_data,
    VdpOutputSurface    destination_surface,
    const VdpRect      *destination_rect,
    VdpOutputSurface    source_surface,
    const VdpRect      *source_rect,
    const VdpColor     *colors,
    const VdpOutputSurfaceRenderBlendState *blend_state,
    uint32_t            flags
)
{
    return VDPAU_INVOKE(output_surface_render_output_surface,
                        destination_surface,
                        destination_rect,
                        source_surface,
                        source_rect,
                        colors,
                        blend_state,
                        flags);
}

// VdpOutputSurfaceQueryPutBitsIndexedCapabilities
VdpStatus
vdpau_output_surface_query_put_bits_indexed_capabilities(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    VdpIndexedFormat     bits_indexed_format,
    VdpColorTableFormat  color_table_format,
    VdpBool             *is_supported
)
{
    return VDPAU_INVOKE(output_surface_query_put_bits_indexed_capabilities,
                        device,
                        rgba_format,
                        bits_indexed_format,
                        color_table_format,
                        is_supported);
}

// VdpOutputSurfacePutBitsIndexed
VdpStatus
vdpau_output_surface_put_bits_indexed(
    vdpau_driver_data_p  driver_data,
    VdpOutputSurface     surface,
    VdpIndexedFormat     source_indexed_format,
    const uint8_t      **source_data,
    const uint32_t      *source_pitch,
    const VdpRect       *destination_rect,
    VdpColorTableFormat  color_table_format,
    const void          *color_table
)
{
    return VDPAU_INVOKE(output_surface_put_bits_indexed,
                        surface,
                        source_indexed_format,
                        (const void **)source_data,
                        source_pitch,
                        destination_rect,
                        color_table_format,
                        color_table);
}

// VdpBitmapSurfaceQueryCapabilities
VdpStatus
vdpau_bitmap_surface_query_capabilities(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    VdpBool             *is_supported,
    uint32_t            *max_width,
    uint32_t            *max_height
)
{
    return VDPAU_INVOKE(bitmap_surface_query_capabilities,
                        device,
                        rgba_format,
                        is_supported,
                        max_width,
                        max_height);
}

// VdpBitmapSurfaceCreate
VdpStatus
vdpau_bitmap_surface_create(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpBool              frequently_accessed,
    VdpBitmapSurface    *surface
)
{
    return VDPAU_INVOKE(bitmap_surface_create,
                        device,
                        rgba_format,
                        width,
                        height,
                        frequently_accessed,
                        surface);
}

// VdpBitmapSurfaceDestroy
VdpStatus
vdpau_bitmap_surface_destroy(
    vdpau_driver_data_t *driver_data,
    VdpBitmapSurface     surface
)
{
    return VDPAU_INVOKE(bitmap_surface_destroy, surface);
}

// VdpBitmapSurfacePutBitsNative
VdpStatus
vdpau_bitmap_surface_put_bits_native(
    vdpau_driver_data_t *driver_data,
    VdpBitmapSurface     surface,
    const uint8_t      **src,
    const uint32_t      *stride,
    const VdpRect       *dest_rect
)
{
    return VDPAU_INVOKE(bitmap_surface_put_bits_native,
                        surface,
                        (const void * const *)src,
                        stride,
                        dest_rect);
}

// VdpVideoMixerCreate
VdpStatus
vdpau_video_mixer_create(
    vdpau_driver_data_t          *driver_data,
    VdpDevice                     device,
    uint32_t                      feature_count,
    VdpVideoMixerFeature const   *features,
    uint32_t                      parameter_count,
    VdpVideoMixerParameter const *parameters,
    const void                   *parameter_values,
    VdpVideoMixer                *mixer
)
{
    return VDPAU_INVOKE(video_mixer_create,
                        device,
                        feature_count,
                        features,
                        parameter_count,
                        parameters,
                        parameter_values,
                        mixer);
}

// VdpVideoMixerDestroy
VdpStatus
vdpau_video_mixer_destroy(
    vdpau_driver_data_t *driver_data,
    VdpVideoMixer        mixer
)
{
    return VDPAU_INVOKE(video_mixer_destroy, mixer);
}

// VdpVideoMixerRender
VdpStatus
vdpau_video_mixer_render(
    vdpau_driver_data_t          *driver_data,
    VdpVideoMixer                 mixer,
    VdpOutputSurface              background_surface,
    const VdpRect                *background_source_rect,
    VdpVideoMixerPictureStructure current_picture_structure,
    uint32_t                      video_surface_past_count,
    const VdpVideoSurface        *video_surface_past,
    VdpVideoSurface               video_surface_current,
    uint32_t                      video_surface_future_count,
    const VdpVideoSurface        *video_surface_future,
    const VdpRect                *video_source_rect,
    VdpOutputSurface              destination_surface,
    const VdpRect                *destination_rect,
    const VdpRect                *destination_video_rect,
    uint32_t                      layer_count,
    const VdpLayer               *layers
)
{
    return VDPAU_INVOKE(video_mixer_render,
                        mixer,
                        background_surface,
                        background_source_rect,
                        current_picture_structure,
                        video_surface_past_count,
                        video_surface_past,
                        video_surface_current,
                        video_surface_future_count,
                        video_surface_future,
                        video_source_rect,
                        destination_surface,
                        destination_rect,
                        destination_video_rect,
                        layer_count,
                        layers);
}

// VdpVideoMixerQueryFeatureSupport
VdpStatus
vdpau_video_mixer_query_feature_support(
    vdpau_driver_data_p         driver_data,
    VdpDevice                   device,
    VdpVideoMixerFeature        feature,
    VdpBool                    *is_supported
)
{
    return VDPAU_INVOKE(video_mixer_query_feature_support,
                        device,
                        feature,
                        is_supported);
}

// VdpVideoMixerGetFeatureEnables
VdpStatus
vdpau_video_mixer_get_feature_enables(
    vdpau_driver_data_p         driver_data,
    VdpVideoMixer               mixer,
    uint32_t                    feature_count,
    const VdpVideoMixerFeature *features,
    VdpBool                    *feature_enables
)
{
    return VDPAU_INVOKE(video_mixer_get_feature_enables,
                        mixer,
                        feature_count,
                        features,
                        feature_enables);
}

// VdpVideoMixerSetFeatureEnables
VdpStatus
vdpau_video_mixer_set_feature_enables(
    vdpau_driver_data_p         driver_data,
    VdpVideoMixer               mixer,
    uint32_t                    feature_count,
    const VdpVideoMixerFeature *features,
    const VdpBool              *feature_enables
)
{
    return VDPAU_INVOKE(video_mixer_set_feature_enables,
                        mixer,
                        feature_count,
                        features,
                        feature_enables);
}

// VdpVideoMixerQueryAttributeSupport
VdpStatus
vdpau_video_mixer_query_attribute_support(
    vdpau_driver_data_p         driver_data,
    VdpDevice                   device,
    VdpVideoMixerAttribute      attribute,
    VdpBool                    *is_supported
)
{
    return VDPAU_INVOKE(video_mixer_query_attribute_support,
                        device,
                        attribute,
                        is_supported);
}

// VdpVideoMixerGetAttributeValues
VdpStatus
vdpau_video_mixer_get_attribute_values(
    vdpau_driver_data_p           driver_data,
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    void                        **attribute_values
)
{
    return VDPAU_INVOKE(video_mixer_get_attribute_values,
                        mixer,
                        attribute_count,
                        attributes,
                        attribute_values);
}

// VdpVideoMixerSetAttributeValues
VdpStatus
vdpau_video_mixer_set_attribute_values(
    vdpau_driver_data_p           driver_data,
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    const void                  **attribute_values
)
{
    return VDPAU_INVOKE(video_mixer_set_attribute_values,
                        mixer,
                        attribute_count,
                        attributes,
                        attribute_values);
}

// VdpPresentationQueueCreate
VdpStatus
vdpau_presentation_queue_create(
    vdpau_driver_data_t       *driver_data,
    VdpDevice                  device,
    VdpPresentationQueueTarget presentation_queue_target,
    VdpPresentationQueue      *presentation_queue
)
{
    return VDPAU_INVOKE(presentation_queue_create,
                        device,
                        presentation_queue_target,
                        presentation_queue);
}

// VdpPresentationQueueDestroy
VdpStatus
vdpau_presentation_queue_destroy(
    vdpau_driver_data_t *driver_data,
    VdpPresentationQueue presentation_queue
)
{

    return VDPAU_INVOKE(presentation_queue_destroy, presentation_queue);
}

// VdpPresentationQueueSetBackgroundColor
VdpStatus
vdpau_presentation_queue_set_background_color(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    const VdpColor      *background_color
)
{
    return VDPAU_INVOKE(presentation_queue_set_background_color,
                        presentation_queue,
                        (VdpColor *)background_color);
}

//VdpPresentationQueueGetBackgroundColor
VdpStatus
vdpau_presentation_queue_get_background_color(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    VdpColor            *background_color
)
{
    return VDPAU_INVOKE(presentation_queue_get_background_color,
                        presentation_queue,
                        background_color);
}

// VdpPresentationQueueDisplay
VdpStatus
vdpau_presentation_queue_display(
    vdpau_driver_data_t *driver_data,
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    uint32_t             clip_width,
    uint32_t             clip_height,
    VdpTime              earliest_presentation_time
)
{
    return VDPAU_INVOKE(presentation_queue_display,
                        presentation_queue,
                        surface,
                        clip_width,
                        clip_height,
                        earliest_presentation_time);
}

// VdpPresentationQueueBlockUntilSurfaceIdle
VdpStatus
vdpau_presentation_queue_block_until_surface_idle(
    vdpau_driver_data_t *driver_data,
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    VdpTime             *first_presentation_time
)
{
    return VDPAU_INVOKE(presentation_queue_block_until_surface_idle,
                        presentation_queue,
                        surface,
                        first_presentation_time);
}

// VdpPresentationQueueQuerySurfaceStatus
VdpStatus
vdpau_presentation_queue_query_surface_status(
    vdpau_driver_data_t        *driver_data,
    VdpPresentationQueue        presentation_queue,
    VdpOutputSurface            surface,
    VdpPresentationQueueStatus *status,
    VdpTime                    *first_presentation_time
)
{
    return VDPAU_INVOKE(presentation_queue_query_surface_status,
                        presentation_queue,
                        surface,
                        status,
                        first_presentation_time);
}

// VdpPresentationQueueTargetCreateX11
VdpStatus
vdpau_presentation_queue_target_create_x11(
    vdpau_driver_data_t        *driver_data,
    VdpDevice                   device,
    Drawable                    drawable,
    VdpPresentationQueueTarget *target
)
{
    return VDPAU_INVOKE(presentation_queue_target_create_x11,
                        device,
                        drawable,
                        target);
}

// VdpPresentationQueueTargetDestroy
VdpStatus
vdpau_presentation_queue_target_destroy(
    vdpau_driver_data_t       *driver_data,
    VdpPresentationQueueTarget presentation_queue_target
)
{
    return VDPAU_INVOKE(presentation_queue_target_destroy,
                        presentation_queue_target);
}

// VdpDecoderCreate
VdpStatus
vdpau_decoder_create(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpDecoderProfile    profile,
    uint32_t             width,
    uint32_t             height,
    uint32_t             max_references,
    VdpDecoder          *decoder
)
{
    return VDPAU_INVOKE(decoder_create,
                        device,
                        profile,
                        width,
                        height,
                        max_references,
                        decoder);
}

// VdpDecoderDestroy
VdpStatus
vdpau_decoder_destroy(
    vdpau_driver_data_t *driver_data,
    VdpDecoder           decoder
)
{
    return VDPAU_INVOKE(decoder_destroy, decoder);
}

// VdpDecoderRender
VdpStatus
vdpau_decoder_render(
    vdpau_driver_data_t      *driver_data,
    VdpDecoder                decoder,
    VdpVideoSurface           target,
    VdpPictureInfo const     *picture_info,
    uint32_t                  bitstream_buffers_count,
    VdpBitstreamBuffer const *bitstream_buffers
)
{
    return VDPAU_INVOKE(decoder_render,
                        decoder,
                        target,
                        picture_info,
                        bitstream_buffers_count,
                        bitstream_buffers);
}

// VdpDecoderQueryCapabilities
VdpStatus
vdpau_decoder_query_capabilities(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpDecoderProfile    profile,
    VdpBool             *is_supported,
    uint32_t            *max_level,
    uint32_t            *max_references,
    uint32_t            *max_width,
    uint32_t            *max_height
)
{
    return VDPAU_INVOKE(decoder_query_capabilities,
                        device,
                        profile,
                        is_supported,
                        max_level,
                        max_references,
                        max_width,
                        max_height);
}

// VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities
VdpStatus
vdpau_video_surface_query_ycbcr_caps(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpChromaType        surface_chroma_type,
    VdpYCbCrFormat       bits_ycbcr_format,
    VdpBool             *is_supported
)
{
    return VDPAU_INVOKE(video_surface_query_ycbcr_caps,
                        device,
                        surface_chroma_type,
                        bits_ycbcr_format,
                        is_supported);
}

// VdpOutputSurfaceQueryGetPutBitsNativeCapabilities
VdpStatus
vdpau_output_surface_query_rgba_caps(
    vdpau_driver_data_t *driver_data,
    VdpDevice            device,
    VdpRGBAFormat        surface_rgba_format,
    VdpBool             *is_supported
)
{
    return VDPAU_INVOKE(output_surface_query_rgba_caps,
                        device,
                        surface_rgba_format,
                        is_supported);
}

// VdpGetApiVersion
VdpStatus
vdpau_get_api_version(vdpau_driver_data_t *driver_data, uint32_t *api_version)
{
    return VDPAU_INVOKE(get_api_version, api_version);
}

// VdpGetInformationString
VdpStatus
vdpau_get_information_string(
    vdpau_driver_data_t *driver_data,
    const char         **info_string
)
{
    return VDPAU_INVOKE(get_information_string, info_string);
}

// VdpGetErrorString
const char *
vdpau_get_error_string(vdpau_driver_data_t *driver_data, VdpStatus vdp_status)
{
    return VDPAU_INVOKE_(NULL, get_error_string, vdp_status);
}
