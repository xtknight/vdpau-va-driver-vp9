/*
 *  utils_glx.c - GLX utilities
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

#define _GNU_SOURCE 1 /* RTLD_DEFAULT */
#include "sysdeps.h"
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <pthread.h>
#include "utils.h"
#include "utils_glx.h"
#include "utils_x11.h"

#define DEBUG 1
#include "debug.h"

/**
 * gl_get_error_string:
 * @error: an OpenGL error enumeration
 *
 * Retrieves the string representation the OpenGL @error.
 *
 * Return error: the static string representing the OpenGL @error
 */
const char *
gl_get_error_string(GLenum error)
{
    static const struct {
        GLenum val;
        const char *str;
    }
    gl_errors[] = {
        { GL_NO_ERROR,          "no error" },
        { GL_INVALID_ENUM,      "invalid enumerant" },
        { GL_INVALID_VALUE,     "invalid value" },
        { GL_INVALID_OPERATION, "invalid operation" },
        { GL_STACK_OVERFLOW,    "stack overflow" },
        { GL_STACK_UNDERFLOW,   "stack underflow" },
        { GL_OUT_OF_MEMORY,     "out of memory" },
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION_EXT
        { GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation" },
#endif
        { ~0, NULL }
    };

    unsigned int i;
    for (i = 0; gl_errors[i].str; i++) {
        if (gl_errors[i].val == error)
            return gl_errors[i].str;
    }
    return "unknown";
}

/**
 * gl_purge_errors:
 *
 * Purges all OpenGL errors. This function is generally useful to
 * clear up the pending errors prior to calling gl_check_error().
 */
void
gl_purge_errors(void)
{
    while (glGetError() != GL_NO_ERROR)
        ; /* nothing */
}

/**
 * gl_check_error:
 *
 * Checks whether there is any OpenGL error pending.
 *
 * Return value: 1 if an error was encountered
 */
int
gl_check_error(void)
{
    GLenum error;
    int has_errors = 0;

    while ((error = glGetError()) != GL_NO_ERROR) {
        D(bug("glError: %s caught", gl_get_error_string(error)));
        has_errors = 1;
    }
    return has_errors;
}

/**
 * gl_get_current_color:
 * @color: the RGBA color components
 *
 * This function is a wrapper around glGetFloatv() that does extra error checking.
 *
 * Return value: 1 on success
 */
int
gl_get_current_color(float color[4])
{
    gl_purge_errors();
    glGetFloatv(GL_CURRENT_COLOR, color);
    return gl_check_error();
}

/**
 * gl_get_param:
 * @param: the parameter name
 * @pval: return location for the value
 *
 * This function is a wrapper around glGetIntegerv() that does extra
 * error checking.
 *
 * Return value: 1 on success
 */
int
gl_get_param(GLenum param, unsigned int *pval)
{
    GLint val;

    gl_purge_errors();
    glGetIntegerv(param, &val);
    if (gl_check_error())
        return 0;

    if (pval)
        *pval = val;
    return 1;
}

/**
 * gl_get_texture_param:
 * @target: the target to which the texture is bound
 * @param: the parameter name
 * @pval: return location for the value
 *
 * This function is a wrapper around glGetTexLevelParameteriv() that
 * does extra error checking.
 *
 * Return value: 1 on success
 */
int
gl_get_texture_param(GLenum target, GLenum param, unsigned int *pval)
{
    GLint val;

    gl_purge_errors();
    glGetTexLevelParameteriv(target, 0, param, &val);
    if (gl_check_error())
        return 0;

    if (pval)
        *pval = val;
    return 1;
}

/**
 * gl_set_bgcolor:
 * @color: the requested RGB color
 *
 * Sets background color to the RGB @color. This basically is a
 * wrapper around glClearColor().
 */
void
gl_set_bgcolor(uint32_t color)
{
    glClearColor(
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        ( color        & 0xff) / 255.0f,
        1.0f
    );
}

/**
 * gl_set_texture_scaling:
 * @target: the target to which the texture is currently bound
 * @scale: scaling algorithm
 *
 * Sets scaling algorithm used for the texture currently bound.
 */
void
gl_set_texture_scaling(GLenum target, GLenum scale)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, scale);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, scale);
}

/**
 * gl_perspective:
 * @fovy: the field of view angle, in degrees, in the y direction
 * @aspect: the aspect ratio that determines the field of view in the
 *   x direction.  The aspect ratio is the ratio of x (width) to y
 *   (height)
 * @zNear: the distance from the viewer to the near clipping plane
 *   (always positive)
 * @zFar: the distance from the viewer to the far clipping plane
 *   (always positive)
 *
 * Specified a viewing frustum into the world coordinate system. This
 * basically is the Mesa implementation of gluPerspective().
 */
static void
frustum(GLdouble left, GLdouble right,
        GLdouble bottom, GLdouble top, 
        GLdouble nearval, GLdouble farval)
{
    GLdouble x, y, a, b, c, d;
    GLdouble m[16];

    x = (2.0 * nearval) / (right - left);
    y = (2.0 * nearval) / (top - bottom);
    a = (right + left) / (right - left);
    b = (top + bottom) / (top - bottom);
    c = -(farval + nearval) / ( farval - nearval);
    d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)  m[col*4+row]
    M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
    M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
    M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
    M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M

    glMultMatrixd(m);
}

