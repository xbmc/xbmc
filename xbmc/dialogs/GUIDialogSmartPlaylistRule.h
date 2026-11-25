/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "media/LibraryQuery.h"

#include <string>
#include <utility>
#include <vector>

class CGUIDialogSmartPlaylistRule :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistRule(void);
  ~CGUIDialogSmartPlaylistRule(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  static bool EditRule(
      KODI::MEDIA::CLibraryQueryRule& rule,
      KODI::MEDIA::LIBRARY_QUERY::TYPE type = KODI::MEDIA::LIBRARY_QUERY::TYPE::SONGS);

protected:
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void OnBrowse();
  std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>> GetValidOperators(
      const KODI::MEDIA::CLibraryQueryRule& rule) const;

  KODI::MEDIA::CLibraryQueryRule m_rule;
  bool m_cancelled;
  KODI::MEDIA::LIBRARY_QUERY::TYPE m_type{KODI::MEDIA::LIBRARY_QUERY::TYPE::SONGS};
};
