#include "assrender.h"
#include "render.h"
#include "sub.h"
#include "timecodes.h"

static char* read_file_bytes(FILE* fp, size_t* bufsize)
{
    int res;
    long sz;
    long bytes_read;
    char* buf;
    res = fseek(fp, 0, SEEK_END);
    if (res == -1) {
        fclose(fp);
        return 0;
    }

    sz = ftell(fp);
    rewind(fp);

    buf = sz < SIZE_MAX ? malloc(sz + 1) : NULL;
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    bytes_read = 0;
    do {
        res = fread(buf + bytes_read, 1, sz - bytes_read, fp);
        if (res <= 0) {
            fclose(fp);
            free(buf);
            return 0;
        }
        bytes_read += res;
    } while (sz - bytes_read > 0);
    buf[sz] = '\0';
    fclose(fp);

    if (bufsize)
        *bufsize = sz;
    return buf;
}

static const char* detect_bom(const char* buf, const size_t bufsize) {
    if (bufsize >= 4) {
        if (!strncmp(buf, "\xef\xbb\xbf", 3))
            return "UTF-8";
        if (!strncmp(buf, "\x00\x00\xfe\xff", 4))
            return "UTF-32BE";
        if (!strncmp(buf, "\xff\xfe\x00\x00", 4))
            return "UTF-32LE";
        if (!strncmp(buf, "\xfe\xff", 2))
            return "UTF-16BE";
        if (!strncmp(buf, "\xff\xfe", 2))
            return "UTF-16LE";
    }
    return "UTF-8";
}

#ifdef _WIN32
static wchar_t *utf8_to_utf16le(const char *data) {
    const int out_size = MultiByteToWideChar(CP_UTF8, 0, data, -1, NULL, 0);
    wchar_t *out = malloc(out_size * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, data, -1, out, out_size);
    return out;
}
#endif

bool file_exists(const char *path) {
#ifdef _WIN32
    wchar_t *path_utf16le = utf8_to_utf16le(path);
    DWORD dwAttrib = GetFileAttributesW(path_utf16le);
    free(path_utf16le);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return false;
    }
    return S_ISREG(path_stat.st_mode);
#endif
}

static FILE* open_utf8_filename(const char* f, const char* m)
{
    if (!file_exists(f)) return NULL;
#ifdef _WIN32
    wchar_t* file_name = utf8_to_utf16le(f);
    wchar_t* mode = utf8_to_utf16le(m);
    FILE* fp = _wfopen(file_name, mode);
    free(file_name);
    free(mode);
    return fp;
#else
    return fopen(f, m);
#endif
}

