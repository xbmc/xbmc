#ifndef LIBXDMX_COMMON_H_
#define LIBXDMX_COMMON_H_

#include <string.h>
#include <stdlib.h>
#include "xdmx.h"

#if defined(__GNUC__)
  void* xdmx_aligned_malloc(size_t s, size_t boundary);
  void xdmx_aligned_free(void *p);
#else
  #define xdmx_aligned_malloc _aligned_malloc
  #define xdmx_aligned_free _aligned_free
#endif

extern XdmxLogFuncPtr g_xdmxlog;
extern int g_xdmxlevel;

#define XDMX_LOG_DEBUG(fmt, ...) if (g_xdmxlevel >= XDMX_LOG_LEVEL_DEBUG) g_xdmxlog(fmt "\n", __VA_ARGS__)
#define XDMX_LOG_INFO(fmt, ...) if (g_xdmxlevel >= XDMX_LOG_LEVEL_INFO) g_xdmxlog(fmt "\n", __VA_ARGS__)
#define XDMX_LOG_WARN(fmt, ...) if (g_xdmxlevel >= XDMX_LOG_LEVEL_WARNING) g_xdmxlog(fmt "\n", __VA_ARGS__)
#define XDMX_LOG_ERROR(fmt, ...) if (g_xdmxlevel >= XDMX_LOG_LEVEL_ERROR) g_xdmxlog(fmt "\n", __VA_ARGS__)


#endif // LIBXDMX_COMMON_H_

