/*
 *  vdpau_video_glx.c - VDPAU backend for VA-API (GLX rendering)
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

#define _GNU_SOURCE 1 /* RTLD_NEXT */
#include "sysdeps.h"
#include "vdpau_mixer.h"
#include "vdpau_video.h"
#include "vdpau_video_glx.h"
#include "vdpau_video_x11.h"
#include "utils.h"
#include "utils_glx.h"
#include <dlfcn.h>
#include <GL/glext.h>
#include <GL/glxext.h>

#define DEBUG 1
#include "debug.h"


/* Use VDPAU/GL interop:
 * 1: VdpVideoSurface (TODO)
 * 2: VdpOutputSurface
 */
#define VDPAU_GL_INTEROP 2

static int get_vdpau_gl_interop_env(void)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    if (!gl_vtable || !gl_vtable->has_vdpau_interop)
        return 0;

    int vdpau_gl_interop;
    if (getenv_int("VDPAU_VIDEO_GL_INTEROP", &vdpau_gl_interop) < 0)
        vdpau_gl_interop = VDPAU_GL_INTEROP;
    if (vdpau_gl_interop < 0)
        vdpau_gl_interop = 0;
    else if (vdpau_gl_interop > 2)
        vdpau_gl_interop = 2;
    return vdpau_gl_interop;
}

static inline int vdpau_gl_interop(void)
{
    static int g_vdpau_gl_interop = -1;
    if (g_vdpau_gl_interop < 0)
        g_vdpau_gl_interop = get_vdpau_gl_interop_env();
    return g_vdpau_gl_interop;
}

// Ensure GLX TFP and FBO extensions are available
static inline int ensure_extensions(void)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    /* XXX: GLX_EXT_texture_from_pixmap is checked later */
    return (gl_vtable &&
            gl_vtable->has_framebuffer_object);
}

// Render GLX Pixmap to texture
static void
render_pixmap(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
)
{
    float tw, th;
    const GLenum target  = obj_glx_surface->target;
    const unsigned int w = obj_glx_surface->width;
    const unsigned int h = obj_glx_surface->height;

    if (vdpau_gl_interop()) {
        GLVdpSurface *  const gl_surface = obj_glx_surface->gl_surface;
        glBindTexture(gl_surface->target, gl_surface->textures[0]);

        object_output_p const obj_output = obj_glx_surface->gl_output;
        switch (target) {
        case GL_TEXTURE_2D:
            tw = (float)obj_output->width / obj_output->max_width;
            th = (float)obj_output->height / obj_output->max_height;
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            tw = (float)obj_output->width;
            th = (float)obj_output->height;
            break;
        default:
            tw = 0.0f;
            th = 0.0f;
            ASSERT(target == GL_TEXTURE_2D ||
                   target == GL_TEXTURE_RECTANGLE_ARB);
            break;
        }
    }
    else {
        switch (target) {
        case GL_TEXTURE_2D:
            tw = 1.0f;
            th = 1.0f;
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            tw = (float)w;
            th = (float)h;
            break;
        default:
            tw = 0.0f;
            th = 0.0f;
            ASSERT(target == GL_TEXTURE_2D ||
                   target == GL_TEXTURE_RECTANGLE_ARB);
            break;
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, th  ); glVertex2i(0, h);
        glTexCoord2f(tw  , th  ); glVertex2i(w, h);
        glTexCoord2f(tw  , 0.0f); glVertex2i(w, 0);
    }
    glEnd();
}

// Destroy VA/GLX surface
static void
destroy_surface(vdpau_driver_data_t *driver_data, VASurfaceID surface)
{
    object_glx_surface_p obj_glx_surface = VDPAU_GLX_SURFACE(surface);

    if (obj_glx_surface->gl_surface) {
        gl_vdpau_destroy_surface(obj_glx_surface->gl_surface);
        obj_glx_surface->gl_surface = NULL;
    }

    if (obj_glx_surface->gl_output) {
        output_surface_destroy(driver_data, obj_glx_surface->gl_output);
        obj_glx_surface->gl_output = NULL;
    }

    if (vdpau_gl_interop())
        gl_vdpau_exit();

    if (obj_glx_surface->fbo) {
        gl_destroy_framebuffer_object(obj_glx_surface->fbo);
        obj_glx_surface->fbo = NULL;
    }

    if (obj_glx_surface->pixo) {
        gl_destroy_pixmap_object(obj_glx_surface->pixo);
        obj_glx_surface->pixo = NULL;
    }

    object_heap_free(&driver_data->glx_surface_heap,
                     (object_base_p)obj_glx_surface);
}

