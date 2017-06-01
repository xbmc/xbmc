#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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

extern "C"
{

struct AddonGlobalInterface;
struct gui_context_menu_pair;

namespace ADDON
{
  class CAddonDll;

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/Window.h"
   */
  struct Interface_GUIWindow
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static void* create(void* kodiBase, const char* xml_filename, const char* default_skin, bool as_dialog, bool is_media);
    static void destroy(void* kodiBase, void* handle);
    static void set_callbacks(void* kodiBase,
                              void* handle,
                              void* clienthandle,
                              bool (*CBInit)(void*),
                              bool (*CBFocus)(void*, int),
                              bool (*CBClick)(void*, int),
                              bool (*CBOnAction)(void*, int),
                              void (*CBGetContextButtons)(void* , int, gui_context_menu_pair*, unsigned int*),
                              bool (*CBOnContextButton)(void*, int, unsigned int));
    static bool show(void* kodiBase, void* handle);
    static bool close(void* kodiBase, void* handle);
    static bool do_modal(void* kodiBase, void* handle);
    //@}

  private:
    static int GetNextAvailableWindowId();
  };

  class CGUIAddonWindow : public CGUIMediaWindow
  {
  friend struct Interface_GUIWindow;

  public:
    CGUIAddonWindow(int id, const std::string& strXML, ADDON::CAddonDll* addon, bool isMedia);
    virtual ~CGUIAddonWindow() = default;

    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    void AllocResources(bool forceLoad = false) override;
    void Render() override;
    bool IsMediaWindow() const override { return m_isMedia; }

    /* Addon to Kodi call functions */
    void PulseActionEvent();
    void AddItem(CFileItemPtr* fileItem, int itemPosition);
    void RemoveItem(int itemPosition);
    void RemoveItem(CFileItemPtr* fileItem);
    void ClearList();
    CFileItemPtr* GetListItem(int position);
    int GetListSize();
    int GetCurrentListPosition();
    void SetCurrentListPosition(int item);
    CGUIControl* GetAddonControl(int controlId, CGUIControl::GUICONTROLTYPES type, std::string typeName);

  protected:
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    void SetupShares() override;

    /* kodi to addon callback function addresses */
    void* m_clientHandle;
    bool (*CBOnInit)(void* cbhdl);
    bool (*CBOnFocus)(void* cbhdl, int controlId);
    bool (*CBOnClick)(void* cbhdl, int controlId);
    bool (*CBOnAction)(void* cbhdl, int actionId);
    void (*CBGetContextButtons)(void* cbhdl, int itemNumber, gui_context_menu_pair* buttons, unsigned int* size);
    bool (*CBOnContextButton)(void* cbhdl, int itemNumber, unsigned int button);

    const int m_windowId;
    int m_oldWindowId;

  private:
    void WaitForActionEvent(unsigned int timeout);

    CEvent m_actionEvent;
    ADDON::CAddonDll* m_addon;
    std::string m_mediaDir;
    bool m_isMedia;
  };

  class CGUIAddonWindowDialog : public CGUIAddonWindow
  {
  public:
    CGUIAddonWindowDialog(int id, const std::string& strXML, ADDON::CAddonDll* addon);

    bool IsDialogRunning() const override { return m_bRunning; }
    bool IsDialog() const override { return true; };
    bool IsModalDialog() const  override { return true; };

    void Show(bool show = true);
    void Show_Internal(bool show = true);

  private:
    bool m_bRunning;
  };

} /* namespace ADDON */
} /* extern "C" */
