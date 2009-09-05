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
#ifndef LIB_ADDON_H
#define LIB_ADDON_H

#ifdef WIN32
#define XBMC_API   __declspec( dllexport )
#else
#define XBMC_API extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif

////////////// libAddon Types ////////////////

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
 
 /* An individual setting */
 struct addon_setting;
 typedef struct addon_setting *addon_setting_t;

 /* A list of settings */
 struct addon_settings;
 typedef struct addon_settings *addon_settings_t;

////////////// libRefMem Operations //////////////

/**
 * Decrement a reference count on the structure pointed
 * to by 'p'
 * \param p setting(s) handle
 * \return NULL
 */
 XBMC_API void addon_release(void* p);

////////// AddOn SettingList Operations //////////

/**
 * Create a new settings list structure
 * \return settings handle
 */
 XBMC_API addon_settings_t addon_settings_create(void);

 /*
 * Retrieve the number of elements in the settings list structure
 * in 'list'.
 * \param list settings list handle
 * \return int number of elements or <0 indicating error
 */
 XBMC_API int addon_settings_get_count(addon_settings_t list);

 /**
 * Return a pointer to the settings structure located at position
 * 'index' of a settings list structure 'list'
 * \param list settings list handle
 * \param index position in list
 * \return setting handle
 */
 XBMC_API addon_setting_t addon_settings_get_item(addon_settings_t list, int index);

 /**
 * Add a setting structure to a settings list
 * \param list settings list handle
 * \param setting addon_setting handle to add to list
 * \return int error status
 */
 XBMC_API int addon_settings_add_item(addon_settings_t list, addon_setting_t setting);

//////////// AddOn Setting Operations ////////////
/**
 * Create a new setting structure
 * \return setting handle
 */
 XBMC_API addon_setting_t addon_setting_create(void);

/**
 * Retrieve the 'ID' field of a setting
 * \param setting addon_setting handle
 * \return null-terminated string
 */
 XBMC_API char *addon_setting_id(addon_setting_t setting);

 /**
 * Set the 'id' field of a setting
 * \param setting addon_setting handle
 * \param id char* id
 * \return int error status
 */
 XBMC_API int addon_setting_set_id(addon_setting_t setting, char* id);

/**
 * Retrieve the 'label' field of a setting
 * \param setting addon_setting handle
 * \return null-terminated string
 */
 XBMC_API char *addon_setting_label(addon_setting_t setting);

 /**
 * Set the 'label' field of a setting
 * \param setting addon_setting handle
 * \param label char* label
 * \return int error status
 */
 XBMC_API int addon_setting_set_label(addon_setting_t setting, char *label);

/**
 * Retrieve the 'enable' field of a setting
 * \param setting addon_setting handle
 * \return null-terminated string
 */
 XBMC_API char *addon_setting_enable(addon_setting_t setting);

 /**
 * Set the 'enable' field of a setting
 * \param setting addon_setting handle
 * \param enabled int
 * \return int error status
 */
 XBMC_API int addon_setting_set_enable(addon_setting_t setting, int enabled);

/**
 * Retrieve the 'lvalues' field of a setting
 * \param setting addon_setting handle
 * \return null-terminated string
 */
 XBMC_API char *addon_setting_lvalues(addon_setting_t setting);

 /**
 * Set the 'lvalues' field of a setting
 * \param setting addon_setting handle
 * \param lvalues char* lvalues to set
 * \return int error status
 */
 XBMC_API int addon_setting_set_lvalues(addon_setting_t setting, char *lvalues);

/**
 * Retrieve the type of a setting
 * \param setting addon_setting handle
 * \return addon_setting_type_t
 */
 XBMC_API addon_setting_type_t addon_setting_type(addon_setting_t setting);

/**
 * Set the type of a setting
 * \param setting addon_setting handle
 * \param type addon_setting_type_t to apply
 * \return int error status
 */
 XBMC_API int addon_setting_set_type(addon_setting_t setting, addon_setting_type_t type);

/**
 * Retrieve the 'valid' field of a setting
 * \param setting addon_setting handle
 * \return signed integer
 */
 XBMC_API int addon_setting_valid(addon_setting_t setting);

 /**
 * Set the type of a setting
 * \param setting addon_setting handle
 * \param isvalid int
 * \return int error status
 */
 XBMC_API int addon_setting_set_valid(addon_setting_t setting, int isvalid);

#ifdef __cplusplus
 };
#endif

#endif /* #ifndef LIB_ADDON_H */