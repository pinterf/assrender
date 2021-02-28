#include "render.h"

void make_sub_img(ASS_Image* img, uint8_t** sub_img, uint32_t width, fColMat color_matrix, int bits_per_pixel, int rgb)
{
    uint8_t c1, c2, c3, a, a1;
    uint8_t* src;
    uint8_t* dstC1, * dstC2, * dstC3, * dstA;
    uint32_t i, j, dsta;

    while (img) {
        if (img->w == 0 || img->h == 0) {
            // nothing to render
            img = img->next;
            continue;
        }

        // color comes always in 8 bits
        color_matrix(&img->color, &c1, &c2, &c3);
        a1 = 255 - _a(img->color); // transparency

        src = img->bitmap;
        dstC1 = sub_img[1] + img->dst_y * width + img->dst_x;
        dstC2 = sub_img[2] + img->dst_y * width + img->dst_x;
        dstC3 = sub_img[3] + img->dst_y * width + img->dst_x;
        dstA = sub_img[0] + img->dst_y * width + img->dst_x;

        for (i = 0; i < img->h; i++) {
            for (j = 0; j < img->w; j++) {
                a = div255(src[j] * a1);
                if (a) {
                    if (dstA[j]) {
                        dsta = scale(a, 255, dstA[j]);
                        dstC1[j] = dblend(a, c1, dstA[j], dstC1[j], dsta);
                        dstC2[j] = dblend(a, c2, dstA[j], dstC2[j], dsta);
                        dstC3[j] = dblend(a, c3, dstA[j], dstC3[j], dsta);
                        dstA[j] = div255(dsta);
                    } else {
                        dstC1[j] = c1;
                        dstC2[j] = c2;
                        dstC3[j] = c3;
                        dstA[j] = a;
                    }
                }
            }

            src += img->stride;
            dstC1 += width;
            dstC2 += width;
            dstC3 += width;
            dstA += width;
        }

        img = img->next;
    }
}

void make_sub_img16(ASS_Image* img, uint8_t** sub_img0, uint32_t width, fColMat color_matrix, int bits_per_pixel, int rgb)
{
  uint16_t** sub_img = (uint16_t**)sub_img0;

  uint8_t c1_8, c2_8, c3_8;

  int c1, c2, c3;
  int a1, a;

  uint8_t* src;
  uint16_t* dstC1, * dstC2, * dstC3, * dstA;
  uint32_t i, j, dsta;

  const int fullscale = 0;

  while (img) {
    if (img->w == 0 || img->h == 0) {
      // nothing to render
      img = img->next;
      continue;
    }

    // color comes always in 8 bits
    color_matrix(&img->color, &c1_8, &c2_8, &c3_8);
    a1 = 255 - _a(img->color); // transparency, always 0..255
    if (rgb) {
      const int max_pixel_value = (1 << bits_per_pixel) - 1;
      // rgb needs full scale stretch 8->N bits
      c1 = (int)(c1_8 * max_pixel_value / 255.0f + 0.5f);
      c2 = (int)(c2_8 * max_pixel_value / 255.0f + 0.5f);
      c3 = (int)(c3_8 * max_pixel_value / 255.0f + 0.5f);
    }
    else {
      // YUV: bit shift
      c1 = c1_8 << (bits_per_pixel - 8);
      c2 = c2_8 << (bits_per_pixel - 8);
      c3 = c3_8 << (bits_per_pixel - 8);
    }

    src = img->bitmap; // always 8 bits
    // dst 1..3 is real bit depth 0 (alpha) is 8 bits
    dstC1 = sub_img[1] + img->dst_y * width + img->dst_x;
    dstC2 = sub_img[2] + img->dst_y * width + img->dst_x;
    dstC3 = sub_img[3] + img->dst_y * width + img->dst_x;
    dstA = sub_img[0] + img->dst_y * width + img->dst_x;

    for (i = 0; i < img->h; i++) {
      for (j = 0; j < img->w; j++) {
        a = div255(src[j] * a1);
        if (a) {
          if (dstA[j]) {
            // combine with existing dst transparency
            dsta = scale(a, 255, dstA[j]);
            dstC1[j] = dblend(a, c1, dstA[j], dstC1[j], dsta);
            dstC2[j] = dblend(a, c2, dstA[j], dstC2[j], dsta);
            dstC3[j] = dblend(a, c3, dstA[j], dstC3[j], dsta);
            dstA[j] = div255(dsta); // always 0..255
          }
          else {
            dstC1[j] = c1;
            dstC2[j] = c2;
            dstC3[j] = c3;
            dstA[j] = a; // always 0..255
          }
        }
      }

      src += img->stride;
      dstC1 += width;
      dstC2 += width;
      dstC3 += width;
      dstA += width;
    }

    img = img->next;
  }
}

