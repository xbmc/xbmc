#pragma once
#include "DynamicDll.h"

#ifndef _XBOX
 #ifdef LoadImage
  #undef LoadImage
 #endif
#endif

#ifdef _LINUX
typedef struct tagRGBQUAD {
   BYTE rgbBlue;
   BYTE rgbGreen;
   BYTE rgbRed;
   BYTE rgbReserved;
} RGBQUAD;
#endif

#define EXIF_MAX_COMMENT 1000

typedef struct tag_ExifInfo {
	char  Version      [5];
  char  CameraMake   [32];
  char  CameraModel  [40];
  char  DateTime     [20];
  int   Height, Width;
  int   Orientation;
  int   IsColor;
  int   Process;
  int   FlashUsed;
  float FocalLength;
  float ExposureTime;
  float ApertureFNumber;
  float Distance;
  float CCDWidth;
  float ExposureBias;
  int   Whitebalance;
  int   MeteringMode;
  int   ExposureProgram;
  int   ISOequivalent;
  int   CompressionLevel;
	float FocalplaneXRes;
	float FocalplaneYRes;
	float FocalplaneUnits;
	float Xresolution;
	float Yresolution;
	float ResolutionUnit;
	float Brightness;
  char  Comments[EXIF_MAX_COMMENT];

  unsigned char * ThumbnailPointer;  /* Pointer at the thumbnail */
  unsigned ThumbnailSize;     /* Size of thumbnail. */

	bool  IsExif;
} EXIFINFO;
  
struct ImageInfo
{
  unsigned int width;
  unsigned int height;
  unsigned int originalwidth;
  unsigned int originalheight;
  EXIFINFO exifInfo;
#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 texture;
#else
  RGBQUAD* rawImage;
#endif
};

class DllImageLibInterface
{
public:
#ifdef HAS_SDL
    virtual bool ReleaseImage(RGBQUAD *)=0;
#endif
    virtual bool LoadImage(const char *, unsigned int, unsigned int, ImageInfo *)=0;
    virtual bool CreateThumbnail(const char *, const char *, int, int, bool)=0;
    virtual bool CreateThumbnailFromMemory(BYTE *, unsigned int, const char *, const char *, int, int)=0;
    virtual bool CreateFolderThumbnail(const char **, const char *, int, int)=0;
    virtual bool CreateThumbnailFromSurface(BYTE *, unsigned int, unsigned intB, unsigned int, const char *)=0;
    virtual int  ConvertFile(const char *, const char *, float, int, int, unsigned int)=0;
};

class DllImageLib : public DllDynamic, DllImageLibInterface
{
#ifdef _XBOX
  DECLARE_DLL_WRAPPER(DllImageLib, Q:\\system\\ImageLib.dll)
#elif defined(HAS_SDL)
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllImageLib, Q:\\system\\ImageLib-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllImageLib, Q:\\system\\ImageLib_raw.dll)
#endif
#else
  DECLARE_DLL_WRAPPER(DllImageLib, Q:\\system\\ImageLib_win32.dll)
#endif
#ifdef HAS_SDL
  DEFINE_METHOD1(bool, ReleaseImage, (RGBQUAD *p1))
#endif
  DEFINE_METHOD4(bool, LoadImage, (const char * p1, unsigned int p2, unsigned int p3, ImageInfo * p4))
  DEFINE_METHOD5(bool, CreateThumbnail, (const char * p1, const char * p2, int p3, int p4, bool p5))
  DEFINE_METHOD6(bool, CreateThumbnailFromMemory, (BYTE *p1, unsigned int p2, const char * p3, const char * p4, int p5, int p6))
  DEFINE_METHOD4(bool, CreateFolderThumbnail, (const char ** p1, const char * p2, int p3, int p4))
  DEFINE_METHOD5(bool, CreateThumbnailFromSurface, (BYTE * p1, unsigned int p2, unsigned int p3, unsigned int p4, const char * p5))
  DEFINE_METHOD6(int, ConvertFile, (const char * p1, const char * p2, float p3, int p4, int p5, unsigned int p6))
  BEGIN_METHOD_RESOLVE()
#ifdef HAS_SDL
    RESOLVE_METHOD(ReleaseImage)
#endif
    RESOLVE_METHOD(LoadImage)
    RESOLVE_METHOD(CreateThumbnail)
    RESOLVE_METHOD(CreateThumbnailFromMemory)
    RESOLVE_METHOD(CreateFolderThumbnail)
    RESOLVE_METHOD(CreateThumbnailFromSurface)
    RESOLVE_METHOD(ConvertFile)
  END_METHOD_RESOLVE()
};
