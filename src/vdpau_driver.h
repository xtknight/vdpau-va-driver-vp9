/*
 *  vdpau_driver.h - VDPAU driver
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

#ifndef VDPAU_DRIVER_H
#define VDPAU_DRIVER_H

#include <stdbool.h>
#include <va/va_backend.h>
#include "vaapi_compat.h"
#include "vdpau_gate.h"
#include "object_heap.h"


#define VDPAU_DRIVER_DATA_INIT                           \
        struct vdpau_driver_data *driver_data =          \
            (struct vdpau_driver_data *)ctx->pDriverData

#define VDPAU_OBJECT(id, type) \
    ((object_##type##_p)object_heap_lookup(&driver_data->type##_heap, (id)))

#define VDPAU_CONFIG(id)                VDPAU_OBJECT(id, config)
#define VDPAU_CONTEXT(id)               VDPAU_OBJECT(id, context)
#define VDPAU_SURFACE(id)               VDPAU_OBJECT(id, surface)
#define VDPAU_BUFFER(id)                VDPAU_OBJECT(id, buffer)
#define VDPAU_OUTPUT(id)                VDPAU_OBJECT(id, output)
#define VDPAU_IMAGE(id)                 VDPAU_OBJECT(id, image)
#define VDPAU_GLX_SURFACE(id)           VDPAU_OBJECT(id, glx_surface)
#define VDPAU_SUBPICTURE(id)            VDPAU_OBJECT(id, subpicture)
#define VDPAU_MIXER(id)                 VDPAU_OBJECT(id, mixer)

#define VDPAU_CONFIG_ID_OFFSET          0x01000000
#define VDPAU_CONTEXT_ID_OFFSET         0x02000000
#define VDPAU_SURFACE_ID_OFFSET         0x03000000
#define VDPAU_BUFFER_ID_OFFSET          0x04000000
#define VDPAU_OUTPUT_ID_OFFSET          0x05000000
#define VDPAU_IMAGE_ID_OFFSET           0x06000000
#define VDPAU_SUBPICTURE_ID_OFFSET      0x07000000
#define VDPAU_GLX_SURFACE_ID_OFFSET     0x08000000
#define VDPAU_MIXER_ID_OFFSET           0x09000000

#define VDPAU_MAX_PROFILES              12
#define VDPAU_MAX_ENTRYPOINTS           5
#define VDPAU_MAX_CONFIG_ATTRIBUTES     10
#define VDPAU_MAX_IMAGE_FORMATS         10
#define VDPAU_MAX_SUBPICTURES           8
#define VDPAU_MAX_SUBPICTURE_FORMATS    6
#define VDPAU_MAX_DISPLAY_ATTRIBUTES    6
#define VDPAU_MAX_OUTPUT_SURFACES       2
#define VDPAU_STR_DRIVER_VENDOR         "Splitted-Desktop Systems"
#define VDPAU_STR_DRIVER_NAME           "VDPAU backend for VA-API"

/* Check we have MPEG-4 support in VDPAU and the necessary VAAPI extensions */
#define USE_VDPAU_MPEG4                                         \
    (HAVE_VDPAU_MPEG4 &&                                        \
     (VA_CHECK_VERSION(0,31,1) ||                               \
      (VA_CHECK_VERSION(0,31,0) && VA_SDS_VERSION >= 4)))

typedef enum {
    VDP_IMPLEMENTATION_NVIDIA = 1,
} VdpImplementation;

typedef struct vdpau_driver_data vdpau_driver_data_t;
struct vdpau_driver_data {
    VADriverContextP            va_context;
    unsigned int                va_display_type;
    struct object_heap          config_heap;
    struct object_heap          context_heap;
    struct object_heap          surface_heap;
    struct object_heap          glx_surface_heap;
    struct object_heap          buffer_heap;
    struct object_heap          output_heap;
    struct object_heap          image_heap;
    struct object_heap          subpicture_heap;
    struct object_heap          mixer_heap;
    Display                    *x11_dpy;
    int                         x11_screen;
    Display                    *vdp_dpy;
    VdpDevice                   vdp_device;
    VdpGetProcAddress          *vdp_get_proc_address;
    vdpau_vtable_t              vdp_vtable;
    VdpImplementation           vdp_impl_type;
    uint32_t                    vdp_impl_version;
    VADisplayAttribute          va_display_attrs[VDPAU_MAX_DISPLAY_ATTRIBUTES];
    uint64_t                    va_display_attrs_mtime[VDPAU_MAX_DISPLAY_ATTRIBUTES];
    unsigned int                va_display_attrs_count;
    char                        va_vendor[256];
    bool			x_fallback;
};

typedef struct object_config   *object_config_p;
typedef struct object_context  *object_context_p;
typedef struct object_surface  *object_surface_p;
typedef struct object_buffer   *object_buffer_p;
typedef struct object_output   *object_output_p;
typedef struct object_image    *object_image_p;
typedef struct object_mixer    *object_mixer_p;

// Set display type
int vdpau_set_display_type(vdpau_driver_data_t *driver_data, unsigned int type)
    attribute_hidden;

// Return TRUE if underlying VDPAU implementation is NVIDIA
VdpBool
vdpau_is_nvidia(vdpau_driver_data_t *driver_data, int *major, int *minor)
    attribute_hidden;

// Translate VdpStatus to an appropriate VAStatus
VAStatus
vdpau_get_VAStatus(VdpStatus vdp_status)
    attribute_hidden;

#endif /* VDPAU_DRIVER_H */

