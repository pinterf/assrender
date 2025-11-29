#ifndef _SUB_H_
#define _SUB_H_

// #include <fontconfig/fontconfig.h>
#include "assrender.h"

void ass_read_matrix(FILE* fh, char* csp);

ASS_Track* parse_srt(FILE* fh, udata* ud, const char* srt_font);

int init_ass(int w, int h, double scale, double line_spacing, ASS_Hinting hinting,
             int frame_width, int frame_height, double dar, double sar, bool set_default_storage_size,
             int top, int bottom, int left, int right, int verbosity,
             const char* fontdir, udata* ud);

#endif