// Check internal texture format is supported
static int
is_supported_internal_format(GLenum format)
{
    /* XXX: we don't support other textures than RGBA */
    switch (format) {
    case 4:
    case GL_RGBA:
    case GL_RGBA8:
        return 1;
    }
    return 0;
}

// Create VA/GLX surface
static VASurfaceID
create_surface(vdpau_driver_data_t *driver_data, GLenum target, GLuint texture)
{
    VASurfaceID surface = VA_INVALID_SURFACE;
    object_glx_surface_p obj_glx_surface;
    unsigned int internal_format, border_width, width, height;
    int is_error = 1;

    glBindTexture(target, texture);

    surface = object_heap_allocate(&driver_data->glx_surface_heap);
    if (surface == VA_INVALID_SURFACE)
        goto end;

    obj_glx_surface = VDPAU_GLX_SURFACE(surface);
    if (!obj_glx_surface)
        goto end;

    obj_glx_surface->gl_context = NULL;
    obj_glx_surface->gl_surface = NULL;
    obj_glx_surface->gl_output  = NULL;
    obj_glx_surface->target     = target;
    obj_glx_surface->texture    = texture;
    obj_glx_surface->va_surface = VA_INVALID_SURFACE;
    obj_glx_surface->pixo       = NULL;
    obj_glx_surface->fbo        = NULL;

    if (!gl_get_texture_param(target, GL_TEXTURE_INTERNAL_FORMAT, &internal_format))
        goto end;
    if (!is_supported_internal_format(internal_format))
        goto end;

    /* Check texture dimensions */
    if (!gl_get_texture_param(target, GL_TEXTURE_BORDER, &border_width))
        goto end;
    if (!gl_get_texture_param(target, GL_TEXTURE_WIDTH, &width))
        goto end;
    if (!gl_get_texture_param(target, GL_TEXTURE_HEIGHT, &height))
        goto end;

    width  -= 2 * border_width;
    height -= 2 * border_width;
    if (width == 0 || height == 0)
        goto end;

    obj_glx_surface->width  = width;
    obj_glx_surface->height = height;

    /* Initialize VDPAU/GL layer */
    if (vdpau_gl_interop()) {
        if (!gl_vdpau_init(driver_data->vdp_device,
                           driver_data->vdp_get_proc_address))
            goto end;
    }

    /* Create Pixmaps for TFP */
    else {
        obj_glx_surface->pixo = gl_create_pixmap_object(
            driver_data->x11_dpy,
            target,
            width, height
        );
        if (!obj_glx_surface->pixo)
            goto end;
    }
    is_error = 0;
end:
    glBindTexture(target, 0);
    if (is_error && surface != VA_INVALID_SURFACE) {
        destroy_surface(driver_data, surface);
        surface = VA_INVALID_SURFACE;
    }
    return surface;
}

