/*
 *  vaapi_compat.h - VA-API compatibility glue
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

#ifndef VAAPI_COMPAT_H
#define VAAPI_COMPAT_H

#ifndef VA_INVALID_ID
#define VA_INVALID_ID                   0xffffffff
#endif
#ifndef VA_INVALID_BUFFER
#define VA_INVALID_BUFFER               VA_INVALID_ID
#endif
#ifndef VA_INVALID_SURFACE
#define VA_INVALID_SURFACE              VA_INVALID_ID
#endif
#ifndef VA_STATUS_ERROR_UNIMPLEMENTED
#define VA_STATUS_ERROR_UNIMPLEMENTED   0x00000014
#endif
#ifndef VA_DISPLAY_X11
#define VA_DISPLAY_X11                  1
#endif
#ifndef VA_DISPLAY_GLX
#define VA_DISPLAY_GLX                  2
#endif
#ifndef VA_FILTER_SCALING_DEFAULT
#define VA_FILTER_SCALING_DEFAULT       0x00000000
#endif
#ifndef VA_FILTER_SCALING_FAST
#define VA_FILTER_SCALING_FAST          0x00000100
#endif
#ifndef VA_FILTER_SCALING_HQ
#define VA_FILTER_SCALING_HQ            0x00000200
#endif
#ifndef VA_FILTER_SCALING_NL_ANAMORPHIC
#define VA_FILTER_SCALING_NL_ANAMORPHIC 0x00000300
#endif
#ifndef VA_FILTER_SCALING_MASK
#define VA_FILTER_SCALING_MASK          0x00000f00
#endif
#ifndef VA_SRC_SMPTE_240
#define VA_SRC_SMPTE_240                0x00000040
#endif

#if VA_CHECK_VERSION(0,31,1)
typedef void *VADrawable;
#else
typedef Drawable VADrawable;
#endif

#endif /* VAAPI_COMPAT_H */
