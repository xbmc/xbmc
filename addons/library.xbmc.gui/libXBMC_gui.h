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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef void* GUIHANDLE;

#ifndef _LINUX
#include "../library.xbmc.addon/dlfcn-win32.h"
#define GUI_HELPER_DLL "\\library.xbmc.gui\\libXBMC_gui.dll"
#else
#include <dlfcn.h>
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-powerpc-osx.so"
#else
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-x86-osx.so"
#endif
#elif defined(__x86_64__)
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-x86_64-linux.so"
#elif defined(_POWERPC)
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-powerpc-linux.so"
#elif defined(_POWERPC64)
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-powerpc64-linux.so"
#else /* !__x86_64__ && !__powerpc__ */
#define GUI_HELPER_DLL "/library.xbmc.gui/libXBMC_gui-i486-linux.so"
#endif /* __x86_64__ */
#endif /* _LINUX */

#define ADDON_ACTION_PREVIOUS_MENU          10
#define ADDON_ACTION_CLOSE_DIALOG           51

class cGUIWindow;
class cGUISpinControl;
class cGUIRadioButton;
class cGUIProgressControl;
class cListItem;

class cHelper_libXBMC_gui
{
public:
  cHelper_libXBMC_gui()
  {
    m_libXBMC_gui = NULL;
    m_Handle      = NULL;
  }

  ~cHelper_libXBMC_gui()
  {
    if (m_libXBMC_gui)
    {
      GUI_unregister_me();
      dlclose(m_libXBMC_gui);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += GUI_HELPER_DLL;

    m_libXBMC_gui = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libXBMC_gui == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    GUI_register_me         = (int (*)(void *HANDLE))
      dlsym(m_libXBMC_gui, "GUI_register_me");
    if (GUI_register_me == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GUI_unregister_me       = (void (*)())
      dlsym(m_libXBMC_gui, "GUI_unregister_me");
    if (GUI_unregister_me == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Lock                    = (void (*)())
      dlsym(m_libXBMC_gui, "GUI_lock");
    if (Lock == NULL)                 { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Unlock                  = (void (*)())
      dlsym(m_libXBMC_gui, "GUI_unlock");
    if (Unlock == NULL)               { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetScreenHeight         = (int (*)())
      dlsym(m_libXBMC_gui, "GUI_get_screen_height");
    if (GetScreenHeight == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetScreenWidth          = (int (*)())
      dlsym(m_libXBMC_gui, "GUI_get_screen_width");
    if (GetScreenWidth == NULL)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetVideoResolution      = (int (*)())
      dlsym(m_libXBMC_gui, "GUI_get_video_resolution");
    if (GetVideoResolution == NULL)   { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Window_create           = (cGUIWindow* (*)(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog))
      dlsym(m_libXBMC_gui, "GUI_Window_create");
    if (Window_create == NULL)        { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Window_destroy          = (void (*)(cGUIWindow* p))
      dlsym(m_libXBMC_gui, "GUI_Window_destroy");
    if (Window_destroy == NULL)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_getSpin         = (cGUISpinControl* (*)(cGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_spin");
    if (Control_getSpin == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_releaseSpin     = (void (*)(cGUISpinControl* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_spin");
    if (Control_releaseSpin == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_getRadioButton  = (cGUIRadioButton* (*)(cGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_radiobutton");
    if (Control_getRadioButton == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_releaseRadioButton = (void (*)(cGUIRadioButton* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_radiobutton");
    if (Control_releaseRadioButton == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_getProgress     = (cGUIProgressControl* (*)(cGUIWindow *window, int controlId))
      dlsym(m_libXBMC_gui, "GUI_control_get_progress");
    if (Control_getProgress == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Control_releaseProgress = (void (*)(cGUIProgressControl* p))
      dlsym(m_libXBMC_gui, "GUI_control_release_progress");
    if (Control_releaseProgress == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ListItem_create         = (cListItem* (*)(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path))
      dlsym(m_libXBMC_gui, "GUI_ListItem_create");
    if (ListItem_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ListItem_destroy        = (void (*)(cListItem* p))
      dlsym(m_libXBMC_gui, "GUI_ListItem_destroy");
    if (ListItem_destroy == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }


    return GUI_register_me(m_Handle) > 0;
  }

  void (*Lock)();
  void (*Unlock)();
  int (*GetScreenHeight)();
  int (*GetScreenWidth)();
  int (*GetVideoResolution)();
  cGUIWindow* (*Window_create)(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  void (*Window_destroy)(cGUIWindow* p);
  cGUISpinControl* (*Control_getSpin)(cGUIWindow *window, int controlId);
  void (*Control_releaseSpin)(cGUISpinControl* p);
  cGUIRadioButton* (*Control_getRadioButton)(cGUIWindow *window, int controlId);
  void (*Control_releaseRadioButton)(cGUIRadioButton* p);
  cGUIProgressControl* (*Control_getProgress)(cGUIWindow *window, int controlId);
  void (*Control_releaseProgress)(cGUIProgressControl* p);
  cListItem* (*ListItem_create)(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  void (*ListItem_destroy)(cListItem* p);

protected:
  int (*GUI_register_me)(void *HANDLE);
  void (*GUI_unregister_me)();

private:
  void *m_libXBMC_gui;
  void *m_Handle;
  struct cb_array
  {
    const char* libPath;
  };
};

class cGUISpinControl
{
public:
  cGUISpinControl(cGUIWindow *window, int controlId);
  virtual ~cGUISpinControl(void) {}

  virtual void SetVisible(bool yesNo);
  virtual void SetText(const char *label);
  virtual void Clear();
  virtual void AddLabel(const char *label, int iValue);
  virtual int GetValue();
  virtual void SetValue(int iValue);

private:
  cGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_SpinHandle;
};

class cGUIRadioButton
{
public:
  cGUIRadioButton(cGUIWindow *window, int controlId);
  ~cGUIRadioButton() {}

  virtual void SetVisible(bool yesNo);
  virtual void SetText(const char *label);
  virtual void SetSelected(bool yesNo);
  virtual bool IsSelected();

private:
  cGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_ButtonHandle;
};

class cGUIProgressControl
{
public:
  cGUIProgressControl(cGUIWindow *window, int controlId);
  virtual ~cGUIProgressControl(void) {}

  virtual void SetPercentage(float fPercent);
  virtual float GetPercentage() const;
  virtual void SetInfo(int iInfo);
  virtual int GetInfo() const;
  virtual std::string GetDescription() const;

private:
  cGUIWindow *m_Window;
  int         m_ControlId;
  GUIHANDLE   m_ProgressHandle;
};

class cListItem
{
friend class cGUIWindow;

public:
  cListItem(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  virtual ~cListItem(void) {}

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
};

class cGUIWindow
{
friend class cGUISpinControl;
friend class cGUIRadioButton;
friend class cGUIProgressControl;

public:
  cGUIWindow(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  ~cGUIWindow();

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
  virtual void         AddItem(cListItem *item, int itemPosition = -1);
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
};

