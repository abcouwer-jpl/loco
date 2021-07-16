#include <stdio.h>

//#define STATIC

#include "gtest/gtest.h"
#include <time.h>
#include <sys/time.h>

#include <loco/loco_pub.h>
#include <loco/loco_private.h>

extern "C"
{
#include "imageio.h"
#include "basic.h"
}

#define ABS(x) ( (x < 0) ? -(x) : (x) )

typedef enum {
    LOCO_TEST_8BIT = 8,
    LOCO_TEST_12BIT = 12,
} loco_test_type;

// struct for storing the performance of compression
typedef struct {
    int n_input_pixels; // number of pixels (whatever the size)
    int bits_per_pixel; // number of bits per pixel
    int n_input_bits; // n_pixels x bits per pixels
    int n_compressed_bits;
    double compression_ratio;
//
//    int n_truncated_bits;
//    double truncated_compression_ratio;
//
//    int inexact_pixels;
//    double rmserr;
//    int max_err;

    double compression_dur_cpu_s;
    double compression_dur_wall_s;
    double decompression_dur_cpu_s;
    double decompression_dur_wall_s;

} LocoCompStats;


double time_rgb_unoptimized = 0;
double time_rgb_optimized = 0;
double time_mono_unoptimized = 0;
double time_mono_optimized = 0;

// whether to print image buffers
bool print_images = true;

int performance_iters = 5;

int n_rows;
int n_cols;
int image_buf_bytes;
int compressed_buf_bytes;
LocoCompressState * loco_state;
LocoDecompressState * loco_dec_state;
LocoPixelType * image_truth_buf;
LocoPixelType * image_input_buf;
LocoBitstreamType * image_compressed_buf;
LocoPixelType * image_decompressed_buf;
//LocoPixelType ** image_decompressed2; // FIXME delocal this in decompress

LocoCompStats comp_stats;
bool trust_cpu_time = true;


// FIXME get rid of two times
double get_wall_time(void) {
//    struct timeval time;
//    int ret = gettimeofday(&time,NULL);
//    EXPECT_EQ(ret, 0);
//    return (double)time.tv_sec + (double)time.tv_usec * .000001;
    timespec time;
    int ret = clock_gettime(CLOCK_REALTIME, &time);
    EXPECT_EQ(ret, 0);
    return (double)time.tv_sec + (double)time.tv_nsec * .000000001;


}
double get_cpu_time(void) {
//    return (double)clock() / CLOCKS_PER_SEC;
    return get_wall_time();
}


void alloc_global_bufs(int rows, int cols)
{
    n_rows = rows;
    n_cols = cols;
    image_buf_bytes = n_rows * n_cols * sizeof(LocoPixelType);
    compressed_buf_bytes = image_buf_bytes; //n_rows * n_cols * sizeof(LocoBitstreamType);
    loco_state = (LocoCompressState*) malloc(sizeof(LocoCompressState));
    loco_dec_state = (LocoDecompressState*) malloc(sizeof(LocoDecompressState));
    image_truth_buf = (LocoPixelType*) malloc(image_buf_bytes);
    image_input_buf = (LocoPixelType*) malloc(image_buf_bytes);
    // FIXME revisit output size
    image_compressed_buf = (LocoBitstreamType*) malloc(compressed_buf_bytes);
    image_decompressed_buf = (LocoPixelType*) malloc(image_buf_bytes);
    ASSERT_TRUE(loco_state != NULL);
    ASSERT_TRUE(loco_dec_state != NULL);
    ASSERT_TRUE(image_truth_buf != NULL);
    ASSERT_TRUE(image_input_buf != NULL);
    ASSERT_TRUE(image_compressed_buf != NULL);
    ASSERT_TRUE(image_decompressed_buf != NULL);
}

void free_global_bufs(void)
{
    free(loco_state);
    free(loco_dec_state);
    free(image_truth_buf);
    free(image_input_buf);
    free(image_compressed_buf);
    free(image_decompressed_buf);
}

void make_single_color_input(LocoPixelType color)
{
    printf("make single color input: %d\n", (I32)color);
    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; col < n_cols; col++) {
            image_truth_buf[(row*n_cols)+col] = color;
            image_input_buf[(row*n_cols)+col] = color;
        }
    }
}

void make_random_input(int max_val)
{
    int color;
    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; col < n_cols; col++) {
            color = rand() % max_val;
            image_truth_buf[(row*n_cols)+col] = color;
            image_input_buf[(row*n_cols)+col] = color;
        }
    }
}

