/*
 *  vdpau_dump.h - Dump utilities
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

#ifndef VDPAU_DUMP_H
#define VDPAU_DUMP_H

#include "vdpau_driver.h"
#include "vdpau_decode.h"

// Returns string representation of FOURCC
const char *string_of_FOURCC(uint32_t fourcc)
    attribute_hidden;

// Returns string representation of VABufferType
const char *string_of_VABufferType(VABufferType type)
    attribute_hidden;

// Returns string representation of VdpCodec
const char *string_of_VdpCodec(VdpCodec codec)
    attribute_hidden;

// Dumps VdpPictureInfoMPEG1Or2
void dump_VdpPictureInfoMPEG1Or2(VdpPictureInfoMPEG1Or2 *pic_info)
    attribute_hidden;

// Dumps VdpPictureInfoMPEG4Part2
#if HAVE_VDPAU_MPEG4
void dump_VdpPictureInfoMPEG4Part2(VdpPictureInfoMPEG4Part2 *pic_info)
    attribute_hidden;
#endif

// Dumps VdpPictureInfoH264
void dump_VdpPictureInfoH264(VdpPictureInfoH264 *pic_info)
    attribute_hidden;

// Dumps VdpPictureInfoVC1
void dump_VdpPictureInfoVC1(VdpPictureInfoVC1 *pic_info)
    attribute_hidden;

// Dumps VdpBitstreamBuffer
void dump_VdpBitstreamBuffer(VdpBitstreamBuffer *bitstream_buffer)
    attribute_hidden;

#endif /* VDPAU_DUMP_H */
