#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "threads/Event.h"
#include "windows/GUIMediaWindow.h"

namespace ADDON { class CAddon; }

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace GUI
{
extern "C"
{

  struct CAddOnWindow
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static void*    New(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
    static void     Delete(void *addonData, void* handle);
    static void     SetCallbacks(void *addonData,
                                 void* handle,
                                 void* clienthandle,
                                 bool (*initCB)(void*),
                                 bool (*clickCB)(void*, int),
                                 bool (*focusCB)(void*, int),
                                 bool (*onActionCB)(void* handle, int));
    static bool     Show(void *addonData, void* handle);
    static bool     Close(void *addonData, void* handle);
    static bool     DoModal(void *addonData, void* handle);
    static bool     SetFocusId(void *addonData, void* handle, int iControlId);
    static int      GetFocusId(void *addonData, void* handle);
    static void     SetProperty(void *addonData, void* handle, const char *key, const char *value);
    static void     SetPropertyInt(void *addonData, void* handle, const char *key, int value);
    static void     SetPropertyBool(void *addonData, void* handle, const char *key, bool value);
    static void     SetPropertyDouble(void *addonData, void* handle, const char *key, double value);
    static void     GetProperty(void *addonData, void* handle, const char *key,char &property, unsigned int &iMaxStringSize);
    static int      GetPropertyInt(void *addonData, void* handle, const char *key);
    static bool     GetPropertyBool(void *addonData, void* handle, const char *key);
    static double   GetPropertyDouble(void *addonData, void* handle, const char *key);
    static void     ClearProperties(void *addonData, void* handle);
    static void     ClearProperty(void *addonData, void* handle, const char *key);
    static int      GetListSize(void *addonData, void* handle);
    static void     ClearList(void *addonData, void* handle);
    static void*    AddItem(void *addonData, void* handle, void* item, int itemPosition);
    static void*    AddStringItem(void *addonData, void* handle, const char *itemName, int itemPosition);
    static void     RemoveItem(void *addonData, void* handle, int itemPosition);
    static void     RemoveItemFile(void *addonData, void* handle, void* fileItem);
    static void*    GetListItem(void *addonData, void* handle, int listPos);
    static void     SetCurrentListPosition(void *addonData, void* handle, int listPos);
    static int      GetCurrentListPosition(void *addonData, void* handle);
    static void     SetControlLabel(void *addonData, void* handle, int controlId, const char *label);
    static void     MarkDirtyRegion(void *addonData, void* handle);

    static void*    GetControl_Button(void *addonData, void* handle, int controlId);
    static void*    GetControl_Edit(void *addonData, void* handle, int controlId);
    static void*    GetControl_FadeLabel(void *addonData, void* handle, int controlId);
    static void*    GetControl_Image(void *addonData, void* handle, int controlId);
    static void*    GetControl_Label(void *addonData, void* handle, int controlId);
    static void*    GetControl_Spin(void *addonData, void* handle, int controlId);
    static void*    GetControl_RadioButton(void *addonData, void* handle, int controlId);
    static void*    GetControl_Progress(void *addonData, void* handle, int controlId);
    static void*    GetControl_RenderAddon(void *addonData, void* handle, int controlId);
    static void*    GetControl_Slider(void *addonData, void* handle, int controlId);
    static void*    GetControl_SettingsSlider(void *addonData, void* handle, int controlId);
    static void*    GetControl_TextBox(void *addonData, void* handle, int controlId);
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

    bool (*CBOnInit)(void* cbhdl);
    bool (*CBOnFocus)(void* cbhdl, int controlId);
    bool (*CBOnClick)(void* cbhdl, int controlId);
    bool (*CBOnAction)(void* cbhdl, int);

    void*            m_clientHandle;
    const int        m_iWindowId;
    int              m_iOldWindowId;
    bool             m_bModal;
    bool             m_bIsDialog;

  private:
    CEvent           m_actionEvent;
    ADDON::CAddon*   m_addon;
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

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
