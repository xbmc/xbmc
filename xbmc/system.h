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

#if defined(HAVE_CONFIG_H) && !defined(TARGET_WINDOWS)
#include "config.h"
#define DECLARE_UNUSED(a,b) a __attribute__((unused)) b;
#endif

/*****************
 * All platforms
 *****************/
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

#ifdef HAVE_LIBMICROHTTPD
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#endif

#define HAS_JSONRPC

#ifdef USE_ASAP_CODEC
#define HAS_ASAP_CODEC
#endif

#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_SAP

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

#if defined(HAVE_LIBMDNSEMBEDDED)
  #define HAS_ZEROCONF
  #define HAS_MDNS
  #define HAS_MDNS_EMBEDDED
#endif

/**********************
 * Non-free Components
 **********************/

#if defined(TARGET_WINDOWS)
  #define HAS_FILESYSTEM_RAR
#else
  #if defined(HAVE_XBMC_NONFREE)
    #define HAS_FILESYSTEM_RAR
  #endif
#endif

/*****************
 * Win32 Specific
 *****************/

#if defined(TARGET_WINDOWS)
#define HAS_SDL_JOYSTICK
#define HAS_DVD_DRIVE
#define HAS_WIN32_NETWORK
#define HAS_IRSERVERSUITE
#define HAS_AUDIO
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#define HAVE_LIBSSH
#define HAS_LIBRTMP
#define HAVE_LIBBLURAY
#define HAS_ASAP_CODEC
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_NFS
#define HAS_ZEROCONF
#define HAS_MDNS
#define HAS_AIRPLAY
#define HAS_AIRTUNES
#define HAVE_LIBSHAIRPLAY
#define HAVE_LIBCEC
#define HAVE_LIBMP3LAME
#define HAVE_LIBVORBISENC
#define HAS_MYSQL
#define HAS_UPNP

#define DECLARE_UNUSED(a,b) a b;
#endif

/*****************
 * Mac Specific
 *****************/

#if defined(TARGET_DARWIN)
  #if defined(TARGET_DARWIN_OSX)
    #define HAS_GL
    #define HAS_SDL
    #define HAS_SDL_WIN_EVENTS
  #endif
  #define HAS_ZEROCONF
  #define HAS_LINUX_NETWORK
#endif

/*****************
 * Linux Specific
 *****************/

#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
#define HAS_ZEROCONF
#define HAS_AVAHI
#endif
#ifdef HAVE_DBUS
#define HAS_DBUS
#endif
#define HAS_GL
#ifdef HAVE_X11
#define HAS_GLX
#define HAS_X11_WIN_EVENTS
#endif
#ifdef HAVE_SDL
#define HAS_SDL
#ifndef HAVE_X11
#define HAS_SDL_WIN_EVENTS
#endif
#else
#ifndef HAVE_X11
#define HAS_LINUX_EVENTS
#endif
#endif
#define HAS_LINUX_NETWORK
#ifdef HAVE_LIRC
#define HAS_LIRC
#endif
#ifdef HAVE_LIBPULSE
#define HAS_PULSEAUDIO
#endif
#ifdef HAVE_ALSA
#define HAS_ALSA
#endif
#endif

#ifdef HAVE_LIBSSH
#define HAS_FILESYSTEM_SFTP
#endif

/****************************************
 * Additional platform specific includes
 ****************************************/

#if defined(TARGET_WINDOWS)
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "mmsystem.h"
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#include "PlatformInclude.h"
#ifdef HAS_DX
#include "D3D9.h"   // On Win32, we're always using DirectX for something, whether it be the actual rendering
#include "D3DX9.h"  // or the reference video clock.
#else
#include <d3d9types.h>
#endif
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
#undef HAS_LINUX_EVENTS
#undef HAS_LIRC
#endif

// EGL detected. Dont use GLX!
#ifdef HAVE_LIBEGL
#undef HAS_GLX
#define HAS_EGL
#endif

// GLES2.0 detected. Dont use GL!
#ifdef HAVE_LIBGLESV2
#undef HAS_GL
#define HAS_GLES 2
#endif

// GLES1.0 detected. Dont use GL!
#ifdef HAVE_LIBGLES
#undef HAS_GL
#define HAS_GLES 1
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

/****************
 * default skin
 ****************/
#if defined(HAS_TOUCH_SKIN) && defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
#define DEFAULT_SKIN          "skin.re-touched"
#else
#define DEFAULT_SKIN          "skin.confluence"
#endif
