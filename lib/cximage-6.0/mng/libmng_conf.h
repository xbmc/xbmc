/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_conf.h             copyright (c) G.Juyn 2000-2004   * */
/* * version   : 1.0.9                                                      * */
/* *                                                                        * */
/* * purpose   : main configuration file                                    * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : The configuration file. Change this to include/exclude     * */
/* *             the options you want or do not want in libmng.             * */
/* *                                                                        * */
/* * changes   : 0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - separated configuration-options into this file           * */
/* *             - changed to most likely configuration (?)                 * */
/* *             0.5.2 - 06/03/2000 - G.Juyn                                * */
/* *             - changed options to create a standard so-library          * */
/* *               with everything enabled                                  * */
/* *             0.5.2 - 06/04/2000 - G.Juyn                                * */
/* *             - changed options to create a standard win32-dll           * */
/* *               with everything enabled                                  * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/12/2000 - G.Juyn                                * */
/* *             - added workaround for faulty PhotoShop iCCP chunk         * */
/* *             0.9.3 - 09/16/2000 - G.Juyn                                * */
/* *             - removed trace-options from default SO/DLL builds         * */
/* *                                                                        * */
/* *             1.0.4 - 06/22/2002 - G.Juyn                                * */
/* *             - B526138 - returned IJGSRC6B calling convention to        * */
/* *               default for MSVC                                         * */
/* *                                                                        * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             - added 'supports' call to check function availability     * */
/* *                                                                        * */
/* *             1.0.6 - 06/22/2002 - G.R-P                                 * */
/* *             - added MNG_NO_INCLUDE_JNG conditional                     * */
/* *             - added MNG_SKIPCHUNK_evNT conditional                     * */
/* *             1.0.6 - 07/14/2002 - G.R-P                                 * */
/* *             - added MNG_NO_SUPPORT_FUNCQUERY conditional               * */
/* *                                                                        * */
/* *             1.0.7 - 03/07/2004 - G.R-P                                 * */
/* *             - added MNG_VERSION_QUERY_SUPPORT_ conditional             * */
/* *                                                                        * */
/* *             1.0.9 - 05/12/2004 - G.Juyn                                * */
/* *             - clearified MNG_BIGENDIAN_SUPPORTED conditional           * */
/* *             - added MNG_LITTLEENDIAN_SUPPORTED conditional             * */
/* *                                                                        * */
/* ************************************************************************** */


#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_conf_h_
#define _libmng_conf_h_

#ifdef MNG_MOZILLA_CFG
#include "special\mozcfg\mozlibmngconf.h"
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* *  User-selectable compile-time options                                  * */
/* *                                                                        * */
/* ************************************************************************** */

/* enable exactly one(1) of the MNG-(sub)set selectors */
/* use this to select which (sub)set of the MNG specification you wish
   to support */
/* generally you'll want full support as the library provides it automatically
   for you! if you're really strung on memory-requirements you can opt
   to enable less support (but it's just NOT a good idea!) */
/* NOTE that this isn't actually implemented yet */

#if !defined(MNG_SUPPORT_FULL) && !defined(MNG_SUPPORT_LC) && !defined(MNG_SUPPORT_VLC)
#define MNG_SUPPORT_FULL
/* #define MNG_SUPPORT_LC */
/* #define MNG_SUPPORT_VLC */
#endif

/* ************************************************************************** */

/* enable JPEG support if required */
/* use this to enable the JNG support routines */
/* this requires an external jpeg package;
   currently only IJG's jpgsrc6b is supported! */
/* NOTE that the IJG code can be either 8- or 12-bit (eg. not both);
   so choose the one you've defined in jconfig.h; if you don't know what
   the heck I'm talking about, just leave it at 8-bit support (thank you!) */

#ifndef MNG_NO_INCLUDE_JNG
#ifdef MNG_SUPPORT_FULL                /* full support includes JNG */
#define MNG_SUPPORT_IJG6B
#endif

#ifndef MNG_SUPPORT_IJG6B
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_SUPPORT_IJG6B
#endif
#endif

#if defined(MNG_SUPPORT_IJG6B) && !defined(MNG_SUPPORT_JPEG8) && !defined(MNG_SUPPORT_JPEG12)
#define MNG_SUPPORT_JPEG8
/* #define MNG_SUPPORT_JPEG12 */
#endif

/* The following is required to export the IJG routines from the DLL in
   the Windows-standard calling convention;
   currently this only works for Borland C++ !!! */

#if defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#if defined(MNG_SUPPORT_IJG6B) && defined(__BORLANDC__)
#define MNG_DEFINE_JPEG_STDCALL
#endif
#endif
#endif

/* ************************************************************************** */

/* enable required high-level functions */
/* use this to select the high-level functions you require */
/* if you only need to display a MNG, disable write support! */
/* if you only need to examine a MNG, disable write & display support! */
/* if you only need to copy a MNG, disable display support! */
/* if you only need to create a MNG, disable read & display support! */
/* NOTE that turning all options off will be very unuseful! */

