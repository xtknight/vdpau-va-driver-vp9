/*
 * copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PUT_BITS_H
#define PUT_BITS_H

#ifdef WORDS_BIGENDIAN
#define be2me_32(x) (x)
#else
#define be2me_32(x) bswap_32(x)
#endif

#ifndef bswap_32
static inline uint32_t bswap_32(uint32_t x)
{
    x = ((x << 8) & 0xff00ff00) | ((x >> 8) & 0x00ff00ff);
    x = (x >> 16) | (x << 16);
    return x;
}
#endif

typedef struct PutBitContext {
    uint32_t    bit_buf;
    int         bit_left;
    uint8_t    *buf;
    uint8_t    *buf_ptr;
    uint8_t    *buf_end;
    int         size_in_bits;
} PutBitContext;

/**
 * Initializes the PutBitContext s.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer
 */
static inline void init_put_bits(PutBitContext *s, uint8_t *buffer, int buffer_size)
{
    if (buffer_size < 0) {
        buffer_size = 0;
        buffer = NULL;
    }

    s->size_in_bits = 8 * buffer_size;
    s->buf          = buffer;
    s->buf_end      = s->buf + buffer_size;
    s->buf_ptr      = s->buf;
    s->bit_left     = 32;
    s->bit_buf      = 0;
}

/**
 * Returns the total number of bits written to the bitstream.
 */
static inline int put_bits_count(PutBitContext *s)
{
    return (s->buf_ptr - s->buf) * 8 + 32 - s->bit_left;
}

/**
 * Pads the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s)
{
    s->bit_buf <<= s->bit_left;
    while (s->bit_left < 32) {
        /* XXX: should test end of buffer */
        *s->buf_ptr++ = s->bit_buf >> 24;
        s->bit_buf  <<= 8;
        s->bit_left  += 8;
    }
    s->bit_left = 32;
    s->bit_buf  = 0;
}

/**
 * Write up to 31 bits into a bitstream.
 * Use put_bits32 to write 32 bits.
 */
static inline void put_bits(PutBitContext *s, int n, unsigned int value)
{
    unsigned int bit_buf;
    int bit_left;

    assert(n <= 31 && value < (1U << n));

    bit_buf  = s->bit_buf;
    bit_left = s->bit_left;

    if (n < bit_left) {
        bit_buf   = (bit_buf << n) | value;
        bit_left -= n;
    }
    else {
        bit_buf <<= bit_left;
        bit_buf  |= value >> (n - bit_left);
        if (3 & (uintptr_t)s->buf_ptr) {
            s->buf_ptr[0] = bit_buf >> 24;
            s->buf_ptr[1] = bit_buf >> 16;
            s->buf_ptr[2] = bit_buf >> 8;
            s->buf_ptr[3] = bit_buf;
        }
        else
            *(uint32_t *)s->buf_ptr = be2me_32(bit_buf);
        s->buf_ptr += 4;
        bit_left   += 32 - n;
        bit_buf     = value;
    }

    s->bit_buf = bit_buf;
    s->bit_left = bit_left;
}

/**
 * Pads the bitstream with zeros up to the next byte boundary.
 */
static inline void align_put_bits(PutBitContext *s)
{
    put_bits(s, s->bit_left & 7, 0);
}

#endif /* PUT_BITS_H */
