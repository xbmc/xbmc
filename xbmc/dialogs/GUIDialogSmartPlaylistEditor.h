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

class CFileItemList;

class CGUIDialogSmartPlaylistEditor :
      public CGUIDialog
{
public:
  //! \brief Which playlist types the editor offers.
  enum class Mode
  {
    NONE,
    MUSIC,
    VIDEO,
    PARTY_MUSIC,
    PARTY_VIDEO,
  };

  CGUIDialogSmartPlaylistEditor(void);
  ~CGUIDialogSmartPlaylistEditor(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  static bool EditPlaylist(const std::string& path, Mode mode = Mode::NONE);
  //! \param type "music" or "video", as a new smart playlist placeholder path ends
  static bool NewPlaylist(const std::string &type);

protected:
  void OnRuleList(int item);
  void OnRuleAdd();
  void OnRuleRemove(int item);
  void OnMatch();
  void OnLimit();
  void OnName();
  void OnType();
  void OnOrder();
  void OnOrderDirection();
  void OnGroupBy();
  void OnGroupMixed();
  void OnWatchedMode();
  void OnOK();
  void OnCancel();
  void OnPopupMenu(int item);
  void UpdateButtons();
  void UpdateRuleControlButtons();
  int GetSelectedItem();
  void HighlightItem(int item);
  std::vector<KODI::MEDIA::LIBRARY_QUERY::TYPE> GetAllowedTypes(Mode mode);
  std::string GetLocalizedType(KODI::MEDIA::LIBRARY_QUERY::TYPE type);

  KODI::MEDIA::CLibraryQuery m_playlist;

  // our list of rules for display purposes
  CFileItemList* m_ruleLabels;

  std::string m_path;
  bool m_cancelled;
  Mode m_mode{Mode::NONE};
};