static void
gl_perspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLdouble xmin, xmax, ymin, ymax;

    ymax = zNear * tan(fovy * M_PI / 360.0);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    /* Don't call glFrustum() because of error semantics (covglu) */
    frustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

/**
 * gl_resize:
 * @width: the requested width, in pixels
 * @height: the requested height, in pixels
 *
 * Resizes the OpenGL viewport to the specified dimensions, using an
 * orthogonal projection. (0,0) represents the top-left corner of the
 * window.
 */
void
gl_resize(unsigned int width, unsigned int height)
{
#define FOVY     60.0f
#define ASPECT   1.0f
#define Z_NEAR   0.1f
#define Z_FAR    100.0f
#define Z_CAMERA 0.869f

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gl_perspective(FOVY, ASPECT, Z_NEAR, Z_FAR);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-0.5f, -0.5f, -Z_CAMERA);
    glScalef(1.0f/width, -1.0f/height, 1.0f/width);
    glTranslatef(0.0f, -1.0f*height, 0.0f);
}

/**
 * gl_create_context:
 * @dpy: an X11 #Display
 * @screen: the associated screen of @dpy
 * @parent: the parent #GLContextState, or %NULL if none is to be used
 *
 * Creates a GLX context sharing textures and displays lists with
 * @parent, if not %NULL.
 *
 * Return value: the newly created GLX context
 */
GLContextState *
gl_create_context(Display *dpy, int screen, GLContextState *parent)
{
    GLContextState *cs;
    GLXFBConfig *fbconfigs = NULL;
    int fbconfig_id, val, n, n_fbconfigs;
    Status status;

    static GLint fbconfig_attrs[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER,  True,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8, 
        GLX_BLUE_SIZE,     8,
        None
    };

    cs = malloc(sizeof(*cs));
    if (!cs)
        goto error;

    cs->display = dpy;
    cs->window  = parent ? parent->window : None;
    cs->visual  = NULL;
    cs->context = NULL;

    if (parent && parent->context) {
        status = glXQueryContext(
            parent->display,
            parent->context,
            GLX_FBCONFIG_ID, &fbconfig_id
        );
        if (status != Success)
            goto error;

        fbconfigs = glXGetFBConfigs(dpy, screen, &n_fbconfigs);
        if (!fbconfigs)
            goto error;

        /* Find out a GLXFBConfig compatible with the parent context */
        for (n = 0; n < n_fbconfigs; n++) {
            status = glXGetFBConfigAttrib(
                dpy,
                fbconfigs[n],
                GLX_FBCONFIG_ID, &val
            );
            if (status == Success && val == fbconfig_id)
                break;
        }
        if (n == n_fbconfigs)
            goto error;
    }
    else {
        fbconfigs = glXChooseFBConfig(
            dpy,
            screen,
            fbconfig_attrs, &n_fbconfigs
        );
        if (!fbconfigs)
            goto error;

        /* Select the first one */
        n = 0;
    }

    cs->visual  = glXGetVisualFromFBConfig(dpy, fbconfigs[n]);
    cs->context = glXCreateNewContext(
        dpy,
        fbconfigs[n],
        GLX_RGBA_TYPE,
        parent ? parent->context : NULL,
        True
    );
    if (cs->context)
        goto end;

error:
    gl_destroy_context(cs);
    cs = NULL;
end:
    if (fbconfigs)
        XFree(fbconfigs);
    return cs;
}

/**
 * gl_destroy_context:
 * @cs: a #GLContextState
 *
 * Destroys the GLX context @cs
 */
void
gl_destroy_context(GLContextState *cs)
{
    if (!cs)
        return;

    if (cs->visual) {
        XFree(cs->visual);
        cs->visual = NULL;
    }

    if (cs->display && cs->context) {
        if (glXGetCurrentContext() == cs->context)
            glXMakeCurrent(cs->display, None, NULL);
        glXDestroyContext(cs->display, cs->context);
        cs->display = NULL;
        cs->context = NULL;
    }
    free(cs);
}

/**
 * gl_init_context:
 * @cs: a #GLContextState
 *
 * Initializes the GLX context @cs with a base environment.
 */
void
gl_init_context(GLContextState *cs)
{
    GLContextState old_cs, tmp_cs;

    if (!gl_set_current_context(cs, &old_cs))
        return;

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDrawBuffer(GL_BACK);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl_set_current_context(&old_cs, &tmp_cs);
}

/**
 * gl_get_current_context:
 * @cs: return location to the current #GLContextState
 *
 * Retrieves the current GLX context, display and drawable packed into
 * the #GLContextState struct.
 */
void
gl_get_current_context(GLContextState *cs)
{
    cs->display = glXGetCurrentDisplay();
    cs->window  = glXGetCurrentDrawable();
    cs->context = glXGetCurrentContext();
}