void check_error(void)
{
    // check decompressed image matches input, and input wasn't corrupted

    printf("Check error\n");
    int errors = 0;
    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; col < n_cols; col++) {
            EXPECT_EQ(image_truth_buf[(row*n_cols)+col], image_input_buf[(row*n_cols)+col]);
            EXPECT_EQ(image_truth_buf[(row*n_cols)+col], image_decompressed_buf[(row*n_cols)+col]);
            if(image_truth_buf[(row*n_cols)+col] != image_input_buf[(row*n_cols)+col] ||
            image_truth_buf[(row*n_cols)+col] != image_decompressed_buf[(row*n_cols)+col]) {
                printf("At row %d, col %d, truth = %d, input = %d, output = %d.\n",
                        row, col, image_truth_buf[(row*n_cols)+col],
                        image_input_buf[(row*n_cols)+col],
                        image_decompressed_buf[(row*n_cols)+col]);
                errors++;
            }
            if(errors > 5) { break; }
        }
        if(errors > 5) { break; }
    }
}

void test_compression(loco_test_type test_type, int n_segs = 31) {

//    LocoBitstreamType  *result_boundary[LOCO_MAX_SEGS+1] = {0};

    printf("image size: %d x %d x %d = %d, nsegs = %d\n",
            n_cols, n_rows, (I32)sizeof(LocoPixelType),
            (I32)(n_cols * n_rows * sizeof(LocoPixelType)),
            n_segs);

    I32 flags;
    LocoImage image;
    image.width = n_cols;
    image.height = n_rows;
    image.space_width = n_cols;
    image.n_segs = n_segs;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;

    LocoCompressedImage compressed;
    compressed.size_data_bytes = compressed_buf_bytes;
    compressed.data = image_compressed_buf;

    // get an accurate count of how many bits of info are going in
    comp_stats.n_input_pixels = n_cols * n_rows;
    comp_stats.bits_per_pixel = (test_type == LOCO_TEST_8BIT) ? 8 : 12;
    comp_stats.n_input_bits =
            comp_stats.n_input_pixels*comp_stats.bits_per_pixel;

    double start_cpu;
    double start_wall;
    double end_cpu;
    double end_wall;

    start_cpu = get_cpu_time();
    start_wall = get_wall_time();

    if (test_type == LOCO_TEST_8BIT) {
        image.bit_depth = 8;
        flags = loco_compress(loco_state, &image, &compressed);
    } else {
        image.bit_depth = 12;
        flags = loco_compress(loco_state, &image, &compressed);
    }

    end_cpu = get_cpu_time();
    end_wall = get_wall_time();
    if(end_cpu == start_cpu) {
        trust_cpu_time = false;
    }

    EXPECT_EQ(flags, LOCO_OK);

    comp_stats.compression_dur_cpu_s = end_cpu - start_cpu;
    comp_stats.compression_dur_wall_s = end_wall - start_wall;



    comp_stats.n_compressed_bits = compressed.compressed_size_bytes * 8;
    comp_stats.compression_ratio =
            ((double)comp_stats.n_input_bits) / (double)comp_stats.n_compressed_bits;

    printf("compressed size: %d\n", compressed.compressed_size_bytes);

    printf("input bits: %d, compressed bits: %d\n",
            comp_stats.n_input_bits, comp_stats.n_compressed_bits );

    printf("compressed size: %d, ratio: %f, "
            "time: %f s, compression rate: %f Mb/s\n",
            compressed.compressed_size_bytes,
            comp_stats.compression_ratio,
            trust_cpu_time ? comp_stats.compression_dur_cpu_s :
                             comp_stats.compression_dur_wall_s,
            1e-6 * comp_stats.n_input_bits / (
                    trust_cpu_time ? comp_stats.compression_dur_cpu_s :
                                     comp_stats.compression_dur_wall_s));

    for(int i = 0; i < compressed.segments.n_segs; i++) {
        printf("segment %d bits %d ptr %p\n",
                i, compressed.segments.n_bits[i], compressed.segments.seg_ptr[i]);
    }
    printf("last ptr %p\n",
            compressed.segments.seg_ptr[compressed.segments.n_segs]);


    LocoSegmentData seg_data[LOCO_MAX_SEGS];


    LocoImage decompressed_image;
    decompressed_image.data = image_decompressed_buf;
    decompressed_image.size_data_bytes = image_buf_bytes;

//    printf("seg addr size\n");
//    for(int i = 0; i < n_segs; i++) {
//        data_seg[i] = (U8*)result_boundary[i];
//        n_bits[i] = 8*(I32)((U8*)result_boundary[i+1]-(U8*)result_boundary[i]);
//        printf("%02d %p %05d\n", i, data_seg[i], n_bits[i]);
//    }
//    printf("addr[%d]: %p\n", n_segs, result_boundary[n_segs]);

//    printf("seg addr size\n");
//    for(int i = 0; i < n_segs; i++) {
//        data_seg[i] = (U8*)compressed.segments.seg_bound[i];
//        n_bits[i] = 8*(I32)(compressed.segments.seg_bound[i+1]-compressed.segments.seg_bound[i]);
//        printf("%02d %p %05d\n", i, data_seg[i], n_bits[i]);
//    }
//    printf("addr[%d]: %p\n", n_segs, compressed.segments.seg_bound[n_segs]);

    start_cpu = get_cpu_time();
    start_wall = get_wall_time();

    I32 ret;
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);

    end_cpu = get_cpu_time();
    end_wall = get_wall_time();

    EXPECT_EQ(ret, 0);

    if(end_cpu == start_cpu) {
        trust_cpu_time = false;
    }

    comp_stats.decompression_dur_cpu_s = end_cpu - start_cpu;
    comp_stats.decompression_dur_wall_s = end_wall - start_wall;

    printf("decompression time: %f s, rate: %f Mb/s\n",
            trust_cpu_time ? comp_stats.decompression_dur_cpu_s :
                             comp_stats.decompression_dur_wall_s,
            1e-6 * comp_stats.n_input_bits / (
                    trust_cpu_time ?
                                     comp_stats.decompression_dur_cpu_s :
                                     comp_stats.decompression_dur_wall_s));

    printf("saved info rate: %f Mb/s\n",
            1e-6 * (comp_stats.n_input_bits - comp_stats.n_compressed_bits) / (
                    trust_cpu_time ?
                            (comp_stats.compression_dur_cpu_s
                                    + comp_stats.decompression_dur_cpu_s) :
                            (comp_stats.compression_dur_wall_s
                                    + comp_stats.decompression_dur_wall_s)));


