/***********************************************************************
 * Copyright 2003, 2020 by the California Institute of Technology
 * ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file        loco_private.h
 * @date        2020-06-15
 * @author      Matthew Klimesh, Aaron Kiely, Neil Abcouwer
 * @brief       Private types for LOCO (de)compression
 *
 * This software is based on the LOCO-I algorithm which is
 *   described in:
 *
 *   Marcelo J. Weinberger, Gadiel Seroussi and Guillermo Sapiro,
 *     "LOCO-I: A Low Complexity, Context-Based, Lossless Image
 *     Compression Algorithm", in Proc. Data Compression Conference
 *     (DCC '96), pp. 140-149, 1996.
 *
 * This implementation originates with Matt Klimesh and Aaron Kiely's
 * implementation for MER, and was modified by Neil Abcouwer.
 *
 * This implementation lacks the run length encoding ("embedded
 * alphabet extension") of LOCO-I.  There are other differences as well.
 *
 */


#ifndef LOCO_PRIVATE_H_
#define LOCO_PRIVATE_H_

#include <loco/loco_types_pub.h>

// Constants
#define MSUM_MASK (0x3fffffff)
#define OUT_OF_RANGE_MASK_12BIT (0xfffff000)
#define OUT_OF_RANGE_MASK_8BIT (0xffffff00)

#define PIXEL_MASK_12BIT (0x00000fff)
#define PIXEL_MASK_8BIT (0x000000ff)

#define RESIDUAL_SIGN_BIT_12BIT (0x800)
#define RESIDUAL_SIGN_BIT_8BIT (0x80)

enum {
    /* Bit field widths in LOCO's personal segment headers. */
    HEADER_CODE_BITS    = 2,
    IMAGEWIDTH_BITS    = 12,  /* Must accommodate LOCO_MAX_IMAGE_WIDTH-1 */
    IMAGEHEIGHT_BITS   = 12,  /* Must accommodate LOCO_MAX_IMAGE_HEIGHT-1 */
    SEGINDEX_BITS       = 5,  /* Must accommodate LOCO_MAX_SEGS-1 */

    HEADER_CODE_FOR_12BIT = 01,
    HEADER_CODE_FOR_8BIT = 00,

    BITDEPTH_12BIT = 12,
    BITDEPTH_8BIT = 8,

    MAXN_12BIT = 64,
    MAXN_8BIT = 128,

    PMAX_12BIT = 4095,
    PMAX_8BIT = 255,

    INITCC_12BIT = 1,
    INITCC_8BIT = 2,

    INITCMS_12BIT = 24,
    INITCMS_8BIT = 12,
};

LOCO_COMPILE_ASSERT(LOCO_MAX_IMAGE_WIDTH  <= 1 << IMAGEWIDTH_BITS,
        not_enough_width_bits);
LOCO_COMPILE_ASSERT(LOCO_MAX_IMAGE_HEIGHT <= 1 << IMAGEHEIGHT_BITS,
        not_enough_height_bits);
LOCO_COMPILE_ASSERT(LOCO_MAX_SEGS <= 1 << SEGINDEX_BITS,
        not_enough_segment_bits);

/* The following allows the compressor to produce the same output on
   a little-endian (e.g. Intel) machine.
   This little-endian correction assumes LocoBitstreamType
   is an I32.  If encoding speed is critical on a little-endian machine,
   consider re-writing the bit packing (and unpacking) instead of using this
   correction.

   Note that the LITTLE_ENDIAN option does not do anything with the byte order
   in the image pixels; in some circumstances these would need to be fixed
   before calling the compressor (e.g. possibly if the image is read directly
   from a file to the image array). */

#define FIX_WORD(i, little) ((little) ? ((((i)>>24) & 0x000000ff) | (((i)>>8) & 0x0000ff00) \
                          | (((i)<<8) & 0x00ff0000) | (((i)<<24) & 0xff000000)) \
                          : (i))

// FIX_WORD assumes BitstreamType is 32 bit
LOCO_COMPILE_ASSERT(sizeof(LocoBitstreamType) == sizeof(I32), bad_bitstream_size);

// segment image into n_segs segments. used in both compress and decompress
void loco_setup_segs(I32 image_width, I32 image_height, I32 n_segs,
        LocoRect seg_rect[LOCO_MAX_SEGS]);

#endif
