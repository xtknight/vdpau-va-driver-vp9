/*
 *  utils_glx.h - GLX utilities
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

#ifndef UTILS_GLX_H
#define UTILS_GLX_H

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <vdpau/vdpau.h>

/* GL_NV_vdpau_interop */
#if GL_GLEXT_VERSION < 64
typedef GLintptr GLvdpauSurfaceNV;
typedef void (*PFNGLVDPAUINITNVPROC)(const GLvoid *vdpDevice, const GLvoid *getProcAddress);
typedef void (*PFNGLVDPAUFININVPROC)(void);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)(const GLvoid *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)(const GLvoid *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
typedef GLboolean (*PFNGLVDPAUISSURFACENVPROC)(GLvdpauSurfaceNV surface);
typedef void (*PFNGLVDPAUUNREGISTERSURFACENVPROC)(GLvdpauSurfaceNV surface);
typedef void (*PFNGLVDPAUGETSURFACEIVNVPROC)(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
typedef void (*PFNGLVDPAUSURFACEACCESSNVPROC)(GLvdpauSurfaceNV surface, GLenum access);
typedef void (*PFNGLVDPAUMAPSURFACESNVPROC)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
typedef void (*PFNGLVDPAUUNMAPSURFACESNVPROC)(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces);
#endif

/* GLX_EXT_texture_from_pixmap */
#if GLX_GLXEXT_VERSION < 18
typedef void (*PFNGLXBINDTEXIMAGEEXTPROC)(Display *, GLXDrawable, int, const int *);
typedef void (*PFNGLXRELEASETEXIMAGEEXTPROC)(Display *, GLXDrawable, int);
#endif

#if GL_GLEXT_VERSION >= 85
/* XXX: PFNGLMULTITEXCOORD2FPROC got out of the GL_VERSION_1_3_DEPRECATED
   block and is not defined if GL_VERSION_1_3 is defined in <GL/gl.h>
   Redefine the type here as an interim solution */
typedef void (*PFNGLMULTITEXCOORD2FPROC) (GLenum target, GLfloat s, GLfloat t);
#endif

#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING_EXT
#endif
#ifndef GL_FRAGMENT_PROGRAM
#define GL_FRAGMENT_PROGRAM GL_FRAGMENT_PROGRAM_ARB
#endif
#ifndef GL_PROGRAM_FORMAT_ASCII
#define GL_PROGRAM_FORMAT_ASCII GL_PROGRAM_FORMAT_ASCII_ARB
#endif
#ifndef GL_PROGRAM_ERROR_POSITION
#define GL_PROGRAM_ERROR_POSITION GL_PROGRAM_ERROR_POSITION_ARB
#endif
#ifndef GL_PROGRAM_ERROR_STRING
#define GL_PROGRAM_ERROR_STRING GL_PROGRAM_ERROR_STRING_ARB
#endif
#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS
#define GL_PROGRAM_UNDER_NATIVE_LIMITS GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#endif

const char *
gl_get_error_string(GLenum error)
    attribute_hidden;

void
gl_purge_errors(void)
    attribute_hidden;

int
gl_check_error(void)
    attribute_hidden;

int
gl_get_current_color(float color[4])
    attribute_hidden;

int
gl_get_param(GLenum param, unsigned int *pval)
    attribute_hidden;

int
gl_get_texture_param(GLenum target, GLenum param, unsigned int *pval)
    attribute_hidden;

void
gl_set_bgcolor(uint32_t color)
    attribute_hidden;

void
gl_set_texture_scaling(GLenum target, GLenum scale)
    attribute_hidden;

void
gl_resize(unsigned int width, unsigned int height)
    attribute_hidden;

typedef struct _GLContextState GLContextState;
struct _GLContextState {
    Display     *display;
    Window       window;
    XVisualInfo *visual;
    GLXContext   context;
};

GLContextState *
gl_create_context(Display *dpy, int screen, GLContextState *parent)
    attribute_hidden;

void
gl_destroy_context(GLContextState *cs)
    attribute_hidden;

void
gl_init_context(GLContextState *cs)
    attribute_hidden;

void
gl_get_current_context(GLContextState *cs)
    attribute_hidden;

int
gl_set_current_context(GLContextState *new_cs, GLContextState *old_cs)
    attribute_hidden;

void
gl_swap_buffers(GLContextState *cs)
    attribute_hidden;

GLuint
gl_create_texture(GLenum target, GLenum format, unsigned int width, unsigned int height)
    attribute_hidden;

typedef struct _GLVTable GLVTable;
struct _GLVTable {
    PFNGLXBINDTEXIMAGEEXTPROC             glx_bind_tex_image;
    PFNGLXRELEASETEXIMAGEEXTPROC          glx_release_tex_image;
    PFNGLGENFRAMEBUFFERSEXTPROC           gl_gen_framebuffers;
    PFNGLDELETEFRAMEBUFFERSEXTPROC        gl_delete_framebuffers;
    PFNGLBINDFRAMEBUFFEREXTPROC           gl_bind_framebuffer;
    PFNGLGENRENDERBUFFERSEXTPROC          gl_gen_renderbuffers;
    PFNGLDELETERENDERBUFFERSEXTPROC       gl_delete_renderbuffers;
    PFNGLBINDRENDERBUFFEREXTPROC          gl_bind_renderbuffer;
    PFNGLRENDERBUFFERSTORAGEEXTPROC       gl_renderbuffer_storage;
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC   gl_framebuffer_renderbuffer;
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC      gl_framebuffer_texture_2d;
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC    gl_check_framebuffer_status;
    PFNGLGENPROGRAMSARBPROC               gl_gen_programs;
    PFNGLDELETEPROGRAMSARBPROC            gl_delete_programs;
    PFNGLBINDPROGRAMARBPROC               gl_bind_program;
    PFNGLPROGRAMSTRINGARBPROC             gl_program_string;
    PFNGLGETPROGRAMIVARBPROC              gl_get_program_iv;
    PFNGLPROGRAMLOCALPARAMETER4FVARBPROC  gl_program_local_parameter_4fv;
    PFNGLACTIVETEXTUREPROC                gl_active_texture;
    PFNGLMULTITEXCOORD2FPROC              gl_multi_tex_coord_2f;
    PFNGLVDPAUINITNVPROC                  gl_vdpau_init;
    PFNGLVDPAUFININVPROC                  gl_vdpau_fini;
    PFNGLVDPAUREGISTERVIDEOSURFACENVPROC  gl_vdpau_register_video_surface;
    PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC gl_vdpau_register_output_surface;
    PFNGLVDPAUISSURFACENVPROC             gl_vdpau_is_surface;
    PFNGLVDPAUUNREGISTERSURFACENVPROC     gl_vdpau_unregister_surface;
    PFNGLVDPAUGETSURFACEIVNVPROC          gl_vdpau_get_surface_iv;
    PFNGLVDPAUSURFACEACCESSNVPROC         gl_vdpau_surface_access;
    PFNGLVDPAUMAPSURFACESNVPROC           gl_vdpau_map_surfaces;
    PFNGLVDPAUUNMAPSURFACESNVPROC         gl_vdpau_unmap_surfaces;
    unsigned int                          has_texture_non_power_of_two  : 1;
    unsigned int                          has_texture_rectangle         : 1;
    unsigned int                          has_texture_from_pixmap       : 1;
    unsigned int                          has_framebuffer_object        : 1;
    unsigned int                          has_fragment_program          : 1;
    unsigned int                          has_multitexture              : 1;
    unsigned int                          has_vdpau_interop             : 1;
};

GLVTable *
gl_get_vtable(void)
    attribute_hidden;

typedef struct _GLPixmapObject GLPixmapObject;
struct _GLPixmapObject {
    Display        *dpy;
    GLenum          target;
    GLuint          texture;
    unsigned int    width;
    unsigned int    height;
    Pixmap          pixmap;
    GLXPixmap       glx_pixmap;
    unsigned int    is_bound    : 1;
};

GLPixmapObject *
gl_create_pixmap_object(
    Display     *dpy,
    GLenum       target,
    unsigned int width,
    unsigned int height
) attribute_hidden;

void
gl_destroy_pixmap_object(GLPixmapObject *pixo)
    attribute_hidden;

int
gl_bind_pixmap_object(GLPixmapObject *pixo)
    attribute_hidden;

int
gl_unbind_pixmap_object(GLPixmapObject *pixo)
    attribute_hidden;

typedef struct _GLFramebufferObject GLFramebufferObject;
struct _GLFramebufferObject {
    unsigned int    width;
    unsigned int    height;
    GLuint          fbo;
    GLuint          old_fbo;
    unsigned int    is_bound    : 1;
};

GLFramebufferObject *
gl_create_framebuffer_object(
    GLenum       target,
    GLuint       texture,
    unsigned int width,
    unsigned int height
) attribute_hidden;

void
gl_destroy_framebuffer_object(GLFramebufferObject *fbo)
    attribute_hidden;

int
gl_bind_framebuffer_object(GLFramebufferObject *fbo)
    attribute_hidden;

int
gl_unbind_framebuffer_object(GLFramebufferObject *fbo)
    attribute_hidden;

int
gl_vdpau_init(VdpDevice device, VdpGetProcAddress get_proc_address)
    attribute_hidden;

void
gl_vdpau_exit(void)
    attribute_hidden;

typedef struct _GLVdpSurface GLVdpSurface;
struct _GLVdpSurface {
    GLvdpauSurfaceNV    surface;
    GLenum              target;
    unsigned int        num_textures;
    GLuint              textures[4];
    unsigned int        is_bound        : 1;
};

GLVdpSurface *
gl_vdpau_create_video_surface(GLenum target, VdpVideoSurface surface)
    attribute_hidden;

GLVdpSurface *
gl_vdpau_create_output_surface(GLenum target, VdpOutputSurface surface)
    attribute_hidden;

void
gl_vdpau_destroy_surface(GLVdpSurface *s)
    attribute_hidden;

int
gl_vdpau_bind_surface(GLVdpSurface *s)
    attribute_hidden;

int
gl_vdpau_unbind_surface(GLVdpSurface *s)
    attribute_hidden;

#endif /* UTILS_GLX_H */
