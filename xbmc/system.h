#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#define HAS_UPNP
#define HAS_VIDEO_PLAYBACK
#define HAS_VISUALISATION
#define HAS_PVRCLIENTS

#ifdef HAVE_LIBMICROHTTPD
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#endif

#define HAS_JSONRPC
#define HAS_HTTPAPI

#ifdef USE_ASAP_CODEC
#define HAS_ASAP_CODEC
#endif

#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_FILESYSTEM_HTSP

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

#ifdef HAVE_LIBSHAIRPORT
  #define HAS_AIRTUNES
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
#define HAS_SDL
#define HAS_SDL_JOYSTICK
#define HAS_DVD_DRIVE
#define HAS_WIN32_NETWORK
#define HAS_IRSERVERSUITE
#define HAS_AUDIO
#define HAVE_LIBCRYSTALHD 2
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#define HAVE_LIBSSH
#define HAS_LIBRTMP
#define HAVE_LIBBLURAY
#define HAS_ASAP_CODEC
#define HAVE_YAJL_YAJL_VERSION_H
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_NFS
#define HAS_ZEROCONF
#define HAS_AIRPLAY
#define HAVE_LIBCEC

#define DECLARE_UNUSED(a,b) a b;
#endif

/*****************
 * Mac Specific
 *****************/

#if defined(TARGET_DARWIN)
  #if defined(TARGET_DARWIN_OSX)
    #define HAS_GL
    #define HAS_SDL
    #define HAS_SDL_AUDIO
    #define HAS_SDL_OPENGL
    #define HAS_SDL_WIN_EVENTS
  #endif
  #define HAS_ZEROCONF
  #define HAS_LINUX_NETWORK
#endif

/*****************
 * Linux Specific
 *****************/

#if defined(TARGET_LINUX)
#if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
#define HAS_ZEROCONF
#define HAS_AVAHI
#endif
#define HAS_LCD
#ifdef HAVE_DBUS
#define HAS_DBUS
#endif
#define HAS_GL
#ifdef HAVE_X11
#define HAS_GLX
#endif
#ifdef HAVE_SDL
#define HAS_SDL
#ifndef HAS_SDL_OPENGL
#define HAS_SDL_OPENGL
#endif
#define HAS_SDL_AUDIO
#define HAS_SDL_WIN_EVENTS
#endif
#define HAS_LINUX_NETWORK
#define HAS_LIRC
#ifdef HAVE_LIBPULSE
#define HAS_PULSEAUDIO
#endif
#ifdef HAVE_LIBXRANDR
#define HAS_XRANDR
#endif
#endif

#ifdef HAVE_LIBSSH
#define HAS_FILESYSTEM_SFTP
#endif

/*****************
 * Git revision
 *****************/

#if defined(TARGET_DARWIN)
#include "../git_revision.h"
#endif

#ifndef GIT_REV
#define GIT_REV "Unknown"
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

// ARM does not support certain features... disable them here!
#ifdef _ARMEL
#undef HAS_VISUALISATION
#undef HAS_FILESYSTEM_HTSP
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


#ifdef HAS_GL
#if defined(TARGET_WINDOWS)
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/wglext.h>
#elif defined(TARGET_DARWIN)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#elif defined(TARGET_LINUX)
#include <GL/glew.h>
#include <GL/gl.h>
#endif
#endif

#if HAS_GLES == 2
  #if defined(TARGET_DARWIN)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
  #else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
  #endif
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

