/*
 *  vdpau_mixer.c - VDPAU backend for VA-API (video mixer abstraction)
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
#include "vdpau_mixer.h"
#include "vdpau_video.h"
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
#include <math.h>

#define VDPAU_MAX_VIDEO_MIXER_PARAMS    4
#define VDPAU_MAX_VIDEO_MIXER_FEATURES  20

static inline int
video_mixer_check_params(
    object_mixer_p       obj_mixer,
    object_surface_p     obj_surface
)
{
    return (obj_mixer->width == obj_surface->width &&
            obj_mixer->height == obj_surface->height &&
            obj_mixer->vdp_chroma_type == obj_surface->vdp_chroma_type);
}

static inline void
video_mixer_init_deint_surfaces(object_mixer_p obj_mixer)
{
    unsigned int i;
    for (i = 0; i < VDPAU_MAX_VIDEO_MIXER_DEINT_SURFACES; i++)
        obj_mixer->deint_surfaces[i] = VDP_INVALID_HANDLE;
}

/** Checks wether video mixer supports a specific feature */
static inline VdpBool
video_mixer_has_feature(
    vdpau_driver_data_t *driver_data,
    VdpVideoMixerFeature feature
)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus status;

    status = vdpau_video_mixer_query_feature_support(
        driver_data,
        driver_data->vdp_device,
        feature,
        &is_supported
    );
    return (VDPAU_CHECK_STATUS(status, "VdpVideoMixerQueryFeatureSupport()") &&
            is_supported);
}

object_mixer_p
video_mixer_create(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface
)
{
    VAGenericID mixer_id = object_heap_allocate(&driver_data->mixer_heap);
    if (mixer_id == VA_INVALID_ID)
        return NULL;

    object_mixer_p obj_mixer = VDPAU_MIXER(mixer_id);
    if (!obj_mixer)
        return NULL;

    obj_mixer->refcount          = 1;
    obj_mixer->vdp_video_mixer   = VDP_INVALID_HANDLE;
    obj_mixer->width             = obj_surface->width;
    obj_mixer->height            = obj_surface->height;
    obj_mixer->vdp_chroma_type   = obj_surface->vdp_chroma_type;
    obj_mixer->vdp_colorspace    = VDP_COLOR_STANDARD_ITUR_BT_601;
    obj_mixer->vdp_procamp_mtime = 0;
    obj_mixer->vdp_bgcolor_mtime = 0;
    obj_mixer->hqscaling_level   = 0;
    obj_mixer->va_scale          = 0;

    VdpProcamp * const procamp   = &obj_mixer->vdp_procamp;
    procamp->struct_version      = VDP_PROCAMP_VERSION;
    procamp->brightness          = 0.0;
    procamp->contrast            = 1.0;
    procamp->saturation          = 1.0;
    procamp->hue                 = 0.0;

    VdpVideoMixerParameter params[VDPAU_MAX_VIDEO_MIXER_PARAMS];
    void *param_values[VDPAU_MAX_VIDEO_MIXER_PARAMS];
    unsigned int n_params = 0;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH;
    param_values[n_params++] = &obj_mixer->width;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT;
    param_values[n_params++] = &obj_mixer->height;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE;
    param_values[n_params++] = &obj_mixer->vdp_chroma_type;

    VdpVideoMixerFeature feature, features[VDPAU_MAX_VIDEO_MIXER_FEATURES];
    unsigned int i, n_features = 0;
    for (i = 1; i <= 9; i++) {
        feature = VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 + i - 1;
        if (video_mixer_has_feature(driver_data, feature)) {
            features[n_features++] = feature;
            obj_mixer->hqscaling_level = i;
        }
    }

    video_mixer_init_deint_surfaces(obj_mixer);

    VdpStatus vdp_status;
    vdp_status = vdpau_video_mixer_create(
        driver_data,
        driver_data->vdp_device,
        n_features, features,
        n_params, params, param_values,
        &obj_mixer->vdp_video_mixer
    );
    if (!VDPAU_CHECK_STATUS(vdp_status, "VdpVideoMixerCreate()")) {
        video_mixer_destroy(driver_data, obj_mixer);
        return NULL;
    }
    return obj_mixer;
}

object_mixer_p
video_mixer_create_cached(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface
)
{
    object_mixer_p obj_mixer = obj_surface->video_mixer;

    if (obj_mixer)
        return video_mixer_ref(driver_data, obj_mixer);

    object_heap_iterator iter;
    object_base_p obj = object_heap_first(&driver_data->mixer_heap, &iter);
    while (obj) {
        obj_mixer = (object_mixer_p)obj;
        if (video_mixer_check_params(obj_mixer, obj_surface))
            return video_mixer_ref(driver_data, obj_mixer);
        obj = object_heap_next(&driver_data->mixer_heap, &iter);
    }
    return video_mixer_create(driver_data, obj_surface);
}