//    printf("decompressed: width: %d height: %d segs: %d bit_depth: %d\n",
//            p_width, p_height, p_n_segs, p_bit_depth);
    printf("decompressed: width: %d height: %d segs: %d bit_depth: %d\n",
            decompressed_image.width, decompressed_image.height,
            decompressed_image.n_segs, decompressed_image.bit_depth);
    EXPECT_EQ(image.width, decompressed_image.width);
    EXPECT_EQ(image.height, decompressed_image.height);
    EXPECT_EQ(image.n_segs, decompressed_image.n_segs);
    EXPECT_EQ(image.bit_depth, decompressed_image.bit_depth);

    printf("realsegnum status first_line first_sample n_lines, n_samples, n_missing\n");
    for(int i = 0; i < n_segs; i++) {
        EXPECT_EQ(seg_data[i].status, 0);
        EXPECT_EQ(seg_data[i].n_missing_pixels, 0);
        printf("%d %d %d %d %d %d %d\n",
                seg_data[i].real_num,
                seg_data[i].status,
                seg_data[i].bound_first_line,
                seg_data[i].bound_first_sample,
                seg_data[i].bound_n_lines,
                seg_data[i].bound_n_samples,
                seg_data[i].n_missing_pixels);
    }

    check_error();

//    compress();

//    check_demosaicing_error(args, rms_error_bound);
}

void test_compression_single_color(
        loco_test_type test_type, LocoPixelType color)
{
    printf("color image %u\n", color);

    make_single_color_input(color);

    test_compression(test_type);
}

void test_compression_random_image(
        loco_test_type test_type, int max_val)
{
    make_random_input(max_val);

    test_compression(test_type);
}

TEST(LocoTest, SingleColors8) {
    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FF;

    alloc_global_bufs(n_rows, n_cols);

    printf("\nall black image\n");
    test_compression_single_color(LOCO_TEST_8BIT, 0);

    printf("\nall white image\n");
    test_compression_single_color(LOCO_TEST_8BIT, max_val);

    printf("\nall grey image\n");
    test_compression_single_color(LOCO_TEST_8BIT, max_val/2);

    free_global_bufs();
}

TEST(LocoTest, SingleColors12) {
    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FFF;

    alloc_global_bufs(n_rows, n_cols);

    printf("\nall black image\n");
    test_compression_single_color(LOCO_TEST_12BIT, 0);

    printf("\nall white image\n");
    test_compression_single_color(LOCO_TEST_12BIT, max_val);

    printf("\nall grey image\n");
    test_compression_single_color(LOCO_TEST_12BIT, max_val/2);

    free_global_bufs();
}

TEST(LocoTest, Random8) {
    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FF;

    alloc_global_bufs(n_rows, n_cols);

    test_compression_random_image(LOCO_TEST_8BIT, max_val);

    free_global_bufs();
}

TEST(LocoTest, Random12) {
    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FFF;

    alloc_global_bufs(n_rows, n_cols);

    test_compression_random_image(LOCO_TEST_12BIT, max_val);

    free_global_bufs();
}

