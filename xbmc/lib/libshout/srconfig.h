#ifndef __SRCONFIG_H__
#define __SRCONFIG_H__

#if defined (WIN32)
#include "confw32.h"
#elif defined (HAVE_CONFIG_H)
#include "config.h"
#else
/* Do something */
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_UINT32_T 1
#define HAVE_U_INT32_T 1
#endif

#endif /* __SRCONFIG_H__ */
