/*****************************************************************
|
|   Neptune - Console
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"

#if defined(NPT_CONFIG_HAVE_STDARG_H)
#include <stdarg.h>
#endif

#include "NptConfig.h"
#include "NptConsole.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   NPT_ConsoleOutputFunction
+---------------------------------------------------------------------*/
static void
NPT_ConsoleOutputFunction(void*, const char* message)
{
    NPT_Console::Output(message);
}

/*----------------------------------------------------------------------
|   NPT_ConsoleOutputF
+---------------------------------------------------------------------*/
void 
NPT_Console::OutputF(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    NPT_FormatOutput(NPT_ConsoleOutputFunction, NULL, format, args);

    va_end(args);
}

