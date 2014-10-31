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
 * All HAVE_* and HAS_* defines specific for Linux platforms
 */

/* 
 * Include POSIX unconditional defines
 */
#ifndef POSIX_PLATFORMCONFIG_DEFINES
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/* Guard against of being included on unsupported platforms */
#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)

/* 
 * Linux specific unconditional defines comes after POSIX unconditional defines
 */
#if defined(POSIX_PLATFORMCONFIG_DEFINES) && !defined(LINUX_PLATFORMCONFIG_DEFINES)
#define LINUX_PLATFORMCONFIG_DEFINES
/* ********************************************** *
 *          Linux unconditional defines           *
 * ********************************************** */
#define HAS_GL

#define HAS_LINUX_NETWORK
#define HAS_LIRC

/* ********************************************** *
 *       End of Linux unconditional defines       *
 * ********************************************** */
#endif /* LINUX_PLATFORMCONFIG_DEFINES */


/* 
 * Include POSIX conditional defines and overrides
 */
#ifndef POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/* 
 * Linux conditional defines and overrides comes after POSIX
 */
#if defined(POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS) && !defined(LINUX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS)
#define LINUX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
/* ********************************************** *
 *     Linux conditional defines and overrides    *
 * ********************************************** */
#if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
  #define HAS_ZEROCONF
  #define HAS_AVAHI
#endif

#ifdef HAVE_DBUS
  #define HAS_DBUS
#endif

#ifdef HAVE_X11
  #define HAS_GLX
  #define HAS_X11_WIN_EVENTS
#endif

#ifdef HAVE_SDL
  #define HAS_SDL
  #ifndef HAS_SDL_OPENGL
    #define HAS_SDL_OPENGL
  #endif
  #ifndef HAVE_X11
    #define HAS_SDL_WIN_EVENTS
  #endif
#else
  #ifndef HAVE_X11
    #define HAS_LINUX_EVENTS
  #endif
#endif

#ifdef HAVE_LIBPULSE
  #define HAS_PULSEAUDIO
#endif

#ifdef HAVE_LIBXRANDR
  #define HAS_XRANDR
#endif

#ifdef HAVE_ALSA
  #define HAS_ALSA
#endif

/* ********************************************** *
 * End of Linux conditional defines and overrides *
 * ********************************************** */
#endif /* LINUX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS */


#endif /* TARGET_LINUX || TARGET_FREEBSD */
