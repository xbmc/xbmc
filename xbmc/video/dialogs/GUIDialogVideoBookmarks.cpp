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

#include "system.h"
#include "GUIDialogVideoBookmarks.h"
#include "video/VideoDatabase.h"
#include "Application.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/RenderCapture.h"
#endif
#include "pictures/Picture.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "view/ViewState.h"
#include "profiles/ProfilesManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "guilib/Texture.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Crc32.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "Util.h"
#include "cores/IPlayer.h"

using namespace std;

#define BOOKMARK_THUMB_WIDTH g_advancedSettings.GetThumbSize()

#define CONTROL_ADD_BOOKMARK           2
#define CONTROL_CLEAR_BOOKMARKS        3
#define CONTROL_ADD_EPISODE_BOOKMARK   4

#define CONTROL_LIST                  10
#define CONTROL_THUMBS                11

CGUIDialogVideoBookmarks::CGUIDialogVideoBookmarks()
    : CGUIDialog(WINDOW_DIALOG_VIDEO_BOOKMARKS, "VideoOSDBookmarks.xml")
{
  m_vecItems = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVideoBookmarks::~CGUIDialogVideoBookmarks()
{
  delete m_vecItems;
}

bool CGUIDialogVideoBookmarks::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CUtil::DeleteVideoDatabaseDirectoryCache();
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
        Update();
      }
      else if (iControl == CONTROL_CLEAR_BOOKMARKS)
      {
        ClearBookmarks();
      }
      else if (iControl == CONTROL_ADD_EPISODE_BOOKMARK)
      {
        AddEpisodeBookmark();
        Update();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iAction == ACTION_DELETE_ITEM)
        {
          Delete(iItem);
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          GotoBookmark(iItem);
        }
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
  case GUI_MSG_REFRESH_LIST:
    {
      OnRefreshList();
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogVideoBookmarks::OnAction(const CAction &action)
{
  switch(action.GetID())
  {
  case ACTION_CONTEXT_MENU:
  case ACTION_MOUSE_RIGHT_CLICK:
    {
      OnPopupMenu(m_viewControl.GetSelectedItem());
      return true;
    }
  }
  return CGUIDialog::OnAction(action);
}


void CGUIDialogVideoBookmarks::OnPopupMenu(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;
  
    // highlight the item
  (*m_vecItems)[item]->Select(true);
  
  CContextButtons choices;
  
  int langID = 20404; //"Remove bookmark"
  if (m_bookmarks[item].type == CBookmark::EPISODE)
    langID = 20405;   //"Remove episode bookmark"
  choices.Add(1, langID); 

  
  int button = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  
    // unhighlight the item
  (*m_vecItems)[item]->Select(false);
  
  if (button == 1)
    Delete(item);
}

void CGUIDialogVideoBookmarks::Delete(int item)
{
  if ( item>=0 && (unsigned)item < m_bookmarks.size() )
  {
    CVideoDatabase videoDatabase;
    videoDatabase.Open();
    std::string path(g_application.CurrentFile());
    if (g_application.CurrentFileItem().HasProperty("original_listitem_url") &&
       !URIUtils::IsVideoDb(g_application.CurrentFileItem().GetProperty("original_listitem_url").asString()))
      path = g_application.CurrentFileItem().GetProperty("original_listitem_url").asString();
    videoDatabase.ClearBookMarkOfFile(path, m_bookmarks[item], m_bookmarks[item].type);
    videoDatabase.Close();
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  Update();
}

void CGUIDialogVideoBookmarks::OnRefreshList()
{
  m_bookmarks.clear();
  CBookmark resumemark;
  
    // open the d/b and retrieve the bookmarks for the current movie
  std::string path = g_application.CurrentFile();
  if (g_application.CurrentFileItem().HasProperty("original_listitem_url") && 
     !URIUtils::IsVideoDb(g_application.CurrentFileItem().GetProperty("original_listitem_url").asString()))
    path = g_application.CurrentFileItem().GetProperty("original_listitem_url").asString();
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetBookMarksForFile(path, m_bookmarks);
  videoDatabase.GetBookMarksForFile(path, m_bookmarks, CBookmark::EPISODE, true);
  /* push in the resume mark first */
  if( videoDatabase.GetResumeBookMark(path, resumemark) )
    m_bookmarks.push_back(resumemark);
  
  videoDatabase.Close();
  m_vecItems->Clear();
    // cycle through each stored bookmark and add it to our list control
  for (unsigned int i = 0; i < m_bookmarks.size(); ++i)
  {
    if (m_bookmarks[i].type == CBookmark::RESUME)
      m_bookmarks[i].thumbNailImage = "bookmark-resume.png";
    
    std::string bookmarkTime;
    if (m_bookmarks[i].type == CBookmark::EPISODE)
      bookmarkTime = StringUtils::Format("%s %li %s %li", g_localizeStrings.Get(20373).c_str(), m_bookmarks[i].seasonNumber, g_localizeStrings.Get(20359).c_str(), m_bookmarks[i].episodeNumber);
    else
      bookmarkTime = StringUtils::SecondsToTimeString((long)m_bookmarks[i].timeInSeconds, TIME_FORMAT_HH_MM_SS);
    
    CFileItemPtr item(new CFileItem(bookmarkTime));
    item->SetArt("thumb", m_bookmarks[i].thumbNailImage);
    m_vecItems->Add(item);
  }
  m_viewControl.SetItems(*m_vecItems);
}

void CGUIDialogVideoBookmarks::Update()
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();

  if (g_application.CurrentFileItem().HasVideoInfoTag() && g_application.CurrentFileItem().GetVideoInfoTag()->m_iEpisode > -1)
  {
    vector<CVideoInfoTag> episodes;
    videoDatabase.GetEpisodesByFile(g_application.CurrentFile(),episodes);
    if (episodes.size() > 1)
    {
      CONTROL_ENABLE(CONTROL_ADD_EPISODE_BOOKMARK);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_ADD_EPISODE_BOOKMARK);
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_ADD_EPISODE_BOOKMARK);
  }


  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_ICONS);

  // empty the list ready for population
  Clear();

  OnRefreshList();
  
  g_graphicsContext.Unlock();
  
  videoDatabase.Close();
}

