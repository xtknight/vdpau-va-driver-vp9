/*
 *  vdpau_video_x11.h - VDPAU backend for VA-API (X11 rendering)
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

#ifndef VDPAU_VIDEO_X11_H
#define VDPAU_VIDEO_X11_H

#include "vdpau_driver.h"
#include <pthread.h>
#include "uasyncqueue.h"

typedef struct object_output object_output_t;
struct object_output {
    struct object_base          base;
    unsigned int                refcount;
    Drawable                    drawable;
    unsigned int                width;
    unsigned int                height;
    unsigned int                max_width;
    unsigned int                max_height;
    VdpPresentationQueue        vdp_flip_queue;
    VdpPresentationQueueTarget  vdp_flip_target;
    VdpOutputSurface            vdp_output_surfaces[VDPAU_MAX_OUTPUT_SURFACES];
    unsigned int                vdp_output_surfaces_dirty[VDPAU_MAX_OUTPUT_SURFACES];
    pthread_mutex_t             vdp_output_surfaces_lock;
    unsigned int                current_output_surface;
    unsigned int                displayed_output_surface;
    unsigned int                queued_surfaces;
    unsigned int                fields;
    unsigned int                is_window    : 1; /* drawable is a window */
    unsigned int                size_changed : 1; /* size changed since previous vaPutSurface() and user noticed the change */
};

// Create output surface
object_output_p
output_surface_create(
    vdpau_driver_data_t *driver_data,
    Drawable             drawable,
    unsigned int         width,
    unsigned int         height
) attribute_hidden;

// Destroy output surface
void
output_surface_destroy(
    vdpau_driver_data_t *driver_data,
    object_output_p      obj_output
) attribute_hidden;

// Reference output surface
object_output_p
output_surface_ref(
    vdpau_driver_data_t *driver_data,
    object_output_p      obj_output
) attribute_hidden;

// Unreference output surface
// NOTE: this destroys the surface if refcount reaches zero
void
output_surface_unref(
    vdpau_driver_data_t *driver_data,
    object_output_p      obj_output
) attribute_hidden;

// Looks up output surface
object_output_p
output_surface_lookup(object_surface_p obj_surface, Drawable drawable)
    attribute_hidden;

// Ensure output surface size matches drawable size
int
output_surface_ensure_size(
    vdpau_driver_data_t *driver_data,
    object_output_p      obj_output,
    unsigned int         width,
    unsigned int         height
) attribute_hidden;

// Render surface to the VDPAU output surface
VAStatus
render_surface(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface,
    object_output_p      obj_output,
    const VARectangle   *source_rect,
    const VARectangle   *target_rect,
    unsigned int         flags
) attribute_hidden;

// Render subpictures to the VDPAU output surface
VAStatus
render_subpictures(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface,
    object_output_p      obj_output,
    const VARectangle   *source_rect,
    const VARectangle   *target_rect
) attribute_hidden;

// Render surface to a Drawable
VAStatus
put_surface(
    vdpau_driver_data_t *driver_data,
    VASurfaceID          surface,
    Drawable             drawable,
    unsigned int         drawable_width,
    unsigned int         drawable_height,
    const VARectangle   *source_rect,
    const VARectangle   *target_rect,
    unsigned int         flags
) attribute_hidden;

// Queue surface for display
VAStatus
queue_surface(
    vdpau_driver_data_t *driver_data,
    object_surface_p     obj_surface,
    object_output_p      obj_output
) attribute_hidden;

// vaPutSurface
VAStatus
vdpau_PutSurface(
    VADriverContextP    ctx,
    VASurfaceID         surface,
    VADrawable          draw,
    short               srcx,
    short               srcy,
    unsigned short      srcw,
    unsigned short      srch,
    short               destx,
    short               desty,
    unsigned short      destw,
    unsigned short      desth,
    VARectangle        *cliprects,
    unsigned int        number_cliprects,
    unsigned int        flags
) attribute_hidden;

#endif /* VDPAU_VIDEO_X11_H */
