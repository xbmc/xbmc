//**************************************************************************************************
// Type Redefinitions

#ifndef __TYPES_H__
#define __TYPES_H__

typedef void v0;

typedef unsigned char b8;

typedef char c8;
#if defined WINVER
typedef wchar_t c16;
#else
typedef unsigned short c16;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if defined _MSC_VER || defined __BORLANDC__
typedef unsigned __int64 u64;
#else
typedef	unsigned long long int u64;
#endif

typedef char s8;
typedef short s16;
typedef int s32;
#if defined _MSC_VER || defined __BORLANDC__
typedef __int64 s64;
#else
typedef	long long int s64;
#endif

typedef float f32;
typedef double f64;
typedef long double f80;
#endif

