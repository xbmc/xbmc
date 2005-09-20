#pragma once
#include "DynamicDll.h"

struct ImageInfo
{
  unsigned int width;
  unsigned int height;
  unsigned int originalwidth;
  unsigned int originalheight;
  int rotation;
  LPDIRECT3DTEXTURE8 texture;
};

class DllImageLibInterface
{
public:
    virtual bool LoadImage(const char *, unsigned int, unsigned int, ImageInfo *)=0;
    virtual bool CreateThumbnail(const char *, const char *)=0;
    virtual bool CreateThumbnailFromMemory(BYTE *, unsigned int, const char *, const char *)=0;
    virtual bool CreateFolderThumbnail(const char **, const char *)=0;
    virtual bool CreateExifThumbnail(const char *, const char *)=0;
    virtual bool CreateThumbnailFromSurface(BYTE *, unsigned int, unsigned int, unsigned int, const char *)=0;
    virtual int  ConvertFile(const char *, const char *, float, int, int, unsigned int)=0;
};

class DllImageLib : public DllDynamic, DllImageLibInterface
{
  DECLARE_DLL_WRAPPER(DllImageLib, Q:\\system\\ImageLib.dll)
  DEFINE_METHOD4(bool, LoadImage, (const char * p1, unsigned int p2, unsigned int p3, ImageInfo * p4))
  DEFINE_METHOD2(bool, CreateThumbnail, (const char * p1, const char * p2))
  DEFINE_METHOD4(bool, CreateThumbnailFromMemory, (BYTE *p1, unsigned int p2, const char * p3, const char * p4))
  DEFINE_METHOD2(bool, CreateFolderThumbnail, (const char ** p1, const char * p2))
  DEFINE_METHOD2(bool, CreateExifThumbnail, (const char * p1, const char * p2))
  DEFINE_METHOD5(bool, CreateThumbnailFromSurface, (BYTE * p1, unsigned int p2, unsigned int p3, unsigned int p4, const char * p5))
  DEFINE_METHOD6(int, ConvertFile, (const char * p1, const char * p2, float p3, int p4, int p5, unsigned int p6))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(LoadImage)
    RESOLVE_METHOD(CreateThumbnail)
    RESOLVE_METHOD(CreateThumbnailFromMemory)
    RESOLVE_METHOD(CreateFolderThumbnail)
    RESOLVE_METHOD(CreateExifThumbnail)
    RESOLVE_METHOD(CreateThumbnailFromSurface)
    RESOLVE_METHOD(ConvertFile)
  END_METHOD_RESOLVE()
};
