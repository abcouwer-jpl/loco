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
 * @file        loco_decompress.c
 * @date        2020-06-15
 * @author      Matthew Klimesh, Aaron Kiely, Neil Abcouwer
 * @brief       Function definitions for LOCO decompression
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
 * FIXME loco decompression code is not very optimized, relative to compression code.
 * This is because for Mars missions, compression was the only on-board operation.
 * Could be improved by switching funtions that are called in the encoding loop
 * to macros/inlining
 *
 */

#include <loco/loco_pub.h>
#include <loco/loco_conf_private.h>
#include <loco/loco_private.h>

// Macros for performing context-determination-related table lookups
#define G_TO_CTXT_8BIT(g)  deloco_g_table_8bit[(g) & 511]
#define G_TO_CTXT_12BIT(g)  deloco_g_table_12bit[((g)>>3) & 1023]
#define GFOUR_TO_CTXT_8BIT(g)  deloco_gfour_table_8bit[(g) & 511]
#define GFOUR_TO_CTXT_12BIT(g)  deloco_gfour_table_12bit[((g)>>3) & 1023]

// Constants
enum {
    PRANGE_12BIT = 4096,
    PRANGE_8BIT = 256,
    RMIN_12BIT = -2048,
    RMIN_8BIT = -128,
    RMAX_12BIT = 2047,
    RMAX_8BIT = 127,
};

// lookup tables
LOCO_PRIVATE const U8 deloco_g_table_8bit[512] =
    {0,0,1,1,1,2,2,2, 2,2,2,2,2,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,

     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,6,6,6,6, 6,6,6,6,5,5,5,0};

LOCO_PRIVATE const U8 deloco_gfour_table_8bit[512] =
    {0,0,0,0,0,0,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,0,0,0,0,0};

LOCO_PRIVATE const U8 deloco_g_table_12bit[1024] =
    {0,0,1,1,1,1,2,2, 2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,

     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,

     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,

     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7, 6,6,6,6,6,6,6,6, 6,6,5,5,5,5,0,0};

LOCO_PRIVATE const U8 deloco_gfour_table_12bit[1024] =
    {0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,

     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
     3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 0,0,0,0,0,0,0,0};


// function prototypes
LOCO_PRIVATE void deloco_init_bitstream(LocoDecompressState * state,
        U8 *datastart, I32 segdatabits);
LOCO_PRIVATE I32 deloco_read_int(LocoDecompressState * state, I32 *pval, I32 nbits);
LOCO_PRIVATE I32 deloco_read_bit(LocoDecompressState * deloco);
LOCO_PRIVATE I32 deloco_decompress_segment(LocoDecompressState * deloco, I32 seg);
LOCO_PRIVATE I32 deloco_decode_value(LocoDecompressState * deloco, I32 k);
LOCO_PRIVATE void deloco_find_context(LocoDecompressState * deloco,
        I32 x, I32 y, I32 xstart, I32 xend, I32 ystart);
LOCO_PRIVATE I32 deloco_g_to_ctxt(LocoDecompressState * deloco, I32 g);
LOCO_PRIVATE I32 deloco_gfour_to_ctxt(LocoDecompressState * deloco, I32 g);
LOCO_PRIVATE I32 deloco_estimate(LocoDecompressState * deloco,
        I32 x, I32 y, I32 xstart, I32 ystart);

// functions

