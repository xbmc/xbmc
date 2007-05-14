/* cfg.h
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#ifndef CFG_H
#define CFG_H
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_CONFIG2_H
#include "config2.h"
#endif
#endif

/* Xbox Configuration! */

#ifndef __XBOX__
#define __XBOX__

#if XBOX_USE_SECTIONS
#pragma code_seg( "LNKSBOKS" )
#pragma data_seg( "LBKS_RW" )
#pragma bss_seg( "LBKS_RW" )
#pragma const_seg( "LBKS_RD" )
#endif

#define DEBUG_KEYBOARD
#define DEBUG_MOUSE

#include "../lpng125/png.h"

#include <xtl.h>
#include "xbox-wrapper.h"
#include "xbox-dns.h"

#define G

#define JS
#define CHCEME_FLEXI_LIBU
#define GLOBHIST

#define HAVE_JPEG
#define HAVE_PNG
#define PNG_THREAD_UNSAFE_OK
#define HAVE_STRFTIME

#ifdef XBMC_LINKS_DLL
#undef feof
#endif

#define HAVE_SSL

/* for XBMC */
#define convert_string		linksboks_convert_string
#define aspect				linksboks_aspect
#define error				linksboks_error

#ifdef XBMC_LINKS_DLL
#ifdef XBOX_USE_FREETYPE
#define HAVE_FREETYPE
/* assumes XBMC */
#pragma comment (lib, "../../../guilib/freetype2/freetype221.lib" )
#endif
#endif

#endif


/* no one will probably ever port svgalib on atheos or beos or port atheos
   interface to beos, but anyway: make sure they don't clash */

#ifdef __BEOS__
#ifdef GRDRV_SVGALIB
#undef GRDRV_SVGALIB
#endif
#ifdef GRDRV_ATHEOS
#undef GRDRV_ATHEOS
#endif
#endif

#ifdef GRDRV_ATHEOS
#ifdef GRDRV_SVGALIB
#undef GRDRV_SVGALIB
#endif
#endif