void apply_rgba(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcA, *srcR, *srcG, *srcB, *dstA, *dstR, *dstG, *dstB;
    uint32_t i, j, k, dsta;

    srcR = sub_img[1];
    srcG = sub_img[2];
    srcB = sub_img[3];
    srcA = sub_img[0];

    // Move destination pointer to the bottom right corner of the
    // bounding box that contains the current overlay bitmap.
    // Remember that avisynth RGB bitmaps are upside down, hence we
    // need to render upside down.
    dstB = data[0] + pitch[0] * (height - 1);
    dstG = dstB + 1;
    dstR = dstB + 2;
    dstA = dstB + 3;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                k = j * 4;
                dsta = scale(srcA[j], 255, dstA[k]);
                dstR[k] = dblend(srcA[j], srcR[j], dstA[k], dstR[k], dsta);
                dstG[k] = dblend(srcA[j], srcG[j], dstA[k], dstG[k], dsta);
                dstB[k] = dblend(srcA[j], srcB[j], dstA[k], dstB[k], dsta);
                dstA[k] = div255(dsta);
            }
        }

        srcR += width;
        srcG += width;
        srcB += width;
        srcA += width;
        dstR -= pitch[0];
        dstG -= pitch[0];
        dstB -= pitch[0];
        dstA -= pitch[0];
    }
}

void apply_rgb(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcR, *srcG, *srcB, *srcA, *dstR, *dstG, *dstB;
    uint32_t i, j, k;

    srcR = sub_img[1];
    srcG = sub_img[2];
    srcB = sub_img[3];
    srcA = sub_img[0];

    // Move destination pointer to the bottom right corner of the
    // bounding box that contains the current overlay bitmap.
    // Remember that avisynth RGB bitmaps are upside down, hence we
    // need to render upside down.
    dstB = data[0] + pitch[0] * (height - 1);
    dstG = dstB + 1;
    dstR = dstB + 2;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                k = j * 3;
                dstR[k] = blend(srcA[j], srcR[j], dstR[k]);
                dstG[k] = blend(srcA[j], srcG[j], dstG[k]);
                dstB[k] = blend(srcA[j], srcB[j], dstB[k]);
            }
        }

        srcR += width;
        srcG += width;
        srcB += width;
        srcA += width;
        dstR -= pitch[0];
        dstG -= pitch[0];
        dstB -= pitch[0];
    }
}

void apply_rgb64(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint16_t* srcA, * srcR, * srcG, * srcB, * dstA, * dstR, * dstG, * dstB;
  uint32_t i, j, k, dsta;
  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  srcR = sub_img[1];
  srcG = sub_img[2];
  srcB = sub_img[3];
  srcA = sub_img[0]; // 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);

  // Move destination pointer to the bottom right corner of the
  // bounding box that contains the current overlay bitmap.
  // Remember that avisynth RGB bitmaps are upside down, hence we
  // need to render upside down.
  dstB = data[0] + pitch0 * (height - 1);
  // fixme pf: don't use 4 pointers, maybe they are not recognized to optimize out
  dstG = dstB + 1;
  dstR = dstB + 2;
  dstA = dstB + 3;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      if (srcA[j]) {
        k = j * 4;
        dsta = scale(srcA[j], 255, dstA[k]);
        dstR[k] = dblend(srcA[j], srcR[j], dstA[k], dstR[k], dsta);
        dstG[k] = dblend(srcA[j], srcG[j], dstA[k], dstG[k], dsta);
        dstB[k] = dblend(srcA[j], srcB[j], dstA[k], dstB[k], dsta);
        dstA[k] = div255(dsta);
      }
    }

    srcR += width;
    srcG += width;
    srcB += width;
    srcA += width;
    dstR -= pitch0;
    dstG -= pitch0;
    dstB -= pitch0;
    dstA -= pitch0;
  }
}

