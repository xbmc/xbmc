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

#undef HAS_XBOX_D3D
#undef HAS_RAM_CONTROL
#undef HAS_XFONT
#undef HAS_FILESYSTEM_CDDA
#undef HAS_FILESYSTEM_SMB
#undef HAS_FILESYSTEM_RTV
#undef HAS_FILESYSTEM_DAAP
#undef HAS_FILESYSTEM
#undef HAS_GAMEPAD
#undef HAS_IR_REMOTE
#undef HAS_DVD_DRIVE
#undef HAS_XBOX_HARDWARE
#undef HAS_XBOX_NETWORK
#ifdef HAS_SDL
#define HAS_VIDEO_PLAYBACK
#define HAS_DVDPLAYER
#else
#undef HAS_VIDEO_PLAYBACK
#undef HAS_DVDPLAYER
#endif
#undef HAS_MPLAYER
#undef HAS_AC3_CODEC
#undef HAS_DTS_CODEC
#undef HAS_AC3_CDDA_CODEC
#undef HAS_DTS_CDDA_CODEC
#define HAS_WMA_CODEC
#undef HAS_XBOX_AUDIO
#undef HAS_AUDIO_PASS_THROUGH
#undef HAS_FTP_SERVER
#define HAS_WEB_SERVER
#undef HAS_TIME_SERVER
#undef HAS_VISUALISATION
#define HAS_KARAOKE
#undef HAS_CREDITS
#undef HAS_MODPLAYER
#undef HAS_SYSINFO
#define HAS_SCREENSAVER
#undef HAS_MIKMOD
#undef HAS_SECTIONS
#define HAS_UPNP
#undef HAS_LCD
#undef HAS_UNDOCUMENTED
#undef HAS_SECTIONS
#undef HAS_CDDA_RIPPER
#define HAS_PYTHON
#define HAS_AUDIO
#define HAS_SHOUTCAST
#define HAS_RAR
#undef  HAS_LIRC

#ifndef _LINUX
// additional includes and defines
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#undef HAS_CCXSTREAM
#endif

//zeroconf
//on osx enabled by default
//linux only if avahi is present
#ifdef _LINUX
#ifdef __APPLE__
#define HAS_ZEROCONF
#else 
#ifdef HAS_AVAHI
#define HAS_ZEROCONF
#endif
#endif
#endif

#ifdef _LINUX
#ifndef __APPLE__
#define HAS_LCD
#include "../config.h"
#endif
#define HAS_KARAOKE
#define HAS_PYTHON
#define HAS_WEB_SERVER
#define HAS_EVENT_SERVER
#define HAS_UPNP
#undef  HAS_AUDIO
#define  HAS_SHOUTCAST
#define HAS_SDL
#define HAS_RAR
#ifndef __APPLE__
#define HAS_HAL
#define HAS_DBUS
#define HAS_DBUS_SERVER
#endif
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM
#define HAS_SYSINFO
#define HAS_VIDEO_PLAYBACK
#undef  HAS_MPLAYER
#define HAS_VISUALISATION
#define HAS_DVDPLAYER
#define HAS_DVD_DRIVE
#define HAS_WMA_CODEC
#define HAS_CCXSTREAM
#ifndef __APPLE__
#define HAS_LIRC
#endif
#define HAS_AC3_CODEC
#define HAS_DTS_CODEC
#define HAS_CDDA_RIPPER
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_VTP
#define HAS_PERFORMANCE_SAMPLE
#define HAS_LINUX_NETWORK

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

#ifdef HAS_SDL
#define HAS_SDL_AUDIO
#define HAS_DVD_SWSCALE
#ifndef HAS_SDL_2D
#define HAS_SDL_OPENGL
#ifdef _LINUX
#ifndef __APPLE__
#define HAS_GLX
#endif
#endif
#endif
#ifdef _WIN32
#define _WIN32PC       // precompiler definition for the windows build
#define HAS_AC3_CODEC
#define HAS_DTS_CODEC
#define HAS_CDDA_RIPPER
#define HAS_DVD_SWSCALE
#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_DVD_DRIVE
#define HAS_VISUALISATION
#define HAS_CCXSTREAM
#define HAS_EVENT_SERVER
#define HAS_SHOUTCAST
#define HAS_WIN32_NETWORK
#define HAS_LIRC
#define HAS_IRSERVERSUITE // depends on HAS_LIRC define
#define HAS_SYSINFO
#define HAS_SDL_JOYSTICK
#define HAS_KARAOKE

#undef HAS_SDL_AUDIO   // use dsound for audio on win32
#undef HAS_PERFORMANCE_SAMPLE // no performance sampling
#undef HAS_LINUX_NETWORK

#include "../xbmc/win32/PlatformInclude.h"
#endif
#endif

#ifndef SVN_REV
#define SVN_REV "Unknown"
#endif
