#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * Common data structures shared between XBMC and Add-On's
 *
 * The following data structures and definitions are used
 * by all the types of AddOn's and must be handled by the
 * XBMC part of the AddOn type.
 *
 * Following AddOn Types are currently defined but not all of
 * them are supported at this time.
 *
 * Identifier               ID  Description
 * --------------------------------------------------------
 * ADDON_MULTITYPE          0   General usage, like a DVD Burning Tool
 * ADDON_VIZ                1   Visualisation AddOn
 * ADDON_SKIN               2   Skin Theme AddOn
 * ADDON_PVRDLL             3   PVR Client<->Backend Interface
 * ADDON_SCRIPT             4   Phyton script
 * ADDON_SCRAPER_PVR        5   PVR Scraper
 * ADDON_SCRAPER_VIDEO      6   Video Scraper
 * ADDON_SCRAPER_MUSIC      7   Music Scraper
 * ADDON_SCRAPER_PROGRAM    8   Program Scraper
 * ADDON_SCREENSAVER        9   Screensaver
 * ADDON_PLUGIN_PVR         10  PVR/TV plugin extension
 * ADDON_PLUGIN_MUSIC       11  Music plugin extension
 * ADDON_PLUGIN_VIDEO       12  Video plugin extension
 * ADDON_PLUGIN_PROGRAM     13  Program plugin extension
 * ADDON_PLUGIN_PICTURES    14  Pictures plugin extension
 * ADDON_PLUGIN_WEATHER     15  Weather plugin extension
 * ADDON_DSP_AUDIO          16  Audio DSP like Surround decoders
 *
 */

#ifndef XBMC_ADDON_TYPES_H
#define XBMC_ADDON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* ADDON_HANDLE;

typedef enum addon_log {
  LOG_DEBUG,
  LOG_INFO,
  LOG_NOTICE,
  LOG_ERROR
} addon_log_t;

typedef struct addon_string_list {
  const char**    Strings;
  int             Items;
} addon_string_list_t;

typedef enum addon_setting_type {
  SETTING_TEXT,
  SETTING_INT,
  SETTING_BOOL,
  SETTING_ENUM,
  SETTING_LBLENUM,
  SETTING_IPADDR,
  SETTING_FILEENUM,
  SETTING_SEP,
  SETTING_LSEP
} addon_setting_type_t;

typedef enum addon_status {
  STATUS_OK,                 /* Normally not returned (everything is ok) */
  STATUS_LOST_CONNECTION,    /* AddOn lost connection to his backend (for ones that use Network) */
  STATUS_NEED_RESTART,       /* Request to restart the AddOn and data structures need updated */
  STATUS_NEED_EMER_RESTART,  /* Request to restart XBMC (hope no AddOn need or do this) */
  STATUS_NEED_SETTINGS,       /* A setting value is needed/invalid */
  STATUS_MISSING_FILE,       /* A AddOn file is missing (check log's for missing data) */
  STATUS_UNKNOWN             /* A unknown event is occurred */
} addon_status_t;
typedef addon_status_t ADDON_STATUS; /* XBMC uses "ADDON_STATUS" for "addon_status_t" */

#ifdef __cplusplus
}
#endif

#endif /* XBMC_ADDON_TYPES_H */