/**
 * gl_set_current_context:
 * @new_cs: the requested new #GLContextState
 * @old_cs: return location to the context that was previously current
 *
 * Makes the @new_cs GLX context the current GLX rendering context of
 * the calling thread, replacing the previously current context if
 * there was one.
 *
 * If @old_cs is non %NULL, the previously current GLX context and
 * window are recorded.
 *
 * Return value: 1 on success
 */
int
gl_set_current_context(GLContextState *new_cs, GLContextState *old_cs)
{
    /* If display is NULL, this could be that new_cs was retrieved from
       gl_get_current_context() with none set previously. If that case,
       the other fields are also NULL and we don't return an error */
    if (!new_cs->display)
        return !new_cs->window && !new_cs->context;

    if (old_cs) {
        if (old_cs == new_cs)
            return 1;
        gl_get_current_context(old_cs);
        if (old_cs->display == new_cs->display &&
            old_cs->window  == new_cs->window  &&
            old_cs->context == new_cs->context)
            return 1;
    }
    return glXMakeCurrent(new_cs->display, new_cs->window, new_cs->context);
}

/**
 * gl_swap_buffers:
 * @cs: a #GLContextState
 *
 * Promotes the contents of the back buffer of the @win window to
 * become the contents of the front buffer. This simply is wrapper
 * around glXSwapBuffers().
 */
void
gl_swap_buffers(GLContextState *cs)
{
    glXSwapBuffers(cs->display, cs->window);
}

/**
 * gl_create_texture:
 * @target: the target to which the texture is bound
 * @format: the format of the pixel data
 * @width: the requested width, in pixels
 * @height: the requested height, in pixels
 *
 * Creates a texture with the specified dimensions and @format. The
 * internal format will be automatically derived from @format.
 *
 * Return value: the newly created texture name
 */
GLuint
gl_create_texture(GLenum target, GLenum format, unsigned int width, unsigned int height)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    GLenum internal_format;
    GLuint texture;
    unsigned int bytes_per_component;

    switch (target) {
    case GL_TEXTURE_2D:
        if (!gl_vtable->has_texture_non_power_of_two) {
            D(bug("Unsupported GL_ARB_texture_non_power_of_two extension\n"));
            return 0;
        }
        break;
    case GL_TEXTURE_RECTANGLE_ARB:
        if (!gl_vtable->has_texture_rectangle) {
            D(bug("Unsupported GL_ARB_texture_rectangle extension\n"));
            return 0;
        }
        break;
    default:
        D(bug("Unsupported texture target 0x%04x\n", target));
        return 0;
    }

    internal_format = format;
    switch (format) {
    case GL_LUMINANCE:
        bytes_per_component = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        bytes_per_component = 2;
        break;
    case GL_RGBA:
    case GL_BGRA:
        internal_format = GL_RGBA;
        bytes_per_component = 4;
        break;
    default:
        bytes_per_component = 0;
        break;
    }
    ASSERT(bytes_per_component > 0);

    glEnable(target);
    glGenTextures(1, &texture);
    glBindTexture(target, texture);
    gl_set_texture_scaling(target, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, bytes_per_component);
    glTexImage2D(
        target,
        0,
        internal_format,
        width, height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        NULL
    );
    glBindTexture(target, 0);
    return texture;
}

/**
 * get_proc_address:
 * @name: the name of the OpenGL extension function to lookup
 *
 * Returns the specified OpenGL extension function
 *
 * Return value: the OpenGL extension matching @name, or %NULL if none
 *   was found
 */
typedef void (*GLFuncPtr)(void);
typedef GLFuncPtr (*GLXGetProcAddressProc)(const char *);

static GLFuncPtr
get_proc_address_default(const char *name)
{
    return NULL;
}

static GLXGetProcAddressProc
get_proc_address_func(void)
{
    GLXGetProcAddressProc get_proc_func;

    dlerror();
    get_proc_func = (GLXGetProcAddressProc)
        dlsym(RTLD_DEFAULT, "glXGetProcAddress");
    if (!dlerror())
        return get_proc_func;

    get_proc_func = (GLXGetProcAddressProc)
        dlsym(RTLD_DEFAULT, "glXGetProcAddressARB");
    if (!dlerror())
        return get_proc_func;

    return get_proc_address_default;
}

static inline GLFuncPtr
get_proc_address(const char *name)
{
    static GLXGetProcAddressProc get_proc_func = NULL;
    if (!get_proc_func)
        get_proc_func = get_proc_address_func();
    return get_proc_func(name);
}

/**
 * gl_init_vtable:
 *
 * Initializes the global #GLVTable.
 *
 * Return value: the #GLVTable filled in with OpenGL extensions, or
 *   %NULL on error.
 */
static GLVTable gl_vtable_static;

