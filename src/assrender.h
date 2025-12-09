#ifndef _ASSRENDER_H_
#define _ASSRENDER_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ass/ass.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/stat.h>
#endif

#include "avs/config.h"
#ifdef AVS_WINDOWS
#    include "avisynth_c.h"
#else
#    include <avisynth/avisynth_c.h>
#endif

#if defined(_MSC_VER)
#    define __NO_ISOCEXT
#    define __NO_INLINE__

#    define strcasecmp _stricmp
#    define atoll _atoi64
#endif

typedef struct {
    // premultiplied coefficients for integer scaled arithmetics
    int y_r, y_g, y_b;
    int u_r, u_g, u_b;
    int v_r, v_g, v_b;
    int offset_y;
    bool valid;
} ConversionMatrix;

typedef enum {
    MATRIX_NONE = 0,
    MATRIX_BT601,
    MATRIX_PC601,
    MATRIX_BT709,
    MATRIX_PC709,
    MATRIX_PC2020,
    MATRIX_BT2020,
    MATRIX_TVFCC,
    MATRIX_PCFCC,
    MATRIX_TV240M,
    MATRIX_PC240M
} matrix_type;

typedef void (* fPixel)(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
typedef void (* fMakeSubImg)(ASS_Image* img, uint8_t** sub_img, uint32_t width, int bits_per_pixel, int rgb, ConversionMatrix* m);

void col2yuv(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v, ConversionMatrix* m);
void col2rgb(uint32_t* c, uint8_t* r, uint8_t* g, uint8_t* b);

typedef struct {
    uint8_t* sub_img[4];
    uint32_t isvfr;
    ASS_Track* ass;
    ASS_Library* ass_library;
    ASS_Renderer* ass_renderer;
    int64_t* timestamp;
    ConversionMatrix mx;
    fPixel apply;
    fMakeSubImg f_make_sub_img;
    int bits_per_pixel;
    int pixelsize;
    int rgb_fullscale;
    int greyscale;
} udata;

#endif
