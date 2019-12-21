/*
 *  debug.h - Debugging utilities
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

#ifndef DEBUG_H
#define DEBUG_H

void vdpau_error_message(const char *msg, ...)
     attribute_hidden;

void vdpau_information_message(const char *msg, ...)
     attribute_hidden;

void debug_message(const char *msg, ...)
    attribute_hidden;

#if DEBUG && USE_DEBUG
# define D(x) x
# define bug debug_message
#else
# define D(x)
#endif

// Returns TRUE if debug trace is enabled
int trace_enabled(void)
    attribute_hidden;

// Increase or decrease indentation level
void trace_indent(int inc)
    attribute_hidden;

// Print message on the debug trace output
void trace_print(const char *format, ...)
    attribute_hidden;

#endif /* DEBUG_H */
