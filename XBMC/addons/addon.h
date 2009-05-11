/*
 *  Copyright (C) 2004-2006, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ADDON_H
#define __ADDON_H

/*
 * ----------------------------------------------------------------
 *  Types
 * ----------------------------------------------------------------
 */

/**
 * XBMC logging levels
 */
typedef enum {
  LOG_DEBUG,
  LOG_INFO,
  LOG_ERROR
} addon_log_t;

/**
 * Add-On status
 */
typedef enum {
  STATUS_OK,
  STATUS_NEED_SETTINGS,
  STATUS_NEED_RESTART
} addon_status_t;

typedef enum {
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

/**
 * AddOn Settings
 */

/* An individual setting */
struct addon_setting;
typedef struct addon_setting *addon_setting_t;

/* A list of settings */
struct addon_settings;
typedef struct addon_settings *addon_settings_t;

/*
 * AddOn Setting Operations
 */

/**
 * Create a new settings list structure
 * \return settings handle
 */
extern addon_settings_t addon_settings_create(void);

/**
 * Create a new setting structure
 * \return setting handle
 */
extern addon_setting_t addon_setting_create(void);

/**
 * Retrieve the ID of a setting
 * \param set setting handle
 * \return null-terminated string
 */
extern char *addon_setting_id(addon_setting_t set);

/**
 * Retrieve the type of a setting
 * \param set setting handle
 * \return settingtype handle
 */
extern addon_setting_type_t addon_setting_type(addon_setting_t set);


#endif /* __ADDON_H */
