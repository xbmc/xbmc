#ifndef __MPUTILS_H__
#define __MPUTILS_H__

#include "help_mp.h"

unsigned utf8_get_char(char **str);

void my_mp_msg(int lvl, char *lvl_str, char *fmt, ...);

#ifdef __VISUALC__
static void mp_msg(int mod, int level, const char *fmt, ...) {
	// MSVC doesn't like the # used all around for mp_msg, so it breaks va_arg
}
#else
#define mp_msg(mod, level, args...) my_mp_msg(level, #level, args)
#endif

#define MSGT_ASS 43

#define MSGL_FATAL 0
#define MSGL_ERR 1
#define MSGL_WARN 2
#define MSGL_INFO 4
#define MSGL_V 6
#define MSGL_DBG2 7

void blur(unsigned char *buffer, unsigned short *tmp2, int width, int height,
          int stride, int *m2, int r, int mwidth);

void* guess_buffer_cp(unsigned char* buffer, int buflen, char *preferred_language, char *fallback);

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#endif
