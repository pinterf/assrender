#include "sub.h"

void ass_read_matrix(const char* f, char* csp) {
    char buf[BUFSIZ];
    FILE* fh = fopen(f, "r");

    if (!fh)
        return;

    while (fgets(buf, BUFSIZ - 1, fh) != NULL) {
        if (buf[0] == 0 || buf[0] == '\n' || buf[0] == '\r')
            continue;

        if (sscanf(buf, "YCbCr Matrix: %s", csp) == 1)
            break;

        if (sscanf(buf, "Video Colorspace: %s", csp) == 1)
          break;

        if (!strcmp(buf, "[Events]"))
            break;
    }

    fclose(fh);
}

ASS_Track* parse_srt(const char* f, udata* ud, const char* srt_font)
{
    char l[BUFSIZ], buf[BUFSIZ];
    int start[4], end[4], isn;
    ASS_Track* ass = ass_new_track(ud->ass_library);
    FILE* fh = fopen(f, "r");

    if (!fh)
        return NULL;

    sprintf(buf, "[V4+ Styles]\nStyle: Default,%s,20,&H1EFFFFFF,&H00FFFFFF,"
            "&H29000000,&H3C000000,0,0,0,0,100,100,0,0,1,1,1.2,2,10,10,"
            "12,1\n\n[Events]\n",
            srt_font);

    ass_process_data(ass, buf, BUFSIZ - 1);

    while (fgets(l, BUFSIZ - 1, fh) != NULL) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r')
            continue;

        if (sscanf(l, "%d:%d:%d,%d --> %d:%d:%d,%d", &start[0], &start[1],
                   &start[2], &start[3], &end[0], &end[1], &end[2],
                   &end[3]) == 8) {
            sprintf(buf, "Dialogue: 0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,"
                    "Default,,0,0,0,,{\\blur0.7}",
                    start[0], start[1], start[2],
                    (int)((double)start[3] / 10.0 + 0.5), end[0], end[1],
                    end[2], (int)((double)end[3] / 10.0 + 0.5));
            isn = 0;

            while (fgets(l, BUFSIZ - 1, fh) != NULL) {
                if (l[0] == 0 || l[0] == '\n' || l[0] == '\r')
                    break;

                if (l[strlen(l) - 1] == '\n' || l[strlen(l) - 1] == '\r')
                    l[strlen(l) - 1] = 0;

                if (isn) {
                    strcat(buf, "\\N");
                }

                strncat(buf, l, BUFSIZ - 1);
                isn = 1;
            }

            ass_process_data(ass, buf, BUFSIZ - 1);
        }
    }

    fclose(fh);

    return ass;
}

void msg_callback(int level, const char* fmt, va_list va, void* data)
{
    if (level > (int)data)
        return;

    fprintf(stderr, "libass: ");
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
}

int init_ass(int w, int h, double scale, double line_spacing,
             ASS_Hinting hinting, double dar, double sar, int top,
             int bottom, int left, int right, int verbosity,
             const char* fontdir, udata* ud)
{
    ASS_Renderer* ass_renderer;
    ASS_Library* ass_library = ass_library_init();

    if (!ass_library)
        return 0;

    ass_set_message_cb(ass_library, msg_callback, (void*)verbosity);
    ass_set_extract_fonts(ass_library, 0);
    ass_set_style_overrides(ass_library, 0);

    ass_renderer = ass_renderer_init(ass_library);

    if (!ass_renderer)
        return 0;

    ass_set_font_scale(ass_renderer, scale);
    ass_set_hinting(ass_renderer, hinting);
    ass_set_frame_size(ass_renderer, w, h);
    ass_set_margins(ass_renderer, top, bottom, left, right);
    ass_set_use_margins(ass_renderer, 1);

    if (line_spacing)
        ass_set_line_spacing(ass_renderer, line_spacing);

    if (dar && sar)
        ass_set_aspect_ratio(ass_renderer, dar, sar);

    if (strcmp(fontdir, ""))
        ass_set_fonts_dir(ass_library, fontdir);

    ass_set_fonts(ass_renderer, NULL, NULL, 1, NULL, 1);
    ud->ass_library = ass_library;
    ud->ass_renderer = ass_renderer;

    return 1;
}
