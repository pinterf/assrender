#ifndef _ASSRENDER_H_
#define _ASSRENDER_H_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <ass/ass.h>

#include <avisynth/avisynth.h>

#if defined(_MSC_VER)
#define __NO_ISOCEXT
#define __NO_INLINE__

#define strcasecmp _stricmp
#define atoll _atoi64
#endif

    struct ConversionMatrixA {
        // premultiplied coefficients for integer scaled arithmetics
        int y_r, y_g, y_b;
        int u_r, u_g, u_b;
        int v_r, v_g, v_b;
        int offset_y;
        bool valid;
    };

    enum class matrix_type {
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
    };

    typedef void (* fPixel)(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
    typedef void (* fMakeSubImg)(ASS_Image* img, uint8_t** sub_img, uint32_t width, int bits_per_pixel, int rgb, ConversionMatrixA* m);

    void col2yuv(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v, ConversionMatrixA* m);
    void col2rgb(uint32_t* c, uint8_t* r, uint8_t* g, uint8_t* b);

    struct udata {
        uint8_t* sub_img[4];
        uint32_t isvfr;
        ASS_Track* ass;
        ASS_Library* ass_library;
        ASS_Renderer* ass_renderer;
        int64_t* timestamp;
        ConversionMatrixA mx;
        fPixel apply;
        fMakeSubImg f_make_sub_img;
        int bits_per_pixel;
        int pixelsize;
        int rgb_fullscale;
        int greyscale;
    };

//template <typename T>
class assrender : public GenericVideoFilter
{

public:
    assrender(PClip _child, const char* f, const char* vfr, int h, double scale, double line_spacing, double dar, double sar,
                     int top, int bottom, int left, int right, const char* cs, int debuglevel, const char* fontdir, const char* srt_font,
                     const char* colorspace, IScriptEnvironment* env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_SERIALIZED : 0;
    }
    ~assrender() override;
};

#endif
