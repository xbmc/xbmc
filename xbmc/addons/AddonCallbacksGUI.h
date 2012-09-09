#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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


#include "AddonCallbacks.h"
#include "windows/GUIMediaWindow.h"
#include "threads/Event.h"

class CGUISpinControlEx;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUIEditControl;

namespace ADDON
{

class CAddonCallbacksGUI
{
public:
  CAddonCallbacksGUI(CAddon* addon);
  ~CAddonCallbacksGUI();

  /**! \name General Functions */
  CB_GUILib *GetCallbacks() { return m_callbacks; }

  static void         Lock();
  static void         Unlock();
  static int          GetScreenHeight();
  static int          GetScreenWidth();
  static int          GetVideoResolution();

  static GUIHANDLE    Window_New(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  static void         Window_Delete(void *addonData, GUIHANDLE handle);
  static void         Window_SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*initCB)(GUIHANDLE), bool (*clickCB)(GUIHANDLE, int), bool (*focusCB)(GUIHANDLE, int), bool (*onActionCB)(GUIHANDLE handle, int));
  static bool         Window_Show(void *addonData, GUIHANDLE handle);
  static bool         Window_Close(void *addonData, GUIHANDLE handle);
  static bool         Window_DoModal(void *addonData, GUIHANDLE handle);
  static bool         Window_SetFocusId(void *addonData, GUIHANDLE handle, int iControlId);
  static int          Window_GetFocusId(void *addonData, GUIHANDLE handle);
  static bool         Window_SetCoordinateResolution(void *addonData, GUIHANDLE handle, int res);
  static void         Window_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  static void         Window_SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value);
  static void         Window_SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value);
  static void         Window_SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value);
  static const char * Window_GetProperty(void *addonData, GUIHANDLE handle, const char *key);
  static int          Window_GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key);
  static bool         Window_GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key);
  static double       Window_GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key);
  static void         Window_ClearProperties(void *addonData, GUIHANDLE handle);
  static int          Window_GetListSize(void *addonData, GUIHANDLE handle);
  static void         Window_ClearList(void *addonData, GUIHANDLE handle);
  static GUIHANDLE    Window_AddItem(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition);
  static GUIHANDLE    Window_AddStringItem(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition);
  static void         Window_RemoveItem(void *addonData, GUIHANDLE handle, int itemPosition);
  static GUIHANDLE    Window_GetListItem(void *addonData, GUIHANDLE handle, int listPos);
  static void         Window_SetCurrentListPosition(void *addonData, GUIHANDLE handle, int listPos);
  static int          Window_GetCurrentListPosition(void *addonData, GUIHANDLE handle);
  static GUIHANDLE    Window_GetControl_Spin(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Button(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_RadioButton(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Edit(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Progress(void *addonData, GUIHANDLE handle, int controlId);
  static void         Window_SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label);
  static void         Control_Spin_SetVisible(void *addonData, GUIHANDLE spinhandle, bool yesNo);
  static void         Control_Spin_SetText(void *addonData, GUIHANDLE spinhandle, const char *label);
  static void         Control_Spin_Clear(void *addonData, GUIHANDLE spinhandle);
  static void         Control_Spin_AddLabel(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue);
  static int          Control_Spin_GetValue(void *addonData, GUIHANDLE spinhandle);
  static void         Control_Spin_SetValue(void *addonData, GUIHANDLE spinhandle, int iValue);
  static void         Control_RadioButton_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo);
  static void         Control_RadioButton_SetText(void *addonData, GUIHANDLE handle, const char *label);
  static void         Control_RadioButton_SetSelected(void *addonData, GUIHANDLE handle, bool yesNo);
  static bool         Control_RadioButton_IsSelected(void *addonData, GUIHANDLE handle);
  static void         Control_Progress_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent);
  static float        Control_Progress_GetPercentage(void *addonData, GUIHANDLE handle);
  static void         Control_Progress_SetInfo(void *addonData, GUIHANDLE handle, int iInfo);
  static int          Control_Progress_GetInfo(void *addonData, GUIHANDLE handle);
  static const char * Control_Progress_GetDescription(void *addonData, GUIHANDLE handle);
  static GUIHANDLE    ListItem_Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  static const char * ListItem_GetLabel(void *addonData, GUIHANDLE handle);
  static void         ListItem_SetLabel(void *addonData, GUIHANDLE handle, const char *label);
  static const char * ListItem_GetLabel2(void *addonData, GUIHANDLE handle);
  static void         ListItem_SetLabel2(void *addonData, GUIHANDLE handle, const char *label);
  static void         ListItem_SetIconImage(void *addonData, GUIHANDLE handle, const char *image);
  static void         ListItem_SetThumbnailImage(void *addonData, GUIHANDLE handle, const char *image);
  static void         ListItem_SetInfo(void *addonData, GUIHANDLE handle, const char *info);
  static void         ListItem_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  static const char * ListItem_GetProperty(void *addonData, GUIHANDLE handle, const char *key);
  static void         ListItem_SetPath(void *addonData, GUIHANDLE handle, const char *path);

private:
  CB_GUILib    *m_callbacks;
  CAddon       *m_addon;
};

class CGUIAddonWindow : public CGUIMediaWindow
{
friend class CAddonCallbacksGUI;

public:
  CGUIAddonWindow(int id, CStdString strXML, CAddon* addon);
  virtual ~CGUIAddonWindow(void);

  virtual bool      OnMessage(CGUIMessage& message);
  virtual bool      OnAction(const CAction &action);
  virtual void      AllocResources(bool forceLoad = false);
  virtual void      FreeResources(bool forceUnLoad = false);
  virtual void      Render();
  void              WaitForActionEvent(unsigned int timeout);
  void              PulseActionEvent();
  void              AddItem(CFileItemPtr fileItem, int itemPosition);
  void              RemoveItem(int itemPosition);
  void              ClearList();
  CFileItemPtr      GetListItem(int position);
  int               GetListSize();
  int               GetCurrentListPosition();
  void              SetCurrentListPosition(int item);
  virtual bool      OnClick(int iItem);

protected:
  virtual void     Update();
  virtual void     GetContextButtons(int itemNumber, CContextButtons &buttons);
  void             ClearAddonStrings();
  void             SetupShares();

  bool (*CBOnInit)(GUIHANDLE cbhdl);
  bool (*CBOnFocus)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnClick)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnAction)(GUIHANDLE cbhdl, int);

  GUIHANDLE        m_clientHandle;
  const int m_iWindowId;
  int m_iOldWindowId;
  bool m_bModal;
  bool m_bIsDialog;

private:
  CEvent           m_actionEvent;
  CAddon          *m_addon;
  CStdString       m_mediaDir;
};

class CGUIAddonWindowDialog : public CGUIAddonWindow
{
public:
  CGUIAddonWindowDialog(int id, CStdString strXML, CAddon* addon);
  virtual ~CGUIAddonWindowDialog(void);

  void            Show(bool show = true);
  virtual bool    OnMessage(CGUIMessage &message);
  virtual bool    IsDialogRunning() const { return m_bRunning; }
  virtual bool    IsDialog() const { return true;};
  virtual bool    IsModalDialog() const { return true; };
  virtual bool    IsMediaWindow() const { return false; };

  void Show_Internal(bool show = true);

private:
  bool             m_bRunning;
};

}; /* namespace ADDON */
