#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "../DllLoader/dll.h"

void mplayer_load_dll(DllLoader& dll);
void mplayer_put_key(int code);
int mplayer_process();
int mplayer_open_file(const char* szFileName);
int mplayer_close_file();
int mplayer_init(int argc,char* argv[]);
#ifdef __cplusplus
}
#endif