static GLVTable *
gl_init_vtable(void)
{
    GLVTable * const gl_vtable = &gl_vtable_static;
    const char *gl_extensions = (const char *)glGetString(GL_EXTENSIONS);
    int has_extension;

    /* GL_ARB_texture_non_power_of_two */
    has_extension = (
        find_string("GL_ARB_texture_non_power_of_two", gl_extensions, " ")
    );
    if (has_extension)
        gl_vtable->has_texture_non_power_of_two = 1;

    /* GL_ARB_texture_rectangle */
    has_extension = (
        find_string("GL_ARB_texture_rectangle", gl_extensions, " ")
    );
    if (has_extension)
        gl_vtable->has_texture_rectangle = 1;

    /* GLX_EXT_texture_from_pixmap */
    gl_vtable->glx_bind_tex_image = (PFNGLXBINDTEXIMAGEEXTPROC)
        get_proc_address("glXBindTexImageEXT");
    if (!gl_vtable->glx_bind_tex_image)
        return NULL;
    gl_vtable->glx_release_tex_image = (PFNGLXRELEASETEXIMAGEEXTPROC)
        get_proc_address("glXReleaseTexImageEXT");
    if (!gl_vtable->glx_release_tex_image)
        return NULL;

    /* GL_ARB_framebuffer_object */
    has_extension = (
        find_string("GL_ARB_framebuffer_object", gl_extensions, " ") ||
        find_string("GL_EXT_framebuffer_object", gl_extensions, " ")
    );
    if (has_extension) {
        gl_vtable->gl_gen_framebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC)
            get_proc_address("glGenFramebuffersEXT");
        if (!gl_vtable->gl_gen_framebuffers)
            return NULL;
        gl_vtable->gl_delete_framebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC)
            get_proc_address("glDeleteFramebuffersEXT");
        if (!gl_vtable->gl_delete_framebuffers)
            return NULL;
        gl_vtable->gl_bind_framebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC)
            get_proc_address("glBindFramebufferEXT");
        if (!gl_vtable->gl_bind_framebuffer)
            return NULL;
        gl_vtable->gl_gen_renderbuffers = (PFNGLGENRENDERBUFFERSEXTPROC)
            get_proc_address("glGenRenderbuffersEXT");
        if (!gl_vtable->gl_gen_renderbuffers)
            return NULL;
        gl_vtable->gl_delete_renderbuffers = (PFNGLDELETERENDERBUFFERSEXTPROC)
            get_proc_address("glDeleteRenderbuffersEXT");
        if (!gl_vtable->gl_delete_renderbuffers)
            return NULL;
        gl_vtable->gl_bind_renderbuffer = (PFNGLBINDRENDERBUFFEREXTPROC)
            get_proc_address("glBindRenderbufferEXT");
        if (!gl_vtable->gl_bind_renderbuffer)
            return NULL;
        gl_vtable->gl_renderbuffer_storage = (PFNGLRENDERBUFFERSTORAGEEXTPROC)
            get_proc_address("glRenderbufferStorageEXT");
        if (!gl_vtable->gl_renderbuffer_storage)
            return NULL;
        gl_vtable->gl_framebuffer_renderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)
            get_proc_address("glFramebufferRenderbufferEXT");
        if (!gl_vtable->gl_framebuffer_renderbuffer)
            return NULL;
        gl_vtable->gl_framebuffer_texture_2d = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)
            get_proc_address("glFramebufferTexture2DEXT");
        if (!gl_vtable->gl_framebuffer_texture_2d)
            return NULL;
        gl_vtable->gl_check_framebuffer_status = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)
            get_proc_address("glCheckFramebufferStatusEXT");
        if (!gl_vtable->gl_check_framebuffer_status)
            return NULL;
        gl_vtable->has_framebuffer_object = 1;
    }

    /* GL_ARB_fragment_program */
    has_extension = (
        find_string("GL_ARB_fragment_program", gl_extensions, " ")
    );
    if (has_extension) {
        gl_vtable->gl_gen_programs = (PFNGLGENPROGRAMSARBPROC)
            get_proc_address("glGenProgramsARB");
        if (!gl_vtable->gl_gen_programs)
            return NULL;
        gl_vtable->gl_delete_programs = (PFNGLDELETEPROGRAMSARBPROC)
            get_proc_address("glDeleteProgramsARB");
        if (!gl_vtable->gl_delete_programs)
            return NULL;
        gl_vtable->gl_bind_program = (PFNGLBINDPROGRAMARBPROC)
            get_proc_address("glBindProgramARB");
        if (!gl_vtable->gl_bind_program)
            return NULL;
        gl_vtable->gl_program_string = (PFNGLPROGRAMSTRINGARBPROC)
            get_proc_address("glProgramStringARB");
        if (!gl_vtable->gl_program_string)
            return NULL;
        gl_vtable->gl_get_program_iv = (PFNGLGETPROGRAMIVARBPROC)
            get_proc_address("glGetProgramivARB");
        if (!gl_vtable->gl_get_program_iv)
            return NULL;
        gl_vtable->gl_program_local_parameter_4fv = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)
            get_proc_address("glProgramLocalParameter4fvARB");
        if (!gl_vtable->gl_program_local_parameter_4fv)
            return NULL;
        gl_vtable->has_fragment_program = 1;
    }

    /* GL_ARB_multitexture */
    has_extension = (
        find_string("GL_ARB_multitexture", gl_extensions, " ")
    );
    if (has_extension) {
        gl_vtable->gl_active_texture = (PFNGLACTIVETEXTUREPROC)
            get_proc_address("glActiveTextureARB");
        if (!gl_vtable->gl_active_texture)
            return NULL;
        gl_vtable->gl_multi_tex_coord_2f = (PFNGLMULTITEXCOORD2FPROC)
            get_proc_address("glMultiTexCoord2fARB");
        if (!gl_vtable->gl_multi_tex_coord_2f)
            return NULL;
        gl_vtable->has_multitexture = 1;
    }

    /* GL_NV_vdpau_interop */
    has_extension = (
        find_string("GL_NV_vdpau_interop", gl_extensions, " ")
    );
    if (has_extension) {
        gl_vtable->gl_vdpau_init = (PFNGLVDPAUINITNVPROC)
            get_proc_address("glVDPAUInitNV");
        if (!gl_vtable->gl_vdpau_init)
            return NULL;
        gl_vtable->gl_vdpau_fini = (PFNGLVDPAUFININVPROC)
            get_proc_address("glVDPAUFiniNV");
        if (!gl_vtable->gl_vdpau_fini)
            return NULL;
        gl_vtable->gl_vdpau_register_video_surface = (PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)
            get_proc_address("glVDPAURegisterVideoSurfaceNV");
        if (!gl_vtable->gl_vdpau_register_video_surface)
            return NULL;
        gl_vtable->gl_vdpau_register_output_surface = (PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)
            get_proc_address("glVDPAURegisterOutputSurfaceNV");
        if (!gl_vtable->gl_vdpau_register_output_surface)
            return NULL;
        gl_vtable->gl_vdpau_is_surface = (PFNGLVDPAUISSURFACENVPROC)
            get_proc_address("glVDPAUIsSurfaceNV");
        if (!gl_vtable->gl_vdpau_is_surface)
            return NULL;
        gl_vtable->gl_vdpau_unregister_surface = (PFNGLVDPAUUNREGISTERSURFACENVPROC)
            get_proc_address("glVDPAUUnregisterSurfaceNV");
        if (!gl_vtable->gl_vdpau_unregister_surface)
            return NULL;
        gl_vtable->gl_vdpau_get_surface_iv = (PFNGLVDPAUGETSURFACEIVNVPROC)
            get_proc_address("glVDPAUGetSurfaceivNV");
        if (!gl_vtable->gl_vdpau_get_surface_iv)
            return NULL;
        gl_vtable->gl_vdpau_surface_access = (PFNGLVDPAUSURFACEACCESSNVPROC)
            get_proc_address("glVDPAUSurfaceAccessNV");
        if (!gl_vtable->gl_vdpau_surface_access)
            return NULL;
        gl_vtable->gl_vdpau_map_surfaces = (PFNGLVDPAUMAPSURFACESNVPROC)
            get_proc_address("glVDPAUMapSurfacesNV");
        if (!gl_vtable->gl_vdpau_map_surfaces)
            return NULL;
        gl_vtable->gl_vdpau_unmap_surfaces = (PFNGLVDPAUUNMAPSURFACESNVPROC)
            get_proc_address("glVDPAUUnmapSurfacesNV");
        if (!gl_vtable->gl_vdpau_unmap_surfaces)
            return NULL;
        gl_vtable->has_vdpau_interop = 1;
    }
    return gl_vtable;
}

