/*
 *  vdpau_video_glx.h - VDPAU backend for VA-API (GLX rendering)
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

#ifndef VDPAU_VIDEO_GLX_H
#define VDPAU_VIDEO_GLX_H

#include "vdpau_driver.h"
#include "vdpau_video_x11.h"
#include "utils_glx.h"

typedef struct object_glx_surface  object_glx_surface_t;
typedef struct object_glx_surface *object_glx_surface_p;

struct object_glx_surface {
    struct object_base   base;
    GLContextState      *gl_context;
    GLVdpSurface        *gl_surface;
    object_output_p      gl_output;
    GLenum               target;
    GLuint               texture;
    VASurfaceID          va_surface;
    unsigned int         width;
    unsigned int         height;
    GLPixmapObject      *pixo;
    GLFramebufferObject *fbo;
};

// vaCreateSurfaceGLX
VAStatus
vdpau_CreateSurfaceGLX(
    VADriverContextP    ctx,
    unsigned int        target,
    unsigned int        texture,
    void              **gl_surface
) attribute_hidden;

// vaDestroySurfaceGLX
VAStatus
vdpau_DestroySurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface
) attribute_hidden;

// vaAssociateSurfaceGLX
VAStatus
vdpau_AssociateSurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface,
    VASurfaceID         surface,
    unsigned int        flags
) attribute_hidden;

// vaDeassociateSurfaceGLX
VAStatus
vdpau_DeassociateSurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface
) attribute_hidden;

// vaSyncSurfaceGLX
VAStatus
vdpau_SyncSurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface
) attribute_hidden;

// vaBeginRenderSurfaceGLX
VAStatus
vdpau_BeginRenderSurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface
) attribute_hidden;

// vaEndRenderSurfaceGLX
VAStatus
vdpau_EndRenderSurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface
) attribute_hidden;

// vaCopySurfaceGLX
VAStatus
vdpau_CopySurfaceGLX(
    VADriverContextP    ctx,
    void               *gl_surface,
    VASurfaceID         surface,
    unsigned int        flags
) attribute_hidden;

#endif /* VDPAU_VIDEO_GLX_H */
