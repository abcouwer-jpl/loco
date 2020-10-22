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
 * @file        loco_common.c
 * @date        2020-06-17
 * @author      Matthew Klimesh, Aaron Kiely, Neil Abcouwer
 * @brief       Function definitions for LOCO compression
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

#include <loco/loco_types_pub.h>
#include <loco/loco_conf_private.h>

/*
  PARTITION_INTEGER(length, n_divisions, small_step, n_small_steps)
  This macro is used only in loco_setup_segs.
  Takes integers "length" and "n_divisions" as input, and sets "small_step" and
  "n_small_steps", which must be integer lvalues.  If "length" is divided as evenly
  as possible into "n_divisions" divisions, then there will be "n_small_steps"
  divisions of size "small_step" (where n_small_steps >= 1) and the remaining
  (n_divisions - n_small_steps) divisions will be of size small_step+1.
*/
LOCO_PRIVATE void partition_integer(I32 length, I32 n_divisions,
        I32 * small_step, I32 * n_small_steps)
{
    LOCO_ASSERT(small_step != NULL);
    LOCO_ASSERT(n_small_steps != NULL);

    LOCO_ASSERT(n_divisions != 0);

    *small_step = length/n_divisions;
    *n_small_steps = (*small_step+1)*n_divisions - length;
}

/* Note:  The function deloco_setup_segs uses the PARTITION_INTEGER macro, which
   contains a division operation, and contains one additional division operation.
   It can be proven that the divisor in each case will be strictly positive;
   however, it takes some work to reach this conclusion, so it may be
   sensible to put in a check verifying each divisor is strictly positive,
   and if not, abort compression and return an error flag. */
void loco_setup_segs(I32 image_width, I32 image_height, I32 n_segs,
        LocoRect seg_rect[LOCO_MAX_SEGS])
{
    LOCO_ASSERT(seg_rect != NULL);
    LOCO_ASSERT_1(n_segs >= 0, n_segs);

    /* The parameter checking shoud guarantee that
     * wd*ht >= n_segs*LOCO_MIN_SEGMENT_PIXELS; this fact
     * can be used to show that "x_step" and "y_step" will be strictly positive */
    LOCO_ASSERT_3(image_width*image_height >= n_segs * LOCO_MIN_SEGMENT_PIXELS,
            image_width, image_height, n_segs);

    /* Determine the number of rows of segments.  This code
       guarantees that 1 <= n_rows <= n_segs */
    I32 n_rows=1;
    while(n_rows<n_segs && (n_rows+1)*n_rows*image_width < image_height*n_segs) {
        n_rows++;
    }

    /* Determine the number of rows and columns in the top region */
    I32 n_cols;
    I32 n_rows_top;
    partition_integer(n_segs, n_rows, &n_cols, &n_rows_top);  /* As noted
        above, n_rows >= 1 */

    // height in pixels of the top portion
    I32 top_height = (image_height*n_cols*n_rows_top + n_segs/2)/n_segs;
    if (top_height<n_rows_top) {
        top_height=n_rows_top; // UNREACHABLE if parameters checking passed
    }

    /* Determine the segment dimensions in the top region */
    I32 x_step;
    I32 n_x_small_steps;
    I32 y_step;
    I32 n_y_small_steps;
    partition_integer(image_width, n_cols, &x_step, &n_x_small_steps);
    partition_integer(top_height, n_rows_top, &y_step, &n_y_small_steps);
    LOCO_ASSERT_1(x_step >= 0, x_step);
    LOCO_ASSERT_1(y_step >= 0, y_step);

    // Determine the segment rectangles
    I32 seg = 0;
    I32 y = 0;
    for (I32 i=0; i<n_rows; i++) {
        if (i==n_rows_top) {  /* This actually can't happen when i==0 */
            /* Switch to bottom region parameters */
            n_cols++;
            partition_integer(image_width, n_cols, &x_step, &n_x_small_steps);
            partition_integer(image_height-top_height, n_rows-n_rows_top,
                    &y_step, &n_y_small_steps);
            LOCO_ASSERT_1(x_step >= 0, x_step);
            LOCO_ASSERT_1(y_step >= 0, y_step);
            n_y_small_steps += n_rows_top;
        }
        I32 x = 0;
        for (I32 j=0; j<n_cols; j++) {
            seg_rect[seg].ystart = y;
            seg_rect[seg].yend = y+y_step+(i>=n_y_small_steps);
            seg_rect[seg].xstart = x;
            x += x_step+(j>=n_x_small_steps);
            seg_rect[seg].xend = x;
            seg++;
        }
        y += y_step+(i>=n_y_small_steps);
    }
}