/**
 * gl_get_vtable:
 *
 * Retrieves a VTable for OpenGL extensions.
 *
 * Return value: VTable for OpenGL extensions
 */
GLVTable *
gl_get_vtable(void)
{
    static pthread_mutex_t mutex          = PTHREAD_MUTEX_INITIALIZER;
    static int             gl_vtable_init = 1;
    static GLVTable       *gl_vtable      = NULL;

    pthread_mutex_lock(&mutex);
    if (gl_vtable_init) {
        gl_vtable_init = 0;
        gl_vtable      = gl_init_vtable();
    }
    pthread_mutex_unlock(&mutex);
    return gl_vtable;
}

/**
 * gl_create_pixmap_object:
 * @dpy: an X11 #Display
 * @target: the target to which the texture is bound
 * @width: the request width, in pixels
 * @height: the request height, in pixels
 *
 * Creates a #GLPixmapObject of the specified dimensions. This
 * requires the GLX_EXT_texture_from_pixmap extension.
 *
 * Return value: the newly created #GLPixmapObject object
 */
GLPixmapObject *
gl_create_pixmap_object(
    Display     *dpy,
    GLenum       target,
    unsigned int width,
    unsigned int height
)
{
    GLVTable * const    gl_vtable = gl_get_vtable();
    GLPixmapObject     *pixo;
    GLXFBConfig        *fbconfig;
    int                 screen;
    Window              rootwin;
    XWindowAttributes   wattr;
    int                *attr;
    int                 n_fbconfig_attrs;

    int fbconfig_attrs[32] = {
        GLX_DRAWABLE_TYPE,      GLX_PIXMAP_BIT,
        GLX_DOUBLEBUFFER,       GL_FALSE,
        GLX_RENDER_TYPE,        GLX_RGBA_BIT,
        GLX_X_RENDERABLE,       GL_TRUE,
        GLX_Y_INVERTED_EXT,     GL_TRUE,
        GLX_RED_SIZE,           8,
        GLX_GREEN_SIZE,         8,
        GLX_BLUE_SIZE,          8,
        GL_NONE,
    };

    int pixmap_attrs[10] = {
        GLX_MIPMAP_TEXTURE_EXT, GL_FALSE,
        GL_NONE,
    };

    if (!gl_vtable)
        return NULL;

    screen  = DefaultScreen(dpy);
    rootwin = RootWindow(dpy, screen);

    /* XXX: this won't work for different displays */
    if (!gl_vtable->has_texture_from_pixmap) {
        const char *glx_extensions;
        int glx_major, glx_minor;
        glx_extensions = glXQueryExtensionsString(dpy, screen);
        if (!glx_extensions)
            return NULL;
        if (!find_string("GLX_EXT_texture_from_pixmap", glx_extensions, " "))
            return NULL;
        if (!glXQueryVersion(dpy, &glx_major, &glx_minor))
            return NULL;
        if (glx_major < 1 || (glx_major == 1 && glx_minor < 3)) /* 1.3 */
            return NULL;
        gl_vtable->has_texture_from_pixmap = 1;
    }

    pixo = calloc(1, sizeof(*pixo));
    if (!pixo)
        return NULL;

    pixo->dpy           = dpy;
    pixo->target        = target;
    pixo->width         = width;
    pixo->height        = height;
    pixo->pixmap        = None;
    pixo->glx_pixmap    = None;
    pixo->is_bound      = 0;

    XGetWindowAttributes(dpy, rootwin, &wattr);
    pixo->pixmap  = XCreatePixmap(dpy, rootwin, width, height, wattr.depth);
    if (!pixo->pixmap)
        goto error;

    /* Initialize FBConfig attributes */
    for (attr = fbconfig_attrs; *attr != GL_NONE; attr += 2)
        ;
    *attr++ = GLX_DEPTH_SIZE;                 *attr++ = wattr.depth;
    if (wattr.depth == 32) {
    *attr++ = GLX_ALPHA_SIZE;                 *attr++ = 8;
    *attr++ = GLX_BIND_TO_TEXTURE_RGBA_EXT;   *attr++ = GL_TRUE;
    }
    else {
    *attr++ = GLX_BIND_TO_TEXTURE_RGB_EXT;    *attr++ = GL_TRUE;
    }
    *attr++ = GL_NONE;

    fbconfig = glXChooseFBConfig(
        dpy,
        screen,
        fbconfig_attrs, &n_fbconfig_attrs
    );
    if (!fbconfig)
        goto error;

    /* Initialize GLX Pixmap attributes */
    for (attr = pixmap_attrs; *attr != GL_NONE; attr += 2)
        ;
    *attr++ = GLX_TEXTURE_TARGET_EXT;
    switch (target) {
    case GL_TEXTURE_2D:
        *attr++ = GLX_TEXTURE_2D_EXT;
        break;
    case GL_TEXTURE_RECTANGLE_ARB:
        *attr++ = GLX_TEXTURE_RECTANGLE_EXT;
        break;
    default:
        goto error;
    }
    *attr++ = GLX_TEXTURE_FORMAT_EXT;
    if (wattr.depth == 32)
    *attr++ = GLX_TEXTURE_FORMAT_RGBA_EXT;
    else
    *attr++ = GLX_TEXTURE_FORMAT_RGB_EXT;
    *attr++ = GL_NONE;

    x11_trap_errors();
    pixo->glx_pixmap = glXCreatePixmap(dpy, fbconfig[0], pixo->pixmap, pixmap_attrs);
    free(fbconfig);
    if (x11_untrap_errors() != 0)
        goto error;

    glEnable(pixo->target);
    glGenTextures(1, &pixo->texture);
    glBindTexture(pixo->target, pixo->texture);
    gl_set_texture_scaling(pixo->target, GL_LINEAR);
    glBindTexture(pixo->target, 0);
    return pixo;

error:
    gl_destroy_pixmap_object(pixo);
    return NULL;
}