void apply_rgb48(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint16_t* srcR, * srcG, * srcB, * srcA, * dstR, * dstG, * dstB;
  uint32_t i, j, k;
  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  srcR = sub_img[1];
  srcG = sub_img[2];
  srcB = sub_img[3];
  srcA = sub_img[0]; // 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);

  // Move destination pointer to the bottom right corner of the
  // bounding box that contains the current overlay bitmap.
  // Remember that avisynth RGB bitmaps are upside down, hence we
  // need to render upside down.
  dstB = data[0] + pitch0 * (height - 1);
  // fixme pf: don't use 3 pointers, maybe they are not recognized to optimize out
  dstG = dstB + 1;
  dstR = dstB + 2;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      if (srcA[j]) {
        k = j * 3;
        dstR[k] = blend(srcA[j], srcR[j], dstR[k]);
        dstG[k] = blend(srcA[j], srcG[j], dstG[k]);
        dstB[k] = blend(srcA[j], srcB[j], dstB[k]);
      }
    }

    srcR += width;
    srcG += width;
    srcB += width;
    srcA += width;
    dstR -= pitch0;
    dstG -= pitch0;
    dstB -= pitch0;
  }
}

void apply_yuy2(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY0, *srcY1, *srcU0, *srcU1, *srcV0, *srcV1, *srcA0, *srcA1;
    uint8_t *dstY0, *dstU, *dstY1, *dstV;
    uint32_t i, j, k;

    srcY0 = sub_img[1];
    srcY1 = sub_img[1] + 1;
    srcU0 = sub_img[2];
    srcU1 = sub_img[2] + 1;
    srcV0 = sub_img[3];
    srcV1 = sub_img[3] + 1;
    srcA0 = sub_img[0];
    srcA1 = sub_img[0] + 1;

    // YUYV
    dstY0 = data[0];
    dstU  = data[0] + 1;
    dstY1 = data[0] + 2;
    dstV  = data[0] + 3;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j += 2) {
            if (srcA0[j] + srcA1[j]) {
                k = j * 2;
                dstY0[k] =  blend(srcA0[j], srcY0[j], dstY0[k]);
                dstY1[k] =  blend(srcA1[j], srcY1[j], dstY1[k]);
                dstU[k]  = blend2(srcA0[j], srcU0[j],
                                  srcA1[j], srcU1[j], dstU[k]);
                dstV[k]  = blend2(srcA0[j], srcV0[j],
                                  srcA1[j], srcV1[j], dstV[k]);
            }
        }

        srcY0 += width;
        srcY1 += width;
        srcU0 += width;
        srcU1 += width;
        srcV0 += width;
        srcV1 += width;
        srcA0 += width;
        srcA1 += width;
        dstY0 += pitch[0];
        dstU  += pitch[0];
        dstY1 += pitch[0];
        dstV  += pitch[0];
    }
}

