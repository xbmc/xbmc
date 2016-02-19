#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

#include "threads/Event.h"
#include "windows/GUIMediaWindow.h"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

  struct CB_AddOnLib;

  struct CAddOnWindow
  {
    static void Init(::V2::KodiAPI::CB_AddOnLib *callbacks);

    static GUIHANDLE    New(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
    static void         Delete(void *addonData, GUIHANDLE handle);
    static void         SetCallbacks(void *addonData,
                                     GUIHANDLE handle,
                                     GUIHANDLE clienthandle,
                                     bool (*initCB)(GUIHANDLE),
                                     bool (*clickCB)(GUIHANDLE, int),
                                     bool (*focusCB)(GUIHANDLE, int),
                                     bool (*onActionCB)(GUIHANDLE handle, int));
    static bool         Show(void *addonData, GUIHANDLE handle);
    static bool         Close(void *addonData, GUIHANDLE handle);
    static bool         DoModal(void *addonData, GUIHANDLE handle);
    static bool         SetFocusId(void *addonData, GUIHANDLE handle, int iControlId);
    static int          GetFocusId(void *addonData, GUIHANDLE handle);
    static void         SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
    static void         SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value);
    static void         SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value);
    static void         SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value);
    static void         GetProperty(void *addonData, GUIHANDLE handle, const char *key,char &property, unsigned int &iMaxStringSize);
    static int          GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key);
    static bool         GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key);
    static double       GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key);
    static void         ClearProperties(void *addonData, GUIHANDLE handle);
    static void         ClearProperty(void *addonData, GUIHANDLE handle, const char *key);
    static int          GetListSize(void *addonData, GUIHANDLE handle);
    static void         ClearList(void *addonData, GUIHANDLE handle);
    static GUIHANDLE    AddItem(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition);
    static GUIHANDLE    AddStringItem(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition);
    static void         RemoveItem(void *addonData, GUIHANDLE handle, int itemPosition);
    static void         RemoveItemFile(void *addonData, GUIHANDLE handle, GUIHANDLE fileItem);
    static GUIHANDLE    GetListItem(void *addonData, GUIHANDLE handle, int listPos);
    static void         SetCurrentListPosition(void *addonData, GUIHANDLE handle, int listPos);
    static int          GetCurrentListPosition(void *addonData, GUIHANDLE handle);
    static void         SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label);
    static void         MarkDirtyRegion(void *addonData, GUIHANDLE handle);

    static GUIHANDLE    GetControl_Button(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Edit(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_FadeLabel(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Image(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Label(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Spin(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_RadioButton(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Progress(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_RenderAddon(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_Slider(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_SettingsSlider(void *addonData, GUIHANDLE handle, int controlId);
    static GUIHANDLE    GetControl_TextBox(void *addonData, GUIHANDLE handle, int controlId);
  };

  class CGUIAddonWindow : public CGUIMediaWindow
  {
  friend class CAddOnWindow;

  public:
    CGUIAddonWindow(int id, const std::string& strXML, ADDON::CAddon* addon);
    virtual ~CGUIAddonWindow(void);

    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    void AllocResources(bool forceLoad = false) override;
    void FreeResources(bool forceUnLoad = false) override;
    void Render() override;
    void WaitForActionEvent(unsigned int timeout);
    void PulseActionEvent();
    void AddItem(CFileItemPtr fileItem, int itemPosition);
    void RemoveItem(int itemPosition);
    void RemoveItem(CFileItem* fileItem);
    void ClearList();
    CFileItemPtr GetListItem(int position);
    int GetListSize();
    int GetCurrentListPosition();
    void SetCurrentListPosition(int item);
    bool OnClick(int iItem, const std::string &player) override;

    CGUIControl* GetAddonControl(int controlId, CGUIControl::GUICONTROLTYPES type, std::string typeName);

  protected:
    void Update();
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    void SetupShares() override;

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
    ADDON::CAddon   *m_addon;
    std::string      m_mediaDir;
  };

  class CGUIAddonWindowDialog : public CGUIAddonWindow
  {
  public:
    CGUIAddonWindowDialog(int id, const std::string& strXML, ADDON::CAddon* addon);
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

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
