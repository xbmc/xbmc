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
    /* Window creation functions */
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

    /* Window control functions */
    static bool set_focus_id(void* kodiBase, void* handle, int control_id);
    static int get_focus_id(void* kodiBase, void* handle);
    static void set_control_label(void* kodiBase, void* handle, int control_id, const char* label);

    /* Window property functions */
    static void set_property(void* kodiBase, void* handle, const char* key, const char* value);
    static void set_property_int(void* kodiBase, void* handle, const char* key, int value);
    static void set_property_bool(void* kodiBase, void* handle, const char* key, bool value);
    static void set_property_double(void* kodiBase, void* handle, const char* key, double value);
    static char* get_property(void* kodiBase, void* handle, const char* key);
    static int get_property_int(void* kodiBase, void* handle, const char* key);
    static bool get_property_bool(void* kodiBase, void* handle, const char* key);
    static double get_property_double(void* kodiBase, void* handle, const char* key);
    static void clear_properties(void* kodiBase, void* handle);
    static void clear_property(void* kodiBase, void* handle, const char* key);

    /* List item functions */
    static void clear_item_list(void* kodiBase, void* handle);
    static void add_list_item(void* kodiBase, void* handle, void* item, int list_position);
    static void remove_list_item_from_position(void* kodiBase, void* handle, int list_position);
    static void remove_list_item(void* kodiBase, void* handle, void* item);
    static void* get_list_item(void* kodiBase, void* handle, int list_position);
    static void set_current_list_position(void* kodiBase, void* handle, int list_position);
    static int get_current_list_position(void* kodiBase, void* handle);
    static int get_list_size(void* kodiBase, void* handle);
    static void set_container_property(void* kodiBase, void* handle, const char* key, const char* value);
    static void set_container_content(void* kodiBase, void* handle, const char* value);
    static int get_current_container_id(void* kodiBase, void* handle);

    /* Various functions */
    static void mark_dirty_region(void* kodiBase, void* handle);

    /* GUI control access functions */
    static void* get_control_button(void* kodiBase, void* handle, int control_id);
    static void* get_control_edit(void* kodiBase, void* handle, int control_id);
    static void* get_control_fade_label(void* kodiBase, void* handle, int control_id);
    static void* get_control_image(void* kodiBase, void* handle, int control_id);
    static void* get_control_label(void* kodiBase, void* handle, int control_id);
    static void* get_control_radio_button(void* kodiBase, void* handle, int control_id);
    static void* get_control_progress(void* kodiBase, void* handle, int control_id);
    static void* get_control_render_addon(void* kodiBase, void* handle, int control_id);
    static void* get_control_settings_slider(void* kodiBase, void* handle, int control_id);
    static void* get_control_slider(void* kodiBase, void* handle, int control_id);
    static void* get_control_spin(void* kodiBase, void* handle, int control_id);
    static void* get_control_text_box(void* kodiBase, void* handle, int control_id);
    //@}

  private:
    static void* GetControl(void* kodiBase, void* handle, int control_id, const char* function, CGUIControl::GUICONTROLTYPES type, const std::string& typeName);
    static int GetNextAvailableWindowId();
  };

  class CGUIAddonWindow : public CGUIMediaWindow
  {
  friend struct Interface_GUIWindow;

  public:
    CGUIAddonWindow(int id, const std::string& strXML, ADDON::CAddonDll* addon, bool isMedia);
    ~CGUIAddonWindow() override = default;

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
    void SetContainerProperty(const std::string& key, const std::string& value);
    void SetContainerContent(const std::string& value);
    int GetCurrentContainerControlId();
    CGUIControl* GetAddonControl(int controlId, CGUIControl::GUICONTROLTYPES type, const std::string& typeName);

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
