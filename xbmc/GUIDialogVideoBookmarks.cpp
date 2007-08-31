/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogVideoBookmarks.h"
#include "VideoDatabase.h"
#include "Application.h"
#include "Util.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "Picture.h"

#define BOOKMARK_THUMB_WIDTH g_advancedSettings.m_thumbSize

#define CONTROL_ADD_BOOKMARK     2
#define CONTROL_CLEAR_BOOKMARKS  3

#define CONTROL_LIST             10
#define CONTROL_THUMBS           11

CGUIDialogVideoBookmarks::CGUIDialogVideoBookmarks()
    : CGUIDialog(WINDOW_DIALOG_VIDEO_BOOKMARKS, "VideoOSDBookmarks.xml")
{
}

CGUIDialogVideoBookmarks::~CGUIDialogVideoBookmarks()
{
}

bool CGUIDialogVideoBookmarks::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      Update();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_ADD_BOOKMARK)
      {
        AddBookmark();
      }
      else if (iControl == CONTROL_CLEAR_BOOKMARKS)
      {
        ClearBookmarks();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iAction == ACTION_DELETE_ITEM)
        {
          if( (unsigned)iItem < m_bookmarks.size() )
          {
            CVideoDatabase videoDatabase;
            videoDatabase.Open();
            videoDatabase.ClearBookMarkOfFile(g_application.CurrentFile(),m_bookmarks[iItem]);
            videoDatabase.Close();
          }
          Update();
        }
        else if (iAction == ACTION_SELECT_ITEM)
          GotoBookmark(iItem);
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVideoBookmarks::Update()
{
  m_bookmarks.clear();
  CBookmark resumemark;

  // open the d/b and retrieve the bookmarks for the current movie
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetBookMarksForFile(g_application.CurrentFile(), m_bookmarks);

  /* push in the resume mark first */
  if( videoDatabase.GetResumeBookMark(g_application.CurrentFile(), resumemark) )
    m_bookmarks.insert(m_bookmarks.begin(), resumemark);

  videoDatabase.Close();

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_ICONS);

  // empty the list ready for population
  Clear();

  // cycle through each stored bookmark and add it to our list control
  for (unsigned int i = 0; i < m_bookmarks.size(); ++i)
  {
    if( m_bookmarks[i].type == CBookmark::RESUME )
      m_bookmarks[i].thumbNailImage = "bookmark-resume.png";

    CStdString bookmarkTime;
    StringUtils::SecondsToTimeString((long)m_bookmarks[i].timeInSeconds, bookmarkTime, TIME_FORMAT_HH_MM_SS);

    CFileItem *item = new CFileItem(bookmarkTime);
    item->SetThumbnailImage(m_bookmarks[i].thumbNailImage);
    m_vecItems.Add(item);
  }
  m_viewControl.SetItems(m_vecItems);
  g_graphicsContext.Unlock();
}

void CGUIDialogVideoBookmarks::Clear()
{
  m_viewControl.Clear();
  m_vecItems.Clear();
}

void CGUIDialogVideoBookmarks::GotoBookmark(int item)
{
  if (item < 0 || item >= (int)m_bookmarks.size()) return;
  if (g_application.m_pPlayer)
  {
    g_application.m_pPlayer->SetPlayerState(m_bookmarks[item].playerState);
    g_application.SeekTime((double)m_bookmarks[item].timeInSeconds);
  }
}

void CGUIDialogVideoBookmarks::ClearBookmarks()
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.ClearBookMarksOfFile(g_application.CurrentFile(), CBookmark::STANDARD);
  videoDatabase.ClearBookMarksOfFile(g_application.CurrentFile(), CBookmark::RESUME);
  videoDatabase.Close();
  Update();
}

void CGUIDialogVideoBookmarks::AddBookmark()
{
  CVideoDatabase videoDatabase;
  CBookmark bookmark;
  bookmark.timeInSeconds = (int)g_application.GetTime();

  if( g_application.m_pPlayer )
    bookmark.playerState = g_application.m_pPlayer->GetPlayerState();
  else
    bookmark.playerState.Empty();

  bookmark.player = CPlayerCoreFactory::GetPlayerName(g_application.GetCurrentPlayer());

  // create the thumbnail image
#ifdef HAS_VIDEO_PLAYBACK
  float aspectRatio = g_renderManager.GetAspectRatio();
#else
  float aspectRatio = 1.0f;
#endif
  int width = BOOKMARK_THUMB_WIDTH;
  int height = (int)(BOOKMARK_THUMB_WIDTH / aspectRatio);
  if (height > BOOKMARK_THUMB_WIDTH)
  {
    height = BOOKMARK_THUMB_WIDTH;
    width = (int)(BOOKMARK_THUMB_WIDTH * aspectRatio);
  }
  CSingleLock lock(g_graphicsContext);
  LPDIRECT3DTEXTURE8 texture = NULL;
  if (D3D_OK == D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &texture))
  {
    LPDIRECT3DSURFACE8 surface = NULL;
    texture->GetSurfaceLevel(0, &surface);
#ifdef HAS_VIDEO_PLAYBACK
    g_renderManager.CreateThumbnail(surface, width, height);
#endif
    D3DLOCKED_RECT lockedRect;
    surface->LockRect(&lockedRect, NULL, NULL);
    // compute the thumb name + create the thumb image
    Crc32 crc;
    crc.ComputeFromLowerCase(g_application.CurrentFile());
    bookmark.thumbNailImage.Format("%s\\%08x_%i.jpg", g_settings.GetBookmarksThumbFolder().c_str(), crc, m_vecItems.Size() + 1);
    CPicture pic;
    if (!pic.CreateThumbnailFromSurface((BYTE *)lockedRect.pBits, width, height, lockedRect.Pitch, bookmark.thumbNailImage))
      bookmark.thumbNailImage.Empty();
    surface->UnlockRect();
    surface->Release();
    texture->Release();
  }
  lock.Leave();
  videoDatabase.Open();
  videoDatabase.AddBookMarkToFile(g_application.CurrentFile(), bookmark, CBookmark::STANDARD);
  videoDatabase.Close();
  Update();
}

void CGUIDialogVideoBookmarks::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_THUMBS));
}

void CGUIDialogVideoBookmarks::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogVideoBookmarks::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}