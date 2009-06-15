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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "xbmc_addon_lib.h"
#include "addon_local.h"

AddonCB *m_cb = NULL;

#ifdef __cplusplus
extern "C" {
#endif


int XBMC_register_me(ADDON_HANDLE hdl)
{
  if (hdl == NULL || m_cb != NULL)
    return 0;

  m_cb = (AddonCB*) hdl;

  return 1;
}

void XBMC_log(const addon_log_t loglevel, const char *format, ... )
{
  if (m_cb == NULL)
    return;

  char buffer[16384];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  m_cb->AddOn.Log(m_cb->addonData, loglevel, buffer);
}


void XBMC_status_callback(const addon_status_t status, const char * msg)
{
  if (m_cb == NULL)
    return;

  return m_cb->AddOn.ReportStatus(m_cb->addonData, status, msg);
}

int XBMC_get_setting(const char * settingName, void * settingValue)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->AddOn.GetSetting(m_cb->addonData, settingName, settingValue);
}

void XBMC_open_dialog_settings()
{
  if (m_cb == NULL)
    return;

  m_cb->AddOn.OpenSettings(m_cb->addonData);
}

char * XBMC_get_addon_directory()
{
  if (m_cb == NULL)
    return NULL;

  return m_cb->AddOn.GetAddonDirectory(m_cb->addonData);
}

char * XBMC_get_user_directory()
{
  if (m_cb == NULL)
    return NULL;

  return m_cb->AddOn.GetUserDirectory(m_cb->addonData);
}



void XBMC_enable_nav_sounds(int yesNo)
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.EnableNavSounds(yesNo);
}

void XBMC_execute_built_in(const char *function)
{
  if (m_cb == NULL)
    return;

  return m_cb->Utils.ExecuteBuiltIn(function);
}

char * XBMC_execute_http_api(char *httpcommand)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.ExecuteHttpApi(httpcommand);
  return buffer;
}

void XBMC_execute_script(const char *script)
{
  if (m_cb == NULL)
    return;

  return m_cb->Utils.ExecuteScript(script);
}

char * XBMC_get_cache_thumb_name(const char *path)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetCacheThumbName(path);
  return buffer;
}

int XBMC_get_cond_visibility(const char *condition)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Utils.GetCondVisibility(condition);
}

int XBMC_get_dvd_state()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Utils.GetDVDState();
}

int XBMC_get_free_memory()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Utils.GetFreeMem();
}

int XBMC_get_global_idle_time()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Utils.GetGlobalIdleTime();
}

char * XBMC_get_info_image(const char *infotag)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetInfoImage(infotag);
  return buffer;
}

char * XBMC_get_info_label(const char *infotag)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetInfoLabel(infotag);
  return buffer;
}

char * XBMC_get_ip_address()
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetIPAddress();
  return buffer;
}

char * XBMC_get_language()
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetLanguage();
  return buffer;
}

char * XBMC_get_localized_string(long dwCode)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.LocalizedString(m_cb->addonData, dwCode);
  return buffer;
}

char * XBMC_get_region(int id)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetRegion(id);
  return buffer;
}

char * XBMC_get_supported_media(int media)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetSupportedMedia(media);
  return buffer;
}

void XBMC_play_sfx(const char *filename)
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.PlaySFX(filename);
}

char * XBMC_get_skin_dir()
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.GetSkinDir();
  return buffer;
}

char * XBMC_make_legal_filename(const char *filename)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.MakeLegalFilename(filename);
  return buffer;
}

int XBMC_skin_has_image(const char *filename)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Utils.SkinHasImage(filename);
}

char * XBMC_translate_path(const char * path)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.TranslatePath(path);
  return buffer;
}

char * XBMC_unknown_to_utf8(const char * str)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Utils.UnknownToUTF8(str);
  return buffer;
}

void XBMC_shutdown()
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.Shutdown();
}

void XBMC_restart()
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.Restart();
}

void XBMC_dashboard()
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.Dashboard();
}

int XBMC_dialog_open_ok(const char* heading, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Dialog.OpenOK(heading, line0, line1, line2);
}

int XBMC_dialog_open_yesno(const char* heading, const char* line0, const char* line1, const char* line2, const char* nolabel, const char* yeslabel)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Dialog.OpenYesNo(heading, line0, line1, line2, nolabel, yeslabel);
}

char * XBMC_dialog_open_browse(int type, const char* heading, const char* shares, const char* mask, int use_thumbs, int treat_as_folder, const char* default_path)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Dialog.OpenBrowse(type, heading, shares, mask, use_thumbs, treat_as_folder, default_path);
  return buffer;
}

char * XBMC_dialog_open_numeric(int type, const char* heading, const char* default_value)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Dialog.OpenNumeric(type, heading, default_value);
  return buffer;
}

char * XBMC_dialog_open_keyboard(const char* heading, const char* default_text, int hidden)
{
  if (m_cb == NULL)
    return NULL;

  char *buffer = m_cb->Dialog.OpenKeyboard(heading, default_text, hidden);
  return buffer;
}

int XBMC_dialog_open_select(const char * heading, addon_string_list_s * list)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Dialog.OpenSelect(heading, list);
}

int XBMC_dialog_progress_create(const char* heading, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Dialog.ProgressCreate(heading, line0, line1, line2);
}

void XBMC_dialog_progress_update(int percent, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return;

  m_cb->Dialog.ProgressUpdate(percent, line0, line1, line2);
}

int XBMC_dialog_progress_is_canceled()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->Dialog.ProgressIsCanceled();
}

void XBMC_dialog_progress_close()
{
  if (m_cb == NULL)
    return ;

  m_cb->Dialog.ProgressClose();
}



void XBMC_gui_lock()
{
  if (m_cb == NULL)
    return;

  m_cb->GUI.Lock();
}
void XBMC_gui_unlock()
{
  if (m_cb == NULL)
    return;

  m_cb->GUI.Unlock();
}

int XBMC_gui_get_current_window_id()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->GUI.GetCurrentWindowId();
}

int XBMC_gui_get_current_window_dialog_id()
{
  if (m_cb == NULL)
    return 0;

  return m_cb->GUI.GetCurrentWindowDialogId();
}

#ifdef __cplusplus
}
#endif


