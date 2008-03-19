#ifndef __PLATFORM_DEFS_H__
#define __PLATFORM_DEFS_H__

#ifdef _WIN32

#define snprintf _snprintf

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif
#endif 

#endif //__PLATFORM_DEFS_H__