I32 loco_decompress(
    LocoDecompressState * state,
    const LocoCompressedSegments * compressed_in,
    LocoImage *image_out,
    LocoSegmentData seg_data[LOCO_MAX_SEGS])
{
    LOCO_ASSERT(state != NULL);
    LOCO_ASSERT(image_out != NULL);
    LOCO_ASSERT(image_out->data != NULL);
    LOCO_ASSERT(compressed_in != NULL);
    LOCO_ASSERT(seg_data != NULL);

    I32 status;
    I32 have_parameters;
    I32 i;
    I32 j;
    I32 header_code;
    I32 width;
    I32 height;
    I32 cur_n_segs;
    I32 seg;
    I32 seg_decoded[LOCO_MAX_SEGS];
    I32 x;
    I32 y;
    U32 seg_flag_shortdataseg = 0x0;
    U32 seg_flag_inconsistentdata = 0x0;
    U32 seg_flag_baddata = 0x0;
    U32 seg_flag_duplicateseg = 0x0;
    U32 seg_flag_badheadercode = 0x0;
    U32 seg_flag_missingdata = 0x0;

    LOCO_COMPILE_ASSERT(LOCO_MAX_SEGS <= 32, too_many_loco_segs);

    status = 0;
    if (compressed_in->n_segs<1 || compressed_in->n_segs>LOCO_MAX_SEGS) {
        status |= DELOCO_BADNUMDATASEG_FLAG;
        LOCO_WARN2(LOCO_DECOMPRESS_BAD_NSEGS,
                "In loco_decompress(), compressed_in->n_segs (%d) "
                "was less than 1 or greater than %d.",
                compressed_in->n_segs, LOCO_MAX_SEGS);
        return status;
    }

    have_parameters = 0;

    for (i=0; i<compressed_in->n_segs; i++) {
        seg_data[i].status = 0;
        LOCO_ASSERT(compressed_in->seg_ptr[i] != NULL);
        deloco_init_bitstream(state, compressed_in->seg_ptr[i],
                compressed_in->n_bits[i]);
        (void)deloco_read_int(state, &header_code, HEADER_CODE_BITS);
        (void)deloco_read_int(state, &width, IMAGEWIDTH_BITS);
        (void)deloco_read_int(state, &height, IMAGEHEIGHT_BITS);
        (void)deloco_read_int(state, &cur_n_segs, SEGINDEX_BITS);
        (void)deloco_read_int(state, &seg, SEGINDEX_BITS);
        if (state->out_of_bits) {
            seg_data[i].status |= DELOCO_SHORTDATASEG_FLAG;
            seg_flag_shortdataseg |= (0x1<<i);
            continue;
        }
        width++;
        height++;
        cur_n_segs++;

        /* Record actual segment number (before checking whether it is valid) */
        seg_data[i].real_num = seg;

        if (have_parameters) {
            if (state->header_code!=header_code || state->image_width!=width ||
                    state->image_height!=height || state->n_segs!=cur_n_segs) {
                seg_data[i].status |= DELOCO_INCONSISTENTDATA_FLAG;
                seg_flag_inconsistentdata |= (0x1<<i);
                continue;
            }
            if (seg >= state->n_segs) {
                seg_data[i].status |= DELOCO_BADDATA_FLAG;
                seg_flag_baddata |= (0x1<<i);
                continue;
            }
            if (seg_decoded[seg]) {
                seg_data[i].status |= DELOCO_DUPLICATESEG_FLAG;
                seg_flag_duplicateseg |= (0x1<<i);
                continue;
            }
            // else everything is fine, decompress the segment
        } else { // Don't yet have the basic image parameters

            if (header_code != HEADER_CODE_FOR_12BIT &&
                    header_code != HEADER_CODE_FOR_8BIT) {
                seg_data[i].status |= DELOCO_BAD_HEADER_CODE_FLAG;
                seg_flag_badheadercode |= (0x1<<i);
                continue;
            }
            if (width<LOCO_MIN_IMAGE_WIDTH || width>LOCO_MAX_IMAGE_WIDTH ||
                    height<LOCO_MIN_IMAGE_HEIGHT || height>LOCO_MAX_IMAGE_HEIGHT ||
                    cur_n_segs<1 || cur_n_segs>LOCO_MAX_SEGS ||
                    width*height < cur_n_segs*LOCO_MIN_SEGMENT_PIXELS) {
                seg_data[i].status |= DELOCO_BADDATA_FLAG;
                seg_flag_baddata |= (0x1<<i);
                continue;
            }

            /* Record image parameters */
            have_parameters = 1;
            state->header_code = header_code;
            state->image_width = width;
            state->image_height = height;
            state->n_segs = cur_n_segs;

            image_out->bit_depth = 8*(header_code==HEADER_CODE_FOR_8BIT) +
                    12*(header_code==HEADER_CODE_FOR_12BIT);
            image_out->width = width;
            image_out->space_width = width; // FIXME revisit
            image_out->height = height;
            image_out->n_segs = cur_n_segs;

            // check output buffer large enough
            if (image_out->size_data_bytes < width*height*sizeof(LocoPixelType)) {
                status |= DELOCO_BUFTOOSMALL_FLAG;
                LOCO_WARN4(LOCO_DECOMPRESS_BUFTOOSMALL,
                        "In loco_decompress(), %d B output buffer "
                        "could not hold %d x %d x %u B image.",
                        image_out->size_data_bytes,
                        width, height, (U32)sizeof(LocoPixelType));
                return status;
            }

            for (j=0; j<cur_n_segs; j++) {
                seg_decoded[j] = 0;
            }
            state->image[0] = image_out->data;
            for (j=1; j<height; j++) {
                state->image[j] = state->image[j-1]+width;
            }
            for (y=0; y<height; y++) {
                for (x=0; x<width; x++) {
                    state->image[y][x] = 0;
                }
            }

            loco_setup_segs(state->image_width, state->image_height, state->n_segs,
                    state->seg_bound);
        }

        /* If we haven't moved on to the next data segment at this point,
           we can decompress the current segment (but first we record the segment
           bounding box). */
        seg_data[i].bound_first_line = state->seg_bound[seg].ystart;
        seg_data[i].bound_first_sample = state->seg_bound[seg].xstart;
        seg_data[i].bound_n_lines = state->seg_bound[seg].yend
                - state->seg_bound[seg].ystart;
        seg_data[i].bound_n_samples = state->seg_bound[seg].xend
                - state->seg_bound[seg].xstart;
        seg_data[i].n_missing_pixels = deloco_decompress_segment(state, seg);
        if (seg_data[i].n_missing_pixels > 0) {
            seg_data[i].status |= DELOCO_MISSING_DATA_FLAG;
            seg_flag_missingdata |= (0x1<<i);
        }
        seg_decoded[seg] = 1;
    }

    if (!have_parameters) {
        status |= DELOCO_NOGOODSEGMENTS_FLAG;
        LOCO_WARN0(LOCO_DECOMPRESS_NOGOODSEGS,
                "In loco_decompress(), no good segments found.");
    }

    if (seg_flag_shortdataseg || seg_flag_inconsistentdata
            || seg_flag_baddata || seg_flag_duplicateseg
            || seg_flag_badheadercode || seg_flag_missingdata) {
        LOCO_WARN6(LOCO_DECOMPRESS_BAD_SEGS,
                "In loco_decompress(), one or more segment issues: "
                "Short data: 0x%04x Inconsistent: 0x%04x Bad data: 0x%04x "
                "Duplicates: 0x%04x Bad header: 0x%04x Missing data: 0x%04x.",
                seg_flag_shortdataseg, seg_flag_inconsistentdata, seg_flag_baddata,
                seg_flag_duplicateseg, seg_flag_badheadercode, seg_flag_missingdata);
    }

    return status;
}


