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
 * All HAVE_* and HAS_* defines specific for Darwin platforms
 */

/*
 * Include POSIX unconditional defines
 */
#ifndef POSIX_PLATFORMCONFIG_DEFINES
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/* 
 * Darwin specific unconditional defines comes after POSIX unconditional defines
 */
#if defined(POSIX_PLATFORMCONFIG_DEFINES) && !defined(DARWIN_PLATFORMCONFIG_DEFINES)
#define DARWIN_PLATFORMCONFIG_DEFINES
/* *********************************************** *
 *          Darwin unconditional defines           *
 * *********************************************** */
/* Note: TARGET_DARWIN_OSX and TARGET_DARWIN_IOS are defined by precompiler flags  
         so macros with dependency on those defines are used as unconditional */
#define HAS_ZEROCONF
#define HAS_LINUX_NETWORK

#if defined(TARGET_DARWIN_OSX)
/* ***************** *
 *   OS X Specific   *
 * ***************** */
  #define HAS_GL
  #define HAS_SDL
  #define HAS_SDL_OPENGL
  #define HAS_SDL_WIN_EVENTS
#endif

/* *********************************************** *
 *       End of Darwin unconditional defines       *
 * *********************************************** */
#endif /* DARWIN_PLATFORMCONFIG_DEFINES */

/* 
 * Include POSIX conditional defines and overrides
 */
#ifndef POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/* 
 * Darwin conditional defines and overrides comes after POSIX
 */
#if defined(POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS) && !defined(DARWIN_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS)
#define DARWIN_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
/* *********************************************** *
 *     Darwin conditional defines and overrides    *
 * *********************************************** */

/* Empty currently */

/* *********************************************** *
 * End of Darwin conditional defines and overrides *
 * *********************************************** */
#endif /* DARWIN_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS */
