#ifndef _ASSRENDER_H_
#define _ASSRENDER_H_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <ass/ass.h>
#include "avisynth_c.h"

#if defined(_MSC_VER)
#define __NO_ISOCEXT
#define __NO_INLINE__

#define strcasecmp _stricmp
#define atoll _atoi64
#endif

typedef void (* fColMat)(uint32_t* col, uint8_t* c1, uint8_t* c2, uint8_t* c3);
typedef void (* fPixel)(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
typedef void (* fMakeSubImg)(ASS_Image* img, uint8_t** sub_img, uint32_t width, fColMat color_matrix, int bits_per_pixel, int rgb);

typedef struct {
    uint8_t* sub_img[4];
    uint32_t isvfr;
    ASS_Track* ass;
    ASS_Library* ass_library;
    ASS_Renderer* ass_renderer;
    int64_t* timestamp;
    fColMat color_matrix;
    fPixel apply;
    fMakeSubImg f_make_sub_img;
    int bits_per_pixel;
    int pixelsize;
    int rgb_fullscale;
    int greyscale;
} udata;

#endif