LOCO_PRIVATE void deloco_init_bitstream(
        LocoDecompressState * state, U8 *datastart, I32 segdatabits)
{
    LOCO_ASSERT(state != NULL);
    LOCO_ASSERT(datastart != NULL);
    state->data_start = datastart;
    state->seg_data_bits = segdatabits;
    state->bit_count = 0;
    state->out_of_bits = 0;
}

LOCO_PRIVATE I32 deloco_read_int(LocoDecompressState * state, I32 *pval, I32 nbits)
{
    LOCO_ASSERT(pval != NULL);

    I32 i;
    I32 bit;
    I32 ret = 1;

    *pval = 0;
    for (i=0; i<nbits; i++) {
        bit = deloco_read_bit(state);
        if (bit != -1) { // have a bit
            *pval |= bit<<i;
        } else { // ran out of bits. fail
            ret = 0;
            break;
        }
    }
    return ret;
}

LOCO_PRIVATE I32 deloco_read_bit (LocoDecompressState * deloco)
{
    I32 bit;

    if (deloco->bit_count < deloco->seg_data_bits) {
        bit = !!(deloco->data_start[deloco->bit_count/8] & (1<<(7-deloco->bit_count%8)));
        deloco->bit_count++;
        return bit;
    } else {
        deloco->out_of_bits = 1;
        return -1;
    }
}