#if !defined(MNG_SUPPORT_READ) && !defined(MNG_SUPPORT_WRITE) && !defined(MNG_SUPPORT_DISPLAY)
#define MNG_SUPPORT_READ
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_SUPPORT_WRITE
#endif
#define MNG_SUPPORT_DISPLAY
#endif

/* ************************************************************************** */

/* enable chunk access functions */
/* use this to select whether you need access to the individual chunks */
/* useful if you want to examine a read MNG (you'll also need MNG_STORE_CHUNKS !)*/
/* required if you need to create & write a new MNG! */

#ifndef MNG_ACCESS_CHUNKS
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_ACCESS_CHUNKS
#endif
#endif

/* ************************************************************************** */

/* enable exactly one(1) of the color-management functionality selectors */
/* use this to select the level of automatic color support */
/* MNG_FULL_CMS requires the lcms (little cms) external package ! */
/* if you want your own app (or the OS) to handle color-management
   select MNG_APP_CMS */

#define MNG_GAMMA_ONLY
/* #define MNG_FULL_CMS */
/* #define MNG_APP_CMS */

/* ************************************************************************** */

/* enable automatic dithering */
/* use this if you need dithering support to convert high-resolution
   images to a low-resolution output-device */
/* NOTE that this is not supported yet */

/* #define MNG_AUTO_DITHER */

/* ************************************************************************** */

/* enable whether chunks should be stored for reference later */
/* use this if you need to examine the chunks of a MNG you have read,
   or (re-)write a MNG you have read */
/* turn this off if you want to reduce memory-consumption */

#ifndef MNG_STORE_CHUNKS
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_STORE_CHUNKS
#endif
#endif

/* ************************************************************************** */

/* enable internal memory management (if your compiler supports it) */
/* use this if your compiler supports the 'standard' memory functions
   (calloc & free), and you want the library to use these functions and not
   bother your app with memory-callbacks */

/* #define MNG_INTERNAL_MEMMNGMT */

/* ************************************************************************** */

/* enable internal tracing-functionality (manual debugging purposes) */
/* use this if you have trouble location bugs or problems */
/* NOTE that you'll need to specify the trace callback function! */

/* #define MNG_SUPPORT_TRACE */

/* ************************************************************************** */

/* enable extended error- and trace-telltaling */
/* use this if you need explanatory messages with errors and/or tracing */

#if !defined(MNG_ERROR_TELLTALE) && !defined(MNG_TRACE_TELLTALE)
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_ERROR_TELLTALE
#define MNG_TRACE_TELLTALE
#endif
#endif

/* ************************************************************************** */

/* enable BIG/LITTLE endian optimizations */
/* enable BIG if you're on an architecture that supports big-endian reads
   and writes that aren't word-aligned */
/* according to reliable sources this only works for PowerPC (bigendian mode)
   and 680x0 */
/* enable LITTLE if you're on an architecture that supports little-endian */
/* when in doubt leave both off !!! */

/* #define MNG_BIGENDIAN_SUPPORTED */
/* #define MNG_LITTLEENDIAN_SUPPORTED */

/* ************************************************************************** */
/* enable 'version' functions */
#if !defined(MNG_VERSION_QUERY_SUPPORT) && \
    !defined(MNG_NO_VERSION_QUERY_SUPPORT)
#define MNG_VERSION_QUERY_SUPPORT
#endif

/* enable 'supports' function */
/* use this if you need to query the availability of functions at runtime;
   useful for apps that dynamically load the library and that need specific
   functions */

#if !defined(MNG_NO_SUPPORT_FUNCQUERY) && !defined(MNG_SUPPORT_FUNCQUERY)
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || \
    defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_SUPPORT_FUNCQUERY
#endif
#endif

/* ************************************************************************** */

/* enable dynamic MNG features */
/* use this if you would like to have dynamic support for specifically
   designed MNGs; eg. this is useful for 'rollover' effects such as common
   on the world wide web */

#ifndef MNG_SUPPORT_DYNAMICMNG
#if defined(MNG_BUILD_SO) || defined(MNG_USE_SO) || defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_SUPPORT_DYNAMICMNG
#endif
#endif
#ifndef MNG_SUPPORT_DYNAMICMNG
#ifndef MNG_SKIPCHUNK_evNT
#define MNG_SKIPCHUNK_evNT
#endif
#endif

#ifdef MNG_INCLUDE_JNG
#ifndef MNG_NO_ACCESS_JPEG
#ifndef MNG_ACCESS_JPEG
#define MNG_ACCESS_JPEG
#endif
#endif
#endif

#ifdef MNG_INCLUDE_ZLIB
#ifndef MNG_NO_ACCESS_ZLIB
#ifndef MNG_ACCESS_ZLIB
#define MNG_ACCESS_ZLIB
#endif
#endif
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* *  End of user-selectable compile-time options                           * */
/* *                                                                        * */
/* ************************************************************************** */

#endif /* _libmng_conf_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