void apply_yv12(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY00, *srcU00, *srcV00, *srcA00;
    uint8_t *srcY01, *srcU01, *srcV01, *srcA01;
    uint8_t *srcY10, *srcU10, *srcV10, *srcA10;
    uint8_t *srcY11, *srcU11, *srcV11, *srcA11;
    uint8_t *dstY00, *dstY01, *dstY10, *dstY11, *dstU, *dstV;
    uint32_t i, j, k;

    srcY00 = sub_img[1];
    srcY01 = sub_img[1] + 1;
    srcY10 = sub_img[1] + width;
    srcY11 = sub_img[1] + width + 1;
    srcU00 = sub_img[2];
    srcU01 = sub_img[2] + 1;
    srcU10 = sub_img[2] + width;
    srcU11 = sub_img[2] + width + 1;
    srcV00 = sub_img[3];
    srcV01 = sub_img[3] + 1;
    srcV10 = sub_img[3] + width;
    srcV11 = sub_img[3] + width + 1;
    srcA00 = sub_img[0];
    srcA01 = sub_img[0] + 1;
    srcA10 = sub_img[0] + width;
    srcA11 = sub_img[0] + width + 1;

    dstY00 = data[0];
    dstY01 = data[0] + 1;
    dstY10 = data[0] + pitch[0];
    dstY11 = data[0] + pitch[0] + 1;
    dstU   = data[1];
    dstV   = data[2];

    for (i = 0; i < height; i += 2) {
        for (j = 0; j < width; j += 2) {
            k = j >> 1;
            if (srcA00[j] + srcA01[j] + srcA10[j] + srcA11[j]) {
                dstY00[j] =  blend(srcA00[j], srcY00[j], dstY00[j]);
                dstY01[j] =  blend(srcA01[j], srcY01[j], dstY01[j]);
                dstY10[j] =  blend(srcA10[j], srcY10[j], dstY10[j]);
                dstY11[j] =  blend(srcA11[j], srcY11[j], dstY11[j]);
                dstU[k]   = blend4(srcA00[j], srcU00[j],
                                   srcA01[j], srcU01[j],
                                   srcA10[j], srcU10[j],
                                   srcA11[j], srcU11[j], dstU[k]);
                dstV[k]   = blend4(srcA00[j], srcV00[j],
                                   srcA01[j], srcV01[j],
                                   srcA10[j], srcV10[j],
                                   srcA11[j], srcV11[j], dstV[k]);
            }
        }

        srcY00 += width * 2;
        srcY01 += width * 2;
        srcY10 += width * 2;
        srcY11 += width * 2;
        srcU00 += width * 2;
        srcU01 += width * 2;
        srcU10 += width * 2;
        srcU11 += width * 2;
        srcV00 += width * 2;
        srcV01 += width * 2;
        srcV10 += width * 2;
        srcV11 += width * 2;
        srcA00 += width * 2;
        srcA01 += width * 2;
        srcA10 += width * 2;
        srcA11 += width * 2;
        dstY00 += pitch[0] * 2;
        dstY01 += pitch[0] * 2;
        dstY10 += pitch[0] * 2;
        dstY11 += pitch[0] * 2;
        dstU   += pitch[1];
        dstV   += pitch[1];
    }
}

