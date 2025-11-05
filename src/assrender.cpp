#include "assrender.h"
#include "render.h"
#include "sub.h"
#include "timecodes.h"

assrender::assrender(PClip _child, const char* f, const char* vfr, int h, double scale, double line_spacing, double dar, double sar,
                     int top, int bottom, int left, int right, const char* cs, int debuglevel, const char* fontdir, const char* srt_font,
                     const char* colorspace, IScriptEnvironment* env)
    : GenericVideoFilter(_child)
{
    char e[250];
    char* tmpcsp = static_cast<char*>(calloc(1, BUFSIZ));
    strncpy(tmpcsp, colorspace, BUFSIZ - 1);

    ASS_Hinting hinting;
    udata* data;
    ASS_Track* ass;

    /*
    no unsupported colorspace left, bitness is checked at other place
    if (0 == 1) {
        v = avs_new_value_error(
                "AssRender: unsupported colorspace");
        avs_release_clip(c);
        return v;
    }
    */

    if (f == "") {
        env->ThrowError("AssRender: no input file specified");
    }

    switch (h) {
    case 0:
        hinting = ASS_HINTING_NONE;
        break;
    case 1:
        hinting = ASS_HINTING_LIGHT;
        break;
    case 2:
        hinting = ASS_HINTING_NORMAL;
        break;
    case 3:
        hinting = ASS_HINTING_NATIVE;
        break;
    default:
        env->ThrowError("AssRender: invalid hinting mode");
    }

    //data = static_cast<udata*>(_aligned_malloc(sizeof(udata), 64));
    data = static_cast<udata*>(malloc(sizeof(udata)));

    if (!init_ass(vi.width, vi.height, scale, line_spacing,
                  hinting, dar, sar, top, bottom, left, right,
                  debuglevel, fontdir, data)) {
        env->ThrowError("AssRender: failed to initialize");
    }

    if (!strcasecmp(strrchr(f, '.'), ".srt"))
        ass = parse_srt(f, data, srt_font);
    else {
        ass = ass_read_file(data->ass_library, f, cs);
        ass_read_matrix(f, tmpcsp);
    }

    if (!ass) {
        sprintf(e, "AssRender: unable to parse '%s'", f);
        env->ThrowError(e);
    }

    data->ass = ass;

    if (vfr != "") {
        int ver;
        FILE* fh = fopen(vfr, "r");

        if (!fh) {
            sprintf(e, "AssRender: could not read timecodes file '%s'", vfr);
            env->ThrowError(e);
        }

        data->isvfr = 1;

        if (fscanf(fh, "# timecode format v%d", &ver) != 1) {
            sprintf(e, "AssRender: invalid timecodes file '%s'", vfr);
            env->ThrowError(e);
        }

        switch (ver) {
        case 1:

            if (!parse_timecodesv1(fh, vi.num_frames, data)) {
                env->ThrowError("AssRender: error parsing timecodes file");
            }

            break;
        case 2:

            if (!parse_timecodesv2(fh, vi.num_frames, data)) {
                env->ThrowError("AssRender: timecodes file had less frames than "
                                "expected");
            }

            break;
        }

        fclose(fh);
    } else {
        data->isvfr = 0;
    }

    matrix_type color_mt;

    if (vi.IsRGB()) {
      color_mt = matrix_type::MATRIX_NONE; // no RGB->YUV conversion
    } else {
        // .ASS "YCbCr Matrix" valid values are
        // "none" "tv.601" "pc.601" "tv.709" "pc.709" "tv.240m" "pc.240m" "tv.fcc" "pc.fcc"
      if (!strcasecmp(tmpcsp, "bt.709") || !strcasecmp(tmpcsp, "rec709") || !strcasecmp(tmpcsp, "tv.709")) {
        color_mt = matrix_type::MATRIX_BT709;
      }
      else if (!strcasecmp(tmpcsp, "pc.709")) {
        color_mt = matrix_type::MATRIX_PC709;
      }
      else if (!strcasecmp(tmpcsp, "bt.601") || !strcasecmp(tmpcsp, "rec601") || !strcasecmp(tmpcsp, "tv.601")) {
        color_mt = matrix_type::MATRIX_BT601;
      }
      else if (!strcasecmp(tmpcsp, "pc.601")) {
        color_mt = matrix_type::MATRIX_PC601;
      }
      else if (!strcasecmp(tmpcsp, "tv.fcc")) {
        color_mt = matrix_type::MATRIX_TVFCC;
      }
      else if (!strcasecmp(tmpcsp, "pc.fcc")) {
        color_mt = matrix_type::MATRIX_PCFCC;
      }
      else if (!strcasecmp(tmpcsp, "tv.240m")) {
        color_mt = matrix_type::MATRIX_TV240M;
      }
      else if (!strcasecmp(tmpcsp, "pc.240m")) {
        color_mt = matrix_type::MATRIX_PC240M;
      }
      else if (!strcasecmp(tmpcsp, "bt.2020") || !strcasecmp(tmpcsp, "rec2020")) {
        color_mt = matrix_type::MATRIX_BT2020;
      }
      else if (!strcasecmp(tmpcsp, "none") || !strcasecmp(tmpcsp, "guess")) {
        /* not yet
        * Theoretically only for 10 and 12 bits:
        if (fi->vi.width > 1920 || fi->vi.height > 1080)
          color_mt = MATRIX_BT2020;
        else
        */
        if (vi.width > 1280 || vi.height > 576) {
          color_mt = matrix_type::MATRIX_PC709;
        } else {
          color_mt = matrix_type::MATRIX_PC601;
        }
      }
      else {
        color_mt = matrix_type::MATRIX_BT601;
      }
    }

    FillMatrix(&data->mx, color_mt);

    const int bits_per_pixel = vi.BitsPerComponent();
    const int pixelsize = vi.ComponentSize();
    const int greyscale = vi.IsY();

    if (bits_per_pixel == 8) {
      data->f_make_sub_img = make_sub_img;
    } else if(bits_per_pixel <= 16)
      data->f_make_sub_img = make_sub_img16;
    else {
      env->ThrowError("AssRender: unsupported bit depth: 32");
    }

    switch (vi.pixel_type)
    {
    case VideoInfo::CS_YV12:
    case VideoInfo::CS_I420:
        data->apply = apply_yv12;
        break;
    case VideoInfo::CS_YUV420P10:
    case VideoInfo::CS_YUV420P12:
    case VideoInfo::CS_YUV420P14:
    case VideoInfo::CS_YUV420P16:
        data->apply = apply_yuv420;
        break;
    case VideoInfo::CS_YV16:
        data->apply = apply_yv16;
        break;
    case VideoInfo::CS_YUV422P10:
    case VideoInfo::CS_YUV422P12:
    case VideoInfo::CS_YUV422P14:
    case VideoInfo::CS_YUV422P16:
        data->apply = apply_yuv422;
        break;
    case VideoInfo::CS_YV24:
    case VideoInfo::CS_RGBP:
    case VideoInfo::CS_RGBAP:
        data->apply = apply_yv24;
        break;
    case VideoInfo::CS_YUV444P10:
    case VideoInfo::CS_YUV444P12:
    case VideoInfo::CS_YUV444P14:
    case VideoInfo::CS_YUV444P16:
    case VideoInfo::CS_RGBP10:
    case VideoInfo::CS_RGBP12:
    case VideoInfo::CS_RGBP14:
    case VideoInfo::CS_RGBP16:
    case VideoInfo::CS_RGBAP10:
    case VideoInfo::CS_RGBAP12:
    case VideoInfo::CS_RGBAP14:
    case VideoInfo::CS_RGBAP16:
        data->apply = apply_yuv444;
        break;
    case VideoInfo::CS_Y8:
        data->apply = apply_y8;
        break;
    case VideoInfo::CS_Y10:
    case VideoInfo::CS_Y12:
    case VideoInfo::CS_Y14:
    case VideoInfo::CS_Y16:
        data->apply = apply_y;
        break;
    case VideoInfo::CS_YUY2:
        data->apply = apply_yuy2;
        break;
    case VideoInfo::CS_BGR24:
        data->apply = apply_rgb;
        break;
    case VideoInfo::CS_BGR32:
        data->apply = apply_rgba;
        break;
    case VideoInfo::CS_BGR48:
        data->apply = apply_rgb48;
        break;
    case VideoInfo::CS_BGR64:
        data->apply = apply_rgb64;
        break;
    case VideoInfo::CS_YV411:
        data->apply = apply_yv411;
        break;
    default:
        env->ThrowError("AssRender: unsupported pixel type");
    }

    free(tmpcsp);

    const int buffersize = vi.width * vi.height * pixelsize;

    data->sub_img[0] = static_cast<uint8_t*>(malloc(buffersize));
    data->sub_img[1] = static_cast<uint8_t*>(malloc(buffersize));
    data->sub_img[2] = static_cast<uint8_t*>(malloc(buffersize));
    data->sub_img[3] = static_cast<uint8_t*>(malloc(buffersize));

    data->bits_per_pixel = bits_per_pixel;
    data->pixelsize = pixelsize;
    data->rgb_fullscale = vi.IsRGB(); //avs_is_rgb(&fi->vi);
    data->greyscale = greyscale;
}