// vaCreateSurfaceGLX
VAStatus
vdpau_CreateSurfaceGLX(
    VADriverContextP    ctx,
    unsigned int        target,
    unsigned int        texture,
    void              **gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    if (!gl_surface)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    /* Make sure it is a valid GL texture object */
    if (!glIsTexture(texture))
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    GLContextState old_cs, *new_cs;
    gl_get_current_context(&old_cs);
    new_cs = gl_create_context(driver_data->x11_dpy, driver_data->x11_screen, &old_cs);
    if (!new_cs)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    if (!gl_set_current_context(new_cs, NULL))
        return VA_STATUS_ERROR_OPERATION_FAILED;
    gl_init_context(new_cs);

    VASurfaceID surface = create_surface(driver_data, target, texture);
    if (surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    object_glx_surface_p obj_glx_surface = VDPAU_GLX_SURFACE(surface);
    *gl_surface = obj_glx_surface;
    obj_glx_surface->gl_context = new_cs;

    gl_set_current_context(&old_cs, NULL);
    return VA_STATUS_SUCCESS;
}

// vaDestroySurfaceGLX
VAStatus
vdpau_DestroySurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs, *new_cs = obj_glx_surface->gl_context;
    if (!gl_set_current_context(new_cs, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    destroy_surface(driver_data, obj_glx_surface->base.id);

    gl_destroy_context(new_cs);
    gl_set_current_context(&old_cs, NULL);
    return VA_STATUS_SUCCESS;
}

// Forward declarations
static VAStatus
deassociate_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
);

// vaAssociateSurfaceGLX
static VAStatus
associate_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface,
    object_surface_p     obj_surface,
    unsigned int         flags
)
{
    /* XXX: optimise case where we are associating the same VA surface
       as before an no changed occurred to it */
    VAStatus va_status;
    va_status = deassociate_glx_surface(driver_data, obj_glx_surface);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    VARectangle src_rect, dst_rect;
    src_rect.x      = 0;
    src_rect.y      = 0;
    src_rect.width  = obj_surface->width;
    src_rect.height = obj_surface->height;

    /* Render to VDPAU output surface */
    if (vdpau_gl_interop()) {
        if (!obj_glx_surface->gl_output) {
            obj_glx_surface->gl_output = output_surface_create(
                driver_data,
                None,
                obj_surface->width,
                obj_surface->height
            );
            if (!obj_glx_surface->gl_output)
                return VA_STATUS_ERROR_ALLOCATION_FAILED;

            /* XXX: use multiple output surfaces? */
            int status;
            status = output_surface_ensure_size(
                driver_data,
                obj_glx_surface->gl_output,
                obj_surface->width,
                obj_surface->height
            );
            if (status < 0)
                return VA_STATUS_ERROR_ALLOCATION_FAILED;

            obj_glx_surface->gl_surface = gl_vdpau_create_output_surface(
                obj_glx_surface->target,
                obj_glx_surface->gl_output->vdp_output_surfaces[0]
            );
            if (!obj_glx_surface->gl_surface)
                return VA_STATUS_ERROR_ALLOCATION_FAILED;

            /* Make sure background color is black with alpha set to 0xff */
            VdpStatus vdp_status;
            static const VdpColor bgcolor = { 0.0f, 0.0f, 0.0f, 1.0f };
            vdp_status = video_mixer_set_background_color(
                driver_data,
                obj_surface->video_mixer,
                &bgcolor
            );
            if (vdp_status != VDP_STATUS_OK)
                return vdpau_get_VAStatus(vdp_status);
        }

        dst_rect.x      = 0;
        dst_rect.y      = 0;
        dst_rect.width  = obj_surface->width;
        dst_rect.height = obj_surface->height;

        /* Render the video surface to the output surface */
        va_status = render_surface(
            driver_data,
            obj_surface,
            obj_glx_surface->gl_output,
            &src_rect,
            &dst_rect,
            flags | VA_CLEAR_DRAWABLE
        );
        if (va_status != VA_STATUS_SUCCESS)
            return va_status;

        /* Render subpictures to the output surface, applying scaling */
        va_status = render_subpictures(
            driver_data,
            obj_surface,
            obj_glx_surface->gl_output,
            &src_rect,
            &dst_rect
        );
        if (va_status != VA_STATUS_SUCCESS)
            return va_status;
    }

    /* Render to Pixmap */
    else {
        dst_rect.x      = 0;
        dst_rect.y      = 0;
        dst_rect.width  = obj_glx_surface->width;
        dst_rect.height = obj_glx_surface->height;

        va_status = put_surface(
            driver_data,
            obj_surface->base.id,
            obj_glx_surface->pixo->pixmap,
            obj_glx_surface->width,
            obj_glx_surface->height,
            &src_rect,
            &dst_rect,
            flags | VA_CLEAR_DRAWABLE
        );
        if (va_status != VA_STATUS_SUCCESS)
            return va_status;

        /* Force rendering of fields now */
        if ((flags ^ (VA_TOP_FIELD|VA_BOTTOM_FIELD)) != 0) {
            object_output_p obj_output;
            obj_output = output_surface_lookup(
                obj_surface,
                obj_glx_surface->pixo->pixmap
            );
            ASSERT(obj_output);
            if (obj_output && obj_output->fields) {
                va_status = queue_surface(driver_data, obj_surface, obj_output);
                if (va_status != VA_STATUS_SUCCESS)
                    return va_status;
            }
        }
    }

    obj_glx_surface->va_surface = obj_surface->base.id;
    return VA_STATUS_SUCCESS;
}

VAStatus
vdpau_AssociateSurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface,
    VASurfaceID      surface,
    unsigned int     flags
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    object_surface_p obj_surface = VDPAU_SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = associate_glx_surface(
        driver_data,
        obj_glx_surface,
        obj_surface,
        flags
    );

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}

