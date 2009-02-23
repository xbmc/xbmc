/*****************************************************************
|
|   Neptune - Debug Support: Win32 Implementation
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>
#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "NptConfig.h"
#include "NptDefs.h"
#include "NptTypes.h"
#include "NptDebug.h"
#include "NptLogging.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define NPT_DEBUG_LOCAL_BUFFER_SIZE 1024
#define NPT_DEBUG_BUFFER_INCREMENT  4096
#define NPT_DEBUG_BUFFER_MAX_SIZE   65536

#if defined(NPT_CONFIG_ENABLE_LOGGING)
/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune,debug.win32")

/*----------------------------------------------------------------------
|   NPT_Print
+---------------------------------------------------------------------*/
static void
NPT_Print(const char* message)
{
#if !defined(_WIN32_WCE)
    OutputDebugString(message);
#endif
    NPT_LOG_FINER_1("%s", message);
}
#elif defined(NPT_DEBUG)
/*----------------------------------------------------------------------
|   NPT_Print
+---------------------------------------------------------------------*/
static void
NPT_Print(const char* message)
{
#if !defined(_WIN32_WCE)
    OutputDebugString(message);
#endif
    printf("%s", message);
}
#endif

/*----------------------------------------------------------------------
|   NPT_Debug
+---------------------------------------------------------------------*/
void
NPT_Debug(const char* format, ...)
{
#if defined(NPT_DEBUG) || defined(NPT_CONFIG_ENABLE_LOGGING)
    char         local_buffer[NPT_DEBUG_LOCAL_BUFFER_SIZE];
    unsigned int buffer_size = NPT_DEBUG_LOCAL_BUFFER_SIZE;
    char*        buffer = local_buffer;
    va_list      args;

    va_start(args, format);

    for(;;) {
        int result;

        /* try to format the message (it might not fit) */
#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !defined(_WIN32_WCE)
        /* use the secure function for VC 8 and above */
        result = _vsnprintf_s(buffer, buffer_size, _TRUNCATE, format, args);
#else
        result = _vsnprintf(buffer, buffer_size-1, format, args);
#endif
        buffer[buffer_size-1] = 0; /* force a NULL termination */
        if (result >= 0) break;

        /* the buffer was too small, try something bigger */
        buffer_size = (buffer_size+NPT_DEBUG_BUFFER_INCREMENT)*2;
        if (buffer_size > NPT_DEBUG_BUFFER_MAX_SIZE) break;
        if (buffer != local_buffer) delete[] buffer;
        buffer = new char[buffer_size];
        if (buffer == NULL) return;
    }

    NPT_Print(buffer);
    if (buffer != local_buffer) delete[] buffer;

    va_end(args);
#else
    NPT_COMPILER_UNUSED(format);
#endif
}