void apply_yuv420(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint16_t* srcY00, * srcU00, * srcV00, * srcA00;
  uint16_t* srcY01, * srcU01, * srcV01, * srcA01;
  uint16_t* srcY10, * srcU10, * srcV10, * srcA10;
  uint16_t* srcY11, * srcU11, * srcV11, * srcA11;
  uint16_t* dstY00, * dstY01, * dstY10, * dstY11, * dstU, * dstV;
  uint32_t i, j, k;

  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  // sub_img[0] is 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);
  const int pitchUV = pitch[1] / sizeof(uint16_t);

  srcY00 = sub_img[1];
  srcY01 = sub_img[1] + 1;
  srcY10 = sub_img[1] + width;
  srcY11 = sub_img[1] + width + 1;
  srcU00 = sub_img[2];
  srcU01 = sub_img[2] + 1;
  srcU10 = sub_img[2] + width;
  srcU11 = sub_img[2] + width + 1;
  srcV00 = sub_img[3];
  srcV01 = sub_img[3] + 1;
  srcV10 = sub_img[3] + width;
  srcV11 = sub_img[3] + width + 1;
  srcA00 = sub_img[0];
  srcA01 = sub_img[0] + 1;
  srcA10 = sub_img[0] + width;
  srcA11 = sub_img[0] + width + 1;

  dstY00 = data[0];
  dstY01 = data[0] + 1;
  dstY10 = data[0] + pitch0;
  dstY11 = data[0] + pitch0 + 1;
  dstU = data[1];
  dstV = data[2];

  //4:2:0 : 2x2 Y for each chroma
  for (i = 0; i < height; i += 2) {
    for (j = 0; j < width; j += 2) {
      k = j >> 1;
      if (srcA00[j] + srcA01[j] + srcA10[j] + srcA11[j]) {
        dstY00[j] = blend(srcA00[j], srcY00[j], dstY00[j]);
        dstY01[j] = blend(srcA01[j], srcY01[j], dstY01[j]);
        dstY10[j] = blend(srcA10[j], srcY10[j], dstY10[j]);
        dstY11[j] = blend(srcA11[j], srcY11[j], dstY11[j]);
        dstU[k] = blend4(srcA00[j], srcU00[j],
          srcA01[j], srcU01[j],
          srcA10[j], srcU10[j],
          srcA11[j], srcU11[j], dstU[k]);
        dstV[k] = blend4(srcA00[j], srcV00[j],
          srcA01[j], srcV01[j],
          srcA10[j], srcV10[j],
          srcA11[j], srcV11[j], dstV[k]);
      }
    }

    srcY00 += width * 2;
    srcY01 += width * 2;
    srcY10 += width * 2;
    srcY11 += width * 2;
    srcU00 += width * 2;
    srcU01 += width * 2;
    srcU10 += width * 2;
    srcU11 += width * 2;
    srcV00 += width * 2;
    srcV01 += width * 2;
    srcV10 += width * 2;
    srcV11 += width * 2;
    srcA00 += width * 2;
    srcA01 += width * 2;
    srcA10 += width * 2;
    srcA11 += width * 2;
    dstY00 += pitch0 * 2;
    dstY01 += pitch0 * 2;
    dstY10 += pitch0 * 2;
    dstY11 += pitch0 * 2;
    dstU += pitchUV;
    dstV += pitchUV;
  }
}

void apply_yv411(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint8_t* srcY, * srcU, * srcV, * srcA;
  uint8_t* dstY0, * dstV, *dstU;

  srcY = sub_img[1];
  srcU = sub_img[2];
  srcV = sub_img[3];
  srcA = sub_img[0];

  dstY0 = data[0];
  dstU = data[1];
  dstV = data[2];

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width / 4; j++) {
      int jy = j * 4;
      if (srcA[jy] + srcA[jy + 1] + srcA[jy + 2] + srcA[jy + 3]) {
        dstY0[jy + 0] = blend(srcA[jy + 0], srcY[jy + 0], dstY0[jy + 0]);
        dstY0[jy + 1] = blend(srcA[jy + 1], srcY[jy + 1], dstY0[jy + 1]);
        dstY0[jy + 2] = blend(srcA[jy + 2], srcY[jy + 2], dstY0[jy + 2]);
        dstY0[jy + 3] = blend(srcA[jy + 3], srcY[jy + 3], dstY0[jy + 3]);
        dstU[j] = blend4(srcA[jy + 0], srcU[jy + 0],
          srcA[jy + 1], srcU[jy + 1],
          srcA[jy + 2], srcU[jy + 2],
          srcA[jy + 3], srcU[jy + 3],
          dstU[j]);
        dstV[j] = blend4(srcA[jy + 0], srcV[jy + 0],
          srcA[jy + 1], srcV[jy + 1],
          srcA[jy + 2], srcV[jy + 2],
          srcA[jy + 3], srcV[jy + 3],
          dstV[j]);
      }
    }

    srcY += width;
    srcU += width;
    srcV += width;
    srcA += width;
    dstY0 += pitch[0];
    dstU += pitch[1];
    dstV += pitch[1];
  }
}

