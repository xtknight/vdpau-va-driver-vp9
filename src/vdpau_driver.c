/*
 *  vdpau_driver.c - VDPAU driver
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
#include <ctype.h>
#include "vdpau_driver.h"
#include "vdpau_buffer.h"
#include "vdpau_decode.h"
#include "vdpau_image.h"
#include "vdpau_subpic.h"
#include "vdpau_mixer.h"
#include "vdpau_video.h"
#include "vdpau_video_x11.h"
#if USE_GLX
#include "vdpau_video_glx.h"
#include <va/va_backend_glx.h>
#endif

#define DEBUG 1
#include "debug.h"


// Set display type
int vdpau_set_display_type(vdpau_driver_data_t *driver_data, unsigned int type)
{
    if (driver_data->va_display_type == 0) {
        driver_data->va_display_type = type;
        return 1;
    }
    return driver_data->va_display_type == type;
}

// Return TRUE if underlying VDPAU implementation is NVIDIA
VdpBool
vdpau_is_nvidia(vdpau_driver_data_t *driver_data, int *major, int *minor)
{
    uint32_t nvidia_version = 0;

    if (driver_data->vdp_impl_type == VDP_IMPLEMENTATION_NVIDIA)
        nvidia_version = driver_data->vdp_impl_version;
    if (major)
        *major = nvidia_version >> 16;
    if (minor)
        *minor = nvidia_version & 0xffff;
    return nvidia_version != 0;
}

// Translate VdpStatus to an appropriate VAStatus
VAStatus
vdpau_get_VAStatus(VdpStatus vdp_status)
{
    VAStatus va_status;

    switch (vdp_status) {
    case VDP_STATUS_OK:
        va_status = VA_STATUS_SUCCESS;
        break;
    case VDP_STATUS_NO_IMPLEMENTATION:
        va_status = VA_STATUS_ERROR_UNIMPLEMENTED;
        break;
    case VDP_STATUS_INVALID_CHROMA_TYPE:
        va_status = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        break;
    case VDP_STATUS_INVALID_DECODER_PROFILE:
        va_status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    case VDP_STATUS_RESOURCES:
        va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
        break;
    default:
        va_status = VA_STATUS_ERROR_UNKNOWN;
        break;
    }
    return va_status;
}

// Destroy BUFFER objects
static void destroy_buffer_cb(object_base_p obj, void *user_data)
{
    object_buffer_p const obj_buffer = (object_buffer_p)obj;
    vdpau_driver_data_t * const driver_data = user_data;

    destroy_va_buffer(driver_data, obj_buffer);
}

// Destroy MIXER objects
static void destroy_mixer_cb(object_base_p obj, void *user_data)
{
    object_mixer_p const obj_mixer = (object_mixer_p)obj;
    vdpau_driver_data_t * const driver_data = user_data;

    video_mixer_destroy(driver_data, obj_mixer);
}

// Destroy object heap
typedef void (*destroy_heap_func_t)(object_base_p obj, void *user_data);

static void
destroy_heap(
    const char         *name,
    object_heap_p       heap,
    destroy_heap_func_t destroy_func,
    void               *user_data
)
{
    object_base_p obj;
    object_heap_iterator iter;

    if (!heap)
        return;

    obj = object_heap_first(heap, &iter);
    while (obj) {
        vdpau_information_message("vaTerminate(): %s ID 0x%08x is still allocated, destroying\n", name, obj->id);
        if (destroy_func)
            destroy_func(obj, user_data);
        else
            object_heap_free(heap, obj);
        obj = object_heap_next(heap, &iter);
    }
    object_heap_destroy(heap);
}

#define DESTROY_HEAP(heap, func) \
        destroy_heap(#heap, &driver_data->heap##_heap, func, driver_data)

#define CREATE_HEAP(type, id) do {              \
        int result;                             \
        result = object_heap_init(              \
            &driver_data->type##_heap,          \
            sizeof(struct object_##type),       \
            VDPAU_##id##_ID_OFFSET              \
        );                                      \
        if (result != 0)                        \
            return VA_STATUS_ERROR_UNKNOWN;     \
    } while (0)

// vaTerminate
static void
vdpau_common_Terminate(vdpau_driver_data_t *driver_data)
{
    DESTROY_HEAP(buffer,      destroy_buffer_cb);
    DESTROY_HEAP(image,       NULL);
    DESTROY_HEAP(subpicture,  NULL);
    DESTROY_HEAP(output,      NULL);
    DESTROY_HEAP(surface,     NULL);
    DESTROY_HEAP(context,     NULL);
    DESTROY_HEAP(config,      NULL);
    DESTROY_HEAP(mixer,       destroy_mixer_cb);
#if USE_GLX
    DESTROY_HEAP(glx_surface, NULL);
#endif

    if (driver_data->vdp_device != VDP_INVALID_HANDLE) {
        vdpau_device_destroy(driver_data, driver_data->vdp_device);
        driver_data->vdp_device = VDP_INVALID_HANDLE;
    }
    vdpau_gate_exit(driver_data);

    if (!driver_data->x_fallback && driver_data->vdp_dpy) {
        XCloseDisplay(driver_data->vdp_dpy);
        driver_data->vdp_dpy = NULL;
    }
}

// vaInitialize
static VAStatus
vdpau_common_Initialize(vdpau_driver_data_t *driver_data)
{
    /* Create a dedicated X11 display for VDPAU purposes */
    const char * const x11_dpy_name = XDisplayString(driver_data->x11_dpy);
    driver_data->vdp_dpy = XOpenDisplay(x11_dpy_name);
    /* Fallback to existing X11 display */
    driver_data->x_fallback = false;
    if (!driver_data->vdp_dpy) {
        driver_data->x_fallback = true;
        driver_data->vdp_dpy = driver_data->x11_dpy;
        printf("Failed to create dedicated X11 display!\n");
    }
        
    VdpStatus vdp_status;
    driver_data->vdp_device = VDP_INVALID_HANDLE;
    vdp_status = vdp_device_create_x11(
        driver_data->vdp_dpy,
        driver_data->x11_screen,
        &driver_data->vdp_device,
        &driver_data->vdp_get_proc_address
    );
    if (vdp_status != VDP_STATUS_OK)
        return VA_STATUS_ERROR_UNKNOWN;

    if (vdpau_gate_init(driver_data) < 0)
        return VA_STATUS_ERROR_UNKNOWN;

    uint32_t api_version;
    vdp_status = vdpau_get_api_version(driver_data, &api_version);
    if (vdp_status != VDP_STATUS_OK)
        return vdpau_get_VAStatus(vdp_status);
    if (api_version != VDPAU_VERSION)
        return VA_STATUS_ERROR_UNKNOWN;

    const char *impl_string = NULL;
    vdp_status = vdpau_get_information_string(driver_data, &impl_string);
    if (vdp_status != VDP_STATUS_OK)
        return vdpau_get_VAStatus(vdp_status);
    if (impl_string) {
        const char *str, *version_string = NULL;
        D(bug("%s\n", impl_string));

        if (strncmp(impl_string, "NVIDIA", 6) == 0) {
            /* NVIDIA VDPAU Driver Shared Library  <version>  <date> */
            driver_data->vdp_impl_type = VDP_IMPLEMENTATION_NVIDIA;
            for (str = impl_string; *str; str++) {
                if (isdigit(*str)) {
                    version_string = str;
                    break;
                }
            }
        }

        if (version_string) {
            int major, minor;
            if (sscanf(version_string, "%d.%d", &major, &minor) == 2)
                driver_data->vdp_impl_version = (major << 16) | minor;
        }
    }

    sprintf(driver_data->va_vendor, "%s %s - %d.%d.%d",
            VDPAU_STR_DRIVER_VENDOR,
            VDPAU_STR_DRIVER_NAME,
            VDPAU_VIDEO_MAJOR_VERSION,
            VDPAU_VIDEO_MINOR_VERSION,
            VDPAU_VIDEO_MICRO_VERSION);

    if (VDPAU_VIDEO_PRE_VERSION > 0) {
        const int len = strlen(driver_data->va_vendor);
        sprintf(&driver_data->va_vendor[len], ".pre%d", VDPAU_VIDEO_PRE_VERSION);
    }

    CREATE_HEAP(config,         CONFIG);
    CREATE_HEAP(context,        CONTEXT);
    CREATE_HEAP(surface,        SURFACE);
    CREATE_HEAP(buffer,         BUFFER);
    CREATE_HEAP(output,         OUTPUT);
    CREATE_HEAP(image,          IMAGE);
    CREATE_HEAP(subpicture,     SUBPICTURE);
    CREATE_HEAP(mixer,          MIXER);
