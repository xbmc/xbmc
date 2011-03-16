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

#if defined(HAVE_CONFIG_H) && !defined(_WIN32)
#include "config.h"
#endif

/*****************
 * All platforms
 *****************/
#ifndef HAS_SDL
#define HAS_SDL
#endif

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
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_FILESYSTEM_HTSP

/**********************
 * Non-free Components
 **********************/

#if defined(_LINUX) || defined(__APPLE__)
  #if defined(HAVE_XBMC_NONFREE)
    #define HAS_FILESYSTEM_RAR
    #define HAS_FILESYSTEM_CCX
  #endif
#else
  #define HAS_FILESYSTEM_RAR
  #define HAS_FILESYSTEM_CCX
#endif

/*****************
 * Win32 Specific
 *****************/

#ifdef _WIN32
#define HAS_DVD_DRIVE
#define HAS_SDL_JOYSTICK
#define HAS_WIN32_NETWORK
#define HAS_IRSERVERSUITE
#define HAS_AUDIO
#define HAVE_LIBCRYSTALHD 1
#define HAS_WEB_SERVER
#define HAS_WEB_INTERFACE
#define HAVE_LIBSSH
#define HAS_LIBRTMP
#define HAVE_LIBBLURAY
#endif

/*****************
 * Mac Specific
 *****************/

#ifdef __APPLE__
  #if defined(__arm__)
    #undef HAS_GL
    #undef HAS_SDL
    #define HAVE_LIBEGL
    #define HAVE_LIBGLESV2
  #else
    #define HAS_GL
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

#if defined(_LINUX) && !defined(__APPLE__)
#ifndef HAS_SDL_OPENGL
#define HAS_SDL_OPENGL
#endif
#if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
#define HAS_ZEROCONF
#define HAS_AVAHI
#endif
#define HAS_LCD
#define HAS_DBUS
#define HAS_DBUS_SERVER
#define HAS_GL
#define HAS_GLX
#define HAS_LINUX_NETWORK
#define HAS_SDL_AUDIO
#define HAS_LIRC
#define HAS_SDL_WIN_EVENTS
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

#ifdef __APPLE__
#include "../git_revision.h"
#endif

#ifndef GIT_REV
#define GIT_REV "Unknown"
#endif

/****************************************
 * Additional platform specific includes
 ****************************************/

#ifdef _WIN32
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "mmsystem.h"
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#include "PlatformInclude.h"
#include "D3D9.h"   // On Win32, we're always using DirectX for something, whether it be the actual rendering
#include "D3DX9.h"  // or the reference video clock.
#ifdef HAS_SDL
#include "SDL\SDL.h"
#endif
#endif

#ifdef _LINUX
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include "PlatformInclude.h"
#endif

// ARM does not support certain features... disable them here!
#ifdef _ARMEL
#undef HAS_AVAHI
#undef HAS_ZEROCONF
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
#ifdef _WIN32
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/wglext.h>
#elif defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#elif defined(_LINUX)
#include <GL/glew.h>
#include <GL/gl.h>
#endif
#endif

#if HAS_GLES == 2
  #if defined(__APPLE__)
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