/**
 * gl_destroy_pixmap_object:
 * @pixo: a #GLPixmapObject
 *
 * Destroys the #GLPixmapObject object.
 */
void
gl_destroy_pixmap_object(GLPixmapObject *pixo)
{
    if (!pixo)
        return;

    gl_unbind_pixmap_object(pixo);

    if (pixo->texture) {
        glDeleteTextures(1, &pixo->texture);
        pixo->texture = 0;
    }

    if (pixo->glx_pixmap) {
        glXDestroyPixmap(pixo->dpy, pixo->glx_pixmap);
        pixo->glx_pixmap = None;
    }

    if (pixo->pixmap) {
        XFreePixmap(pixo->dpy, pixo->pixmap);
        pixo->pixmap = None;
    }
    free(pixo);
}

/**
 * gl_bind_pixmap_object:
 * @pixo: a #GLPixmapObject
 *
 * Defines a two-dimensional texture image. The texture image is taken
 * from the @pixo pixmap and need not be copied. The texture target,
 * format and size are derived from attributes of the @pixo pixmap.
 *
 * Return value: 1 on success
 */
int
gl_bind_pixmap_object(GLPixmapObject *pixo)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (pixo->is_bound)
        return 1;

    glBindTexture(pixo->target, pixo->texture);

    x11_trap_errors();
    gl_vtable->glx_bind_tex_image(
        pixo->dpy,
        pixo->glx_pixmap,
        GLX_FRONT_LEFT_EXT,
        NULL
    );
    XSync(pixo->dpy, False);
    if (x11_untrap_errors() != 0) {
        D(bug("failed to bind pixmap"));
        return 0;
    }

    pixo->is_bound = 1;
    return 1;
}

