#ifndef __FILELIB_H__
#define __FILELIB_H__

#include "types.h"
#ifdef _XBOX
#include <xtl.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH		512
#endif
#define MAX_FILENAME	255
#define MAX_DIR_LEN		248
#define MAX_BASE_DIR_LEN (248-strlen("/incomplete/"))


extern error_code	filelib_init(BOOL do_count, BOOL keep_incomplete);
extern error_code	filelib_start(char *filename);
extern error_code	filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath);
extern error_code	filelib_write(char *buf, u_long size);
extern void			filelib_shutdown();
extern error_code	filelib_set_output_directory(char *str);
extern error_code	filelib_remove(char *filename);

#endif //FILELIB