void
video_mixer_destroy(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer
)
{
    if (!obj_mixer)
        return;

    if (obj_mixer->vdp_video_mixer != VDP_INVALID_HANDLE) {
        vdpau_video_mixer_destroy(driver_data, obj_mixer->vdp_video_mixer);
        obj_mixer->vdp_video_mixer = VDP_INVALID_HANDLE;
    }
    object_heap_free(&driver_data->mixer_heap, (object_base_p)obj_mixer);
}

object_mixer_p
video_mixer_ref(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer
)
{
    if (obj_mixer)
        ++obj_mixer->refcount;
    return obj_mixer;
}

void
video_mixer_unref(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer
)
{
    if (obj_mixer && --obj_mixer->refcount == 0)
        video_mixer_destroy(driver_data, obj_mixer);
}

static VdpStatus
video_mixer_update_csc_matrix(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer,
    VdpColorStandard     vdp_colorspace
)
{
    uint64_t new_mtime = obj_mixer->vdp_procamp_mtime;
    unsigned int i;

    for (i = 0; i < driver_data->va_display_attrs_count; i++) {
        VADisplayAttribute * const attr = &driver_data->va_display_attrs[i];
        if (obj_mixer->vdp_procamp_mtime >= driver_data->va_display_attrs_mtime[i])
            continue;

        float *vp, v = attr->value / 100.0;
        switch (attr->type) {
        case VADisplayAttribBrightness: /* VDPAU range: -1.0 to 1.0 */
            vp = &obj_mixer->vdp_procamp.brightness;
            break;
        case VADisplayAttribContrast:   /* VDPAU range: 0.0 to 10.0 */
            vp = &obj_mixer->vdp_procamp.contrast;
            goto do_range_0_10;
        case VADisplayAttribSaturation: /* VDPAU range: 0.0 to 10.0 */
            vp = &obj_mixer->vdp_procamp.saturation;
        do_range_0_10:
            if (attr->value > 0) /* scale VA:0-100 to VDPAU:1.0-10.0 */
                v *= 9.0;
            v += 1.0;
            break;
        case VADisplayAttribHue:        /* VDPAU range: -PI to PI */
            vp = &obj_mixer->vdp_procamp.hue;
            v *= M_PI;
            break;
        default:
            vp = NULL;
            break;
        }

        if (vp) {
            *vp = v;
            if (new_mtime < driver_data->va_display_attrs_mtime[i])
                new_mtime = driver_data->va_display_attrs_mtime[i];
        }
    }

    /* Commit changes, if any */
    if (new_mtime > obj_mixer->vdp_procamp_mtime || vdp_colorspace != obj_mixer->vdp_colorspace) {
        VdpCSCMatrix vdp_matrix;
        VdpStatus vdp_status;
        static const VdpVideoMixerAttribute attrs[1] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
        const void *attr_values[1] = { &vdp_matrix };

        vdp_status = vdpau_generate_csc_matrix(
            driver_data,
            &obj_mixer->vdp_procamp,
            vdp_colorspace,
            &vdp_matrix
        );
        if (!VDPAU_CHECK_STATUS(vdp_status, "VdpGenerateCSCMatrix()"))
            return vdp_status;

        vdp_status = vdpau_video_mixer_set_attribute_values(
            driver_data,
            obj_mixer->vdp_video_mixer,
            1, attrs, attr_values
        );
        if (!VDPAU_CHECK_STATUS(vdp_status, "VdpVideoMixerSetAttributeValues()"))
            return vdp_status;

        obj_mixer->vdp_colorspace    = vdp_colorspace;
        obj_mixer->vdp_procamp_mtime = new_mtime;
    }
    return VDP_STATUS_OK;
}

VdpStatus
video_mixer_set_background_color(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer,
    const VdpColor      *vdp_color
)
{
    VdpStatus vdp_status;
    VdpVideoMixerAttribute attrs[1];
    const void *attr_values[1];

    attrs[0] = VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR;
    attr_values[0] = vdp_color;

    vdp_status = vdpau_video_mixer_set_attribute_values(
        driver_data,
        obj_mixer->vdp_video_mixer,
        1, attrs, attr_values
    );
    return vdp_status;
}

