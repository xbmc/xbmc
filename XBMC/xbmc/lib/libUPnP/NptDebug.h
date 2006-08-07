/*****************************************************************
|
|   Neptune - Debug Utilities
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_DEBUG_H_
#define _NPT_DEBUG_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"

/*----------------------------------------------------------------------
|    standard macros
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_HAVE_ASSERT_H) && defined(NPT_DEBUG)
#include <assert.h>
#define NPT_ASSERT(x) assert(x)
#else
#define NPT_ASSERT(x) ((void)0)
#endif

/*----------------------------------------------------------------------
|   NPT_Debug
+---------------------------------------------------------------------*/
extern void NPT_Debug(const char* format, ...);

#endif // _NPT_DEBUG_H_