/**
 * gl_unbind_pixmap_object:
 * @pixo: a #GLPixmapObject
 *
 * Releases a color buffers that is being used as a texture.
 *
 * Return value: 1 on success
 */
int
gl_unbind_pixmap_object(GLPixmapObject *pixo)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!pixo->is_bound)
        return 1;

    x11_trap_errors();
    gl_vtable->glx_release_tex_image(
        pixo->dpy,
        pixo->glx_pixmap,
        GLX_FRONT_LEFT_EXT
    );
    XSync(pixo->dpy, False);
    if (x11_untrap_errors() != 0) {
        D(bug("failed to release pixmap"));
        return 0;
    }

    glBindTexture(pixo->target, 0);

    pixo->is_bound = 0;
    return 1;
}

/**
 * gl_create_framebuffer_object:
 * @target: the target to which the texture is bound
 * @texture: the GL texture to hold the framebuffer
 * @width: the requested width, in pixels
 * @height: the requested height, in pixels
 *
 * Creates an FBO with the specified texture and size.
 *
 * Return value: the newly created #GLFramebufferObject, or %NULL if
 *   an error occurred
 */
GLFramebufferObject *
gl_create_framebuffer_object(
    GLenum       target,
    GLuint       texture,
    unsigned int width,
    unsigned int height
)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    GLFramebufferObject *fbo;
    GLenum status;

    if (!gl_vtable || !gl_vtable->has_framebuffer_object)
        return NULL;

    fbo = calloc(1, sizeof(*fbo));
    if (!fbo)
        return NULL;

    fbo->width    = width;
    fbo->height   = height;
    fbo->fbo      = 0;
    fbo->old_fbo  = 0;
    fbo->is_bound = 0;

    gl_get_param(GL_FRAMEBUFFER_BINDING, &fbo->old_fbo);
    gl_vtable->gl_gen_framebuffers(1, &fbo->fbo);
    gl_vtable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, fbo->fbo);
    gl_vtable->gl_framebuffer_texture_2d(
        GL_FRAMEBUFFER_EXT,
        GL_COLOR_ATTACHMENT0_EXT,
        target, texture,
        0
    );

    status = gl_vtable->gl_check_framebuffer_status(GL_DRAW_FRAMEBUFFER_EXT);
    gl_vtable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, fbo->old_fbo);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        goto error;
    return fbo;

error:
    gl_destroy_framebuffer_object(fbo);
    return NULL;
}

/**
 * gl_destroy_framebuffer_object:
 * @fbo: a #GLFramebufferObject
 *
 * Destroys the @fbo object.
 */
void
gl_destroy_framebuffer_object(GLFramebufferObject *fbo)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!fbo)
        return;

    gl_unbind_framebuffer_object(fbo);

    if (fbo->fbo) {
        gl_vtable->gl_delete_framebuffers(1, &fbo->fbo);
        fbo->fbo = 0;
    }
    free(fbo);
}

/**
 * gl_bind_framebuffer_object:
 * @fbo: a #GLFramebufferObject
 *
 * Binds @fbo object.
 *
 * Return value: 1 on success
 */
int
gl_bind_framebuffer_object(GLFramebufferObject *fbo)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    const unsigned int width   = fbo->width;
    const unsigned int height  = fbo->height;

    const unsigned int attribs = (GL_VIEWPORT_BIT |
                                  GL_CURRENT_BIT  |
                                  GL_ENABLE_BIT   |
                                  GL_TEXTURE_BIT  |
                                  GL_COLOR_BUFFER_BIT);

    if (fbo->is_bound)
        return 1;

    gl_get_param(GL_FRAMEBUFFER_BINDING, &fbo->old_fbo);
    gl_vtable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, fbo->fbo);
    glPushAttrib(attribs);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(0, 0, width, height);
    glTranslatef(-1.0f, -1.0f, 0.0f);
    glScalef(2.0f / width, 2.0f / height, 1.0f);

    fbo->is_bound = 1;
    return 1;
}

/**
 * gl_unbind_framebuffer_object:
 * @fbo: a #GLFramebufferObject
 *
 * Releases @fbo object.
 *
 * Return value: 1 on success
 */
int
gl_unbind_framebuffer_object(GLFramebufferObject *fbo)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!fbo->is_bound)
        return 1;

    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    gl_vtable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, fbo->old_fbo);

    fbo->is_bound = 0;
    return 1;
}

/**
 * gl_vdpau_init:
 * @device: a #VdpDevice
 * @get_proc_address: the #VdpGetProcAddress generated during
 *   #VdpDevice creation
 *
 * Informs the GL context which VDPAU device to interact with.
 *
 * Return value: 1 on success
 */