TEST(LocoTest, Misc) {
    printf("sizeof(LocoState) = %d\n", (I32)sizeof(LocoCompressState));
    printf("sizeof(DeLocoState) = %d\n", (I32)sizeof(LocoDecompressState));
    printf("BYTE_ORDER = %u, BIG_ENDIAN = %u,  LITTLE_ENDIAN = %u, "
            "BYTE_ORDER == BIG_ENDIAN = %d, BYTE_ORDER == LITTLE_ENDIAN = %d\n",
            (unsigned int)BYTE_ORDER,
            (unsigned int)BIG_ENDIAN,
            (unsigned int)LITTLE_ENDIAN,
            BYTE_ORDER == BIG_ENDIAN,
            BYTE_ORDER == LITTLE_ENDIAN);

#if BYTE_ORDER == BIG_ENDIAN
    printf("BYTE_ORDER == BIG_ENDIAN\n");
#elif BYTE_ORDER == LITTLE_ENDIAN
    printf("BYTE_ORDER == LITTLE_ENDIAN\n");
#endif

    U32 endian = 1;
    I32 is_little_endian = *((unsigned char *)(&endian));
    printf("is_little_endian = %d\n", is_little_endian);
}

TEST(LocoTest, CheckImage) {

    alloc_global_bufs(LOCO_MAX_IMAGE_HEIGHT + 4, LOCO_MAX_IMAGE_WIDTH + 4);

    I32 flags;
    LocoImage image;
    image.width = 400;
    image.height = 400;
    image.space_width = 400;
    image.n_segs = 10;
    image.bit_depth = 12;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_OK);

    image.width = LOCO_MAX_IMAGE_WIDTH;
    image.height = LOCO_MAX_IMAGE_HEIGHT;
    image.space_width = LOCO_MAX_IMAGE_WIDTH;
    image.n_segs = LOCO_MAX_SEGS;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_OK);

    image.width = LOCO_MIN_IMAGE_WIDTH;
    image.height = LOCO_MAX_IMAGE_HEIGHT;
    image.space_width = LOCO_MIN_IMAGE_WIDTH;
    image.n_segs = 4;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_OK);

    image.width = LOCO_MAX_IMAGE_WIDTH;
    image.height = LOCO_MIN_IMAGE_HEIGHT;
    image.space_width = LOCO_MAX_IMAGE_WIDTH;
    image.n_segs = 4;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_OK);

    image.width = LOCO_MAX_IMAGE_WIDTH + 1;
    image.height = LOCO_MAX_IMAGE_HEIGHT;
    image.space_width = LOCO_MAX_IMAGE_WIDTH + 1;
    image.n_segs = LOCO_MAX_SEGS;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BIG_WIDTH_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = LOCO_MAX_IMAGE_WIDTH;
    image.height = LOCO_MAX_IMAGE_HEIGHT + 1;
    image.space_width = LOCO_MAX_IMAGE_WIDTH;
    image.n_segs = LOCO_MAX_SEGS;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BIG_HEIGHT_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = LOCO_MAX_IMAGE_WIDTH;
    image.height = LOCO_MAX_IMAGE_HEIGHT;
    image.space_width = LOCO_MAX_IMAGE_WIDTH-1;
    image.n_segs = LOCO_MAX_SEGS;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BAD_SPACE_WIDTH_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = LOCO_MIN_IMAGE_WIDTH-1;
    image.height = 1000;
    image.space_width = LOCO_MIN_IMAGE_WIDTH-1;
    image.n_segs = 1;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_SMALL_WIDTH_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = 1000;
    image.height = LOCO_MIN_IMAGE_HEIGHT-1;
    image.space_width = 1000;
    image.n_segs = 1;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_SMALL_HEIGHT_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = LOCO_MIN_IMAGE_WIDTH;
    image.height = LOCO_MIN_IMAGE_HEIGHT;
    image.space_width = LOCO_MIN_IMAGE_WIDTH;
    image.n_segs = 1;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_SMALL_IMAGE_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = 400;
    image.height = 400;
    image.space_width = 400;
    image.n_segs = 0;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BAD_N_SEGS_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = 400;
    image.height = 400;
    image.space_width = 400;
    image.n_segs = LOCO_MAX_SEGS + 1;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BAD_N_SEGS_FLAG | LOCO_ABORT_COMPRESSION_FLAG);


    image.height = 400;
    image.space_width = 400;
    image.n_segs = 10;
    image.bit_depth = 13;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_BAD_BIT_DEPTH_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.width = 400;
    image.height = 400;
    image.space_width = 400;
    image.n_segs = 10;
    image.bit_depth = 12;
    image.size_data_bytes =
            image.space_width * image.height * sizeof(LocoPixelType) -1;
    flags = loco_check_image(&image);
    EXPECT_EQ(flags, LOCO_SMALL_BUFFER_FLAG | LOCO_ABORT_COMPRESSION_FLAG);


    free_global_bufs();

}

