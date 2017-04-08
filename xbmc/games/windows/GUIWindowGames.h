/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "games/GameTypes.h"
#include "windows/GUIMediaWindow.h"

class CGUIDialogProgress;

namespace GAME
{
  class CGUIWindowGames : public CGUIMediaWindow
  {
  public:
    CGUIWindowGames();
    virtual ~CGUIWindowGames() { }

    // implementation of CGUIControl via CGUIMediaWindow
    virtual bool OnMessage(CGUIMessage& message) override;

  protected:
    // implementation of CGUIMediaWindow
    virtual void SetupShares() override;
    virtual bool OnClick(int iItem, const std::string &player = "") override;
    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    virtual bool OnAddMediaSource() override;
    virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
    virtual std::string GetStartFolder(const std::string &dir) override;

    bool OnClickMsg(int controlId, int actionId);
    void OnItemInfo(int itemNumber);
    bool PlayGame(const CFileItem &item);

    CGUIDialogProgress *m_dlgProgress;
  };
}
