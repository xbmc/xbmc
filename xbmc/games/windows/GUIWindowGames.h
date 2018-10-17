/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windows/GUIMediaWindow.h"

class CGUIDialogProgress;

namespace KODI
{
namespace GAME
{
  class CGUIWindowGames : public CGUIMediaWindow
  {
  public:
    CGUIWindowGames();
    virtual ~CGUIWindowGames() = default;

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

    CGUIDialogProgress *m_dlgProgress = nullptr;
  };
}
}
