#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../library.xbmc.addon/libXBMC_addon.h"

typedef void* GUIHANDLE;

#ifdef _WIN32
#define GUI_HELPER_DLL "\\library.xbmc.gui\\libXBMC_gui" ADDON_HELPER_EXT
#else
#define GUI_HELPER_DLL_NAME "libXBMC_gui-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#define GUI_HELPER_DLL "/library.xbmc.gui/" GUI_HELPER_DLL_NAME
#endif

#define ADDON_ACTION_PREVIOUS_MENU          10
#define ADDON_ACTION_CLOSE_DIALOG           51

class CAddonGUIWindow;
class CAddonGUISpinControl;
class CAddonGUIRadioButton;
class CAddonGUIProgressControl;
class CAddonListItem;

class CHelper_libXBMC_gui
{
public:
  CHelper_libXBMC_gui()
  {
    m_libXBMC_gui = NULL;
    m_Handle      = NULL;
  }

  ~CHelper_libXBMC_gui()
  {
    if (m_libXBMC_gui)
    {
      GUI_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libXBMC_gui);
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

    m_libXBMC_gui = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libXBMC_gui == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    GUI_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libXBMC_gui, "GUI_register_me");
    if (GUI_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_unregister_me = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_unregister_me");
    if (GUI_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_lock = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_lock");
    if (GUI_lock == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_unlock = (void (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_unlock");
    if (GUI_unlock == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_screen_height = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_get_screen_height");
    if (GUI_get_screen_height == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_screen_width = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_get_screen_width");
    if (GUI_get_screen_width == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_get_video_resolution = (int (*)(void *HANDLE, void *CB))
      dlsym(m_libXBMC_gui, "GUI_get_video_resolution");
    if (GUI_get_video_resolution == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_Window_create = (CAddonGUIWindow* (*)(void *HANDLE, void *CB, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog))
      dlsym(m_libXBMC_gui, "GUI_Window_create");
    if (GUI_Window_create == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_Window_destroy = (void (*)(CAddonGUIWindow* p))
      dlsym(m_libXBMC_gui, "GUI_Window_destroy");
    if (GUI_Window_destroy == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_spin = (CAddonGUISpinControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_spin");
    if (GUI_control_get_spin == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_spin = (void (*)(CAddonGUISpinControl* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_spin");
    if (GUI_control_release_spin == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_radiobutton  = (CAddonGUIRadioButton* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_radiobutton");
    if (GUI_control_get_radiobutton == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_radiobutton = (void (*)(CAddonGUIRadioButton* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_radiobutton");
    if (GUI_control_release_radiobutton == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_get_progress     = (CAddonGUIProgressControl* (*)(void *HANDLE, void *CB, CAddonGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_progress");
    if (GUI_control_get_progress == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_control_release_progress = (void (*)(CAddonGUIProgressControl* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_progress");
    if (GUI_control_release_progress == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_ListItem_create = (CAddonListItem* (*)(void *HANDLE, void *CB, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path))
      dlsym(m_libXBMC_gui, "GUI_ListItem_create");
    if (GUI_ListItem_create == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_ListItem_destroy = (void (*)(CAddonListItem* p))
      dlsym(m_libXBMC_gui, "GUI_ListItem_destroy");
    if (GUI_ListItem_destroy == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }


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

private:
  void *m_libXBMC_gui;
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
