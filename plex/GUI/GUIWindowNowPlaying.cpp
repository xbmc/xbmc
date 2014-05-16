/*
 *      Copyright (C) 2009 Plex
 *      http://www.plexapp.com
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <boost/foreach.hpp>
#include "Key.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "PlayListPlayer.h"
#include "PlayList.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "PlexApplication.h"

#include "GUIWindowNowPlaying.h"
#include "PlexTypes.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowNowPlaying::CGUIWindowNowPlaying()
  : CGUIWindow(WINDOW_NOW_PLAYING, "NowPlaying.xml")
//  , m_thumbLoader(1, 200) TODO: figure out if we need to modify thumloader or not.
  , m_isFlipped(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowNowPlaying::~CGUIWindowNowPlaying()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowNowPlaying::OnAction(const CAction &action)
{
  CStdString strAction = action.GetName();
  strAction = strAction.ToLower();
  
  if (action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.GetID() == ACTION_SHOW_GUI ||
           action.GetID() == ACTION_NAV_BACK)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.GetID() == ACTION_NEXT_ITEM)
  {
    g_playlistPlayer.PlayNext();
    return true;
  }
  else if (action.GetID() == ACTION_PREV_ITEM)
  {
    g_playlistPlayer.PlayPrevious();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowNowPlaying::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      g_plexApplication.timer->RemoveTimeout(this);
      break;
    }
    case GUI_MSG_WINDOW_INIT:
    {
      if (g_windowManager.GetTopMostModalDialogID(true) != WINDOW_INVALID)
        g_windowManager.GetWindow(g_windowManager.GetTopMostModalDialogID(true))->Close();

      m_isFlipped = false;
      g_plexApplication.timer->SetTimeout(g_advancedSettings.m_nowPlayingFlipTime * 1000, this);
      break;
    }
  }
  
  return CGUIWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowNowPlaying::OnTimeout()
{
  m_isFlipped = !m_isFlipped;
  SetInvalid();

  g_plexApplication.timer->RestartTimeout(g_advancedSettings.m_nowPlayingFlipTime * 1000, this);
}
