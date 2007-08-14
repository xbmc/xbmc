/*****************************************************************
|
|   Neptune Utils
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_UTILS_H_
#define _NPT_UTILS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptStrings.h"

#if defined (NPT_CONFIG_HAVE_STDIO_H)
#include <stdio.h>
#endif

#if defined (NPT_CONFIG_HAVE_STRING_H)
#include <string.h>
#endif

#if defined(NPT_CONFIG_HAVE_STDARG_H)
#include <stdarg.h>
#endif

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define NPT_ARRAY_SIZE(_a) (sizeof(_a)/sizeof((_a)[0]))

/*----------------------------------------------------------------------
|   byte I/O
+---------------------------------------------------------------------*/
extern void NPT_BytesFromInt32Be(unsigned char* buffer, NPT_UInt32 value);
extern void NPT_BytesFromInt16Be(unsigned char* buffer, NPT_UInt16 value);
extern NPT_UInt32 NPT_BytesToInt32Be(const unsigned char* buffer);
extern NPT_UInt16 NPT_BytesToInt16Be(const unsigned char* buffer);

extern void NPT_BytesFromInt32Le(unsigned char* buffer, NPT_UInt32 value);
extern void NPT_BytesFromInt16Le(unsigned char* buffer, NPT_UInt16 value);
extern NPT_UInt32 NPT_BytesToInt32Le(const unsigned char* buffer);
extern NPT_UInt16 NPT_BytesToInt16Le(const unsigned char* buffer);

extern void NPT_ByteToHex(NPT_Byte b, char* buffer);
extern NPT_Result NPT_HexToByte(const char* buffer, NPT_Byte& b);

/*----------------------------------------------------------------------
|    conversion utilities
+---------------------------------------------------------------------*/
extern NPT_Result 
NPT_ParseFloat(const char* str, float& result, bool relaxed = true);

extern NPT_Result 
NPT_ParseInteger(const char* str, long& result, bool relaxed = true, NPT_Cardinal* chars_used = 0);

extern NPT_Result 
NPT_ParseInteger32(const char* str, NPT_Int32& result, bool relaxed = true);

/*----------------------------------------------------------------------
|    formatting
+---------------------------------------------------------------------*/
void
NPT_FormatOutput(void        (*function)(void* parameter, const char* message),
                 void*       function_parameter,
                 const char* format, 
                 va_list     args);

/*----------------------------------------------------------------------
|    environment variables
+---------------------------------------------------------------------*/
NPT_Result NPT_GetEnvironment(const char* name, NPT_String& value);

/*----------------------------------------------------------------------
|   string utils
+---------------------------------------------------------------------*/
#if defined (NPT_CONFIG_HAVE_STDIO_H)
#include <stdio.h>
#endif

#if defined (NPT_CONFIG_HAVE_STRING_H)
#include <string.h>
#endif

#if defined (NPT_CONFIG_HAVE_SNPRINTF)
#define NPT_FormatString NPT_snprintf
#else
int NPT_FormatString(char* str, NPT_Size size, const char* format, ...);
#endif

#if defined(NPT_CONFIG_HAVE_VSNPRINTF)
#define NPT_FormatStringVN(s,c,f,a) NPT_vsnprintf(s,c,f,a)
#else
extern int NPT_FormatStringVN(char *buffer, size_t count, const char *format, va_list argptr);
#endif

#if defined(NPT_CONFIG_HAVE_MEMCPY)
#define NPT_CopyMemory memcpy
#else
extern void NPT_CopyMemory(void* dest, void* src, NPT_Size size);
#endif

#if defined(NPT_CONFIG_HAVE_STRCMP)
#define NPT_StringsEqual(s1, s2) \
(strcmp((s1), (s2)) == 0)
#else
extern int NPT_StringsEqual(const char* s1, const char* s2);
#endif

#if defined(NPT_CONFIG_HAVE_STRNCMP)
#define NPT_StringsEqualN(s1, s2, n) \
(strncmp((s1), (s2), (n)) == 0)
#else
extern int NPT_StringsEqualN(const char* s1, const char* s2, unsigned long size);
#endif

#if defined(NPT_CONFIG_HAVE_STRLEN)
#define NPT_StringLength(s) \
(NPT_Size)(strlen(s))
#else
extern unsigned long NPT_StringLength(const char* s);
#endif

#if defined(NPT_CONFIG_HAVE_STRCPY)
#define NPT_CopyString(dst, src) ((void)strcpy((dst), (src)))
#else
extern void NPT_CopyString(char* dst, const char* src);
#endif

#if defined(NPT_CONFIG_HAVE_STRNCPY)
#define NPT_CopyStringN(dst, src, n) ((void)NPT_strncpy((dst), (src), n))
#else
extern int NPT_CopyStringN(char* dst, const char* src, unsigned long n);
#endif

#if defined(NPT_CONFIG_HAVE_MEMSET)
#define NPT_SetMemory memset
#else
extern void NPT_SetMemory(void* dest, int c, NPT_Size size);
#endif

#if defined(NPT_CONFIG_HAVE_MEMCMP)
#define NPT_MemoryEqual(s1, s2, n) (memcmp((s1), (s2), (n)) == 0) 
#else 
extern int NPT_MemoryEqual(const void* s1, const void* s2, unsigned long n); 
#endif

#endif // _NPT_UTILS_H_
