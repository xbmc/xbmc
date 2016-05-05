/*
 *      Copyright (C) 2016 Team Kodi
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
#include "video/VideoDatabase.h" //! @todo
#include "view/GUIViewControl.h"

#include <memory>
#include <string>

class CFileItemList;
class CVideoInfoTag;

namespace KODI
{
namespace GAME
{
  class CGUIDialogSavestates : public CGUIDialog
  {
  public:
    CGUIDialogSavestates(void);
    virtual ~CGUIDialogSavestates(void) = default;

    // implementation of CGUIControl via CGUIDialog
    virtual bool OnMessage(CGUIMessage& message) override;
    virtual bool OnAction(const CAction &action) override;

    // implementation of CGUIControlGroup via CGUIDialog
    virtual CGUIControl* GetFirstFocusableControl(int id) override;

    // implementation of CGUIWindow via CGUIDialog
    virtual void OnWindowLoaded() override;
    virtual void OnWindowUnload() override;

  protected:
    void OnContextMenu(const CFileItem& save);
    void CreateSavestate();
    void ClearSavestates();
    void LoadSavestate(const CFileItem& save);
    void RenameSavestate(const CFileItem& save);
    void DeleteSavestate(const CFileItem& save);

  private:
    void Update();

    // Window properties
    std::unique_ptr<CFileItemList> m_vecItems;
    CGUIViewControl m_viewControl;

    // Game properties
    std::string m_gamePath;
    std::string m_gameClient;
  };
}
}