#if USE_GLX
    CREATE_HEAP(glx_surface,    GLX_SURFACE);
#endif
    return VA_STATUS_SUCCESS;
}

#if VA_MAJOR_VERSION == 0 && VA_MINOR_VERSION >= 31
#define VA_INIT_VERSION_MAJOR   0
#define VA_INIT_VERSION_MINOR   31
#define VA_INIT_VERSION_MICRO   0
#define VA_INIT_SUFFIX          0_31_0
#include "vdpau_driver_template.h"

#define VA_INIT_VERSION_MAJOR   0
#define VA_INIT_VERSION_MINOR   31
#define VA_INIT_VERSION_MICRO   1
#define VA_INIT_SUFFIX          0_31_1
#define VA_INIT_GLX             USE_GLX
#include "vdpau_driver_template.h"

#define VA_INIT_VERSION_MAJOR   0
#define VA_INIT_VERSION_MINOR   31
#define VA_INIT_VERSION_MICRO   2
#define VA_INIT_SUFFIX          0_31_2
#define VA_INIT_GLX             USE_GLX
#include "vdpau_driver_template.h"

VAStatus __vaDriverInit_0_31(void *ctx)
{
    VADriverContextP_0_31_0 const ctx0 = ctx;
    VADriverContextP_0_31_1 const ctx1 = ctx;
    VADriverContextP_0_31_2 const ctx2 = ctx;

    /* Assume a NULL display implies VA-API 0.31.1 struct with the
       vtable_tpi field placed just after the vtable, thus replacing
       original native_dpy field */
    if (ctx0->native_dpy)
        return vdpau_Initialize_0_31_0(ctx);
    if (ctx1->native_dpy)
        return vdpau_Initialize_0_31_1(ctx);
    if (ctx2->native_dpy)
        return vdpau_Initialize_0_31_2(ctx);
    return VA_STATUS_ERROR_INVALID_DISPLAY;
}
#endif

#define VA_INIT_VERSION_MAJOR   VA_MAJOR_VERSION
#define VA_INIT_VERSION_MINOR   VA_MINOR_VERSION
#define VA_INIT_VERSION_MICRO   VA_MICRO_VERSION
#define VA_INIT_GLX             USE_GLX
#include "vdpau_driver_template.h"

VAStatus VA_DRIVER_INIT_FUNC(void *ctx)
{
#if VA_MAJOR_VERSION == 0 && VA_MINOR_VERSION == 31
    return __vaDriverInit_0_31(ctx);
#endif
    return vdpau_Initialize_Current(ctx);
}
