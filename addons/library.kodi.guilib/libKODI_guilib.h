#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libXBMC_addon.h"

typedef void* GUIHANDLE;

#ifdef _WIN32
#define GUI_HELPER_DLL "\\library.kodi.guilib\\libKODI_guilib" ADDON_HELPER_EXT
#else
#define GUI_HELPER_DLL_NAME "libKODI_guilib-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#define GUI_HELPER_DLL "/library.kodi.guilib/" GUI_HELPER_DLL_NAME
#endif

/* current ADDONGUI API version */
#define KODI_GUILIB_API_VERSION "5.10.0"

/* min. ADDONGUI API version */
#define KODI_GUILIB_MIN_API_VERSION "5.10.0"

#define ADDON_ACTION_PREVIOUS_MENU          10
#define ADDON_ACTION_CLOSE_DIALOG           51
#define ADDON_ACTION_NAV_BACK               92

class CAddonGUIWindow;
class CAddonGUISpinControl;
class CAddonGUIRadioButton;
class CAddonGUIProgressControl;
class CAddonListItem;
class CAddonGUIRenderingControl;
class CAddonGUISliderControl;
class CAddonGUISettingsSliderControl;

class CHelper_libKODI_guilib
{
public:
  CHelper_libKODI_guilib()
  {
    m_libKODI_guilib = NULL;
    m_Handle = NULL;
  }

