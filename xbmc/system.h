#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if !defined(TARGET_WINDOWS)
#define DECLARE_UNUSED(a,b) a __attribute__((unused)) b;
#endif

/*****************
 * All platforms
 *****************/
#define HAS_EVENT_SERVER

#ifdef HAVE_LIBMICROHTTPD
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#endif

#define HAS_JSONRPC

#define HAS_FILESYSTEM_CDDA

#ifdef HAVE_LIBSMBCLIENT
  #define HAS_FILESYSTEM_SMB
#endif

#ifdef HAVE_LIBNFS
  #define HAS_FILESYSTEM_NFS
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

#if defined(HAVE_LIBMDNS)
  #define HAS_ZEROCONF
  #define HAS_MDNS
  #if defined(HAVE_LIBMDNSEMBEDDED)
    #define HAS_MDNS_EMBEDDED
  #endif
#endif

/*****************
 * Win32 Specific
 *****************/

#if defined(TARGET_WINDOWS)
#define HAS_IRSERVERSUITE
#if defined(TARGET_WINDOWS_DESKTOP)
#  define HAS_WIN32_NETWORK
#  define HAS_FILESYSTEM_SMB
#elif defined(TARGET_WINDOWS_STORE)
#  define HAS_WIN10_NETWORK
#endif

#if defined(HAVE_LIBBLURAY) && !defined(TARGET_WINDOWS_STORE)
  #define HAVE_LIBBLURAY_BDJ
#endif

#define DECLARE_UNUSED(a,b) a b;
#endif

/*****************
 * Mac Specific
 *****************/

#if defined(TARGET_DARWIN)
  #if defined(TARGET_DARWIN_OSX)
    #define HAS_SDL
  #endif
  #define HAS_ZEROCONF
#endif

/*****************
 * Linux Specific
 *****************/

#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif
#if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
#define HAS_ZEROCONF
#define HAS_AVAHI
#endif
#endif

/****************************************
 * Additional platform specific includes
 ****************************************/

#if defined(TARGET_WINDOWS)
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "mmsystem.h"
#include "DInput.h"
#if defined(TARGET_WINDOWS_DESKTOP)
#include "DSound.h"
#endif
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#include "PlatformInclude.h"
#include "d3d11_1.h"
#include "dxgi.h"
#include "d3dcompiler.h"
#include "directxmath.h"
#ifdef HAS_SDL
#include "SDL\SDL.h"
#endif
#endif

#if defined(TARGET_POSIX)
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "PlatformInclude.h"
#endif

#if defined(TARGET_ANDROID)
#define HAS_ZEROCONF
#endif

#ifdef HAS_DVD_DRIVE
#define HAS_CDDA_RIPPER
#endif

#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#define SAFE_DELETE_ARRAY(p) do { delete[] (p);   (p)=NULL; } while (0)
#define SAFE_RELEASE(p)      do { if(p) { (p)->Release(); (p)=NULL; } } while (0)

// Useful pixel colour manipulation macros
#define GET_A(color)            ((color >> 24) & 0xFF)
#define GET_R(color)            ((color >> 16) & 0xFF)
#define GET_G(color)            ((color >>  8) & 0xFF)
#define GET_B(color)            ((color >>  0) & 0xFF)