int
gl_vdpau_init(VdpDevice device, VdpGetProcAddress get_proc_address)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!gl_vtable || !gl_vtable->has_vdpau_interop)
        return 0;

    gl_vtable->gl_vdpau_init((void *)(uintptr_t)device, get_proc_address);

    return 1;
}

/**
 * gl_vdpau_exit:
 *
 * Disposes the VDPAU/GL interact functionality for the current context.
 */
void
gl_vdpau_exit(void)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!gl_vtable || !gl_vtable->has_vdpau_interop)
        return;

    gl_vtable->gl_vdpau_fini();
}

/**
 * gl_vdpau_create_video_surface:
 * @target: the texture target
 * @surface: the VDPAU video surface to wrap
 *
 * Creates a VDPAU/GL surface from the specified @surface, which is a
 * #VdpVideoSurface.
 *
 * Return value: the newly created #GLVdpSurface, or %NULL if an error
 *   occurred
 */
GLVdpSurface *
gl_vdpau_create_video_surface(GLenum target, VdpVideoSurface surface)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    GLVdpSurface *s;
    unsigned int i;

    if (!gl_vtable || !gl_vtable->has_vdpau_interop)
        return NULL;

    s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    s->target           = target;
    s->num_textures     = 4;
    s->is_bound         = 0;

    glEnable(s->target);
    glGenTextures(s->num_textures, &s->textures[0]);

    s->surface = gl_vtable->gl_vdpau_register_video_surface(
        (void *)(uintptr_t)surface,
        s->target,
        s->num_textures, s->textures
    );
    if (!s->surface)
        goto error;

    for (i = 0; i < s->num_textures; i++) {
        glBindTexture(s->target, s->textures[i]);
        gl_set_texture_scaling(s->target, GL_LINEAR);
        glTexParameteri(s->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(s->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(s->target, 0);
    }

    /* XXX: optimize for reading only */
    gl_vtable->gl_vdpau_surface_access(s->surface, GL_READ_ONLY);
    return s;

error:
    gl_vdpau_destroy_surface(s);
    return NULL;
}

/**
 * gl_vdpau_create_output_surface:
 * @target: the texture target
 * @surface: the VDPAU output surface to wrap
 *
 * Creates a VDPAU/GL surface from the specified @surface, which is a
 * #VdpOutputSurface.
 *
 * Return value: the newly created #GLVdpSurface, or %NULL if an error
 *   occurred
 */
GLVdpSurface *
gl_vdpau_create_output_surface(GLenum target, VdpOutputSurface surface)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    GLVdpSurface *s;

    if (!gl_vtable || !gl_vtable->has_vdpau_interop)
        return NULL;

    s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    s->target           = target;
    s->num_textures     = 1;
    s->is_bound         = 0;

    glEnable(s->target);
    glGenTextures(1, &s->textures[0]);

    s->surface = gl_vtable->gl_vdpau_register_output_surface(
        (void *)(uintptr_t)surface,
        s->target,
        s->num_textures, s->textures
    );
    if (!s->surface)
        goto error;

    glBindTexture(s->target, s->textures[0]);
    gl_set_texture_scaling(s->target, GL_LINEAR);
    glBindTexture(s->target, 0);

    /* XXX: optimize for reading only */
    gl_vtable->gl_vdpau_surface_access(s->surface, GL_READ_ONLY);
    return s;

error:
    gl_vdpau_destroy_surface(s);
    return NULL;
}

/**
 * gl_vdpau_destroy_surface:
 * @s: a #GLVdpSurface
 *
 * Destroys the @s object.
 */
void
gl_vdpau_destroy_surface(GLVdpSurface *s)
{
    GLVTable * const gl_vtable = gl_get_vtable();
    unsigned int i;

    if (!s)
        return;

    gl_vdpau_unbind_surface(s);

    if (s->surface) {
        gl_vtable->gl_vdpau_unregister_surface(s->surface);
        s->surface = 0;
    }

    if (s->num_textures > 0) {
        glDeleteTextures(s->num_textures, s->textures);
        for (i = 0; i < s->num_textures; i++)
            s->textures[i] = 0;
        s->num_textures = 0;
    }
    free(s);
}

/**
 * gl_vdpau_bind_surface:
 * @s: a #GLVdpSurface
 *
 * Binds surface @s.
 *
 * Return value: 1 on success
 */
int
gl_vdpau_bind_surface(GLVdpSurface *s)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (s->is_bound)
        return 1;

    gl_vtable->gl_vdpau_map_surfaces(1, &s->surface);
    s->is_bound = 1;
    return 1;
}

/**
 * gl_vdpau_unbind_surface:
 * @s: a #GLVdpSurface
 *
 * Releases surface @s.
 *
 * Return value: 1 on success
 */
int
gl_vdpau_unbind_surface(GLVdpSurface *s)
{
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!s->is_bound)
        return 1;

    gl_vtable->gl_vdpau_unmap_surfaces(1, &s->surface);
    s->is_bound = 0;
    return 1;
}