  ~CHelper_libKODI_guilib()
  {
    if (m_libKODI_guilib)
    {
      GUI_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libKODI_guilib);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += GUI_HELPER_DLL;

#if defined(ANDROID)
      struct stat st;
      if(stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + GUI_HELPER_DLL_NAME;
      }
#endif

    m_libKODI_guilib = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_guilib == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    GUI_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libKODI_guilib, "GUI_register_me");
    if (GUI_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_unregister_me = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_unregister_me");
    if (GUI_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_lock = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_lock");
    if (GUI_lock == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_unlock = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_unlock");
    if (GUI_unlock == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_screen_height = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_get_screen_height");
    if (GUI_get_screen_height == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_screen_width = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_get_screen_width");
    if (GUI_get_screen_width == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_video_resolution = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_get_video_resolution");
    if (GUI_get_video_resolution == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_Window_create = (CAddonGUIWindow* (*)(void *HANDLE, void *CB, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog))
      dlsym(m_libKODI_guilib, "GUI_Window_create");
    if (GUI_Window_create == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_Window_destroy = (void (*)(CAddonGUIWindow* p))
      dlsym(m_libKODI_guilib, "GUI_Window_destroy");
    if (GUI_Window_destroy == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_spin = (CAddonGUISpinControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_spin");
    if (GUI_control_get_spin == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_spin = (void (*)(CAddonGUISpinControl* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_spin");
    if (GUI_control_release_spin == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_radiobutton  = (CAddonGUIRadioButton* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_radiobutton");
    if (GUI_control_get_radiobutton == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_radiobutton = (void (*)(CAddonGUIRadioButton* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_radiobutton");
    if (GUI_control_release_radiobutton == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_progress     = (CAddonGUIProgressControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_progress");
    if (GUI_control_get_progress == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_progress = (void (*)(CAddonGUIProgressControl* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_progress");
    if (GUI_control_release_progress == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_ListItem_create = (CAddonListItem* (*)(void *HANDLE, void *CB, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path))
      dlsym(m_libKODI_guilib, "GUI_ListItem_create");
    if (GUI_ListItem_create == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_ListItem_destroy = (void (*)(CAddonListItem* p))
      dlsym(m_libKODI_guilib, "GUI_ListItem_destroy");
    if (GUI_ListItem_destroy == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_rendering = (CAddonGUIRenderingControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_rendering");
    if (GUI_control_get_rendering == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_rendering = (void (*)(CAddonGUIRenderingControl* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_rendering");
    if (GUI_control_release_rendering == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_slider = (CAddonGUISliderControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_slider");
    if (GUI_control_get_slider == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_slider = (void (*)(CAddonGUISliderControl* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_slider");
    if (GUI_control_release_slider == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_settings_slider = (CAddonGUISettingsSliderControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libKODI_guilib, "GUI_control_get_settings_slider");
    if (GUI_control_get_settings_slider == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_settings_slider = (void (*)(CAddonGUISettingsSliderControl* p))
      dlsym(m_libKODI_guilib, "GUI_control_release_settings_slider");
    if (GUI_control_release_settings_slider == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_get_input_with_head = (bool (*)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_get_input_with_head");
    if (GUI_dialog_keyboard_show_and_get_input_with_head == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_get_input = (bool (*)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_get_input");
    if (GUI_dialog_keyboard_show_and_get_input == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_get_new_password_with_head = (bool (*)(void *HANDLE, void *CB, char &newPassword, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_get_new_password_with_head");
    if (GUI_dialog_keyboard_show_and_get_new_password_with_head == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_get_new_password = (bool (*)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_get_new_password");
    if (GUI_dialog_keyboard_show_and_get_new_password == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_verify_new_password_with_head = (bool (*)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_verify_new_password_with_head");
    if (GUI_dialog_keyboard_show_and_verify_new_password_with_head == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_verify_new_password = (bool (*)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_verify_new_password");
    if (GUI_dialog_keyboard_show_and_verify_new_password == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_verify_password = (int (*)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_verify_password");
    if (GUI_dialog_keyboard_show_and_verify_password == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_show_and_get_filter = (bool (*)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_show_and_get_filter");
    if (GUI_dialog_keyboard_show_and_get_filter == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_send_text_to_active_keyboard = (bool (*)(void *HANDLE, void *CB, const char *aTextString, bool closeKeyboard))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_send_text_to_active_keyboard");
    if (GUI_dialog_keyboard_send_text_to_active_keyboard == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_keyboard_is_activated = (bool (*)(void *HANDLE, void *CB))
      dlsym(m_libKODI_guilib, "GUI_dialog_keyboard_is_activated");
    if (GUI_dialog_keyboard_is_activated == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_verify_new_password = (bool (*)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_verify_new_password");
    if (GUI_dialog_numeric_show_and_verify_new_password == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_verify_password = (int (*)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_verify_password");
    if (GUI_dialog_numeric_show_and_verify_password == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_verify_input = (bool (*)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_verify_input");
    if (GUI_dialog_numeric_show_and_verify_input == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_get_time = (bool (*)(void *HANDLE, void *CB, tm &time, const char *strHeading))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_get_time");
    if (GUI_dialog_numeric_show_and_get_time == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_get_date = (bool (*)(void *HANDLE, void *CB, tm &date, const char *strHeading))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_get_date");
    if (GUI_dialog_numeric_show_and_get_date == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_get_ipaddress = (bool (*)(void *HANDLE, void *CB, char &IPAddress, unsigned int iMaxStringSize, const char *strHeading))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_get_ipaddress");
    if (GUI_dialog_numeric_show_and_get_ipaddress == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_get_number = (bool (*)(void *HANDLE, void *CB, char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_get_number");
    if (GUI_dialog_numeric_show_and_get_number == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_numeric_show_and_get_seconds = (bool (*)(void *HANDLE, void *CB, char &strTime, unsigned int iMaxStringSize, const char *strHeading))
      dlsym(m_libKODI_guilib, "GUI_dialog_numeric_show_and_get_seconds");
    if (GUI_dialog_numeric_show_and_get_seconds == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_filebrowser_show_and_get_file = (bool (*)(void *HANDLE, void *CB, const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList))
      dlsym(m_libKODI_guilib, "GUI_dialog_filebrowser_show_and_get_file");
    if (GUI_dialog_filebrowser_show_and_get_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_ok_show_and_get_input_single_text = (void (*)(void *HANDLE, void *CB, const char *heading, const char *text))
      dlsym(m_libKODI_guilib, "GUI_dialog_ok_show_and_get_input_single_text");
    if (GUI_dialog_ok_show_and_get_input_single_text == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_ok_show_and_get_input_line_text = (void (*)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2))
      dlsym(m_libKODI_guilib, "GUI_dialog_ok_show_and_get_input_line_text");
    if (GUI_dialog_ok_show_and_get_input_line_text == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_yesno_show_and_get_input_singletext = (bool (*)(void *HANDLE, void *CB, const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel))
      dlsym(m_libKODI_guilib, "GUI_dialog_yesno_show_and_get_input_singletext");
    if (GUI_dialog_yesno_show_and_get_input_singletext == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_yesno_show_and_get_input_linetext = (bool (*)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel))
      dlsym(m_libKODI_guilib, "GUI_dialog_yesno_show_and_get_input_linetext");
    if (GUI_dialog_yesno_show_and_get_input_linetext == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_yesno_show_and_get_input_linebuttontext = (bool (*)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel))
      dlsym(m_libKODI_guilib, "GUI_dialog_yesno_show_and_get_input_linebuttontext");
    if (GUI_dialog_yesno_show_and_get_input_linebuttontext == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_text_viewer = (void (*)(void *hdl, void *cb, const char *heading, const char *text))
      dlsym(m_libKODI_guilib, "GUI_dialog_text_viewer");
    if (GUI_dialog_text_viewer == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_dialog_select = (int (*)(void *hdl, void *cb, const char *heading, const char *entries[], unsigned int size, int selected))
      dlsym(m_libKODI_guilib, "GUI_dialog_select");
    if (GUI_dialog_select == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    m_Callbacks = GUI_register_me(m_Handle);
    return m_Callbacks != NULL;
  }

  void Lock()
  {
    return GUI_lock(m_Handle, m_Callbacks);
  }

  void Unlock()
  {
    return GUI_unlock(m_Handle, m_Callbacks);
  }

  int GetScreenHeight()
  {
    return GUI_get_screen_height(m_Handle, m_Callbacks);
  }

  int GetScreenWidth()
  {
    return GUI_get_screen_width(m_Handle, m_Callbacks);
  }

  int GetVideoResolution()
  {
    return GUI_get_video_resolution(m_Handle, m_Callbacks);
  }

  CAddonGUIWindow* Window_create(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
  {
    return GUI_Window_create(m_Handle, m_Callbacks, xmlFilename, defaultSkin, forceFallback, asDialog);
  }

  void Window_destroy(CAddonGUIWindow* p)
  {
    return GUI_Window_destroy(p);
  }

  CAddonGUISpinControl* Control_getSpin(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_spin(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSpin(CAddonGUISpinControl* p)
  {
    return GUI_control_release_spin(p);
  }

  CAddonGUIRadioButton* Control_getRadioButton(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_radiobutton(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseRadioButton(CAddonGUIRadioButton* p)
  {
    return GUI_control_release_radiobutton(p);
  }

  CAddonGUIProgressControl* Control_getProgress(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_progress(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseProgress(CAddonGUIProgressControl* p)
  {
    return GUI_control_release_progress(p);
  }

  CAddonListItem* ListItem_create(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
  {
    return GUI_ListItem_create(m_Handle, m_Callbacks, label, label2, iconImage, thumbnailImage, path);
  }

  void ListItem_destroy(CAddonListItem* p)
  {
    return GUI_ListItem_destroy(p);
  }

  CAddonGUIRenderingControl* Control_getRendering(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_rendering(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseRendering(CAddonGUIRenderingControl* p)
  {
    return GUI_control_release_rendering(p);
  }

  CAddonGUISliderControl* Control_getSlider(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_slider(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSlider(CAddonGUISliderControl* p)
  {
    return GUI_control_release_slider(p);
  }

  CAddonGUISettingsSliderControl* Control_getSettingsSlider(CAddonGUIWindow *window, int controlId)
  {
    return GUI_control_get_settings_slider(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSettingsSlider(CAddonGUISettingsSliderControl* p)
  {
    return GUI_control_release_settings_slider(p);
  }

  /*! @name GUI Keyboard functions */
  //@{
  bool Dialog_Keyboard_ShowAndGetInput(char &strText, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_get_input_with_head(m_Handle, m_Callbacks, strText, iMaxStringSize, strHeading, allowEmptyResult, hiddenInput, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetInput(char &strText, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_get_input(m_Handle, m_Callbacks, strText, iMaxStringSize, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_get_new_password_with_head(m_Handle, m_Callbacks, strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_get_new_password(m_Handle, m_Callbacks, strNewPassword, iMaxStringSize, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_verify_new_password_with_head(m_Handle, m_Callbacks, strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_verify_new_password(m_Handle, m_Callbacks, strNewPassword, iMaxStringSize, autoCloseMs);
  }

  int Dialog_Keyboard_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_verify_password(m_Handle, m_Callbacks, strPassword, iMaxStringSize, strHeading, iRetries, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetFilter(char &strText, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs = 0)
  {
    return GUI_dialog_keyboard_show_and_get_filter(m_Handle, m_Callbacks, strText, iMaxStringSize, searching, autoCloseMs);
  }

  bool Dialog_Keyboard_SendTextToActiveKeyboard(const char *aTextString, bool closeKeyboard = false)
  {
    return GUI_dialog_keyboard_send_text_to_active_keyboard(m_Handle, m_Callbacks, aTextString, closeKeyboard);
  }

  bool Dialog_Keyboard_isKeyboardActivated()
  {
    return GUI_dialog_keyboard_is_activated(m_Handle, m_Callbacks);
  }
  //@}

  /*! @name GUI Numeric functions */
  //@{
  bool Dialog_Numeric_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize)
  {
    return GUI_dialog_numeric_show_and_verify_new_password(m_Handle, m_Callbacks, strNewPassword, iMaxStringSize);
  }

  int Dialog_Numeric_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries)
  {
    return GUI_dialog_numeric_show_and_verify_password(m_Handle, m_Callbacks, strPassword, iMaxStringSize, strHeading, iRetries);
  }

  bool Dialog_Numeric_ShowAndVerifyInput(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput)
  {
    return GUI_dialog_numeric_show_and_verify_input(m_Handle, m_Callbacks, strPassword, iMaxStringSize, strHeading, bGetUserInput);
  }

  bool Dialog_Numeric_ShowAndGetTime(tm &time, const char *strHeading)
  {
    return GUI_dialog_numeric_show_and_get_time(m_Handle, m_Callbacks, time, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetDate(tm &date, const char *strHeading)
  {
    return GUI_dialog_numeric_show_and_get_date(m_Handle, m_Callbacks, date, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetIPAddress(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading)
  {
    return GUI_dialog_numeric_show_and_get_ipaddress(m_Handle, m_Callbacks, strIPAddress, iMaxStringSize, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetNumber(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs = 0)
  {
    return GUI_dialog_numeric_show_and_get_number(m_Handle, m_Callbacks, strInput, iMaxStringSize, strHeading, iAutoCloseTimeoutMs);
  }

  bool Dialog_Numeric_ShowAndGetSeconds(char &strTime, unsigned int iMaxStringSize, const char *strHeading)
  {
    return GUI_dialog_numeric_show_and_get_seconds(m_Handle, m_Callbacks, strTime, iMaxStringSize, strHeading);
  }
  //@}

  /*! @name GUI File browser functions */
  //@{
  bool Dialog_FileBrowser_ShowAndGetFile(const char *directory, const char *mask, const char *heading, char &strPath, unsigned int iMaxStringSize, bool useThumbs = false, bool useFileDirectories = false, bool singleList = false)
  {
    return GUI_dialog_filebrowser_show_and_get_file(m_Handle, m_Callbacks, directory, mask, heading, strPath, iMaxStringSize, useThumbs, useFileDirectories, singleList);
  }
  //@}

  /*! @name GUI OK Dialog functions */
  //@{
  void Dialog_OK_ShowAndGetInput(const char *heading, const char *text)
  {
    GUI_dialog_ok_show_and_get_input_single_text(m_Handle, m_Callbacks, heading, text);
  }

  void Dialog_OK_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2)
  {
    GUI_dialog_ok_show_and_get_input_line_text(m_Handle, m_Callbacks, heading, line0, line1, line2);
  }
  //@}

  /*! @name GUI Yes No Dialog functions */
  //@{
  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *text, bool& bCanceled, const char *noLabel = "", const char *yesLabel = "")
  {
    return GUI_dialog_yesno_show_and_get_input_singletext(m_Handle, m_Callbacks, heading, text, bCanceled, noLabel, yesLabel);
  }

  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel = "", const char *yesLabel = "")
  {
    return GUI_dialog_yesno_show_and_get_input_linetext(m_Handle, m_Callbacks, heading, line0, line1, line2, noLabel, yesLabel);
  }

  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel = "", const char *yesLabel = "")
  {
    return GUI_dialog_yesno_show_and_get_input_linebuttontext(m_Handle, m_Callbacks, heading, line0, line1, line2, bCanceled, noLabel, yesLabel);
  }
  //@}

  /*! @name GUI Text viewer Dialog */
  //@{
  void Dialog_TextViewer(const char *heading, const char *text)
  {
    return GUI_dialog_text_viewer(m_Handle, m_Callbacks, heading, text);
  }
  //@}

  /*! @name GUI select Dialog */
  //@{
  int Dialog_Select(const char *heading, const char *entries[], unsigned int size, int selected = -1)
  {
    return GUI_dialog_select(m_Handle, m_Callbacks, heading, entries, size, selected);
  }
  //@}

protected:
  void* (*GUI_register_me)(void *HANDLE);
  void (*GUI_unregister_me)(void *HANDLE, void* CB);
  void (*GUI_lock)(void *HANDLE, void* CB);
  void (*GUI_unlock)(void *HANDLE, void* CB);
  int (*GUI_get_screen_height)(void *HANDLE, void* CB);
  int (*GUI_get_screen_width)(void *HANDLE, void* CB);
  int (*GUI_get_video_resolution)(void *HANDLE, void* CB);
  CAddonGUIWindow* (*GUI_Window_create)(void *HANDLE, void* CB, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  void (*GUI_Window_destroy)(CAddonGUIWindow* p);
  CAddonGUISpinControl* (*GUI_control_get_spin)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_spin)(CAddonGUISpinControl* p);
  CAddonGUIRadioButton* (*GUI_control_get_radiobutton)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_radiobutton)(CAddonGUIRadioButton* p);
  CAddonGUIProgressControl* (*GUI_control_get_progress)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_progress)(CAddonGUIProgressControl* p);
  CAddonListItem* (*GUI_ListItem_create)(void *HANDLE, void* CB, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  void (*GUI_ListItem_destroy)(CAddonListItem* p);
  CAddonGUIRenderingControl* (*GUI_control_get_rendering)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_rendering)(CAddonGUIRenderingControl* p);
  CAddonGUISliderControl* (*GUI_control_get_slider)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_slider)(CAddonGUISliderControl* p);
  CAddonGUISettingsSliderControl* (*GUI_control_get_settings_slider)(void *HANDLE, void* CB, CAddonGUIWindow *window, int controlId);
  void (*GUI_control_release_settings_slider)(CAddonGUISettingsSliderControl* p);
  bool (*GUI_dialog_keyboard_show_and_get_input_with_head)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_get_input)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_get_new_password_with_head)(void *HANDLE, void *CB, char &newPassword, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_get_new_password)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_verify_new_password_with_head)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_verify_new_password)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  int (*GUI_dialog_keyboard_show_and_verify_password)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_show_and_get_filter)(void *HANDLE, void *CB, char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs);
  bool (*GUI_dialog_keyboard_send_text_to_active_keyboard)(void *HANDLE, void *CB, const char *aTextString, bool closeKeyboard);
  bool (*GUI_dialog_keyboard_is_activated)(void *HANDLE, void *CB);
  bool (*GUI_dialog_numeric_show_and_verify_new_password)(void *HANDLE, void *CB, char &strNewPassword, unsigned int iMaxStringSize);
  int (*GUI_dialog_numeric_show_and_verify_password)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries);
  bool (*GUI_dialog_numeric_show_and_verify_input)(void *HANDLE, void *CB, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput);
  bool (*GUI_dialog_numeric_show_and_get_time)(void *HANDLE, void *CB, tm &time, const char *strHeading);
  bool (*GUI_dialog_numeric_show_and_get_date)(void *HANDLE, void *CB, tm &date, const char *strHeading);
  bool (*GUI_dialog_numeric_show_and_get_ipaddress)(void *HANDLE, void *CB, char &IPAddress, unsigned int iMaxStringSize, const char *strHeading);
  bool (*GUI_dialog_numeric_show_and_get_number)(void *HANDLE, void *CB, char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs);
  bool (*GUI_dialog_numeric_show_and_get_seconds)(void *HANDLE, void *CB, char &strTime, unsigned int iMaxStringSize, const char *strHeading);
  bool (*GUI_dialog_filebrowser_show_and_get_file)(void *HANDLE, void *CB, const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList);
  void (*GUI_dialog_ok_show_and_get_input_single_text)(void *HANDLE, void *CB, const char *heading, const char *text);
  void (*GUI_dialog_ok_show_and_get_input_line_text)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2);
  bool (*GUI_dialog_yesno_show_and_get_input_singletext)(void *HANDLE, void *CB, const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel);
  bool (*GUI_dialog_yesno_show_and_get_input_linetext)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
  bool (*GUI_dialog_yesno_show_and_get_input_linebuttontext)(void *HANDLE, void *CB, const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel);
  void (*GUI_dialog_text_viewer)(void *hdl, void *cb, const char *heading, const char *text);
  int (*GUI_dialog_select)(void *hdl, void *cb, const char *heading, const char *entries[], unsigned int size, int selected);

private:
  void *m_libKODI_guilib;
  void *m_Handle;
  void *m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

class CAddonGUISpinControl
{
public:
  CAddonGUISpinControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUISpinControl(void) {}

  virtual void SetVisible(bool yesNo);
  virtual void SetText(const char *label);
  virtual void Clear();
  virtual void AddLabel(const char *label, int iValue);
  virtual int GetValue();
  virtual void SetValue(int iValue);

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_SpinHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUIRadioButton
{
public:
  CAddonGUIRadioButton(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUIRadioButton() {}

  virtual void SetVisible(bool yesNo);
  virtual void SetText(const char *label);
  virtual void SetSelected(bool yesNo);
  virtual bool IsSelected();

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_ButtonHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUIProgressControl
{
public:
  CAddonGUIProgressControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUIProgressControl(void) {}

  virtual void SetPercentage(float fPercent);
  virtual float GetPercentage() const;
  virtual void SetInfo(int iInfo);
  virtual int GetInfo() const;
  virtual std::string GetDescription() const;

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_ProgressHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUISliderControl
{
public:
  CAddonGUISliderControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUISliderControl(void) {}

  virtual void        SetVisible(bool yesNo);
  virtual std::string GetDescription() const;

  virtual void        SetIntRange(int iStart, int iEnd);
  virtual void        SetIntValue(int iValue);
  virtual int         GetIntValue() const;
  virtual void        SetIntInterval(int iInterval);

  virtual void        SetPercentage(float fPercent);
  virtual float       GetPercentage() const;

  virtual void        SetFloatRange(float fStart, float fEnd);
  virtual void        SetFloatValue(float fValue);
  virtual float       GetFloatValue() const;
  virtual void        SetFloatInterval(float fInterval);

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_SliderHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUISettingsSliderControl
{
public:
  CAddonGUISettingsSliderControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUISettingsSliderControl(void) {}

  virtual void        SetVisible(bool yesNo);
  virtual void        SetText(const char *label);
  virtual std::string GetDescription() const;

  virtual void        SetIntRange(int iStart, int iEnd);
  virtual void        SetIntValue(int iValue);
  virtual int         GetIntValue() const;
  virtual void        SetIntInterval(int iInterval);

  virtual void        SetPercentage(float fPercent);
  virtual float       GetPercentage() const;

  virtual void        SetFloatRange(float fStart, float fEnd);
  virtual void        SetFloatValue(float fValue);
  virtual float       GetFloatValue() const;
  virtual void        SetFloatInterval(float fInterval);

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_SettingsSliderHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonListItem
{
friend class CAddonGUIWindow;

public:
  CAddonListItem(void *hdl, void *cb, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  virtual ~CAddonListItem(void) {}

  virtual const char  *GetLabel();
  virtual void         SetLabel(const char *label);
  virtual const char  *GetLabel2();
  virtual void         SetLabel2(const char *label);
  virtual void         SetIconImage(const char *image);
  virtual void         SetThumbnailImage(const char *image);
  virtual void         SetInfo(const char *Info);
  virtual void         SetProperty(const char *key, const char *value);
  virtual const char  *GetProperty(const char *key) const;
  virtual void         SetPath(const char *Path);

//    {(char*)"select();
//    {(char*)"isSelected();
protected:
  GUIHANDLE   m_ListItemHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUIWindow
{
friend class CAddonGUISpinControl;
friend class CAddonGUIRadioButton;
friend class CAddonGUIProgressControl;
friend class CAddonGUIRenderingControl;
friend class CAddonGUISliderControl;
friend class CAddonGUISettingsSliderControl;

public:
  CAddonGUIWindow(void *hdl, void *cb, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  virtual ~CAddonGUIWindow();

  virtual bool         Show();
  virtual void         Close();
  virtual void         DoModal();
  virtual bool         SetFocusId(int iControlId);
  virtual int          GetFocusId();
  virtual bool         SetCoordinateResolution(int res);
  virtual void         SetProperty(const char *key, const char *value);
  virtual void         SetPropertyInt(const char *key, int value);
  virtual void         SetPropertyBool(const char *key, bool value);
  virtual void         SetPropertyDouble(const char *key, double value);
  virtual const char  *GetProperty(const char *key) const;
  virtual int          GetPropertyInt(const char *key) const;
  virtual bool         GetPropertyBool(const char *key) const;
  virtual double       GetPropertyDouble(const char *key) const;
  virtual void         ClearProperties();
  virtual int          GetListSize();
  virtual void         ClearList();
  virtual GUIHANDLE    AddStringItem(const char *name, int itemPosition = -1);
  virtual void         AddItem(GUIHANDLE item, int itemPosition = -1);
  virtual void         AddItem(CAddonListItem *item, int itemPosition = -1);
  virtual void         RemoveItem(int itemPosition);
  virtual GUIHANDLE    GetListItem(int listPos);
  virtual void         SetCurrentListPosition(int listPos);
  virtual int          GetCurrentListPosition();
  virtual void         SetControlLabel(int controlId, const char *label);
  virtual void         MarkDirtyRegion();

  virtual bool         OnClick(int controlId);
  virtual bool         OnFocus(int controlId);
  virtual bool         OnInit();
  virtual bool         OnAction(int actionId);

  GUIHANDLE m_cbhdl;
  bool (*CBOnInit)(GUIHANDLE cbhdl);
  bool (*CBOnFocus)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnClick)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnAction)(GUIHANDLE cbhdl, int actionId);

protected:
  GUIHANDLE m_WindowHandle;
  void *m_Handle;
  void *m_cb;
};

class CAddonGUIRenderingControl
{
public:
  CAddonGUIRenderingControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId);
  virtual ~CAddonGUIRenderingControl();
  virtual void Init();

  virtual bool Create(int x, int y, int w, int h, void *device);
  virtual void Render();
  virtual void Stop();
  virtual bool Dirty();

  GUIHANDLE m_cbhdl;
  bool (*CBCreate)(GUIHANDLE cbhdl, int x, int y, int w, int h, void *device);
  void (*CBRender)(GUIHANDLE cbhdl);
  void (*CBStop)(GUIHANDLE cbhdl);
  bool (*CBDirty)(GUIHANDLE cbhdl);

private:
  CAddonGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_RenderingHandle;
  void *m_Handle;
  void *m_cb;
};
