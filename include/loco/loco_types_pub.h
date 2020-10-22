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
 * @file        loco_types_pub.h
 * @date        2020-06-15
 * @author      Matthew Klimesh, Aaron Kiely, Neil Abcouwer
 * @brief       Public types for LOCO (de)compression
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

#ifndef LOCO_TYPES_PUB_H_
#define LOCO_TYPES_PUB_H_

// defines, or includes something that defines, fixed-width integer constants
#include <loco/loco_conf_global_types.h>

// LOCO Constants

/* Flags in the status word returned by loco_compress_8bit and loco_compress_12bit.
   For what it's worth, these have the same values as the roughly analogous flags
   in ICER.  */

/** Compression was aborted because of a problem with the compression parameters.
 *  Note that the status word is negative if and only if this flag is set.
 *  Note also that any detected parameter problem will cause
 *  compression to abort.  */
#define LOCO_ABORT_COMPRESSION_FLAG (0x80000000)
/** image_width was larger than LOCO_MAX_IMAGE_WIDTH.  */
#define LOCO_BIG_WIDTH_FLAG         (0x00000002)
/** image_height was larger than LOCO_MAX_IMAGE_HEIGHT.  */
#define LOCO_BIG_HEIGHT_FLAG        (0x00000004)
/** image_space_width was smaller than image_width.
 *  Note that there is currently no check as to whether
 *  image_space_width is otherwise reasonable.  */
#define LOCO_BAD_SPACE_WIDTH_FLAG   (0x00000008)
/** image_width was smaller than LOCO_MIN_IMAGE_WIDTH.  */
#define LOCO_SMALL_WIDTH_FLAG       (0x00000020)
/** image_height was smaller than LOCO_MIN_IMAGE_HEIGHT.  */
#define LOCO_SMALL_HEIGHT_FLAG      (0x00000040)
/** The number of pixels per segment was too small, or more precisely,
 *  image_width*image_height < n_segs*LOCO_MIN_SEGMENT_PIXELS.  */
#define LOCO_SMALL_IMAGE_FLAG       (0x00000080)
/** n_segs was not in the range [1, LOCO_MAX_SEGS].  */
#define LOCO_BAD_N_SEGS_FLAG        (0x00000100)
/** unacceptable image bit depth */
#define LOCO_BAD_BIT_DEPTH_FLAG     (0x00000200)
/** image size is smaller that space_width*height*pixel size */
#define LOCO_SMALL_BUFFER_FLAG      (0x00000400)
/** The output buffer filled up because the image was not sufficiently
 *  compressible (by LOCO, at least).
 *  This does NOT cause compression to abort, and in fact all of the data
 *  returned is valid and can be used to partially reconstruct the image.
 *  Some compressed segments may have zero length.
 *  (This flag has no approximate equivalent in ICER.) */
#define LOCO_BUFFER_FILLED_FLAG     (0x00002000)
/** Everything is OK */
#define LOCO_OK                     (0x00000000)


/* deloco status flags */

/** The number of data segments providedto the decompressor was not in the range
 * [1, LOCO_MAX_SEGS].  No decompression  was attempted.  */
#define DELOCO_BADNUMDATASEG_FLAG (0x01)
/** None of the data segments yielded valid basic image information
 * (width, height, number of segments).  */
#define DELOCO_NOGOODSEGMENTS_FLAG (0x02)
/** output buffer too small to hold image */
#define DELOCO_BUFTOOSMALL_FLAG (0x04)

/* data segment status flags */

/** There was too little data to do any useful decompression in this segment.  */
#define DELOCO_SHORTDATASEG_FLAG (0x0001)
/** The width, height, or number of segments in the header of this data segment
 *  was inconsistent with that established by a previous data segment.  */
#define DELOCO_INCONSISTENTDATA_FLAG (0x0002)
/** The segment index of this data segment duplicates that
 *  of an earlier data segment. */
#define DELOCO_DUPLICATESEG_FLAG (0x0004)
/** One or more of the basic image information values for this data segment
 *  was not in the valid range.  */
#define DELOCO_BADDATA_FLAG (0x0020)
/** The header code of the data segment was not HEADER_CODE_FOR_8BIT
 *  or HEADER_CODE_FOR_12BIT.  */
#define DELOCO_BAD_HEADER_CODE_FLAG (0x0040)
/** The decompressor ran out of data before decompression of the segment was
 *  complete.  As a result, the reconstructed image will have a gap
 *  (pixels of value 0) in this segment.  */
#define DELOCO_MISSING_DATA_FLAG (0x0080)


enum {
    LOCO_MAX_IMAGE_WIDTH = 4096,   /// Maximum allowed image width
    LOCO_MAX_IMAGE_HEIGHT = 4096,  /// Maximum allowed image height
    LOCO_MIN_IMAGE_WIDTH = 4,      /// Minimum allowed image width
    LOCO_MIN_IMAGE_HEIGHT = 4,     /// Minimum allowed image height
    LOCO_MIN_SEGMENT_PIXELS = 200, /// Minimum number of pixels in an image
    LOCO_MAX_SEGS = 32,            /// Maximum number of segments in an image
    LOCO_NCONTEXTS = 1024,         /// Max number of contexts
};

typedef I16 LocoPixelType;
typedef I32 LocoBitstreamType;

