/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <string>
#include <utility>
#include <vector>


class CMediaSource;

enum CONTEXT_BUTTON
{
  CONTEXT_BUTTON_CANCELLED = 0,
  CONTEXT_BUTTON_RENAME,
  CONTEXT_BUTTON_DELETE,
  CONTEXT_BUTTON_MOVE,
  CONTEXT_BUTTON_SETTINGS,
  CONTEXT_BUTTON_RIP_CD,
  CONTEXT_BUTTON_CANCEL_RIP_CD,
  CONTEXT_BUTTON_RIP_TRACK,
  CONTEXT_BUTTON_EJECT_DRIVE,
  CONTEXT_BUTTON_EDIT_SOURCE,
  CONTEXT_BUTTON_REMOVE_SOURCE,
  CONTEXT_BUTTON_SET_DEFAULT,
  CONTEXT_BUTTON_CLEAR_DEFAULT,
  CONTEXT_BUTTON_SET_THUMB,
  CONTEXT_BUTTON_ADD_LOCK,
  CONTEXT_BUTTON_REMOVE_LOCK,
  CONTEXT_BUTTON_CHANGE_LOCK,
  CONTEXT_BUTTON_RESET_LOCK,
  CONTEXT_BUTTON_REACTIVATE_LOCK,
  CONTEXT_BUTTON_VIEW_SLIDESHOW,
  CONTEXT_BUTTON_RECURSIVE_SLIDESHOW,
  CONTEXT_BUTTON_REFRESH_THUMBS,
  CONTEXT_BUTTON_SWITCH_MEDIA,
  CONTEXT_BUTTON_MOVE_ITEM,
  CONTEXT_BUTTON_MOVE_HERE,
  CONTEXT_BUTTON_CANCEL_MOVE,
  CONTEXT_BUTTON_MOVE_ITEM_UP,
  CONTEXT_BUTTON_MOVE_ITEM_DOWN,
  CONTEXT_BUTTON_CLEAR,
  CONTEXT_BUTTON_QUEUE_ITEM,
  CONTEXT_BUTTON_PLAY_ITEM,
  CONTEXT_BUTTON_PLAY_WITH,
  CONTEXT_BUTTON_PLAY_PARTYMODE,
  CONTEXT_BUTTON_PLAY_PART,
  CONTEXT_BUTTON_EDIT,
  CONTEXT_BUTTON_EDIT_SMART_PLAYLIST,
  CONTEXT_BUTTON_INFO,
  CONTEXT_BUTTON_INFO_ALL,
  CONTEXT_BUTTON_CDDB,
  CONTEXT_BUTTON_SCAN,
  CONTEXT_BUTTON_SCAN_TO_LIBRARY,
  CONTEXT_BUTTON_SET_ARTIST_THUMB,
  CONTEXT_BUTTON_SET_ART,
  CONTEXT_BUTTON_CANCEL_PARTYMODE,
  CONTEXT_BUTTON_MARK_WATCHED,
  CONTEXT_BUTTON_MARK_UNWATCHED,
  CONTEXT_BUTTON_SET_CONTENT,
  CONTEXT_BUTTON_EDIT_PARTYMODE,
  CONTEXT_BUTTON_LINK_MOVIE,
  CONTEXT_BUTTON_UNLINK_MOVIE,
  CONTEXT_BUTTON_GO_TO_ARTIST,
  CONTEXT_BUTTON_GO_TO_ALBUM,
  CONTEXT_BUTTON_PLAY_OTHER,
  CONTEXT_BUTTON_SET_ACTOR_THUMB,
  CONTEXT_BUTTON_UNLINK_BOOKMARK,
  CONTEXT_BUTTON_ACTIVATE,
  CONTEXT_BUTTON_GROUP_MANAGER,
  CONTEXT_BUTTON_CHANNEL_MANAGER,
  CONTEXT_BUTTON_PLAY_AND_QUEUE,
  CONTEXT_BUTTON_PLAY_ONLY_THIS,
  CONTEXT_BUTTON_UPDATE_EPG,
  CONTEXT_BUTTON_TAGS_ADD_ITEMS,
  CONTEXT_BUTTON_TAGS_REMOVE_ITEMS,
  CONTEXT_BUTTON_SET_MOVIESET,
  CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS,
  CONTEXT_BUTTON_BROWSE_INTO,
  CONTEXT_BUTTON_EDIT_SORTTITLE,
  CONTEXT_BUTTON_DELETE_ALL,
  CONTEXT_BUTTON_HELP,
  CONTEXT_BUTTON_PLAY_NEXT,
  CONTEXT_BUTTON_NAVIGATE,
};

class CContextButtons : public std::vector< std::pair<size_t, std::string> >
{
public:
  void Add(unsigned int, const std::string &label);
  void Add(unsigned int, int label);
};

class CGUIDialogContextMenu :
      public CGUIDialog
{
public:
  CGUIDialogContextMenu(void);
  ~CGUIDialogContextMenu(void) override;
  bool OnMessage(CGUIMessage &message) override;
  bool OnAction(const CAction& action) override;
  void SetPosition(float posX, float posY) override;

  static bool SourcesMenu(const std::string &strType, const CFileItemPtr& item, float posX, float posY);
  static void SwitchMedia(const std::string& strType, const std::string& strPath);

  static void GetContextButtons(const std::string &type, const CFileItemPtr& item, CContextButtons &buttons);
  static bool OnContextButton(const std::string &type, const CFileItemPtr& item, CONTEXT_BUTTON button);

  /*! Show the context menu with the given choices and return the index of the selected item,
    or -1 if cancelled.
   */
  static int Show(const CContextButtons& choices, int focusedButton = 0);

  /*! Legacy method that returns the context menu id, or -1 on cancel */
  static int ShowAndGetChoice(const CContextButtons &choices);

protected:
  void SetupButtons();

  /*! \brief Position the context menu in the middle of the focused control.
   If no control is available it is positioned in the middle of the screen.
   */
  void PositionAtCurrentFocus();

  float GetWidth() const override;
  float GetHeight() const override;
  void OnInitWindow() override;
  void OnWindowLoaded() override;
  void OnDeinitWindow(int nextWindowID) override;
  static std::string GetDefaultShareNameByType(const std::string &strType);
  static void SetDefault(const std::string &strType, const std::string &strDefault);
  static void ClearDefault(const std::string &strType);
  static CMediaSource *GetShare(const std::string &type, const CFileItem *item);

private:
  float m_coordX, m_coordY;
  /// \brief Stored size of background image (height or width depending on grouplist orientation)
  float m_backgroundImageSize;
  int m_initiallyFocusedButtonIdx = 0;
  int m_clickedButton;
  CContextButtons m_buttons;
  const CGUIControl *m_backgroundImage = nullptr;
};
