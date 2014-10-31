/*
 *      Copyright (C) 2014 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


/*
 * All HAVE_* and HAS_* defines specific for POSIX platforms
 */


/* 
 * POSIX unconditional defines comes first
 */
#ifndef POSIX_PLATFORMCONFIG_DEFINES
#define POSIX_PLATFORMCONFIG_DEFINES

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

/* ********************************************** *
 *          POSIX unconditional defines           *
 * ********************************************** */
#define HAS_DVD_SWSCALE
#define HAS_DVDPLAYER
#define HAS_EVENT_SERVER
#define HAS_KARAOKE
#define HAS_SCREENSAVER
#define HAS_PYTHON
#define HAS_SYSINFO
#define HAS_VIDEO_PLAYBACK
#define HAS_VISUALISATION
#define HAS_PVRCLIENTS
#define HAS_JSONRPC

#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_FILESYSTEM_HTSP
/* ********************************************** *
 *       End of POSIX unconditional defines       *
 * ********************************************** */

/* 
 * Include platform specific unconditional defines
 */
#ifdef TARGET_LINUX 
  #include "linux/PlatformConfig.h" /* Included for Android as well */
#endif /* TARGET_LINUX */
#ifdef TARGET_DARWIN
  #include "darwin/PlatformConfig.h"
#endif /* TARGET_DARWIN */
#ifdef TARGET_ANDROID
  #include "android/PlatformConfig.h" /* Included after "linux/PlatformConfig.h" */
#endif /* TARGET_ANDROID */
#ifdef TARGET_FREEBSD
  #include "freebsd/PlatformConfig.h"
#endif /* TARGET_ANDROID */

/* 
 * End of unconditional defines
 */
#endif /* POSIX_PLATFORMCONFIG_DEFINES */


/* ********************************************** *
 *     POSIX conditional defines and overrides    *
 * ********************************************** */
#ifndef POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
#define POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS

#ifdef HAVE_LIBMICROHTTPD
  #define HAS_WEB_SERVER
  #define HAS_WEB_INTERFACE
#endif

#ifdef USE_ASAP_CODEC
  #define HAS_ASAP_CODEC
#endif

#ifdef HAVE_LIBSMBCLIENT
  #define HAS_FILESYSTEM_SMB
#endif

#ifdef HAVE_LIBNFS
  #define HAS_FILESYSTEM_NFS
#endif

#ifdef HAVE_LIBAFPCLIENT
  #define HAS_FILESYSTEM_AFP
#endif

#ifdef HAVE_LIBPLIST
  #define HAS_AIRPLAY
#endif

#if defined(HAVE_LIBSHAIRPLAY)
  #define HAS_AIRTUNES
#endif

#ifdef HAVE_MYSQL
  #define HAS_MYSQL
#endif

#if defined(USE_UPNP)
  #define HAS_UPNP
#endif

#if defined(HAVE_LIBMDNSEMBEDDED)
  #define HAS_ZEROCONF
  #define HAS_MDNS
  #define HAS_MDNS_EMBEDDED
#endif

/* ******************* *
 * Non-free Components *
 * ******************* */
#if defined(HAVE_XBMC_NONFREE)
  #define HAS_FILESYSTEM_RAR
#endif
/* ******************* */

#ifdef HAVE_LIBSSH
  #define HAS_FILESYSTEM_SFTP
#endif

/* EGL detected. Don't use GLX! */
#ifdef HAVE_LIBEGL
  #undef HAS_GLX
  #define HAS_EGL
#endif

/* GLES2.0 detected. Don't use GL! */
#ifdef HAVE_LIBGLESV2
  #undef HAS_GL
  #define HAS_GLES 2
#endif

/* GLES1.0 detected. Don't use GL! */
#ifdef HAVE_LIBGLES
  #undef HAS_GL
  #define HAS_GLES 1
#endif

#ifdef HAS_DVD_DRIVE
  #define HAS_CDDA_RIPPER
#endif
/* ********************************************** *
 * End of POSIX conditional defines and overrides *
 * ********************************************** */

/* 
 * Include platform specific conditional defines and overrides
 */
#ifdef TARGET_LINUX
  #include "linux/PlatformConfig.h" /* Included for Android as well */
#endif /* TARGET_LINUX */
#ifdef TARGET_DARWIN
  #include "darwin/PlatformConfig.h"
#endif /* TARGET_DARWIN */
#ifdef TARGET_ANDROID
  #include "android/PlatformConfig.h" /* Included after "linux/PlatformConfig.h" */
#endif /* TARGET_ANDROID */
#ifdef TARGET_FREEBSD
  #include "freebsd/PlatformConfig.h"
#endif /* TARGET_ANDROID */

/* 
 * End of conditional defines and overrides
 */
#endif /* POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS */
