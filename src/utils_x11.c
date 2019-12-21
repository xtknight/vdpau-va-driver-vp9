/*
 *  utils_x11.h - X11 utilities
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
#include <X11/Xutil.h>
#include "utils_x11.h"
#include "utils.h"

// X error trap
static int x11_error_code = 0;
static int (*old_error_handler)(Display *, XErrorEvent *);

static int error_handler(Display *dpy, XErrorEvent *error)
{
    x11_error_code = error->error_code;
    return 0;
}

void x11_trap_errors(void)
{
    x11_error_code    = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

int x11_untrap_errors(void)
{
    XSetErrorHandler(old_error_handler);
    return x11_error_code;
}

int
x11_get_geometry(
    Display      *dpy,
    Drawable      drawable,
    int          *px,
    int          *py,
    unsigned int *pwidth,
    unsigned int *pheight
)
{
    Window rootwin;
    int x, y;
    unsigned int width, height, border_width, depth;

    x11_trap_errors();
    XGetGeometry(
        dpy,
        drawable,
        &rootwin,
        &x, &y, &width, &height,
        &border_width,
        &depth
    );
    if (x11_untrap_errors())
        return 0;

    if (px)      *px      = x;
    if (py)      *py      = y;
    if (pwidth)  *pwidth  = width;
    if (pheight) *pheight = height;
    return 1;
}
