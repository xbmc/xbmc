#ifndef __GNUC__
#pragma comment(linker, "/merge:RTV_TEXT=LIBRTV")
#pragma comment(linker, "/merge:RTV_DATA=LIBRTV")
#pragma comment(linker, "/merge:RTV_BSS=LIBRTV")
#pragma comment(linker, "/merge:RTV_RD=LIBRTV")
#pragma comment(linker, "/section:LIBRTV,RWE")
#endif

#ifndef RTVINTERFACE_H
#define RTVINTERFACE_H

#include "rtv.h"

struct rtv_data
{
	struct hc * hc;
	short firstReadDone;
};

struct RTV
{
	char hostname[16];
	char friendlyName[32];
};

typedef struct rtv_data * RTVD;

extern u64 rtv_get_filesize(const char* strHostName, const char* strFileName);
extern unsigned long rtv_get_guide(unsigned char ** result, const char * address);
extern int rtv_parse_guide(char * szOutputBuffer, const char * szInput, const size_t InputSize);
extern int rtv_get_guide_xml(unsigned char ** result, const char * address);
extern int rtv_list_files(unsigned char ** result, const char * address, const char * path);
extern RTVD rtv_open_file(const char * address, const char * strFileName, u64 filePos);
extern size_t rtv_read_file(RTVD rtvd, char * lpBuf, size_t uiBufSize);
extern void rtv_close_file(RTVD rtvd);
extern int rtv_discovery(struct RTV ** result, unsigned long msTimeout);

#endif

