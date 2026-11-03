/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebClientManager.h"
#include "guilib/GUIDialog.h"

namespace KODI
{
namespace WEB
{

/*!
 * \brief The bar shown over a web page: its title and address, history
 * navigation, reload and closing the page
 */
class CGUIDialogWebOSD : public CGUIDialog
{
public:
  CGUIDialogWebOSD();

  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void FrameMove() override;

protected:
  void OnInitWindow() override;

private:
  void UpdateControls(bool force);

  CWebClientManager::ViewState m_shown;
};

} // namespace WEB
} // namespace KODI