TEST(LocoTest, CompressFailures) {

    printf("compress, image too small\n");
    int n_rows = 4;
    int n_cols = 4;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FF;
    int n_segs = 31;
    LocoBitstreamType  *result_boundary[LOCO_MAX_SEGS+1] = {0};
    I32 flags;

    alloc_global_bufs(n_rows, n_cols);

    make_single_color_input(max_val/3);

    LocoImage image;
    image.width = n_cols;
    image.height = n_rows;
    image.space_width = n_cols;
    image.n_segs = n_segs;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;

    LocoCompressedImage compressed;
    compressed.size_data_bytes = compressed_buf_bytes;
    compressed.data = image_compressed_buf;

    image.bit_depth = 8;
    flags = loco_compress(loco_state, &image,  &compressed);
    EXPECT_EQ(flags, LOCO_SMALL_IMAGE_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    image.bit_depth = 12;
    flags = loco_compress(loco_state, &image,  &compressed);
    EXPECT_EQ(flags, LOCO_SMALL_IMAGE_FLAG | LOCO_ABORT_COMPRESSION_FLAG);

    free_global_bufs();




    n_rows = 400;
    n_cols = 400;
    n_pix = n_rows*n_cols;
    max_val = 0x0FF;
    n_segs = 31;
    alloc_global_bufs(n_rows, n_cols);
    make_single_color_input(max_val/3);

    image.width = n_cols;
    image.height = n_rows;
    image.space_width = n_cols;
    image.n_segs = n_segs;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;

    compressed.size_data_bytes = compressed_buf_bytes;
    compressed.data = image_compressed_buf;

    printf("compress, tiny output size\n");

    compressed.size_data_bytes = 100;
    image.bit_depth = 8;
    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_BUFFER_FILLED_FLAG);

    image.bit_depth = 12;
    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_BUFFER_FILLED_FLAG);

    printf("compress, negative output size\n");
    compressed.size_data_bytes = -compressed_buf_bytes;

    image.bit_depth = 8;
    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_BUFFER_FILLED_FLAG);

    image.bit_depth = 12;
    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_BUFFER_FILLED_FLAG);

    free_global_bufs();

}