LOCO_PRIVATE I32 deloco_decompress_segment(LocoDecompressState * deloco, I32 seg)
{
    LOCO_ASSERT(deloco != NULL);
    LOCO_ASSERT_1(0 <= seg && seg < LOCO_MAX_SEGS, seg);

    I32 xstart;
    I32 xend;
    I32 ystart;
    I32 yend;
    I32 i;
    I32 est;
    I32 bias;
    I32 residual;
    I32 n;
    I32 msum;
    I32 sum;
    I32 k;
    I32 x;
    I32 y;
    I32 value;
    I32 n_missing;

    n_missing = 0;

    /* Get segment rectangle */
    xstart = deloco->seg_bound[seg].xstart;
    xend = deloco->seg_bound[seg].xend;
    ystart = deloco->seg_bound[seg].ystart;
    yend = deloco->seg_bound[seg].yend;

    /* Initialize parameters that depend on the bit depth */
    if (deloco->header_code == HEADER_CODE_FOR_8BIT) {
        deloco->bitdepth = BITDEPTH_8BIT;
        deloco->maxn = MAXN_8BIT;
        deloco->pmax = PMAX_8BIT;
        deloco->prange = PRANGE_8BIT;
        deloco->rmin = RMIN_8BIT;
        deloco->rmax = RMAX_8BIT;
        deloco->initcc = INITCC_8BIT;
        deloco->initcms = INITCMS_8BIT;
    } else {
        deloco->bitdepth = BITDEPTH_12BIT;
        deloco->maxn = MAXN_12BIT;
        deloco->pmax = PMAX_12BIT;
        deloco->prange = PRANGE_12BIT;
        deloco->rmin = RMIN_12BIT;
        deloco->rmax = RMAX_12BIT;
        deloco->initcc = INITCC_12BIT;
        deloco->initcms = INITCMS_12BIT;
    }

    /* Initialize context statistics */
    for (i=0;i<LOCO_NCONTEXTS;i++) {
        deloco->c_count[i] = deloco->initcc;
        deloco->c_mag_sum[i] = deloco->initcms;
        deloco->c_sum[i] = 0;
        deloco->c_bias[i] = 0;
    }

    /* Read first two pixels directly */
    (void)deloco_read_int(deloco, &value, deloco->bitdepth);
    deloco->image[ystart][xstart]=value;
    (void)deloco_read_int(deloco, &value, deloco->bitdepth);
    deloco->image[ystart][xstart+1]=value;

    /*
      Main decoding loop
    */
    for (y=ystart; y<yend; y++) {
        for (x=xstart+2*(y==ystart); x<xend; x++) {
            /* Determine context */
            deloco_find_context(deloco, x, y, xstart, xend, ystart);

            /* Compute pixel estimate, incorporating the context-based bias */
            bias = deloco->c_bias[deloco->context];
            if (deloco->invert_flag) {
                est = deloco_estimate(deloco, x, y, xstart, ystart) - bias;
            } else {
                est = deloco_estimate(deloco, x, y, xstart, ystart) + bias;
            }

            /* Clip estimate to allowed range */
            if (est < 0) {
                est = 0;
            } else if (est > deloco->pmax) {
                est = deloco->pmax;
            } else {
                // in allowed range already
            }

            /* Retrieve count and sums for the context.  A mask is
               applied to the magnitude sum as a precaution, ensuring that
               the computation of k goes smoothly.  */
            n = deloco->c_count[deloco->context];
            msum = deloco->c_mag_sum[deloco->context] & MSUM_MASK;
            sum = deloco->c_sum[deloco->context];

            /* Compute Golomb-Rice parameter k */
            k=0;
            while ((n<<k)<=msum) {
                k++;
            }

            /* (Formerly limited k to a maximum here, but was not really
                necessary as long as the decoder does the same thing
                as the encoder) */

            /* Decode residual */
            residual = deloco_decode_value(deloco, k);

            /* Adjust sum and bias */
            sum += residual;
            n++;
            if (sum > 0) {
                bias++;
                sum -= n;
            } else if (sum < -n) {
                bias--;
                sum += n;
            } else {
                // no adjustment
            }

            /* Undo range-based residual remapping */
            if (residual < 0) {
                msum -= residual;
            } else {
                msum += residual;
            }

            /* Normalize sums if necessary */
            if (n==deloco->maxn) {
                n >>= 1;
                msum >>= 1;
                sum >>= 1;  /* NOTE: sign extension required (may not be portable) */
            }

            /* Store updated context information */
            deloco->c_count[deloco->context] = n;
            deloco->c_mag_sum[deloco->context] = msum;
            deloco->c_sum[deloco->context] = sum;
            deloco->c_bias[deloco->context] = bias;

            /* Recover pixel value from residual */
            if (deloco->invert_flag) {
                residual = -residual;
            }
            value = est + residual;
            if (value < 0) {
                value += deloco->prange;
            } else if (value > deloco->pmax) {
                value -= deloco->prange;
            } else {
                // no adjustment
            }

            /* Put pixel value into image */
            if (!deloco->out_of_bits) {
                deloco->image[y][x] = value;
            } else {
                n_missing++;
            }
        }
    }

    /* Report how many, if any, pixels were not decompressed due to running
       out of compressed data before decompression of the segment was finished */
    return n_missing;
}




LOCO_PRIVATE I32 deloco_decode_value(LocoDecompressState * deloco, I32 k)
{
    I32 i;
    I32 v;

    v = 0;
    for (i=0; i<k; i++) {
        v |= (deloco_read_bit(deloco) << i);
    }
    while (!deloco_read_bit(deloco) && !deloco->out_of_bits) {
        v += 01<<k;
    }

    if (v & 01) {
        v = -((v+1) >> 1);  /* A simpler expression is available */
    } else {
        v >>= 1;
    }

    return v;
}



