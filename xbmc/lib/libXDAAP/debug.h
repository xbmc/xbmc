/* Copyright (c) David Hammerton, 2003
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "portability.h"

#ifdef __cplusplus
extern "C" {
#endif

enum __DEBUG_CLASS
{
    __DEBUG_TRACE = 0,
    __DEBUG_ERR,
    __DEBUG_FIXME
};

/* exported from debug.c */
int daap_debug_init(const char *const debug_init_string);

int debug_log(enum __DEBUG_CLASS, const char *module,
              const char *function,
              const int line, const char *format, ...);
int debug_printf(const char *format, ...);
void debug_hexdump(void *data, unsigned long len);
int debug_get_debugging(enum __DEBUG_CLASS debug_class, const char *debug_channel);

#ifdef NO_DEBUG
# define __GET_DEBUGGING(debug_class, debug_channel) 0
#else
# define __GET_DEBUGGING(debug_class, debug_channel) \
     debug_get_debugging(__DEBUG##debug_class, debug_channel)
#endif

#ifdef SYSTEM_POSIX 

#define __DEBUG_LOG(debug_class, debug_channel) \
    do { if (__GET_DEBUGGING(debug_class, debug_channel)) { \
        const char * const __channel = debug_channel; \
        const enum __DEBUG_CLASS __class = __DEBUG##debug_class; \
        __DEBUG_LOG_2

#define __DEBUG_LOG_2(args...) \
    debug_log(__class, __channel, __FUNCTION__, __LINE__, args); } } while(0)

#define DPRINTF(args...) \
    debug_printf(args)

#elif defined(SYSTEM_WIN32)
/* does not support variadic macros (visual c++) */

#define __DEBUG_LOG(debug_class, debug_channel) \
    (!__GET_DEBUGGING(debug_class, debug_channel) || \
        (debug_log(__DEBUG##debug_class, debug_channel, __FUNCTION__, __LINE__, "")) ? \
        (void)0 : (void)debug_printf

#define DPRINTF debug_printf

#endif

#define TRACE __DEBUG_LOG(_TRACE, DEFAULT_DEBUG_CHANNEL)
#define TRACE_ON __GET_DEBUGGING(_TRACE, DEFAULT_DEBUG_CHANNEL)

#define ERR __DEBUG_LOG(_ERR, DEFAULT_DEBUG_CHANNEL)
#define ERR_ON __GET_DEBUGGING(_ERR, DEFAULT_DEBUG_CHANNEL)

#define FIXME __DEBUG_LOG(_FIXME, DEFAULT_DEBUG_CHANNEL)
#define FIXME_ON __GET_DEBUGGING(_FIXME, DEFAULT_DEBUG_CHANNEL)


#if defined(WIN32)

#undef TRACE
#define TRACE printf

#undef ERR
#define ERR printf

#undef FIXME
#define FIXME printf

#undef DPRINTF
#define DPRINTF	printf

#endif

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