TEST(LocoTest, DecompressFailures) {

    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FF;

    int n_segs = 10;

    alloc_global_bufs(n_rows, n_cols);

    LocoImage image;
    image.width = n_cols;
    image.height = n_rows;
    image.space_width = n_cols;
    image.n_segs = n_segs;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;
    LocoCompressedImage compressed;
    compressed.size_data_bytes = compressed_buf_bytes;
    compressed.data = image_compressed_buf;

    make_random_input(max_val);

    I32 flags;
    image.bit_depth = 12;
    flags = loco_compress(loco_state, &image,  &compressed);
    ASSERT_EQ(flags, LOCO_OK);

    LocoSegmentData seg_data[LOCO_MAX_SEGS];
    LocoImage decompressed_image;
    decompressed_image.data = image_decompressed_buf;
    decompressed_image.size_data_bytes = image_buf_bytes;

    printf("0 segments\n");
    compressed.segments.n_segs = 0;
    I32 ret;
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, DELOCO_BADNUMDATASEG_FLAG);

    printf("max+1 segments\n");
    compressed.segments.n_segs = LOCO_MAX_SEGS+1;

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, DELOCO_BADNUMDATASEG_FLAG);
    compressed.segments.n_segs = n_segs;

    printf("short data segment\n");
    I32 temp = compressed.segments.n_bits[0];
    compressed.segments.n_bits[0] = 16;
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, DELOCO_SHORTDATASEG_FLAG);
    EXPECT_EQ(seg_data[3].status, 0);
    compressed.segments.n_bits[0] = temp;

    temp = compressed.segments.n_bits[3];
    compressed.segments.n_bits[3] = 16;
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, 0);
    EXPECT_EQ(seg_data[3].status, DELOCO_SHORTDATASEG_FLAG);
    compressed.segments.n_bits[3] = temp;

    printf("corrupt widths for a segment\n");

    U8 temp2 = *(compressed.segments.seg_ptr[3]);
    *(compressed.segments.seg_ptr[3]) ^= 1; // flip one bit
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, 0);
    EXPECT_EQ(seg_data[3].status, DELOCO_INCONSISTENTDATA_FLAG);
    *(compressed.segments.seg_ptr[3]) = temp2;

    printf("corrupt segment number for a segment, higher than number\n");

    int bits;
    int bytes;

    // set the encoded segment number to 31
    for (int i=0; i < SEGINDEX_BITS; i++) {
        bits = HEADER_CODE_BITS + IMAGEWIDTH_BITS + IMAGEHEIGHT_BITS
                + SEGINDEX_BITS + i;
        bytes = bits/8;
    //    printf("bits: %d bytes: %d\n", bits, bytes);
        *(compressed.segments.seg_ptr[3] + bytes) |= 1 << (7 - bits%8); // set one bit
    }


    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, 0);
    EXPECT_EQ(seg_data[3].status, DELOCO_BADDATA_FLAG);
    EXPECT_EQ(seg_data[4].status, 0);

    printf("corrupt segment number for a segment, duplicate\n");

    // set the encoded segment number to 0
    for (int i=0; i < SEGINDEX_BITS; i++) {
        bits = HEADER_CODE_BITS + IMAGEWIDTH_BITS + IMAGEHEIGHT_BITS
                + SEGINDEX_BITS + i;
        bytes = bits/8;
    //    printf("bits: %d bytes: %d\n", bits, bytes);
        *(compressed.segments.seg_ptr[3] + bytes) &= ~(1 << (7 - bits%8)); // clear one bit
    }
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, 0);
    EXPECT_EQ(seg_data[3].status, DELOCO_DUPLICATESEG_FLAG);
    EXPECT_EQ(seg_data[4].status, 0);

    printf("corrupt 0th width code to 0\n");

    // set the encoded segment number to 0
    for (int i=0; i < IMAGEWIDTH_BITS; i++) {
        bits = HEADER_CODE_BITS + i;
        bytes = bits/8;
        *(compressed.segments.seg_ptr[0] + bytes) &= ~(1 << (7 - bits%8)); // clear one bit
    }

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, DELOCO_BADDATA_FLAG);
    EXPECT_EQ(seg_data[1].status, 0);

    printf("corrupt 0th header code\n");

    bits = 1;
    bytes = bits/8;
    *(compressed.segments.seg_ptr[0] + bytes) ^= 1 << (7 - bits%8); // flip one bit

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(seg_data[0].status, DELOCO_BAD_HEADER_CODE_FLAG);
    EXPECT_EQ(seg_data[1].status, 0);

    printf("corrupt all header codes\n");

    for (int i = 0; i < n_segs; i++) {
        bits = 1;
        bytes = bits / 8;
        *(compressed.segments.seg_ptr[i] + bytes) |= 1 << (7 - bits%8); // set one bit
    }

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, DELOCO_NOGOODSEGMENTS_FLAG);
    for (int i = 0; i < n_segs; i++) {
        EXPECT_EQ(seg_data[i].status, DELOCO_BAD_HEADER_CODE_FLAG);
    }

    printf("decompress result "
            "when compression did not have enough output space\n");

    I32 old_size = compressed.size_data_bytes;
    compressed.size_data_bytes /= 2;
    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_BUFFER_FILLED_FLAG);
    compressed.size_data_bytes = old_size;

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);

    int num_missing_data_flags = 0;
    for (int i = 0; i < n_segs; i++) {
        if(seg_data[i].status == DELOCO_MISSING_DATA_FLAG) {
            num_missing_data_flags++;
        }
        printf("data_seg_status[%d] = 0x%04x\n", i, seg_data[i].status);
    }

    EXPECT_GT(num_missing_data_flags, 0);

    printf("output buffer size too small\n");
    decompressed_image.size_data_bytes--;
    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, DELOCO_BUFTOOSMALL_FLAG);

    free_global_bufs();
}



TEST(LocoTest, Image) {

    int n_cols = 0;
    int n_rows = 0;
    uint32_t* frog_image = NULL;


    frog_image = (uint32_t*) ReadImage(&n_cols, &n_rows,
            "../test/frog.bmp", IMAGEIO_U8 | IMAGEIO_RGBA);

    ASSERT_TRUE(frog_image != NULL);

    printf("width = %d, height = %d\n", n_cols, n_rows);

    alloc_global_bufs(n_rows, n_cols);

    // make inputs

    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; col < n_cols; col++) {
            U8* u8p = (U8*)(&frog_image[row*n_cols+col]);
            U8 red = u8p[0];
            U8 green = u8p[1];
            U8 blue = u8p[2];
            LocoPixelType val = (LocoPixelType)(
                    red * .3 + green * .6 + blue * .1);
            image_truth_buf[(row * n_cols) + col] = val;
            image_input_buf[(row * n_cols) + col] = val;
        }
    }


    test_compression(LOCO_TEST_8BIT);

    // make inputs

    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; col < n_cols; col++) {
            U8* u8p = (U8*)(&frog_image[row*n_cols+col]);
            U8 red = u8p[0];
            U8 green = u8p[1];
            U8 blue = u8p[2];
            LocoPixelType val = (LocoPixelType)(16*(
                    red * .3 + green * .6 + blue * .1));
            image_truth_buf[(row * n_cols) + col] = val;
            image_input_buf[(row * n_cols) + col] = val;
        }
    }


    test_compression(LOCO_TEST_12BIT);

    free(frog_image);

    free_global_bufs();

}

