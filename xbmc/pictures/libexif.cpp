// libexif.cpp : Defines the entry point for the console application.
//

#include "libexif.h"

#include "JpegParse.h"

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#include <memory.h>
#include <cstring>
#endif

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
