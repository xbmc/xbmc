/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

class CFileItemList;
class CGUIViewControl;

namespace KODI
{
namespace GAME
{
  class IVideoSelectCallback;

  class CDialogGameVideoSelect : public CGUIDialog
  {
  public:
    ~CDialogGameVideoSelect() override;

    void RegisterCallback(IVideoSelectCallback *callback);
    void UnregisterCallback();

    // implementation of CGUIControl via CGUIDialog
    bool OnMessage(CGUIMessage &message) override;

    // implementation of CGUIWindow via CGUIDialog
    void FrameMove() override;
    void OnDeinitWindow(int nextWindowID) override;

  protected:
    CDialogGameVideoSelect(int windowId);

    // implementation of CGUIWindow via CGUIDialog
    void OnWindowUnload() override;
    void OnWindowLoaded() override;
    void OnInitWindow() override;

    // Video select interface
    virtual void PreInit() = 0;
    virtual void GetItems(CFileItemList &items) = 0;
    virtual void OnItemFocus(unsigned int index) = 0;
    virtual unsigned int GetFocusedItem() const = 0;
    virtual void PostExit() = 0;

  protected:
    IVideoSelectCallback *m_callback = nullptr;

  private:
    void Update();
    void Clear();

    void OnRefreshList();

    void SaveSettings();

    std::unique_ptr<CGUIViewControl> m_viewControl;
    std::unique_ptr<CFileItemList> m_vecItems;
  };
}
}
