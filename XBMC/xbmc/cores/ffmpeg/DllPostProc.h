#pragma once
#include "DynamicDll.h"

extern "C" {
#define HAVE_MMX
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __GNUC__
#pragma warning(disable:4244)
#endif
  
#include "avutil.h"
#include "postprocess.h"
}

class DllPostProcInterface
{
public:
   virtual ~DllPostProcInterface() {}
  virtual void pp_postprocess(uint8_t * src[3], int srcStride[3], uint8_t * dst[3], int dstStride[3],
                   int horizontalSize, int verticalSize, QP_STORE_T *QP_store,  int QP_stride,
		           pp_mode_t *mode, pp_context_t *ppContext, int pict_type)=0;	           
  virtual pp_mode_t *pp_get_mode_by_name_and_quality(char *name, int quality)=0;
  virtual void pp_free_mode(pp_mode_t *mode)=0;
  virtual pp_context_t *pp_get_context(int width, int height, int flags)=0;
  virtual void pp_free_context(pp_context_t *ppContext)=0;
};

#ifdef __APPLE__

// We call directly.
class DllPostProc : public DllDynamic, DllPostProcInterface
{
public:
  
  virtual ~DllPostProc() {}
  virtual void pp_postprocess(uint8_t * src[3], int srcStride[3], uint8_t * dst[3], int dstStride[3],
                  int horizontalSize, int verticalSize, QP_STORE_T *QP_store,  int QP_stride,
                  pp_mode_t *mode, pp_context_t *ppContext, int pict_type) { ::pp_postprocess((const uint8_t** )src, srcStride, dst, dstStride, horizontalSize, verticalSize, QP_store, QP_stride, mode, ppContext, pict_type); }             
  virtual pp_mode_t *pp_get_mode_by_name_and_quality(char *name, int quality) { return ::pp_get_mode_by_name_and_quality(name, quality); }
  virtual void pp_free_mode(pp_mode_t *mode) { ::pp_free_mode(mode); }
  virtual pp_context_t *pp_get_context(int width, int height, int flags) { return ::pp_get_context(width, height, flags); }
  virtual void pp_free_context(pp_context_t *ppContext) { ::pp_free_context(ppContext); }
  
  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() { return true; }
  virtual void Unload() {}
};

#else
class DllPostProc : public DllDynamic, DllPostProcInterface
{
  DECLARE_DLL_WRAPPER(DllPostProc, DLL_PATH_LIBPOSTPROC)
  DEFINE_METHOD11(void, pp_postprocess, (uint8_t* p1[3], int p2[3], uint8_t * p3[3], int p4[3],
                      int p5, int p6, QP_STORE_T *p7,  int p8,
                      pp_mode_t *p9, pp_context_t *p10, int p11))
  DEFINE_METHOD2(pp_mode_t*, pp_get_mode_by_name_and_quality, (char *p1, int p2))
  DEFINE_METHOD1(void, pp_free_mode, (pp_mode_t *p1))
  DEFINE_METHOD3(pp_context_t*, pp_get_context, (int p1, int p2, int p3))
  DEFINE_METHOD1(void, pp_free_context, (pp_context_t *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(pp_postprocess)
    RESOLVE_METHOD(pp_get_mode_by_name_and_quality)
    RESOLVE_METHOD(pp_free_mode)
    RESOLVE_METHOD(pp_get_context)
    RESOLVE_METHOD(pp_free_context)
  END_METHOD_RESOLVE()
};
#endif
