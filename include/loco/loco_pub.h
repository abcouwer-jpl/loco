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
 * @file        loco_pub.h
 * @date        2020-06-15
 * @author      Neil Abcouwer
 * @brief       Function declarations for LOCO (de)compression
 */


#ifndef LOCO_PUB_H
#define LOCO_PUB_H

#include <loco/loco_types_pub.h>

// ensure symbols are not mangled by C++
#ifdef __cplusplus
   extern "C" {
#endif

/**
 * @brief Check whether a given image is acceptable for loco compression
 *
 * @param image Image to be compressed.
 * @return LOCO_OK if image can be compressed, an error code otherwise
 */
I32 loco_check_image(const LocoImage *image);

/**
 * @brief Compress an image
 * @param state Pointer to a state variable for working memory.
 *              Need not be initialized.
 * @param image Image to be compressed.
 * @param result Space where compressed image will be stored, and related output data.
 * @return LOCO_OK if image was compressed, an error code otherwise
 */
I32 loco_compress(
        LocoCompressState *state,
        const LocoImage *image,
        LocoCompressedImage *result);

/**
 * @brief Decompress an image
 * @param state Pointer to a state variable for working memory.
 *              Need not be initialized.
 * @param compressed_in Compressed segements to be decompressed.
 * @param image_out
 * @param seg_data
 * @return LOCO_OK if image was decompressed, an error code otherwise
 */
I32 loco_decompress(
        LocoDecompressState * state,
        const LocoCompressedSegments * compressed_in,
        LocoImage *image_out,
        LocoSegmentData seg_data[LOCO_MAX_SEGS]);

#ifdef __cplusplus
   }
#endif

#endif // LOCO_PUB_H