static VdpStatus
video_mixer_update_background_color(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer
)
{
    unsigned int i;
    for (i = 0; i < driver_data->va_display_attrs_count; i++) {
        VADisplayAttribute * const attr = &driver_data->va_display_attrs[i];
        if (attr->type != VADisplayAttribBackgroundColor)
            continue;

        if (obj_mixer->vdp_bgcolor_mtime < driver_data->va_display_attrs_mtime[i]) {
            VdpStatus vdp_status;
            VdpColor vdp_color;

            vdp_color.red   = ((attr->value >> 16) & 0xff) / 255.0f;
            vdp_color.green = ((attr->value >> 8) & 0xff) / 255.0f;
            vdp_color.blue  = (attr->value & 0xff)/ 255.0f;
            vdp_color.alpha = 1.0f;

            vdp_status = video_mixer_set_background_color(
                driver_data,
                obj_mixer,
                &vdp_color
            );
            if (vdp_status != VDP_STATUS_OK)
                return vdp_status;

            obj_mixer->vdp_bgcolor_mtime = driver_data->va_display_attrs_mtime[i];
            break;
        }
    }
    return VDP_STATUS_OK;
}

static VdpStatus
video_mixer_update_scaling(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer,
    unsigned int         va_scale
)
{
    if (obj_mixer->va_scale == va_scale)
        return VDP_STATUS_OK;

    /* Only HQ scaling is supported, other flags disable HQ scaling */
    if (obj_mixer->hqscaling_level >= 1) {
        VdpVideoMixerFeature features[1];
        VdpBool feature_enables[1];
        features[0] = VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1;
        feature_enables[0] = (va_scale == VA_FILTER_SCALING_HQ ?
                              VDP_TRUE : VDP_FALSE);

        VdpStatus vdp_status;
        vdp_status = vdpau_video_mixer_set_feature_enables(
            driver_data,
            obj_mixer->vdp_video_mixer,
            1,
            features,
            feature_enables
        );
        if (!VDPAU_CHECK_STATUS(vdp_status, "VdpVideoMixerSetFeatureEnables()"))
            return vdp_status;
    }
    obj_mixer->va_scale = va_scale;
    return VDP_STATUS_OK;
}

static inline void
video_mixer_push_deint_surface(
    object_mixer_p   obj_mixer,
    object_surface_p obj_surface
)
{
    unsigned int i;
    for (i = VDPAU_MAX_VIDEO_MIXER_DEINT_SURFACES - 1; i >= 1; i--)
        obj_mixer->deint_surfaces[i] = obj_mixer->deint_surfaces[i - 1];
    obj_mixer->deint_surfaces[0] = obj_surface->vdp_surface;
}

VdpStatus
video_mixer_render(
    vdpau_driver_data_t *driver_data,
    object_mixer_p       obj_mixer,
    object_surface_p     obj_surface,
    VdpOutputSurface     vdp_background,
    VdpOutputSurface     vdp_output_surface,
    const VdpRect       *vdp_src_rect,
    const VdpRect       *vdp_dst_rect,
    unsigned int         flags
)
{
    VdpColorStandard vdp_colorspace;
    if (flags & VA_SRC_SMPTE_240)
        vdp_colorspace = VDP_COLOR_STANDARD_SMPTE_240M;
    else if (flags & VA_SRC_BT709)
        vdp_colorspace = VDP_COLOR_STANDARD_ITUR_BT_709;
    else
        vdp_colorspace = VDP_COLOR_STANDARD_ITUR_BT_601;

    VdpStatus vdp_status;
    vdp_status = video_mixer_update_csc_matrix(
        driver_data,
        obj_mixer,
        vdp_colorspace
    );
    if (vdp_status != VDP_STATUS_OK)
        return vdp_status;

    vdp_status = video_mixer_update_background_color(driver_data, obj_mixer);
    if (vdp_status != VDP_STATUS_OK)
        return vdp_status;

    const unsigned int va_scale = flags & VA_FILTER_SCALING_MASK;
    vdp_status = video_mixer_update_scaling(driver_data, obj_mixer, va_scale);
    if (vdp_status != VDP_STATUS_OK)
        return vdp_status;

    VdpVideoMixerPictureStructure field;
    switch (flags & (VA_TOP_FIELD|VA_BOTTOM_FIELD)) {
    case VA_TOP_FIELD:
        field = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
        break;
    case VA_BOTTOM_FIELD:
        field = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
        break;
    default:
        field = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
        break;
    }
    video_mixer_push_deint_surface(obj_mixer, obj_surface);

    if (flags & VA_CLEAR_DRAWABLE)
        vdp_background = VDP_INVALID_HANDLE;

    vdp_status = vdpau_video_mixer_render(
        driver_data,
        obj_mixer->vdp_video_mixer,
        vdp_background, NULL,
        field,
        VDPAU_MAX_VIDEO_MIXER_DEINT_SURFACES - 1, &obj_mixer->deint_surfaces[1],
        obj_mixer->deint_surfaces[0],
        0, NULL,
        vdp_src_rect,
        vdp_output_surface,
        NULL,
        vdp_dst_rect,
        0, NULL
    );
    video_mixer_push_deint_surface(obj_mixer, obj_surface);
    return vdp_status;
}