void apply_yv16(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY0, *srcY1, *srcU0, *srcU1, *srcV0, *srcV1, *srcA0, *srcA1;
    uint8_t *dstY0, *dstU, *dstY1, *dstV;
    uint32_t i, j, k;

    srcY0 = sub_img[1];
    srcY1 = sub_img[1] + 1;
    srcU0 = sub_img[2];
    srcU1 = sub_img[2] + 1;
    srcV0 = sub_img[3];
    srcV1 = sub_img[3] + 1;
    srcA0 = sub_img[0];
    srcA1 = sub_img[0] + 1;

    dstY0 = data[0];
    dstU  = data[1];
    dstY1 = data[0] + 1;
    dstV  = data[2];

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j += 2) {
            k = j >> 1;
            if (srcA0[j] + srcA1[j]) {
                dstY0[j] =  blend(srcA0[j], srcY0[j], dstY0[j]);
                dstY1[j] =  blend(srcA1[j], srcY1[j], dstY1[j]);
                dstU[k]  = blend2(srcA0[j], srcU0[j],
                                  srcA1[j], srcU1[j], dstU[k]);
                dstV[k]  = blend2(srcA0[j], srcV0[j],
                                  srcA1[j], srcV1[j], dstV[k]);
            }
        }

        srcY0 += width;
        srcY1 += width;
        srcU0 += width;
        srcU1 += width;
        srcV0 += width;
        srcV1 += width;
        srcA0 += width;
        srcA1 += width;
        dstY0 += pitch[0];
        dstY1 += pitch[0];
        dstU  += pitch[1];
        dstV  += pitch[1];
    }
}

void apply_yuv422(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint16_t* srcY0, * srcY1, * srcU0, * srcU1, * srcV0, * srcV1, * srcA0, * srcA1;
  uint16_t* dstY0, * dstU, * dstY1, * dstV;
  uint32_t i, j, k;

  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  // sub_img[0] is 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);
  const int pitchUV = pitch[1] / sizeof(uint16_t);

  srcY0 = sub_img[1];
  srcY1 = sub_img[1] + 1;
  srcU0 = sub_img[2];
  srcU1 = sub_img[2] + 1;
  srcV0 = sub_img[3];
  srcV1 = sub_img[3] + 1;
  srcA0 = sub_img[0];
  srcA1 = sub_img[0] + 1;

  dstY0 = data[0];
  dstU = data[1];
  dstY1 = data[0] + 1;
  dstV = data[2];

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j += 2) {
      k = j >> 1;
      if (srcA0[j] + srcA1[j]) {
        dstY0[j] = blend(srcA0[j], srcY0[j], dstY0[j]);
        dstY1[j] = blend(srcA1[j], srcY1[j], dstY1[j]);
        dstU[k] = blend2(srcA0[j], srcU0[j],
          srcA1[j], srcU1[j], dstU[k]);
        dstV[k] = blend2(srcA0[j], srcV0[j],
          srcA1[j], srcV1[j], dstV[k]);
      }
    }

    srcY0 += width;
    srcY1 += width;
    srcU0 += width;
    srcU1 += width;
    srcV0 += width;
    srcV1 += width;
    srcA0 += width;
    srcA1 += width;
    dstY0 += pitch0;
    dstY1 += pitch0;
    dstU += pitchUV;
    dstV += pitchUV;
  }
}

void apply_yv24(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY, *srcU, *srcV, *srcA, *dstY, *dstU, *dstV;
    uint32_t i, j;

    srcY = sub_img[1];
    srcU = sub_img[2];
    srcV = sub_img[3];
    srcA = sub_img[0];

    dstY = data[0];
    dstU = data[1];
    dstV = data[2];

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
                dstU[j] = blend(srcA[j], srcU[j], dstU[j]);
                dstV[j] = blend(srcA[j], srcV[j], dstV[j]);
            }
        }

        srcY += width;
        srcU += width;
        srcV += width;
        srcA += width;
        dstY += pitch[0];
        dstU += pitch[0];
        dstV += pitch[0];
    }
}

