#pragma once
#include "DynamicDll.h"
#include "lib/libexif/libexif.h"

class DllLibExifInterface
{
public:
    virtual bool process_jpeg(const char *, ExifInfo_t *, IPTCInfo_t *)=0;
    virtual ~DllLibExifInterface() {}
};

class DllLibExif : public DllDynamic, DllLibExifInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllLibExif, Q:\\system\\libexif.dll)
#else
  DECLARE_DLL_WRAPPER(DllLibExif, Q:\\system\\libexif-i486-linux.so)
#endif
  DEFINE_METHOD3(bool, process_jpeg, (const char *p1, ExifInfo_t *p2, IPTCInfo_t *p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(process_jpeg)
  END_METHOD_RESOLVE()
};