/** Uncompressed image.
 *  Passed as input to compression, or as output to decompression.
 *
 *  For compression, all values must be initialized, and the data buffer pointer must
 *  point to an allocated buffer with the image data.
 *
 *  For decompression, the data buffer pointer must
 *  point to an allocated buffer, and the size must be initialized properly.
 *  THe other metadata values are output.
 *
 *  The data buffer size is expected to be at least
 *  height * space_width * sizeof(LocoPixelType) bytes.
 *  Compression will abort otherwise. If a compressed image is found to be
 *  a larger size than the buffer will hold, decompression will fail.
 */
typedef struct {
    // The following values are input to compression, and output from decompression
    I32 width;          /// Number of columns
    I32 height;         /// Number of rows
    I32 space_width;    /// Space between the beginning of two rows, >= width
                        /// addr of row[n+1] = addr of row[n] + space_width
    I32 bit_depth;      /** Number of bits per pixel, must in [1,12]
                            Bit depths of 12 or 8 will work most effectively. */
    I32 n_segs;         /// How many segments the image will be or was broken into

    // The data pointer must be allocated, and the size initialized,
    // before either compression or decompression
    I32 size_data_bytes;        /// Size, in bytes, of the image data buffer
    LocoPixelType * data;       /// Pointer to the image data
} LocoImage;

/** Compressed image info.
 *
 *  Part of output from compression, Input to decompression.
 *
 *  For decompression, for each segment i, seg_bound[i] must point to the segment,
 *  and n_bits[i] must be the size of the segment.
 */
typedef struct {
    // The following values are output from compression, and input to decompression
    I32 n_segs;                      /// How many segments the image was broken into
    U8* seg_ptr[LOCO_MAX_SEGS+1];    /// Pointers to beginning and end of each
                                     /// segment within the data buffer
    I32 n_bits[LOCO_MAX_SEGS];       /// How many bits are in each segment
} LocoCompressedSegments;

/** Compressed image.
 *  Passed as output from compression, or as input to decompression.
 *
 *  For compression, the data buffer pointer must
 *  point to an allocated buffer, and the size must be initialized properly.
 *  The other metadata values are output.
 *
 *  For decompression, all values must be initialized, and the data buffer pointer must
 *  point to an allocated buffer with the image data.
 *
 *  If the the size of the buffer is smaller than the size produced by compression,
 *  compression will stop once the buffer is filled. A partial buffer of this
 *  nature can be decompressed, but some segments will be missing.
 *
 *  A good data buffer size is at least the size of the input image,
 *  which should suceed in all but the worst cases.
 */
typedef struct {
    // The following values are output from compression, and input to decompression
    LocoCompressedSegments segments; /// Number of, size of, and pointer to segments
    I32 compressed_size_bytes;       /// Size to which the image was compressed

    // The data pointer must be allocated, and the size initialized,
    // before either compression or decompression
    I32 size_data_bytes;            /// Size of the compressed data buffer
    LocoBitstreamType * data;       /// Pointer to the compressed data
} LocoCompressedImage;

/// Output info about a decompressed segment
typedef struct {
    I32   real_num;             /// Segment number
    I32   status;               /// Status of the segment
    I32   bound_first_line;     /// First row of the segment
    I32   bound_first_sample;   /// First column of the segment
    I32   bound_n_lines;        /// Rows in the segment
    I32   bound_n_samples;      /// Columns in the segement
    I32   n_missing_pixels;     /// Number of pixels missing from the segment
} LocoSegmentData;

/// A rectangle / segment coordinates
typedef struct {
    I32 xstart; /// left edge
    I32 xend;   /// right edge
    I32 ystart; /// top edge
    I32 yend;   /// bottom edge
} LocoRect;

/// struct for holding loco compressor state
typedef struct {
    I16 c_count[LOCO_NCONTEXTS];
    I32 c_mag_sum[LOCO_NCONTEXTS];
    I32 c_sum[LOCO_NCONTEXTS];
    I16 c_bias[LOCO_NCONTEXTS];

    LocoPixelType *image_rows[LOCO_MAX_IMAGE_HEIGHT];

    LocoRect seg_bound[LOCO_MAX_SEGS]; // FIXME conflict name

    I32 n_segs;
    I32 image_width;
    I32 image_height;

    I32 is_little_endian;

    LocoBitstreamType *p_out;
    LocoBitstreamType *p_stop;
    I32 bit_count;
    LocoBitstreamType out_word;

} LocoCompressState;

/// struct for holding loco decompressor state
typedef struct {
    I16 c_count[LOCO_NCONTEXTS];
    I32 c_mag_sum[LOCO_NCONTEXTS];
    I32 c_sum[LOCO_NCONTEXTS];
    I16 c_bias[LOCO_NCONTEXTS];

    LocoPixelType *image[LOCO_MAX_IMAGE_HEIGHT];

    LocoRect seg_bound[LOCO_MAX_SEGS];
    I32 n_segs;
    I32 image_width;
    I32 image_height;

    I32 is_little_endian;

    I32 header_code;
    I32 invert_flag;
    I32 context;

    U8 *data_start;
    I32 seg_data_bits;
    I32 bit_count;

    I32 out_of_bits;

    /* Encoder constants */
    I32 bitdepth;
    I32 maxn;
    I32 pmax;
    I32 prange;
    I32 rmin;
    I32 rmax;
    I32 initcc;
    I32 initcms;

} LocoDecompressState;

#endif // LOCO_PUB_TYPES_H