void apply_yuv444(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  // planar RGB as well
  uint16_t* srcY, * srcU, * srcV, * srcA, * dstY, * dstU, * dstV;
  uint32_t i, j;

  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  // sub_img[0] is 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);

  srcY = sub_img[1];
  srcU = sub_img[2];
  srcV = sub_img[3];
  srcA = sub_img[0];

  dstY = data[0];
  dstU = data[1];
  dstV = data[2];

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      if (srcA[j]) {
        dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
        dstU[j] = blend(srcA[j], srcU[j], dstU[j]);
        dstV[j] = blend(srcA[j], srcV[j], dstV[j]);
      }
    }

    srcY += width;
    srcU += width;
    srcV += width;
    srcA += width;
    dstY += pitch0;
    dstU += pitch0;
    dstV += pitch0;
  }
}

void apply_y8(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY, *srcA, *dstY;
    uint32_t i, j;

    srcY = sub_img[1];
    srcA = sub_img[0];

    dstY = data[0];

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
            }
        }

        srcY += width;
        srcA += width;
        dstY += pitch[0];
    }
}

void apply_y(uint8_t** sub_img_8, uint8_t** data_8, uint32_t* pitch, uint32_t width, uint32_t height)
{
  uint16_t* srcY, * srcA, * dstY;
  uint32_t i, j;

  uint16_t** sub_img = (uint16_t**)sub_img_8;
  uint16_t** data = (uint16_t**)data_8;

  // sub_img[0] is 0..255 always

  const int pitch0 = pitch[0] / sizeof(uint16_t);

  srcY = sub_img[1];
  srcA = sub_img[0];

  dstY = data[0];

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      if (srcA[j]) {
        dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
      }
    }

    srcY += width;
    srcA += width;
    dstY += pitch0;
  }
}

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n)
{
    udata* ud = (udata*)p->user_data;
    ASS_Image* img;
    AVS_VideoFrame* src;

    int64_t ts;
    int changed;

    src = avs_get_frame(p->child, n);

    avs_make_writable(p->env, &src);

    if (!ud->isvfr) {
        // it’s a casting party!
        ts = (int64_t)n * (int64_t)1000 * (int64_t)p->vi.fps_denominator / (int64_t)p->vi.fps_numerator;
    } else {
        ts = ud->timestamp[n];
    }

    img = ass_render_frame(ud->ass_renderer, ud->ass, ts, &changed);

    if (img) {
        uint32_t height, width, pitch[2];
        uint8_t* data[3];

        if (avs_is_planar(&p->vi) && !ud->greyscale) {
          if (avs_is_rgb(&p->vi)) {
            // planar RGB as 444
            data[0] = avs_get_write_ptr_p(src, AVS_PLANAR_R);
            data[1] = avs_get_write_ptr_p(src, AVS_PLANAR_G);
            data[2] = avs_get_write_ptr_p(src, AVS_PLANAR_B);
            pitch[0] = avs_get_pitch(src); 
          }
          else {
            data[0] = avs_get_write_ptr_p(src, AVS_PLANAR_Y);
            data[1] = avs_get_write_ptr_p(src, AVS_PLANAR_U);
            data[2] = avs_get_write_ptr_p(src, AVS_PLANAR_V);
            pitch[0] = avs_get_pitch_p(src, AVS_PLANAR_Y);
            pitch[1] = avs_get_pitch_p(src, AVS_PLANAR_U);
          }
        }
        else {
          data[0] = avs_get_write_ptr(src);
          pitch[0] = avs_get_pitch(src);
        }

        height = p->vi.height;
        width = p->vi.width;

        if (changed) {
            memset(ud->sub_img[0], 0x00, height * width * ud->pixelsize);
            ud->f_make_sub_img(img, ud->sub_img, width, ud->color_matrix, ud->bits_per_pixel, ud->rgb_fullscale);
        }

        ud->apply(ud->sub_img, data, pitch, width, height);
    }

    return src;
}