LOCO_PRIVATE void deloco_find_context(LocoDecompressState * deloco,
        I32 x, I32 y, I32 xstart, I32 xend, I32 ystart)
{
    I32 ctxt1;
    I32 ctxt2;
    I32 ctxt3;
    I32 ctxt4;

    I32 f1;
    I32 f3;
    I32 f4;

    f1 = 0;
    f3 = 0;
    f4 = 0;

    if (y==ystart) {
        ctxt4 = deloco_gfour_to_ctxt(deloco, (I32)deloco->image[y][x-1]
                - (I32)deloco->image[y][x-2]);
        ctxt1 = 00;
        f1=1;
        ctxt2 = 00;
        ctxt3 = 00;
        f3=1;
    } else if (x==xend-1) {
        ctxt4 = deloco_gfour_to_ctxt(deloco, (I32)deloco->image[y][x-1]
                - (I32)deloco->image[y][x-2]);
        ctxt1 = 00;
        f1=1;
        ctxt2 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x]
                - (I32)deloco->image[y-1][x-1]);
        ctxt3 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x-1]
                - (I32)deloco->image[y][x-1]);
    } else if (x==xstart) {
        ctxt4 = 00;
        f4=1;
        ctxt1 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x+1]
                - (I32)deloco->image[y-1][x]);
        ctxt2 = 00;
        ctxt3 = 00;
        f3=1;
    } else if (x==xstart+1) {
        ctxt4 = 00;
        f4=1;
        ctxt1 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x+1]
                - (I32)deloco->image[y-1][x]);
        ctxt2 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x]
                - (I32)deloco->image[y-1][x-1]);
        ctxt3 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x-1]
                - (I32)deloco->image[y][x-1]);
    } else {
        ctxt4 = deloco_gfour_to_ctxt(deloco, (I32)deloco->image[y][x-1]
                - (I32)deloco->image[y][x-2]);
        ctxt1 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x+1]
                - (I32)deloco->image[y-1][x]);
        ctxt2 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x]
                - (I32)deloco->image[y-1][x-1]);
        ctxt3 = deloco_g_to_ctxt(deloco, (I32)deloco->image[y-1][x-1]
                - (I32)deloco->image[y][x-1]);
    }
    if ((ctxt2 & 04) || (!ctxt2 && ((ctxt1 & 04) || (!ctxt1 && ((ctxt3 & 04)
            || (!ctxt3 && (ctxt4 & 02))))))) {
        deloco->invert_flag = 1;
        if (ctxt2)
            ctxt2 ^= 04;
        if (ctxt1)
            ctxt1 ^= 04;
        if (ctxt3)
            ctxt3 ^= 04;
        if (ctxt4)
            ctxt4 ^= 02;
    } else {
        deloco->invert_flag = 0;
    }
    deloco->context = ctxt4 | (ctxt3 << 2) | (ctxt1 << 5) | (ctxt2 << 8);
    deloco->context |= (f4 << 1) | (f3 << 4) | (f1 << 7);
}


LOCO_PRIVATE I32 deloco_g_to_ctxt(LocoDecompressState * deloco, I32 g)
{
    return (deloco->header_code == HEADER_CODE_FOR_8BIT) ? G_TO_CTXT_8BIT(g)
                                                         : G_TO_CTXT_12BIT(g);
}


LOCO_PRIVATE I32 deloco_gfour_to_ctxt(LocoDecompressState * deloco, I32 g)
{
    return (deloco->header_code == HEADER_CODE_FOR_8BIT) ? GFOUR_TO_CTXT_8BIT(g)
                                                         : GFOUR_TO_CTXT_12BIT(g);
}


LOCO_PRIVATE I32 deloco_estimate(LocoDecompressState * deloco,
        I32 x, I32 y, I32 xstart, I32 ystart)
{
    LOCO_ASSERT(deloco != NULL);

    I32 a;
    I32 b;
    I32 c;
    I32 t;
    I32 est;

    if (x==xstart) {
        est = deloco->image[y-1][x];
    } else if (y==ystart) {
        est = deloco->image[y][x-1];
    } else {
        a = deloco->image[y-1][x];
        b = deloco->image[y][x-1];
        c = deloco->image[y-1][x-1];
        if (a>b) {
            t=a;
            a=b;
            b=t;
        }
        if (c>=b) {
            est = a;
        } else if (c<=a) {
            est = b;
        } else {
            est = a+b-c;
        }
    }

    return est;
}

