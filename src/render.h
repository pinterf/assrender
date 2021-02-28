#ifndef _RENDER_H_
#define _RENDER_H_

#include "assrender.h"

#define _r(c) (( (c) >> 24))
#define _g(c) ((((c) >> 16) & 0xFF))
#define _b(c) ((((c) >> 8)  & 0xFF))
#define _a(c) (( (c)        & 0xFF))

#define div256(x)   (((x + 128)   >> 8))
#define div65536(x) (((x + 32768) >> 16))
#define div255(x)   ((div256(x + div256(x))))
#define div65535(x) ((div65536(x + div65536(x))))

#define blend(srcA, srcC, dstC) \
    ((div255(srcA * srcC + (255 - srcA) * dstC)))
#define blend2(src1A, src1C, src2A, src2C, dstC) \
    ((div255(((src1A * src1C + src2A * src2C + (510 - src1A - src2A) * dstC + 1) >> 1))))
#define blend4(src1A, src1C, src2A, src2C, src3A, src3C, src4A, src4C, dstC) \
    ((div255(((src1A * src1C + src2A * src2C + src3A * src3C + src4A * src4C + (1020 - src1A - src2A - src3A - src4A) * dstC + 2) >> 2))))
#define scale(srcA, srcC, dstC) \
    ((srcA * srcC + (255 - srcA) * dstC))
#define dblend(srcA, srcC, dstA, dstC, outA) \
    (((srcA * srcC * 255 + dstA * dstC * (255 - srcA) + (outA >> 1)) / outA))

static void col2rgb(uint32_t* c, uint8_t* r, uint8_t* g, uint8_t* b)
{
    *r = _r(*c);
    *g = _g(*c);
    *b = _b(*c);
}

static void col2yuv601(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v)
{
    *y = div65536(16829 * _r(*c) + 33039 * _g(*c) + 6416 * _b(*c)) + 16;
    *u = div65536(-9714 * _r(*c) - 19070 * _g(*c) + 28784 * _b(*c)) + 128;
    *v = div65536(28784 * _r(*c) - 24103 * _g(*c) - 4681 * _b(*c)) + 128;
}

static void col2yuv709(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v)
{
    *y = div65536(11966 * _r(*c) + 40254 * _g(*c) + 4064 * _b(*c)) + 16;
    *u = div65536(-6596 * _r(*c) - 22189 * _g(*c) + 28784 * _b(*c)) + 128;
    *v = div65536(28784 * _r(*c) - 26145 * _g(*c) - 2639 * _b(*c)) + 128;
}

static void col2yuv2020(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v)
{
    *y = div65536(14786 * _r(*c) + 38160 * _g(*c) + 3338 * _b(*c)) + 16;
    *u = div65536(-8038 * _r(*c) - 20746 * _g(*c) + 28784 * _b(*c)) + 128;
    *v = div65536(28784 * _r(*c) - 26469 * _g(*c) - 2315 * _b(*c)) + 128;
}

void make_sub_img(ASS_Image* img, uint8_t** sub_img, uint32_t width, fColMat color_matrix, int bits_per_pixel, int rgb);
void make_sub_img16(ASS_Image* img, uint8_t** sub_img, uint32_t width, fColMat color_matrix, int bits_per_pixel, int rgb);

void apply_rgba(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_rgb(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_rgb48(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_rgb64(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yuy2(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv12(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv16(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv24(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_y8(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yuv420(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yuv422(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yuv444(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_y(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv411(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n);

#endif