void CGUIDialogVideoBookmarks::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogVideoBookmarks::GotoBookmark(int item)
{
  if (item < 0 || item >= (int)m_bookmarks.size()) return;
  if (g_application.m_pPlayer->HasPlayer())
  {
    g_application.m_pPlayer->SetPlayerState(m_bookmarks[item].playerState);
    g_application.SeekTime((double)m_bookmarks[item].timeInSeconds);
  }
}

void CGUIDialogVideoBookmarks::ClearBookmarks()
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  std::string path = g_application.CurrentFile();
  if (g_application.CurrentFileItem().HasProperty("original_listitem_url") && 
     !URIUtils::IsVideoDb(g_application.CurrentFileItem().GetProperty("original_listitem_url").asString()))
    path = g_application.CurrentFileItem().GetProperty("original_listitem_url").asString();
  videoDatabase.ClearBookMarksOfFile(path, CBookmark::STANDARD);
  videoDatabase.ClearBookMarksOfFile(path, CBookmark::RESUME);
  videoDatabase.ClearBookMarksOfFile(path, CBookmark::EPISODE);
  videoDatabase.Close();
  Update();
}

bool CGUIDialogVideoBookmarks::AddBookmark(CVideoInfoTag* tag)
{
  CVideoDatabase videoDatabase;
  CBookmark bookmark;
  bookmark.timeInSeconds = (int)g_application.GetTime();
  bookmark.totalTimeInSeconds = (int)g_application.GetTotalTime();

  if( g_application.m_pPlayer->HasPlayer() )
    bookmark.playerState = g_application.m_pPlayer->GetPlayerState();
  else
    bookmark.playerState.clear();

  bookmark.player = CPlayerCoreFactory::Get().GetPlayerName(g_application.GetCurrentPlayer());

  // create the thumbnail image
#ifdef HAS_VIDEO_PLAYBACK
  float aspectRatio = g_renderManager.GetAspectRatio();
#else
  float aspectRatio = 1.0f;
#endif
  int width = BOOKMARK_THUMB_WIDTH;
  int height = (int)(BOOKMARK_THUMB_WIDTH / aspectRatio);
  if (height > (int)BOOKMARK_THUMB_WIDTH)
  {
    height = BOOKMARK_THUMB_WIDTH;
    width = (int)(BOOKMARK_THUMB_WIDTH * aspectRatio);
  }
  {
#ifdef HAS_VIDEO_PLAYBACK
    CRenderCapture* thumbnail = g_renderManager.AllocRenderCapture();

    if (thumbnail)
    {
      g_renderManager.Capture(thumbnail, width, height, CAPTUREFLAG_IMMEDIATELY);

      if (thumbnail->GetUserState() == CAPTURESTATE_DONE)
      {
        Crc32 crc;
        crc.ComputeFromLowerCase(g_application.CurrentFile());
        bookmark.thumbNailImage = StringUtils::Format("%08x_%i.jpg", (unsigned __int32) crc, (int)bookmark.timeInSeconds);
        bookmark.thumbNailImage = URIUtils::AddFileToFolder(CProfilesManager::Get().GetBookmarksThumbFolder(), bookmark.thumbNailImage);
        if (!CPicture::CreateThumbnailFromSurface(thumbnail->GetPixels(), width, height, thumbnail->GetWidth() * 4,
                                            bookmark.thumbNailImage))
          bookmark.thumbNailImage.clear();
      }
      else
        CLog::Log(LOGERROR,"CGUIDialogVideoBookmarks: failed to create thumbnail");

      g_renderManager.ReleaseRenderCapture(thumbnail);
    }
#endif
  }
  videoDatabase.Open();
  if (tag)
    videoDatabase.AddBookMarkForEpisode(*tag, bookmark);
  else
  {
    std::string path = g_application.CurrentFile();
    if (g_application.CurrentFileItem().HasProperty("original_listitem_url") && 
       !URIUtils::IsVideoDb(g_application.CurrentFileItem().GetProperty("original_listitem_url").asString()))
      path = g_application.CurrentFileItem().GetProperty("original_listitem_url").asString();
    videoDatabase.AddBookMarkToFile(path, bookmark, CBookmark::STANDARD);
  }
  videoDatabase.Close();
  return true;
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

bool CGUIDialogVideoBookmarks::AddEpisodeBookmark()
{
  vector<CVideoInfoTag> episodes;
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetEpisodesByFile(g_application.CurrentFile(), episodes);
  videoDatabase.Close();
  if(episodes.size() > 0)
  {
    CContextButtons choices;
    for (unsigned int i=0; i < episodes.size(); ++i)
    {
      std::string strButton = StringUtils::Format("%s %i, %s %i",
                                                 g_localizeStrings.Get(20373).c_str(), episodes[i].m_iSeason,
                                                 g_localizeStrings.Get(20359).c_str(), episodes[i].m_iEpisode);
      choices.Add(i, strButton);
    }

    int pressed = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (pressed >= 0)
    {
      AddBookmark(&episodes[pressed]);
      return true;
    }
  }
  return false;
}



bool CGUIDialogVideoBookmarks::OnAddBookmark()
{
  if (!g_application.CurrentFileItem().IsVideo())
    return false;
  
  if (CGUIDialogVideoBookmarks::AddBookmark()) 
  {
    g_windowManager.SendMessage(GUI_MSG_REFRESH_LIST, 0, WINDOW_DIALOG_VIDEO_BOOKMARKS);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(298),   //"Bookmarks"
                                          g_localizeStrings.Get(21362));//"Bookmark created"
    return true;
  }
  return false;
}

bool CGUIDialogVideoBookmarks::OnAddEpisodeBookmark()
{
  bool bReturn = false;
  if (g_application.CurrentFileItem().HasVideoInfoTag() && g_application.CurrentFileItem().GetVideoInfoTag()->m_iEpisode > -1)
  {
    CVideoDatabase videoDatabase;
    videoDatabase.Open();
    vector<CVideoInfoTag> episodes;
    videoDatabase.GetEpisodesByFile(g_application.CurrentFile(),episodes);
    if (episodes.size() > 1)
    {
      bReturn = CGUIDialogVideoBookmarks::AddEpisodeBookmark();
      if(bReturn)
      {
        g_windowManager.SendMessage(GUI_MSG_REFRESH_LIST, 0, WINDOW_DIALOG_VIDEO_BOOKMARKS);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, 
                                              g_localizeStrings.Get(298),   //"Bookmarks"
                                              g_localizeStrings.Get(21363));//"Episode Bookmark created"
 
      }
    }
    videoDatabase.Close();
  }
  return bReturn;
}


