/*
 *  vdpau_gate.h - VDPAU hooks
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

#ifndef VDPAU_GATE_H
#define VDPAU_GATE_H

#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

typedef struct vdpau_vtable       vdpau_vtable_t;
typedef struct vdpau_vtable      *vdpau_vtable_p;
typedef struct vdpau_driver_data *vdpau_driver_data_p;

// VDPAU VTable
struct vdpau_vtable {
    VdpDeviceDestroy                    *vdp_device_destroy;
    VdpGenerateCSCMatrix                *vdp_generate_csc_matrix;
    VdpVideoSurfaceCreate               *vdp_video_surface_create;
    VdpVideoSurfaceDestroy              *vdp_video_surface_destroy;
    VdpVideoSurfaceGetBitsYCbCr         *vdp_video_surface_get_bits_ycbcr;
    VdpVideoSurfacePutBitsYCbCr         *vdp_video_surface_put_bits_ycbcr;
    VdpOutputSurfaceCreate              *vdp_output_surface_create;
    VdpOutputSurfaceDestroy             *vdp_output_surface_destroy;
    VdpOutputSurfaceGetBitsNative       *vdp_output_surface_get_bits_native;
    VdpOutputSurfacePutBitsNative       *vdp_output_surface_put_bits_native;
    VdpOutputSurfaceRenderBitmapSurface *vdp_output_surface_render_bitmap_surface;
    VdpOutputSurfaceRenderOutputSurface *vdp_output_surface_render_output_surface;
    VdpOutputSurfaceQueryPutBitsIndexedCapabilities *vdp_output_surface_query_put_bits_indexed_capabilities;
    VdpOutputSurfacePutBitsIndexed      *vdp_output_surface_put_bits_indexed;
    VdpBitmapSurfaceQueryCapabilities   *vdp_bitmap_surface_query_capabilities;
    VdpBitmapSurfaceCreate              *vdp_bitmap_surface_create;
    VdpBitmapSurfaceDestroy             *vdp_bitmap_surface_destroy;
    VdpBitmapSurfacePutBitsNative       *vdp_bitmap_surface_put_bits_native;
    VdpVideoMixerCreate                 *vdp_video_mixer_create;
    VdpVideoMixerDestroy                *vdp_video_mixer_destroy;
    VdpVideoMixerRender                 *vdp_video_mixer_render;
    VdpVideoMixerQueryFeatureSupport    *vdp_video_mixer_query_feature_support;
    VdpVideoMixerGetFeatureEnables      *vdp_video_mixer_get_feature_enables;
    VdpVideoMixerSetFeatureEnables      *vdp_video_mixer_set_feature_enables;
    VdpVideoMixerQueryAttributeSupport  *vdp_video_mixer_query_attribute_support;
    VdpVideoMixerGetAttributeValues     *vdp_video_mixer_get_attribute_values;
    VdpVideoMixerSetAttributeValues     *vdp_video_mixer_set_attribute_values;
    VdpPresentationQueueCreate          *vdp_presentation_queue_create;
    VdpPresentationQueueDestroy         *vdp_presentation_queue_destroy;
    VdpPresentationQueueSetBackgroundColor *vdp_presentation_queue_set_background_color;
    VdpPresentationQueueGetBackgroundColor *vdp_presentation_queue_get_background_color;
    VdpPresentationQueueDisplay         *vdp_presentation_queue_display;
    VdpPresentationQueueBlockUntilSurfaceIdle *vdp_presentation_queue_block_until_surface_idle;
    VdpPresentationQueueQuerySurfaceStatus *vdp_presentation_queue_query_surface_status;
    VdpPresentationQueueTargetCreateX11 *vdp_presentation_queue_target_create_x11;
    VdpPresentationQueueTargetDestroy   *vdp_presentation_queue_target_destroy;
    VdpDecoderCreate                    *vdp_decoder_create;
    VdpDecoderDestroy                   *vdp_decoder_destroy;
    VdpDecoderRender                    *vdp_decoder_render;
    VdpDecoderQueryCapabilities         *vdp_decoder_query_capabilities;
    VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *vdp_video_surface_query_ycbcr_caps;
    VdpOutputSurfaceQueryGetPutBitsNativeCapabilities *vdp_output_surface_query_rgba_caps;
    VdpGetApiVersion                    *vdp_get_api_version;
    VdpGetInformationString             *vdp_get_information_string;
    VdpGetErrorString                   *vdp_get_error_string;
};

// Initialize VDPAU hooks
int vdpau_gate_init(vdpau_driver_data_p driver_data)
    attribute_hidden;

// Deinitialize VDPAU hooks
void vdpau_gate_exit(vdpau_driver_data_p driver_data)
    attribute_hidden;

// Check VdpStatus
int vdpau_check_status(
    vdpau_driver_data_p driver_data,
    VdpStatus           vdp_status,
    const char         *msg
) attribute_hidden;

#define VDPAU_CHECK_STATUS(status, msg) \
    vdpau_check_status(driver_data, status, msg)

// VdpGetApiVersion
VdpStatus
vdpau_get_api_version(vdpau_driver_data_p driver_data, uint32_t *api_version)
    attribute_hidden;

// VdpGetInformationString
VdpStatus
vdpau_get_information_string(
    vdpau_driver_data_p  driver_data,
    const char         **info_string
) attribute_hidden;

// VdpGetErrorString
const char *
vdpau_get_error_string(vdpau_driver_data_p driver_data, VdpStatus vdp_status)
    attribute_hidden;

// VdpGenerateCSCMatrix
VdpStatus
vdpau_generate_csc_matrix(
    vdpau_driver_data_p driver_data,
    VdpProcamp         *procamp,
    VdpColorStandard    standard,
    VdpCSCMatrix       *csc_matrix
) attribute_hidden;

// VdpDeviceDestroy
VdpStatus
vdpau_device_destroy(
    vdpau_driver_data_p driver_data,
    VdpDevice           device
) attribute_hidden;

// VdpVideoSurfaceCreate
VdpStatus
vdpau_video_surface_create(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpChromaType        chroma_type,
    uint32_t             width,
    uint32_t             height,
    VdpVideoSurface     *surface
) attribute_hidden;

// VdpVideoSurfaceDestroy
VdpStatus
vdpau_video_surface_destroy(
    vdpau_driver_data_p  driver_data,
    VdpVideoSurface      surface
) attribute_hidden;

// VdpVideoSurfaceGetBitsYCbCr
VdpStatus
vdpau_video_surface_get_bits_ycbcr(
    vdpau_driver_data_p  driver_data,
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **dest,
    uint32_t            *stride
) attribute_hidden;

// VdpVideoSurfacePutBitsYCbCr
VdpStatus
vdpau_video_surface_put_bits_ycbcr(
    vdpau_driver_data_p  driver_data,
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **src,
    uint32_t            *stride
) attribute_hidden;

// VdpOutputSurfaceCreate
VdpStatus
vdpau_output_surface_create(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpOutputSurface    *surface
) attribute_hidden;

// VdpOutputSurfaceDestroy
VdpStatus
vdpau_output_surface_destroy(
    vdpau_driver_data_p  driver_data,
    VdpOutputSurface     surface
) attribute_hidden;

// VdpOutputSurfaceGetBitsNative
VdpStatus
vdpau_output_surface_get_bits_native(
    vdpau_driver_data_p  driver_data,
    VdpOutputSurface     surface,
    const VdpRect       *source_rect,
    uint8_t            **dst,
    uint32_t            *stride
) attribute_hidden;

// VdpOutputSurfacePutBitsNative
VdpStatus
vdpau_output_surface_put_bits_native(
    vdpau_driver_data_p  driver_data,
    VdpOutputSurface     surface,
    const uint8_t      **src,
    uint32_t            *stride,
    const VdpRect       *dest_rect
) attribute_hidden;

// VdpOutputSurfaceRenderBitmapSurface
VdpStatus
vdpau_output_surface_render_bitmap_surface(
    vdpau_driver_data_p  driver_data,
    VdpOutputSurface     destination_surface,
    const VdpRect       *destination_rect,
    VdpBitmapSurface     source_surface,
    const VdpRect       *source_rect,
    const VdpColor      *colors,
    const VdpOutputSurfaceRenderBlendState *blend_state,
    uint32_t             flags
) attribute_hidden;

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
) attribute_hidden;

// VdpOutputSurfaceQueryPutBitsIndexedCapabilities
VdpStatus
vdpau_output_surface_query_put_bits_indexed_capabilities(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    VdpIndexedFormat     bits_indexed_format,
    VdpColorTableFormat  color_table_format,
    VdpBool             *is_supported
) attribute_hidden;

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
) attribute_hidden;

// VdpBitmapSurfaceQueryCapabilities
VdpStatus
vdpau_bitmap_surface_query_capabilities(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    VdpBool             *is_supported,
    uint32_t            *max_width,
    uint32_t            *max_height
) attribute_hidden;

// VdpBitmapSurfaceCreate
VdpStatus
vdpau_bitmap_surface_create(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpBool              frequently_accessed,
    VdpBitmapSurface    *surface
) attribute_hidden;

// VdpBitmapSurfaceDestroy
VdpStatus
vdpau_bitmap_surface_destroy(
    vdpau_driver_data_p  driver_data,
    VdpBitmapSurface     surface
) attribute_hidden;

// VdpBitmapSurfacePutBitsNative
VdpStatus
vdpau_bitmap_surface_put_bits_native(
    vdpau_driver_data_p  driver_data,
    VdpBitmapSurface     surface,
    const uint8_t      **src,
    const uint32_t      *stride,
    const VdpRect       *dest_rect
) attribute_hidden;

// VdpVideoMixerCreate
VdpStatus
vdpau_video_mixer_create(
    vdpau_driver_data_p           driver_data,
    VdpDevice                     device,
    uint32_t                      feature_count,
    VdpVideoMixerFeature const   *features,
    uint32_t                      parameter_count,
    VdpVideoMixerParameter const *parameters,
    const void                   *parameter_values,
    VdpVideoMixer                *mixer
) attribute_hidden;

// VdpVideoMixerDestroy
VdpStatus
vdpau_video_mixer_destroy(
    vdpau_driver_data_p  driver_data,
    VdpVideoMixer        mixer
) attribute_hidden;

// VdpVideoMixerRender
VdpStatus
vdpau_video_mixer_render(
    vdpau_driver_data_p           driver_data,
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
) attribute_hidden;

// VdpVideoMixerQueryFeatureSupport
VdpStatus
vdpau_video_mixer_query_feature_support(
    vdpau_driver_data_p         driver_data,
    VdpDevice                   device,
    VdpVideoMixerFeature        feature,
    VdpBool                    *is_supported
) attribute_hidden;

// VdpVideoMixerGetFeatureEnables
VdpStatus
vdpau_video_mixer_get_feature_enables(
    vdpau_driver_data_p         driver_data,
    VdpVideoMixer               mixer,
    uint32_t                    feature_count,
    const VdpVideoMixerFeature *features,
    VdpBool                    *feature_enables
) attribute_hidden;

// VdpVideoMixerSetFeatureEnables
VdpStatus
vdpau_video_mixer_set_feature_enables(
    vdpau_driver_data_p         driver_data,
    VdpVideoMixer               mixer,
    uint32_t                    feature_count,
    const VdpVideoMixerFeature *features,
    const VdpBool              *feature_enables
) attribute_hidden;

// VdpVideoMixerQueryAttributeSupport
VdpStatus
vdpau_video_mixer_query_attribute_support(
    vdpau_driver_data_p         driver_data,
    VdpDevice                   device,
    VdpVideoMixerAttribute      attribute,
    VdpBool                    *is_supported
) attribute_hidden;

// VdpVideoMixerGetAttributeValues
VdpStatus
vdpau_video_mixer_get_attribute_values(
    vdpau_driver_data_p           driver_data,
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    void                        **attribute_values
) attribute_hidden;

// VdpVideoMixerSetAttributeValues
VdpStatus
vdpau_video_mixer_set_attribute_values(
    vdpau_driver_data_p           driver_data,
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    const void                  **attribute_values
) attribute_hidden;

// VdpPresentationQueueCreate
VdpStatus
vdpau_presentation_queue_create(
    vdpau_driver_data_p        driver_data,
    VdpDevice                  device,
    VdpPresentationQueueTarget presentation_queue_target,
    VdpPresentationQueue      *presentation_queue
) attribute_hidden;

// VdpPresentationQueueDestroy
VdpStatus
vdpau_presentation_queue_destroy(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue
) attribute_hidden;

// VdpPresentationQueueSetBackgroundColor
VdpStatus
vdpau_presentation_queue_set_background_color(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    const VdpColor      *background_color
) attribute_hidden;

//VdpPresentationQueueGetBackgroundColor
VdpStatus
vdpau_presentation_queue_get_background_color(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    VdpColor            *background_color
) attribute_hidden;

// VdpPresentationQueueDisplay
VdpStatus
vdpau_presentation_queue_display(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    uint32_t             clip_width,
    uint32_t             clip_height,
    VdpTime              earliest_presentation_time
) attribute_hidden;

// VdpPresentationQueueBlockUntilSurfaceIdle
VdpStatus
vdpau_presentation_queue_block_until_surface_idle(
    vdpau_driver_data_p  driver_data,
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    VdpTime             *first_presentation_time
) attribute_hidden;

// VdpPresentationQueueQuerySurfaceStatus
VdpStatus
vdpau_presentation_queue_query_surface_status(
    vdpau_driver_data_p         driver_data,
    VdpPresentationQueue        presentation_queue,
    VdpOutputSurface            surface,
    VdpPresentationQueueStatus *status,
    VdpTime                    *first_presentation_time
) attribute_hidden;

// VdpPresentationQueueTargetCreateX11
VdpStatus
vdpau_presentation_queue_target_create_x11(
    vdpau_driver_data_p         driver_data,
    VdpDevice                   device,
    Drawable                    drawable,
    VdpPresentationQueueTarget *target
) attribute_hidden;

// VdpPresentationQueueTargetDestroy
VdpStatus
vdpau_presentation_queue_target_destroy(
    vdpau_driver_data_p        driver_data,
    VdpPresentationQueueTarget presentation_queue_target
) attribute_hidden;

// VdpDecoderCreate
VdpStatus
vdpau_decoder_create(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpDecoderProfile    profile,
    uint32_t             width,
    uint32_t             height,
    uint32_t             max_references,
    VdpDecoder          *decoder
) attribute_hidden;

// VdpDecoderDestroy
VdpStatus
vdpau_decoder_destroy(
    vdpau_driver_data_p  driver_data,
    VdpDecoder           decoder
) attribute_hidden;

// VdpDecoderRender
VdpStatus
vdpau_decoder_render(
    vdpau_driver_data_p       driver_data,
    VdpDecoder                decoder,
    VdpVideoSurface           target,
    VdpPictureInfo const     *picture_info,
    uint32_t                  bitstream_buffers_count,
    VdpBitstreamBuffer const *bitstream_buffers
) attribute_hidden;

// VdpDecoderQueryCapabilities
VdpStatus
vdpau_decoder_query_capabilities(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpDecoderProfile    profile,
    VdpBool             *is_supported,
    uint32_t            *max_level,
    uint32_t            *max_references,
    uint32_t            *max_width,
    uint32_t            *max_height
) attribute_hidden;

// VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities
VdpStatus
vdpau_video_surface_query_ycbcr_caps(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpChromaType        surface_chroma_type,
    VdpYCbCrFormat       bits_ycbcr_format,
    VdpBool             *is_supported
) attribute_hidden;

// VdpOutputSurfaceQueryGetPutBitsNativeCapabilities
VdpStatus
vdpau_output_surface_query_rgba_caps(
    vdpau_driver_data_p  driver_data,
    VdpDevice            device,
    VdpRGBAFormat        surface_rgba_format,
    VdpBool             *is_supported
) attribute_hidden;

#endif /* VDPAU_GATE_H */
