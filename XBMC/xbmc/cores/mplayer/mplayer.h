#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "../DllLoader/dll.h"

void		mplayer_load_dll(DllLoader& dll);
void		mplayer_put_key(int code);
int			mplayer_process();
int			mplayer_open_file(const char* szFileName);
int			mplayer_close_file();
int			mplayer_init(int argc,char* argv[]);
BOOL		mplayer_HasVideo();
BOOL		mplayer_HasAudio();
void		yv12toyuy2(const unsigned char *ysrc, const unsigned char *usrc, const unsigned char *vsrc, unsigned char *dst,unsigned int width, unsigned int height,int lumStride, int chromStride, int dstStride);

__int64 mplayer_get_pts();
#ifdef __cplusplus
}
#endif
