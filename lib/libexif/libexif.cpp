// libexif.cpp : Defines the entry point for the console application.
//

#ifndef _LINUX
#include <windows.h>
#else
#include <memory.h>
#include <cstring>
#endif
#include "JpegParse.h"
#include "libexif.h"

#ifdef __cplusplus
extern "C" {
#endif

bool process_jpeg(const char *filename, ExifInfo_t *exifInfo, IPTCInfo_t *iptcInfo)
{
  if (!exifInfo || !iptcInfo) return false;
  CJpegParse jpeg;
  memset(exifInfo, 0, sizeof(ExifInfo_t));
  memset(iptcInfo, 0, sizeof(IPTCInfo_t));
  if (jpeg.Process(filename))
  {
    memcpy(exifInfo, jpeg.GetExifInfo(), sizeof(ExifInfo_t));
    memcpy(iptcInfo, jpeg.GetIptcInfo(), sizeof(IPTCInfo_t));
    return true;
  }
  return false;
}
#ifdef __cplusplus
}
#endif

#ifndef _DLL
int main(int argc, char* argv[])
{
  ExifInfo_t exifInfo;
  IPTCInfo_t iptcInfo;
  process_jpeg("C:\\Documents and Settings\\jcmarsha\\My Documents\\My Pictures\\396179226_f09242e21e_o.jpg", &exifInfo, &iptcInfo);
	return 0;
}
#endif
