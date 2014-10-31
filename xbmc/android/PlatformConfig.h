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
 * All HAVE_* and HAS_* defines specific for Android platform
 */

/*
 * Include POSIX unconditional defines
 */
#ifndef POSIX_PLATFORMCONFIG_DEFINES
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/*
 * Android specific unconditional defines comes after POSIX unconditional defines
 * and Linux unconditional defines (included by "posix/PlatformConfig.h")
 */

#if defined(POSIX_PLATFORMCONFIG_DEFINES) && !defined(ANDROID_PLATFORMCONFIG_DEFINES)
#define ANDROID_PLATFORMCONFIG_DEFINES
/* ************************************************ *
 *          Android unconditional defines           *
 * ************************************************ */

/* Empty currently */

/* ************************************************ *
 *       End of Android unconditional defines       *
 * ************************************************ */
#endif /* ANDROID_PLATFORMCONFIG_DEFINES */


/* 
 * Include POSIX conditional defines and overrides
 */
#ifndef POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
  #include "posix/PlatformConfig.h"
#endif /* POSIX_PLATFORMCONFIG_DEFINES */

/* 
 * Android conditional defines and overrides comes after POSIX
 * and Linux (included by "posix/PlatformConfig.h")
 */
#if defined(POSIX_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS) && !defined(ANDROID_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS)
#define ANDROID_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS
/* ************************************************ *
 *     Android conditional defines and overrides    *
 * ************************************************ */
#undef HAS_LINUX_EVENTS
#undef HAS_LIRC

/* ************************************************ *
 * End of Android conditional defines and overrides *
 * ************************************************ */
#endif /* ANDROID_PLATFORMCONFIG_OVERRIDES_AND_CONDITIONALS */
