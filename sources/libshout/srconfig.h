#ifndef __SRCONFIG_H__
#define __SRCONFIG_H__

#if defined (WIN32)
#include "confw32.h"
#elif defined (HAVE_CONFIG_H)
#include "config.h"
#else
/* Do something */
#endif

#endif /* __SRCONFIG_H__ */
