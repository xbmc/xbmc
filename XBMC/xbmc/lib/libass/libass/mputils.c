#include "config.h"

#include "mputils.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_ENCA
#include <enca.h>
#endif

void my_mp_msg(int lvl, char *lvl_str, char *fmt, ...) {
	va_list va;
	if(lvl > MSGL_V) return;
	printf("[ass] **%s**: ", lvl_str);
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

unsigned utf8_get_char(char **str) {
  uint8_t *strp = (uint8_t *)*str;
  unsigned c = *strp++;
  unsigned mask = 0x80;
  int len = -1;
  while (c & mask) {
    mask >>= 1;
    len++;
  }
  if (len <= 0 || len > 4)
    goto no_utf8;
  c &= mask - 1;
  while ((*strp & 0xc0) == 0x80) {
    if (len-- <= 0)
      goto no_utf8;
    c = (c << 6) | (*strp++ & 0x3f);
  }
  if (len)
    goto no_utf8;
  *str = (char *)strp;
  return c;

no_utf8:
  strp = (uint8_t *)*str;
  c = *strp++;
  *str = (char *)strp;
  return c;
}

// gaussian blur
void blur(
	unsigned char *buffer,
	unsigned short *tmp2,
	int width,
	int height,
	int stride,
	int *m2,
	int r,
	int mwidth) {

    int x, y;

    unsigned char  *s = buffer;
    unsigned short *t = tmp2+1;
    for(y=0; y<height; y++){
	memset(t-1, 0, (width+1)*sizeof(short));

	for(x=0; x<r; x++){
	    const int src= s[x];
	    if(src){
		register unsigned short *dstp= t + x-r;
		int mx;
		unsigned *m3= m2 + src*mwidth;
		for(mx=r-x; mx<mwidth; mx++){
		    dstp[mx]+= m3[mx];
		}
	    }
	}

	for(; x<width-r; x++){
	    const int src= s[x];
	    if(src){
		register unsigned short *dstp= t + x-r;
		int mx;
		unsigned *m3= m2 + src*mwidth;
		for(mx=0; mx<mwidth; mx++){
		    dstp[mx]+= m3[mx];
		}
	    }
	}

	for(; x<width; x++){
	    const int src= s[x];
	    if(src){
		register unsigned short *dstp= t + x-r;
		int mx;
		const int x2= r+width -x;
		unsigned *m3= m2 + src*mwidth;
		for(mx=0; mx<x2; mx++){
		    dstp[mx]+= m3[mx];
		}
	    }
	}

	s+= stride;
	t+= width + 1;
    }

    t = tmp2;
    for(x=0; x<width; x++){
	for(y=0; y<r; y++){
	    unsigned short *srcp= t + y*(width+1) + 1;
	    int src= *srcp;
	    if(src){
		register unsigned short *dstp= srcp - 1 + width+1;
		const int src2= (src + 128)>>8;
		unsigned *m3= m2 + src2*mwidth;

		int mx;
		*srcp= 128;
		for(mx=r-1; mx<mwidth; mx++){
		    *dstp += m3[mx];
		    dstp+= width+1;
		}
	    }
	}
	for(; y<height-r; y++){
	    unsigned short *srcp= t + y*(width+1) + 1;
	    int src= *srcp;
	    if(src){
		register unsigned short *dstp= srcp - 1 - r*(width+1);
		const int src2= (src + 128)>>8;
		unsigned *m3= m2 + src2*mwidth;

		int mx;
		*srcp= 128;
		for(mx=0; mx<mwidth; mx++){
		    *dstp += m3[mx];
		    dstp+= width+1;
		}
	    }
	}
	for(; y<height; y++){
	    unsigned short *srcp= t + y*(width+1) + 1;
	    int src= *srcp;
	    if(src){
		const int y2=r+height-y;
		register unsigned short *dstp= srcp - 1 - r*(width+1);
		const int src2= (src + 128)>>8;
		unsigned *m3= m2 + src2*mwidth;

		int mx;
		*srcp= 128;
		for(mx=0; mx<y2; mx++){
		    *dstp += m3[mx];
		    dstp+= width+1;
		}
	    }
	}
	t++;
    }

    t = tmp2;
    s = buffer;
    for(y=0; y<height; y++){
	for(x=0; x<width; x++){
	    s[x]= t[x]>>8;
	}
	s+= stride;
	t+= width + 1;
    }
}

#ifdef HAVE_ENCA
void* guess_buffer_cp(unsigned char* buffer, int buflen, char *preferred_language, char *fallback)
{
    const char **languages;
    size_t langcnt;
    EncaAnalyser analyser;
    EncaEncoding encoding;
    char *detected_sub_cp = NULL;
    int i;

    languages = enca_get_languages(&langcnt);
    mp_msg(MSGT_ASS, MSGL_V, "ENCA supported languages: ");
    for (i = 0; i < langcnt; i++) {
	mp_msg(MSGT_ASS, MSGL_V, "%s ", languages[i]);
    }
    mp_msg(MSGT_ASS, MSGL_V, "\n");
    
    for (i = 0; i < langcnt; i++) {
	const char *tmp;
	
	if (strcasecmp(languages[i], preferred_language) != 0) continue;
	analyser = enca_analyser_alloc(languages[i]);
	encoding = enca_analyse_const(analyser, buffer, buflen);
	tmp = enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV);
	if (tmp && encoding.charset != ENCA_CS_UNKNOWN) {
	    detected_sub_cp = strdup(tmp);
	    mp_msg(MSGT_ASS, MSGL_INFO, "ENCA detected charset: %s\n", tmp);
	}
	enca_analyser_free(analyser);
    }
    
    free(languages);

    if (!detected_sub_cp) {
	detected_sub_cp = strdup(fallback);
	mp_msg(MSGT_ASS, MSGL_INFO, "ENCA detection failed: fallback to %s\n", fallback);
    }

    return detected_sub_cp;
}
#endif
