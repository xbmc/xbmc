#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "guilib/GUIDialog.h"

class CMediaSource;

enum CONTEXT_BUTTON { CONTEXT_BUTTON_CANCELLED = 0,
                      CONTEXT_BUTTON_LAUNCH,
                      CONTEXT_BUTTON_RENAME,
                      CONTEXT_BUTTON_DELETE,
                      CONTEXT_BUTTON_COPY,
                      CONTEXT_BUTTON_MOVE,
                      CONTEXT_BUTTON_ADD_FAVOURITE,
                      CONTEXT_BUTTON_SETTINGS,
                      CONTEXT_BUTTON_GOTO_ROOT,
                      CONTEXT_BUTTON_PLAY_DISC,
                      CONTEXT_BUTTON_RESUME_DISC,
                      CONTEXT_BUTTON_RIP_CD,
                      CONTEXT_BUTTON_CANCEL_RIP_CD,
                      CONTEXT_BUTTON_RIP_TRACK,
                      CONTEXT_BUTTON_EJECT_DISC,
                      CONTEXT_BUTTON_EJECT_DRIVE,
                      CONTEXT_BUTTON_ADD_SOURCE,
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
                      CONTEXT_BUTTON_SAVE,
                      CONTEXT_BUTTON_LOAD,
                      CONTEXT_BUTTON_CLEAR,
                      CONTEXT_BUTTON_QUEUE_ITEM,
                      CONTEXT_BUTTON_PLAY_ITEM,
                      CONTEXT_BUTTON_PLAY_WITH,
                      CONTEXT_BUTTON_PLAY_PARTYMODE,
                      CONTEXT_BUTTON_PLAY_PART,
                      CONTEXT_BUTTON_RESUME_ITEM,
                      CONTEXT_BUTTON_RESTART_ITEM,
                      CONTEXT_BUTTON_EDIT,
                      CONTEXT_BUTTON_EDIT_SMART_PLAYLIST,
                      CONTEXT_BUTTON_INFO,
                      CONTEXT_BUTTON_INFO_ALL,
                      CONTEXT_BUTTON_CDDB,
                      CONTEXT_BUTTON_UPDATE_LIBRARY,
                      CONTEXT_BUTTON_UPDATE_TVSHOW,
                      CONTEXT_BUTTON_SCAN,
                      CONTEXT_BUTTON_STOP_SCANNING,
                      CONTEXT_BUTTON_SET_ARTIST_THUMB,
                      CONTEXT_BUTTON_SET_SEASON_ART,
                      CONTEXT_BUTTON_NOW_PLAYING,
                      CONTEXT_BUTTON_CANCEL_PARTYMODE,
                      CONTEXT_BUTTON_MARK_WATCHED,
                      CONTEXT_BUTTON_MARK_UNWATCHED,
                      CONTEXT_BUTTON_SET_CONTENT,
                      CONTEXT_BUTTON_ADD_TO_LIBRARY,
                      CONTEXT_BUTTON_SONG_INFO,
                      CONTEXT_BUTTON_EDIT_PARTYMODE,
                      CONTEXT_BUTTON_LINK_MOVIE,
                      CONTEXT_BUTTON_UNLINK_MOVIE,
                      CONTEXT_BUTTON_GO_TO_ARTIST,
                      CONTEXT_BUTTON_GO_TO_ALBUM,
                      CONTEXT_BUTTON_PLAY_OTHER,
                      CONTEXT_BUTTON_SET_ACTOR_THUMB,
                      CONTEXT_BUTTON_UNLINK_BOOKMARK,
                      CONTEXT_BUTTON_PLUGIN_SETTINGS,
                      CONTEXT_BUTTON_SCRIPT_SETTINGS,
                      CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM,
                      CONTEXT_BUTTON_LASTFM_UNBAN_ITEM,
                      CONTEXT_BUTTON_HIDE,
                      CONTEXT_BUTTON_SHOW_HIDDEN,
                      CONTEXT_BUTTON_ADD,
                      CONTEXT_BUTTON_ACTIVATE,
                      CONTEXT_BUTTON_START_RECORD,
                      CONTEXT_BUTTON_STOP_RECORD,
                      CONTEXT_BUTTON_GROUP_MANAGER,
                      CONTEXT_BUTTON_CHANNEL_MANAGER,
                      CONTEXT_BUTTON_FILTER,
                      CONTEXT_BUTTON_SET_MOVIESET_ART,
                      CONTEXT_BUTTON_BEGIN,
                      CONTEXT_BUTTON_END,
                      CONTEXT_BUTTON_FIND,
                      CONTEXT_BUTTON_DELETE_PLUGIN,
                      CONTEXT_BUTTON_SORTASC,
                      CONTEXT_BUTTON_SORTBY,
                      CONTEXT_BUTTON_SORTBY_CHANNEL,
                      CONTEXT_BUTTON_SORTBY_NAME,
                      CONTEXT_BUTTON_SORTBY_DATE,
                      CONTEXT_BUTTON_MENU_HOOKS,
                      CONTEXT_BUTTON_PLAY_AND_QUEUE,
                      CONTEXT_BUTTON_PLAY_ONLY_THIS,
                      CONTEXT_BUTTON_UPDATE_EPG,
                      CONTEXT_BUTTON_RECORD_ITEM,
                      CONTEXT_BUTTON_TAGS_ADD_ITEMS,
                      CONTEXT_BUTTON_TAGS_REMOVE_ITEMS,
                      CONTEXT_BUTTON_USER1,
                      CONTEXT_BUTTON_USER2,
                      CONTEXT_BUTTON_USER3,
                      CONTEXT_BUTTON_USER4,
                      CONTEXT_BUTTON_USER5,
                      CONTEXT_BUTTON_USER6,
                      CONTEXT_BUTTON_USER7,
                      CONTEXT_BUTTON_USER8,
                      CONTEXT_BUTTON_USER9,
                      CONTEXT_BUTTON_USER10
                    };

class CContextButtons : public std::vector< std::pair<unsigned int, CStdString> >
{
public:
  void Add(unsigned int, const CStdString &label);
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

  static bool SourcesMenu(const CStdString &strType, const CFileItemPtr item, float posX, float posY);
  static void SwitchMedia(const CStdString& strType, const CStdString& strPath);

  static void GetContextButtons(const CStdString &type, const CFileItemPtr item, CContextButtons &buttons);
  static bool OnContextButton(const CStdString &type, const CFileItemPtr item, CONTEXT_BUTTON button);

  /*! \brief Show the context menu with the given choices
   \param choices the choices available for the user.
   \return -1 if no choice is made, else the chosen option.
   */
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
  static CStdString GetDefaultShareNameByType(const CStdString &strType);
  static void SetDefault(const CStdString &strType, const CStdString &strDefault);
  static void ClearDefault(const CStdString &strType);
  static CMediaSource *GetShare(const CStdString &type, const CFileItem *item);

private:
  float m_coordX, m_coordY;
  /// \brief Stored size of background image (height or width depending on grouplist orientation)
  float m_backgroundImageSize;
  int m_clickedButton;
  CContextButtons m_buttons;
};
