#ifndef _SUB_H_
#define _SUB_H_

#include <fontconfig/fontconfig.h>
#include "assrender.h"

void ass_read_matrix(const char* f, char* csp);

ASS_Track* parse_srt(const char* f, udata* ud, const char* srt_font);

int init_ass(int w, int h, double scale, double line_spacing,
             ASS_Hinting hinting, double dar, double sar, int top,
             int bottom, int left, int right, int verbosity,
             const char* fontdir, udata* ud);

#endif