assrender::~assrender()
{
    void* ud;
    ass_renderer_done(((udata*)ud)->ass_renderer);
    ass_library_done(((udata*)ud)->ass_library);
    ass_free_track(((udata*)ud)->ass);

    free(((udata*)ud)->sub_img[0]);
    free(((udata*)ud)->sub_img[1]);
    free(((udata*)ud)->sub_img[2]);
    free(((udata*)ud)->sub_img[3]);

    if (((udata*)ud)->isvfr)
        free(((udata*)ud)->timestamp);

    free(ud);
}

AVSValue AVSC_CC assrender_create(/*AVS_ScriptEnvironment* env,*/ AVSValue args,
                                   void* ud, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();

	AVSValue out = new assrender(
		clip,
		args[1].AsString(""),
		args[2].AsString(""),
		args[3].AsInt(0),
		args[4].AsFloatf(1.0),
		args[5].AsFloatf(1.0),
		args[6].AsFloatf(1.0),
		args[7].AsFloatf(1.0),
		args[8].AsInt(0),
		args[9].AsInt(0),
		args[10].AsInt(0),
		args[11].AsInt(0),
		args[12].AsString("UTF-8"),
		args[13].AsInt(0),
		args[14].AsString("C:/Windows/Fonts"),
		args[15].AsString("arial"),
		args[16].AsString(""),
		env
	);

    return out;
}

/*const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    avs_add_function(env, "assrender",
                     "c[file]s[vfr]s[hinting]i[scale]f[line_spacing]f[dar]f"
                     "[sar]f[top]i[bottom]i[left]i[right]i[charset]s"
                     "[debuglevel]i[fontdir]s[srt_font]s[colorspace]s",
                     assrender_create, 0);
    return "AssRender: draws text subtitles better and faster than ever before";
}*/