TEST(LocoDeathTest, Asserts) {


    if (getenv("LOCO_DISABLE_DEATH_TESTS")) {
        printf("disabling death tests... they don't play well with valgrind.\n");
        return;
    } else {
        printf("death tests enabled.\n");
    }

    ASSERT_DEATH(loco_check_image(NULL), "image");

    int n_rows = 480;
    int n_cols = 480;
    int n_pix = n_rows*n_cols;
    unsigned int max_val = 0x0FF;

    alloc_global_bufs(n_rows, n_cols);

    make_random_input(max_val);

    I32 flags;
    LocoImage image;
    int n_segs = 31;
    image.width = n_cols;
    image.height = n_rows;
    image.space_width = n_cols;
    image.n_segs = n_segs;
    image.data = image_input_buf;
    image.size_data_bytes = image_buf_bytes;
    image.bit_depth = 12;


    LocoCompressedImage compressed;
    compressed.size_data_bytes = compressed_buf_bytes;
    compressed.data = image_compressed_buf;

    ASSERT_DEATH(
            flags = loco_compress(NULL, &image, &compressed),
            "state");
    ASSERT_DEATH(
            flags = loco_compress(loco_state, NULL, &compressed),
            "image");
    ASSERT_DEATH(
            flags = loco_compress(loco_state, &image, NULL),
            "result");

    image.data = NULL;
    ASSERT_DEATH(
            flags = loco_compress(loco_state, &image, &compressed),
            "data");
    image.data = image_input_buf;

    compressed.data = NULL;
    ASSERT_DEATH(
            flags = loco_compress(loco_state, &image, &compressed),
            "data");
    compressed.data = image_compressed_buf;

    flags = loco_compress(loco_state, &image, &compressed);
    EXPECT_EQ(flags, LOCO_OK);
    printf("compressed size: %d\n", compressed.compressed_size_bytes);


    LocoSegmentData seg_data[LOCO_MAX_SEGS];
    LocoImage decompressed_image;
    decompressed_image.data = image_decompressed_buf;
    decompressed_image.size_data_bytes = image_buf_bytes;

    I32 ret;

    ASSERT_DEATH(
            ret = loco_decompress(NULL, &compressed.segments,
                        &decompressed_image, seg_data),
                        "state");
    ASSERT_DEATH(
            ret = loco_decompress(loco_dec_state, NULL,
                        &decompressed_image, seg_data),
                        "compressed_in");
    ASSERT_DEATH(
            loco_decompress(loco_dec_state, &compressed.segments,
                        NULL, seg_data),
                        "image_out");

    ASSERT_DEATH(
            loco_decompress(loco_dec_state, &compressed.segments,
                        &decompressed_image, NULL),
                        "seg_data");

    decompressed_image.data = NULL;
    ASSERT_DEATH(
            loco_decompress(loco_dec_state, &compressed.segments,
                        &decompressed_image, seg_data),
                        "data");
    decompressed_image.data = image_decompressed_buf;

    ret = loco_decompress(loco_dec_state, &compressed.segments,
            &decompressed_image, seg_data);
    EXPECT_EQ(ret, 0);

    free_global_bufs();
    printf("death tests complete.\n");
}

#if LOCO_ENABLE_PERFORMANCE_TESTS

