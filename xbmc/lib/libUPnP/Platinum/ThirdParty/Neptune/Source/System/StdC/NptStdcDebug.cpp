/*****************************************************************
|
|      File: NptStdcDebug.c
|
|      Atomix - Debug Support: StdC Implementation
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>

#include "NptConfig.h"
#include "NptDefs.h"
#include "NptTypes.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|       NPT_Debug
+---------------------------------------------------------------------*/
void
NPT_Debug(const char* format, ...)
{
#if defined(NPT_DEBUG)
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
#else
    NPT_COMPILER_UNUSED(format);
#endif
}