void AVSC_CC assrender_destroy(void* ud, AVS_ScriptEnvironment* env)
{
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

static matrix_type matrix_from_frame_props(
    AVS_ScriptEnvironment *env,
    AVS_Clip              *clip,
    const AVS_VideoInfo   *vi,
    int                   *ok_out)
{
    *ok_out = 0;

    if (!avs_has_video(vi) || !avs_is_yuv(vi))
        return MATRIX_NONE;

    AVS_VideoFrame *frame = avs_get_frame(clip, 0);
    if (!frame)
        return MATRIX_NONE;

    int err;
    const AVS_Map *props = avs_get_frame_props_ro(env, frame);
    if (!props) {
        avs_release_video_frame(frame);
        return MATRIX_NONE;
    }

    // _Matrix: ITU-T H.265 Table E.5 / AviSynth+ AVS_MATRIX_* index
    int matrix = avs_prop_get_int(env, props, "_Matrix", 0, &err);
    if (err) {
        avs_release_video_frame(frame);
        return MATRIX_NONE;
    }

    // _ColorRange: 0 = full (PC), 1 = limited (TV)
    int range = avs_prop_get_int(env, props, "_ColorRange", 0, &err);
    if (err)
        range = 1; // default to TV/limited for YUV if not present

    avs_release_video_frame(frame);

    const int full = (range == 0);

    /*
     * _Matrix values (H.265 / AviSynth+):
     *   0: RGB
     *   1: BT.709
     *   2: unspecified
     *   4: FCC (BT.470M)
     *   5: BT.470BG / Rec.601
     *   6: ST.170M (also 601)
     *   7: ST.240M
     *   9: BT.2020 (non-constant luminance)
     *  10: BT.2020 (constant luminance)
     *  (others ignored here)
     */

    matrix_type mt = MATRIX_NONE;

    switch (matrix) {
    case 1: // BT.709
        mt = full ? MATRIX_PC709  : MATRIX_BT709;
        break;

    case 5: // BT.470BG / Rec.601
    case 6: // ST.170M (treated as 601)
        mt = full ? MATRIX_PC601  : MATRIX_BT601;
        break;

    case 4: // FCC
        mt = full ? MATRIX_PCFCC  : MATRIX_TVFCC;
        break;

    case 7: // 240M
        mt = full ? MATRIX_PC240M : MATRIX_TV240M;
        break;

    case 9: // BT.2020 (ncl)
    case 10:// BT.2020 (cl)
        mt = full ? MATRIX_PC2020 : MATRIX_BT2020;
        break;

    default:
        // 0 (RGB), 2 (unspec), 8 (YCgCo), 12/13/14… → we don’t trust it
        mt = MATRIX_NONE;
        break;
    }

    if (mt != MATRIX_NONE)
        *ok_out = 1;

    return mt;
}

AVS_Value AVSC_CC assrender_create(AVS_ScriptEnvironment* env, AVS_Value args,
                                   void* ud)
{
    AVS_Value v;
    AVS_FilterInfo* fi;
    AVS_Clip* c = avs_new_c_filter(env, &fi, avs_array_elt(args, 0), 1);
    const AVS_VideoInfo *vi = &fi->vi;

    const char* f = avs_as_string(avs_array_elt(args, 1));
    const char* vfr = avs_as_string(avs_array_elt(args, 2));
    int hint = avs_is_int(avs_array_elt(args, 3)) ?
               avs_as_int(avs_array_elt(args, 3)) : 0;
    double scale = avs_is_float(avs_array_elt(args, 4)) ?
                   avs_as_float(avs_array_elt(args, 4)) : 1.0;
    double line_spacing = avs_is_float(avs_array_elt(args, 5)) ?
                          avs_as_float(avs_array_elt(args, 5)) : 0;
    int frame_width = avs_is_int(avs_array_elt(args, 6)) ?
                      avs_as_int(avs_array_elt(args, 6)) : 0;
    int frame_height = avs_is_int(avs_array_elt(args, 7)) ?
                       avs_as_int(avs_array_elt(args, 7)) : 0;
    double dar = avs_is_float(avs_array_elt(args, 8)) ?
                 avs_as_float(avs_array_elt(args, 8)) : 0;
    double sar = avs_is_float(avs_array_elt(args, 9)) ?
                 avs_as_float(avs_array_elt(args, 9)) : 0;
    bool defstorage = avs_as_bool(avs_array_elt(args, 10)) ?
                      avs_as_bool(avs_array_elt(args, 10)) : true;
    int top = avs_is_int(avs_array_elt(args, 11)) ?
              avs_as_int(avs_array_elt(args, 11)) : 0;
    int bottom = avs_is_int(avs_array_elt(args, 12)) ?
                 avs_as_int(avs_array_elt(args, 12)) : 0;
    int left = avs_is_int(avs_array_elt(args, 13)) ?
               avs_as_int(avs_array_elt(args, 13)) : 0;
    int right = avs_is_int(avs_array_elt(args, 14)) ?
                avs_as_int(avs_array_elt(args, 14)) : 0;
    /* Allow charset auto-detection via BOM if omitted */
    const char* cs = avs_as_string(avs_array_elt(args, 15));
    int debuglevel = avs_is_int(avs_array_elt(args, 16)) ?
                     avs_as_int(avs_array_elt(args, 16)) : 0;
    const char* fontdir = avs_as_string(avs_array_elt(args, 17)) ?
#ifdef AVS_WINDOWS
        avs_as_string(avs_array_elt(args, 17)) : "C:/Windows/Fonts";
#else
        avs_as_string(avs_array_elt(args, 17)) : "/usr/share/fonts";
#endif
    const char* srt_font = avs_as_string(avs_array_elt(args, 18)) ?
                           avs_as_string(avs_array_elt(args, 18)) : "sans-serif";
    const char* colorspace = avs_as_string(avs_array_elt(args, 19)) ?
                             avs_as_string(avs_array_elt(args, 19)) : "";

    char* tmpcsp = calloc(1, BUFSIZ);
    strncpy(tmpcsp, colorspace, BUFSIZ - 1);

    ASS_Hinting hinting;
    udata* data;
    ASS_Track* ass;

    FILE* fp;

    /*
    no unsupported colorspace left, bitness is checked at other place
    if (0 == 1) {
        v = avs_new_value_error(
                "AssRender: unsupported colorspace");
        avs_release_clip(c);
        return v;
    }
    */

    if (!f) {
        v = avs_new_value_error("AssRender: no input file specified");
        avs_release_clip(c);
        return v;
    }

    switch (hint) {
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
        v = avs_new_value_error("AssRender: invalid hinting mode");
        avs_release_clip(c);
        return v;
    }

    data = malloc(sizeof(udata));

    if (!init_ass(fi->vi.width, fi->vi.height, scale, line_spacing, hinting,
                  frame_width, frame_height, dar, sar, defstorage,
                  top, bottom, left, right, debuglevel,
                  fontdir, data)) {
        v = avs_new_value_error("AssRender: failed to initialize");
        avs_release_clip(c);
        return v;
    }

    /* Improved Unicode / BOM / file validation loading logic */
    if (!strcasecmp(strrchr(f, '.'), ".srt")) {
        FILE* fp = open_utf8_filename(f, "r");
        if (!fp) {
            const char* e = avs_sprintf(env, "AssRender: input file '%s' does not exist or is not a regular file", f);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }
        ass = parse_srt(fp, data, srt_font);
    } else {
        size_t bufsize = 0;
        char* buf;

        fp = open_utf8_filename(f, "rb");
        if (!fp) {
            const char* e = avs_sprintf(env, "AssRender: input file '%s' does not exist or is not a regular file", f);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }

        buf = read_file_bytes(fp, &bufsize);
        if (!buf) {
            const char* e = avs_sprintf(env, "AssRender: unable to read '%s'", f);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }

        if (cs == NULL)
            cs = detect_bom(buf, bufsize);

        ass = ass_read_memory(data->ass_library, buf, bufsize, (char*)cs);
        free(buf);

        fp = open_utf8_filename(f, "r");
        if (fp) {
            ass_read_matrix(fp, tmpcsp);
            fclose(fp);
        }
    }

    if (!ass) {
        const char* e = avs_sprintf(env, "AssRender: unable to parse '%s'", f);
        v = avs_new_value_error(e);
        avs_release_clip(c);
        return v;
    }

    data->ass = ass;

    if (vfr) {
        int ver;
        FILE* fh = open_utf8_filename(vfr, "r");

        if (!fh) {
            const char* e = avs_sprintf(env, "AssRender: could not read timecodes file '%s'", vfr);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }

        data->isvfr = 1;

        if (fscanf(fh, "# timecode format v%d", &ver) != 1) {
            const char* e = avs_sprintf(env, "AssRender: invalid timecodes file '%s'", vfr);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }

        switch (ver) {
        case 1:

            if (!parse_timecodesv1(fh, fi->vi.num_frames, data)) {
                v = avs_new_value_error(
                        "AssRender: error parsing timecodes file");
                avs_release_clip(c);
                return v;
            }

            break;
        case 2:

            if (!parse_timecodesv2(fh, fi->vi.num_frames, data)) {
                v = avs_new_value_error(
                        "AssRender: timecodes file had less frames than "
                        "expected");
                avs_release_clip(c);
                return v;
            }

            break;
        }

        fclose(fh);
    } else {
        data->isvfr = 0;
    }

    matrix_type color_mt;

    if (avs_is_rgb(&fi->vi)) {
      color_mt = MATRIX_NONE; // no RGB->YUV conversion
    } else {
        // .ASS "YCbCr Matrix" valid values are
        // "none" "tv.601" "pc.601" "tv.709" "pc.709" "tv.240m" "pc.240m" "tv.fcc" "pc.fcc"
      if (!strcasecmp(tmpcsp, "bt.709") || !strcasecmp(tmpcsp, "rec709") || !strcasecmp(tmpcsp, "tv.709")) {
        color_mt = MATRIX_BT709;
      }
      else if (!strcasecmp(tmpcsp, "pc.709")) {
        color_mt = MATRIX_PC709;
      }
      else if (!strcasecmp(tmpcsp, "bt.601") || !strcasecmp(tmpcsp, "rec601") || !strcasecmp(tmpcsp, "tv.601")) {
        color_mt = MATRIX_BT601;
      }
      else if (!strcasecmp(tmpcsp, "pc.601")) {
        color_mt = MATRIX_PC601;
      }
      else if (!strcasecmp(tmpcsp, "tv.fcc")) {
        color_mt = MATRIX_TVFCC;
      }
      else if (!strcasecmp(tmpcsp, "pc.fcc")) {
        color_mt = MATRIX_PCFCC;
      }
      else if (!strcasecmp(tmpcsp, "tv.240m")) {
        color_mt = MATRIX_TV240M;
      }
      else if (!strcasecmp(tmpcsp, "pc.240m")) {
        color_mt = MATRIX_PC240M;
      }
      else if (!strcasecmp(tmpcsp, "bt.2020") || !strcasecmp(tmpcsp, "rec2020")) {
        color_mt = MATRIX_BT2020;
      }
      else if (!strcasecmp(tmpcsp, "none") || !strcasecmp(tmpcsp, "guess")) {
        int mt_from_props_ok = 0;
        matrix_type mt_from_props = MATRIX_NONE;

        if (avs_get_version(c) >= 9) {
            mt_from_props = matrix_from_frame_props(env, c, vi, &mt_from_props_ok);
        }

        if (mt_from_props_ok && mt_from_props != MATRIX_NONE) {
            color_mt = mt_from_props;
        } else {
            /* not yet
            * Theoretically only for 10 and 12 bits:
            if (fi->vi.width > 1920 || fi->vi.height > 1080)
            color_mt = MATRIX_BT2020;
            else
            */
            if (vi->width > 1024 || vi->height > 576) {
                color_mt = MATRIX_BT709;
            } else {
                color_mt = MATRIX_BT601;
            }
        }
      }
      else {
        color_mt = MATRIX_BT601;
      }
    }

    FillMatrix(&data->mx, color_mt);

#ifdef FOR_AVISYNTH_26_ONLY
    const int bits_per_pixel = 8;
    const int pixelsize = 1;
    const int greyscale = avs_is_y8(&fi->vi);
#else
    const int bits_per_pixel = avs_bits_per_component(&fi->vi);
    const int pixelsize = avs_component_size(&fi->vi);
    const int greyscale = avs_is_y(&fi->vi);
#endif

    if (bits_per_pixel == 8)
      data->f_make_sub_img = make_sub_img;
    else if(bits_per_pixel <= 16)
      data->f_make_sub_img = make_sub_img16;
    else {
      v = avs_new_value_error("AssRender: unsupported bit depth: 32");
      avs_release_clip(c);
      return v;
    }

    switch (fi->vi.pixel_type)
    {
    case AVS_CS_YV12:
    case AVS_CS_I420:
        data->apply = apply_yv12;
        break;
    case AVS_CS_YUV420P10:
    case AVS_CS_YUV420P12:
    case AVS_CS_YUV420P14:
    case AVS_CS_YUV420P16:
        data->apply = apply_yuv420;
        break;
    case AVS_CS_YV16:
        data->apply = apply_yv16;
        break;
    case AVS_CS_YUV422P10:
    case AVS_CS_YUV422P12:
    case AVS_CS_YUV422P14:
    case AVS_CS_YUV422P16:
        data->apply = apply_yuv422;
        break;
    case AVS_CS_YV24:
    case AVS_CS_RGBP:
    case AVS_CS_RGBAP:
        data->apply = apply_yv24;
        break;
    case AVS_CS_YUV444P10:
    case AVS_CS_YUV444P12:
    case AVS_CS_YUV444P14:
    case AVS_CS_YUV444P16:
    case AVS_CS_RGBP10:
    case AVS_CS_RGBP12:
    case AVS_CS_RGBP14:
    case AVS_CS_RGBP16:
    case AVS_CS_RGBAP10:
    case AVS_CS_RGBAP12:
    case AVS_CS_RGBAP14:
    case AVS_CS_RGBAP16:
        data->apply = apply_yuv444;
        break;
    case AVS_CS_Y8:
        data->apply = apply_y8;
        break;
    case AVS_CS_Y10:
    case AVS_CS_Y12:
    case AVS_CS_Y14:
    case AVS_CS_Y16:
        data->apply = apply_y;
        break;
    case AVS_CS_YUY2:
        data->apply = apply_yuy2;
        break;
    case AVS_CS_BGR24:
        data->apply = apply_rgb;
        break;
    case AVS_CS_BGR32:
        data->apply = apply_rgba;
        break;
    case AVS_CS_BGR48:
        data->apply = apply_rgb48;
        break;
    case AVS_CS_BGR64:
        data->apply = apply_rgb64;
        break;
    case AVS_CS_YV411:
        data->apply = apply_yv411;
        break;
    default:
        v = avs_new_value_error("AssRender: unsupported pixel type");
        avs_release_clip(c);
        return v;
    }

    free(tmpcsp);

    const int buffersize = fi->vi.width * fi->vi.height * pixelsize;

    data->sub_img[0] = malloc(buffersize);
    data->sub_img[1] = malloc(buffersize);
    data->sub_img[2] = malloc(buffersize);
    data->sub_img[3] = malloc(buffersize);

    data->bits_per_pixel = bits_per_pixel;
    data->pixelsize = pixelsize;
    data->rgb_fullscale = avs_is_rgb(&fi->vi);
    data->greyscale = greyscale;

    fi->user_data = data;

    fi->get_frame = assrender_get_frame;

    v = avs_new_value_clip(c);
    avs_release_clip(c);

    avs_at_exit(env, assrender_destroy, data);

    return v;
}

const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    avs_add_function(env, "assrender",
                     "c[file]s"
                     "[vfr]s"
                     "[hinting]i"
                     "[scale]f"
                     "[line_spacing]f"
                     "[frame_width]i"
                     "[frame_height]i"
                     "[dar]f"
                     "[sar]f"
                     "[set_default_storage_size]b"
                     "[top]i"
                     "[bottom]i"
                     "[left]i"
                     "[right]i"
                     "[charset]s"
                     "[debuglevel]i"
                     "[fontdir]s"
                     "[srt_font]s"
                     "[colorspace]s",
                     assrender_create, 0);
    return "AssRender: draws text subtitles better and faster than ever before";
}