TEST(LocoTest, Performance) {

    static int num_test_images = 24;
    static int num_tests = 3;
    // which images to save to file
    int save_img = 13;
    bool save_all = false;

    // FIXME kodak set is 8 bit images, so using them for 12 bit testing is not accurate
    // find a 12 bit data set
    loco_test_type test_types[num_tests] = {LOCO_TEST_8BIT, LOCO_TEST_8BIT, LOCO_TEST_8BIT    };
    int n_segs[num_tests] =            { 1, 16, 31};
    LocoCompStats test_stats[num_test_images][num_tests];

    int n_cols = 0;
    int n_rows = 0;
    int n_pix;
    int ret;
    int num_completed_imgs = 0;

    double test_start_time_wall = get_wall_time();
    double timeout = 1200;
    bool timed_out = false;

    for (int i_img = 0; i_img < num_test_images; i_img++) {
        uint32_t* image = NULL;
        int adjusted_img_num = i_img + 1; /* 1-indexed*/
        char image_name[100];
        memset(image_name, 0, 100);
        sprintf(image_name, "corpus/kodak/bmp/IMG%04d.bmp",
                adjusted_img_num /* 1-indexed*/);
        printf("\nimage name: %s\n", image_name);
        image = (uint32_t*) ReadImage(&n_cols, &n_rows,
                image_name, IMAGEIO_U8 | IMAGEIO_RGBA);
        ASSERT_TRUE(image != NULL);
        printf("width = %d, height = %d\n", n_cols, n_rows);
        alloc_global_bufs(n_rows, n_cols);

        // make inputs
        for (int row = 0; row < n_rows; row++) {
            for (int col = 0; col < n_cols; col++) {
                U8* u8p = (U8*) (&image[row * n_cols + col]);
                U8 red = u8p[0];
                U8 green = u8p[1];
                U8 blue = u8p[2];
                LocoPixelType val = (LocoPixelType) (
                        red * .3 + green * .6 + blue * .1);
                image_truth_buf[(row * n_cols) + col] = val;
            }
        }
        free(image);

        for (int i_test = 0; i_test < num_tests; i_test++) {
            n_pix = n_rows * n_cols;
            printf("\ntest with type %d, %d segments\n", test_types[i_test],
                    n_segs[i_test]);

            for (int row = 0; row < n_rows; row++) {
                for (int col = 0; col < n_cols; col++) {
                    image_input_buf[(row * n_cols) + col] = image_truth_buf[(row
                            * n_cols) + col];
                }
            }

            test_compression(test_types[i_test], n_segs[i_test]);

            test_stats[i_img][i_test] = comp_stats;

            if (get_wall_time() - test_start_time_wall > timeout) {
                timed_out = true;
                printf("timed out!\n");
                break;
            }

        } // for each test

        free_global_bufs();

        if (timed_out) {
            break;
        }

        // if not timed out, the image has been tested
        num_completed_imgs = i_img + 1;

    } // for each img

    // write results to csv
    FILE * csv_ptr = fopen("performance_loco.csv", "w");
    ASSERT_TRUE(csv_ptr != NULL);
    int printlen = 0;

    printlen += fprintf(csv_ptr, "LOCO compression results\n");
    printlen +=
            fprintf(csv_ptr,
                    "Timing results are averaged over %d test images.\n",
                    /*trust_cpu_time ? "CPU" : "wall",*/ num_completed_imgs);

    printlen += fprintf(csv_ptr,
            "type, segments, "
                    "compression ratio, "
                    "space saving ratio, "
                    "compression duration (s), "
                    "decompression duration (s), "
                    "compression rate (Mb/s), "
                    "decompression rate (Mb/s), "
                    "bits saved rate (Mb/s)\n");

    for (int i_test = 0; i_test < num_tests; i_test++) {
        double avg_compression_ratio = 0;
        double avg_rms_err = 0;
        double avg_max_err = 0;
        double avg_compression_time = 0;
        double avg_compression_rate = 0;
        double avg_decompression_time = 0;
        double avg_decompression_rate = 0;
        double avg_bit_save_rate = 0;

        for (int i_img = 0; i_img < num_completed_imgs; i_img++) {
            avg_compression_ratio +=
                    test_stats[i_img][i_test].compression_ratio;
            double compression_time =
                    trust_cpu_time ?
                            test_stats[i_img][i_test].compression_dur_cpu_s :
                            test_stats[i_img][i_test].compression_dur_wall_s;
            avg_compression_time += compression_time;
            avg_compression_rate +=
                    test_stats[i_img][i_test].n_input_bits
                    / compression_time;
            double decompression_time =
                    trust_cpu_time ?
                            test_stats[i_img][i_test].decompression_dur_cpu_s :
                            test_stats[i_img][i_test].decompression_dur_wall_s;
            avg_decompression_time += decompression_time;
            avg_decompression_rate +=
                    test_stats[i_img][i_test].n_input_bits
                    / decompression_time;

            avg_bit_save_rate +=
                    (test_stats[i_img][i_test].n_input_bits
                     - test_stats[i_img][i_test].n_compressed_bits)
                    / (compression_time + decompression_time);
        }
        avg_compression_ratio /= num_completed_imgs;
        avg_rms_err /= num_completed_imgs;
        avg_max_err /= num_completed_imgs;
        avg_compression_time /= num_completed_imgs;
        avg_compression_rate /= num_completed_imgs;
        avg_decompression_time /= num_completed_imgs;
        avg_decompression_rate /= num_completed_imgs;
        avg_bit_save_rate /= num_completed_imgs;

        printlen += fprintf(csv_ptr,
                "%d, %d, %f, %f, %f, %f, %f, %f, %f\n",
                test_types[i_test],
                n_segs[i_test],
                avg_compression_ratio,
                1 - 1 / avg_compression_ratio,
                avg_compression_time,
                avg_decompression_time,
                1e-6 * avg_compression_rate,
                1e-6 * avg_decompression_rate,
                1e-6 * avg_bit_save_rate);
    }
    printlen += fprintf(csv_ptr,"\n");
    fclose(csv_ptr); // close the csv

    // save processor info as well
    system("printf \"processor info:\n\" >> performance_loco.csv");
    system("lscpu >> performance_loco.csv");

    // regurgitate csv back to stdout
    csv_ptr = fopen("performance_loco.csv", "r");
    ASSERT_TRUE(csv_ptr != NULL);

    char buff[1000];
    char *gets_ret;
    while (fgets(buff, 1000, csv_ptr) != NULL) {
        puts(buff);
    }
    fclose(csv_ptr);
}

#endif
