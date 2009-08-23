/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_dts_types.h
 *
 *  Description: Data types
 *              
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/

#ifndef _BC_DTS_TYPES_H_
#define _BC_DTS_TYPES_H_

typedef unsigned int		U32;
typedef int					S32;
typedef unsigned short  	U16;
typedef short				S16;
typedef unsigned char		U8;
typedef char				S8;

#ifndef PVOID
typedef void	*		PVOID;
#endif

#ifndef BOOL
typedef int	BOOL;
#endif

#ifdef WIN32
    typedef unsigned __int64  	U64;
#else
    typedef unsigned long long  U64;
#endif

#ifdef _WIN64
#if !(defined(POINTER_32))
#define POINTER_32	__ptr32
#endif
#else	//_WIN32
#define POINTER_32
#endif

#if defined(__KERNEL__) || defined(__LINUX_USER__)

#ifdef __LINUX_USER__	// Don't include these for KERNEL..
typedef unsigned long	ULONG;
typedef long		LONG;
typedef void		*HANDLE;
typedef void		VOID;
typedef void		*LPVOID;
typedef unsigned int	DWORD;
typedef unsigned int	UINT32;
typedef unsigned int	*LPDWORD;
typedef unsigned char	*PUCHAR;

#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#define TEXT
#else
/* For Kernel usage.. */
typedef bool	bc_bool_t;
#endif

#else

#ifndef uint64_t
typedef struct _uint64_t {
	unsigned long low_dw;
	unsigned long hi_dw;
} uint64_t;
#endif

#ifndef int32_t
typedef signed long		int32_t;
#endif

#ifndef uint32_t
typedef unsigned long	uint32_t;
#endif

#ifndef uint16_t
typedef unsigned short	uint16_t;
#endif

#ifndef uint8_t
typedef unsigned char	uint8_t;
#endif
#endif

#endif

