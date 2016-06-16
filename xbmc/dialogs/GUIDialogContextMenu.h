#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <utility>
#include <vector>

#include "guilib/GUIDialog.h"


class CMediaSource;

enum CONTEXT_BUTTON { CONTEXT_BUTTON_CANCELLED = 0,
                      CONTEXT_BUTTON_RENAME,
                      CONTEXT_BUTTON_DELETE,
                      CONTEXT_BUTTON_MOVE,
                      CONTEXT_BUTTON_ADD_FAVOURITE,
                      CONTEXT_BUTTON_SETTINGS,
                      CONTEXT_BUTTON_RIP_CD,
                      CONTEXT_BUTTON_CANCEL_RIP_CD,
                      CONTEXT_BUTTON_RIP_TRACK,
                      CONTEXT_BUTTON_EJECT_DISC,
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
                      CONTEXT_BUTTON_RESUME_ITEM,
                      CONTEXT_BUTTON_EDIT,
                      CONTEXT_BUTTON_EDIT_SMART_PLAYLIST,
                      CONTEXT_BUTTON_INFO,
                      CONTEXT_BUTTON_INFO_ALL,
                      CONTEXT_BUTTON_CDDB,
                      CONTEXT_BUTTON_SCAN,
                      CONTEXT_BUTTON_SET_ARTIST_THUMB,
                      CONTEXT_BUTTON_SET_SEASON_ART,
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
                      CONTEXT_BUTTON_ADD,
                      CONTEXT_BUTTON_ACTIVATE,
                      CONTEXT_BUTTON_START_RECORD,
                      CONTEXT_BUTTON_ADD_TIMER,
                      CONTEXT_BUTTON_STOP_RECORD,
                      CONTEXT_BUTTON_EDIT_TIMER,
                      CONTEXT_BUTTON_EDIT_TIMER_RULE,
                      CONTEXT_BUTTON_DELETE_TIMER,
                      CONTEXT_BUTTON_DELETE_TIMER_RULE,
                      CONTEXT_BUTTON_GROUP_MANAGER,
                      CONTEXT_BUTTON_CHANNEL_MANAGER,
                      CONTEXT_BUTTON_SET_MOVIESET_ART,
                      CONTEXT_BUTTON_BEGIN,
                      CONTEXT_BUTTON_END,
                      CONTEXT_BUTTON_NOW,
                      CONTEXT_BUTTON_FIND,
                      CONTEXT_BUTTON_MENU_HOOKS,
                      CONTEXT_BUTTON_PLAY_AND_QUEUE,
                      CONTEXT_BUTTON_PLAY_ONLY_THIS,
                      CONTEXT_BUTTON_UPDATE_EPG,
                      CONTEXT_BUTTON_TAGS_ADD_ITEMS,
                      CONTEXT_BUTTON_TAGS_REMOVE_ITEMS,
                      CONTEXT_BUTTON_SET_MOVIESET,
                      CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS,
                      CONTEXT_BUTTON_BROWSE_INTO,
                      CONTEXT_BUTTON_EDIT_SORTTITLE,
                      CONTEXT_BUTTON_UNDELETE,
                      CONTEXT_BUTTON_DELETE_ALL,
                      CONTEXT_BUTTON_HELP,
                      CONTEXT_BUTTON_ACTIVE_ADSP_SETTINGS,
                    };

class CContextButtons : public std::vector< std::pair<unsigned int, std::string> >
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
  virtual ~CGUIDialogContextMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction& action);
  virtual void SetPosition(float posX, float posY);

  static bool SourcesMenu(const std::string &strType, const CFileItemPtr& item, float posX, float posY);
  static void SwitchMedia(const std::string& strType, const std::string& strPath);

  static void GetContextButtons(const std::string &type, const CFileItemPtr& item, CContextButtons &buttons);
  static bool OnContextButton(const std::string &type, const CFileItemPtr& item, CONTEXT_BUTTON button);

  /*! Show the context menu with the given choices and return the index of the selected item,
    or -1 if cancelled.
   */
  static int Show(const CContextButtons& choices);

  /*! Legacy method that returns the context menu id, or -1 on cancel */
  static int ShowAndGetChoice(const CContextButtons &choices);

protected:
  void SetupButtons();

  /*! \brief Position the context menu in the middle of the focused control.
   If no control is available it is positioned in the middle of the screen.
   */
  void PositionAtCurrentFocus();

  virtual float GetWidth() const;
  virtual float GetHeight() const;
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  virtual void OnDeinitWindow(int nextWindowID);
  static std::string GetDefaultShareNameByType(const std::string &strType);
  static void SetDefault(const std::string &strType, const std::string &strDefault);
  static void ClearDefault(const std::string &strType);
  static CMediaSource *GetShare(const std::string &type, const CFileItem *item);

private:
  float m_coordX, m_coordY;
  /// \brief Stored size of background image (height or width depending on grouplist orientation)
  float m_backgroundImageSize;
  int m_clickedButton;
  CContextButtons m_buttons;
};
