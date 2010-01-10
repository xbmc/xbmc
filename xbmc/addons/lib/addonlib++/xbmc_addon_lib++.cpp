#include <stdio.h>
#include <stdlib.h>
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

#include <stdarg.h>
#include "xbmc_addon_lib++.h"
#include "addon_local.h"

using namespace std;

AddonCB *m_cb = NULL;

void XBMC_register_me(ADDON_HANDLE hdl)
{
  m_cb = (AddonCB*) hdl;

  return;
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

void XBMC_status_callback(const addon_status_t status, const char* msg)
{
  if (m_cb == NULL)
    return;

  return m_cb->AddOn.ReportStatus(m_cb->addonData, status, msg);
}

bool XBMC_get_setting(string settingName, void *settingValue)
{
  if (m_cb == NULL)
    return false;

  return m_cb->AddOn.GetSetting(m_cb->addonData, settingName.c_str(), settingValue);
}

void XBMC_open_dialog_settings()
{
  if (m_cb == NULL)
    return;

  m_cb->AddOn.OpenSettings(m_cb->addonData);
}

string XBMC_get_addon_directory()
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->AddOn.GetAddonDirectory(m_cb->addonData);
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_user_directory()
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->AddOn.GetUserDirectory(m_cb->addonData);
  string str = buffer;
  free(buffer);
  return str;
}


bool XBMC_create_directory(std::string dir)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.CreateDirectory(dir.c_str());
}

void XBMC_enable_nav_sounds(bool yesNo)
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.EnableNavSounds(yesNo);
}

void XBMC_execute_built_in(string function)
{
  if (m_cb == NULL)
    return;

  return m_cb->Utils.ExecuteBuiltIn(function.c_str());
}

string XBMC_execute_http_api(string httpcommand)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.ExecuteHttpApi(httpcommand.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

void XBMC_execute_script(string script)
{
  if (m_cb == NULL)
    return;

  return m_cb->Utils.ExecuteScript(script.c_str());
}

string XBMC_get_cache_thumb_name(string path)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetCacheThumbName(path.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

bool XBMC_get_cond_visibility(string condition)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.GetCondVisibility(condition.c_str());
}

int XBMC_get_dvd_state()
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.GetDVDState();
}

int XBMC_get_free_memory()
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.GetFreeMem();
}

int XBMC_get_global_idle_time()
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.GetGlobalIdleTime();
}

string XBMC_get_info_image(string infotag)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetInfoImage(infotag.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_info_label(string infotag)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetInfoLabel(infotag.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_ip_address()
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetIPAddress();
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_language()
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetLanguage();
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_localized_string(long dwCode)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.LocalizedString(m_cb->addonData, dwCode);
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_region(int id)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetRegion(id);
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_get_supported_media(int media)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetSupportedMedia(media);
  string str = buffer;
  free(buffer);
  return str;
}

void XBMC_play_sfx(string filename)
{
  if (m_cb == NULL)
    return;

  m_cb->Utils.PlaySFX(filename.c_str());
}

string XBMC_get_skin_dir()
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.GetSkinDir();
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_make_legal_filename(string filename)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.MakeLegalFilename(filename.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

bool XBMC_skin_has_image(string filename)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Utils.SkinHasImage(filename.c_str());
}

string XBMC_translate_path(string path)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Utils.TranslatePath(path.c_str());
  string str = buffer;
  free(buffer);
  return str;
}

void XBMC_unknown_to_utf8(string &str)
{
  if (m_cb == NULL)
    return;

  string buffer = m_cb->Utils.UnknownToUTF8(str.c_str());
  str = buffer;
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

bool XBMC_dialog_open_ok(const char* heading, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Dialog.OpenOK(heading, line0, line1, line2);
}

bool XBMC_dialog_open_yesno(const char* heading, const char* line0, const char* line1, const char* line2, const char* nolabel, const char* yeslabel)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Dialog.OpenYesNo(heading, line0, line1, line2, nolabel, yeslabel);
}

string XBMC_dialog_open_browse(int type, const char* heading, const char* shares, const char* mask, bool use_thumbs, bool treat_as_folder, const char* default_path)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Dialog.OpenBrowse(type, heading, shares, mask, use_thumbs, treat_as_folder, default_path);
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_dialog_open_numeric(int type, const char* heading, const char* default_value)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Dialog.OpenNumeric(type, heading, default_value);
  string str = buffer;
  free(buffer);
  return str;
}

string XBMC_dialog_open_keyboard(const char* heading, const char* default_text, bool hidden)
{
  if (m_cb == NULL)
    return "";

  char *buffer = m_cb->Dialog.OpenKeyboard(heading, default_text, hidden);
  string str = buffer;
  free(buffer);
  return str;
}

int XBMC_dialog_open_select(const char* heading, vector<string> &list)
{
  if (m_cb == NULL)
    return 0;

  unsigned int listsize = list.size();
  int ret = 0;
  if (listsize > 0)
  {
    addon_string_list c_list;
    c_list.Items = listsize;
    c_list.Strings = (const char**) calloc (c_list.Items+1,sizeof(const char*));
    for (unsigned int i = 0; i < listsize; i++)
    {
      c_list.Strings[i] = list[i].c_str();
    }
    ret = m_cb->Dialog.OpenSelect(heading, &c_list);
    free(c_list.Strings);
  }
  return ret;
}

bool XBMC_dialog_progress_create(const char* heading, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return false;

  return m_cb->Dialog.ProgressCreate(heading, line0, line1, line2);
}

void XBMC_dialog_progress_update(int percent, const char* line0, const char* line1, const char* line2)
{
  if (m_cb == NULL)
    return;

  m_cb->Dialog.ProgressUpdate(percent, line0, line1, line2);
}

bool XBMC_dialog_progress_is_canceled()
{
  if (m_cb == NULL)
    return false;

  return m_cb->Dialog.ProgressIsCanceled();
}


void XBMC_dialog_progress_close()
{
  if (m_cb == NULL)
    return;

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