// vaDeassociateSurfaceGLX
static VAStatus
deassociate_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
)
{
    if (!vdpau_gl_interop()) {
        if (!gl_unbind_pixmap_object(obj_glx_surface->pixo))
            return VA_STATUS_ERROR_OPERATION_FAILED;
    }

    obj_glx_surface->va_surface = VA_INVALID_SURFACE;
    return VA_STATUS_SUCCESS;
}

VAStatus
vdpau_DeassociateSurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = deassociate_glx_surface(driver_data, obj_glx_surface);

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}

// vaSyncSurfaceGLX
static inline VAStatus
sync_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
)
{
    object_surface_p obj_surface = VDPAU_SURFACE(obj_glx_surface->va_surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    return sync_surface(driver_data, obj_surface);
}

VAStatus
vdpau_SyncSurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = sync_glx_surface(driver_data, obj_glx_surface);

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}

// vaBeginRenderSurfaceGLX
static inline VAStatus
begin_render_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
)
{
    VAStatus va_status = sync_glx_surface(driver_data, obj_glx_surface);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    if (vdpau_gl_interop()) {
        if (!gl_vdpau_bind_surface(obj_glx_surface->gl_surface))
            return VA_STATUS_ERROR_OPERATION_FAILED;
    }
    else {
        if (!gl_bind_pixmap_object(obj_glx_surface->pixo))
            return VA_STATUS_ERROR_OPERATION_FAILED;
    }
    return VA_STATUS_SUCCESS;
}

VAStatus
vdpau_BeginRenderSurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = begin_render_glx_surface(driver_data, obj_glx_surface);

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}

// vaEndRenderSurfaceGLX
static inline VAStatus
end_render_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface
)
{
    if (vdpau_gl_interop()) {
        if (!gl_vdpau_unbind_surface(obj_glx_surface->gl_surface))
            return VA_STATUS_ERROR_OPERATION_FAILED;
    }
    else {
        if (!gl_unbind_pixmap_object(obj_glx_surface->pixo))
            return VA_STATUS_ERROR_OPERATION_FAILED;
    }
    return VA_STATUS_SUCCESS;
}

VAStatus
vdpau_EndRenderSurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = end_render_glx_surface(driver_data, obj_glx_surface);

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}

// vaCopySurfaceGLX
static VAStatus
copy_glx_surface(
    vdpau_driver_data_t *driver_data,
    object_glx_surface_p obj_glx_surface,
    object_surface_p     obj_surface,
    unsigned int         flags
)
{
    /* Create framebuffer surface */
    if (!obj_glx_surface->fbo) {
        obj_glx_surface->fbo = gl_create_framebuffer_object(
            obj_glx_surface->target,
            obj_glx_surface->texture,
            obj_glx_surface->width,
            obj_glx_surface->height
        );
        if (!obj_glx_surface->fbo)
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    ASSERT(obj_glx_surface->fbo);

    /* Associate VA surface */
    VAStatus va_status;
    va_status = associate_glx_surface(
        driver_data,
        obj_glx_surface,
        obj_surface,
        flags
    );
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    /* Render to FBO */
    gl_bind_framebuffer_object(obj_glx_surface->fbo);
    va_status = begin_render_glx_surface(driver_data, obj_glx_surface);
    if (va_status == VA_STATUS_SUCCESS) {
        render_pixmap(driver_data, obj_glx_surface);
        va_status = end_render_glx_surface(driver_data, obj_glx_surface);
    }
    gl_unbind_framebuffer_object(obj_glx_surface->fbo);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    va_status = deassociate_glx_surface(driver_data, obj_glx_surface);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    return VA_STATUS_SUCCESS;
}

VAStatus
vdpau_CopySurfaceGLX(
    VADriverContextP ctx,
    void            *gl_surface,
    VASurfaceID      surface,
    unsigned int     flags
)
{
    VDPAU_DRIVER_DATA_INIT;

    vdpau_set_display_type(driver_data, VA_DISPLAY_GLX);

    /* Make sure we have the necessary GLX extensions */
    if (!ensure_extensions())
        return VA_STATUS_ERROR_OPERATION_FAILED;

    object_glx_surface_p obj_glx_surface = gl_surface;
    if (!obj_glx_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    object_surface_p obj_surface = VDPAU_SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    GLContextState old_cs;
    if (!gl_set_current_context(obj_glx_surface->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    VAStatus va_status;
    va_status = copy_glx_surface(
        driver_data,
        obj_glx_surface,
        obj_surface,
        flags
    );

    gl_set_current_context(&old_cs, NULL);
    return va_status;
}
