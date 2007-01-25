/*****************************************************************
|
|   Platinum - Log Utilities
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDebug.h"
#include "PltLog.h"
#include "stdio.h"
#include "stdarg.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define PLT_DEBUG_MAX_BUFFER 2048

unsigned long LogLevel = 0;

/*----------------------------------------------------------------------
|   PLT_SetLogLevel
+---------------------------------------------------------------------*/
void
PLT_SetLogLevel(unsigned long level)
{
    LogLevel = level;
}

/*----------------------------------------------------------------------
|   PLT_Print
+---------------------------------------------------------------------*/
void
PLT_Print(const char* message)
{
#if defined(PLT_LOG)
    NPT_Debug(message); // depends on NPT_DEBUG, so not cool
#else
    NPT_COMPILER_UNUSED(message);
#endif
}

/*----------------------------------------------------------------------
|   PLT_Print
+---------------------------------------------------------------------*/
void
PLT_Print(unsigned long level, const char* message)
{
#if defined(PLT_LOG)
    if (level <= LogLevel) {
        PLT_Print(message);
    }
#else
    NPT_COMPILER_UNUSED(level);
    NPT_COMPILER_UNUSED(message);
#endif
}

/*----------------------------------------------------------------------
|   PLT_Log
+---------------------------------------------------------------------*/
void
PLT_Log(unsigned long level, const char* format, ...)
{
#if defined(PLT_LOG)
    if (level <= LogLevel) {
        char buffer[PLT_DEBUG_MAX_BUFFER];

        va_list args;
        va_start(args, format);

        vsnprintf(buffer, sizeof(buffer)-1, format, args);
        buffer[sizeof(buffer)-1] = '\0';
        PLT_Print(level, buffer);

        va_end(args);
    }
#else
    NPT_COMPILER_UNUSED(level);
    NPT_COMPILER_UNUSED(format);
#endif
}
