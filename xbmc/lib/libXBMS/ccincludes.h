/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Misc includes.
 *  ----------------------------------------------------------------------
 *
 *  Copyright (c) 2002-2003 by PuhPuh
 *  
 *  This code is copyrighted property of the author.  It can still
 *  be used for any non-commercial purpose following conditions:
 *  
 *      1) This copyright notice is not removed.
 *      2) Source code follows any distribution of the software
 *         if possible.
 *      3) Copyright notice above is found in the documentation
 *         of the distributed software.
 *  
 *  Any express or implied warranties are disclaimed.  Author is
 *  not liable for any direct or indirect damages caused by the use
 *  of this software.
 *
 *  ----------------------------------------------------------------------
 *
 *  This code has been integrated into XBMC Media Center.  
 *  As such it can me copied, redistributed and modified under
 *  the same conditions as the XBMC itself.
 *
 */


#ifndef CCINCLUDES_H_INCLUDED
#define CCINCLUDES_H_INCLUDED 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "ccutil.h"

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif /* _XBOX */

extern int errno;

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif /* ! PATH_MAX */

#define CC_UINT_64_TYPE_NAME      UINT64
#define CC_UINT_64_PRINTF_FORMAT  "%lu"

#endif /* CCINCLUDES_H_INCLUDED */
/* eof (ccincludes.h) */
