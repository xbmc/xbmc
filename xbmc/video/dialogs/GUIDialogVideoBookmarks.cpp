/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoBookmarks.h"

#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "Util.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "pictures/Picture.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"
#include "view/ViewState.h"

#include <string>
#include <vector>

using namespace KODI::MESSAGING;

#define BOOKMARK_THUMB_WIDTH CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes

#define CONTROL_ADD_BOOKMARK           2
#define CONTROL_CLEAR_BOOKMARKS        3
#define CONTROL_ADD_EPISODE_BOOKMARK   4

#define CONTROL_THUMBS                11

CGUIDialogVideoBookmarks::CGUIDialogVideoBookmarks()
    : CGUIDialog(WINDOW_DIALOG_VIDEO_BOOKMARKS, "VideoOSDBookmarks.xml"),
    CJobQueue(false, 1, CJob::PRIORITY_NORMAL)
{
  m_vecItems = new CFileItemList;
  m_loadType = LOAD_EVERY_TIME;
  m_jobsStarted = 0;
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
      // don't init this dialog if we don't playback a file
      if (!g_application.GetAppPlayer().IsPlaying())
        return false;

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
      switch (message.GetParam1())
      {
      case 0:
        OnRefreshList();
        break;
      case 1:
        UpdateItem(message.GetParam2());
        break;
      default:
        break;
      }
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
  if (item < 0 || item >= (int) m_bookmarks.size())
    return;

  // highlight the item
  (*m_vecItems)[item]->Select(true);

  CContextButtons choices;
  choices.Add(1, (m_bookmarks[item].type == CBookmark::EPISODE ? 20405 : 20404)); // "Remove episode bookmark" or "Remove bookmark"

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

void CGUIDialogVideoBookmarks::UpdateItem(unsigned int chapterIdx)
{
  CSingleLock lock(m_refreshSection);

  int itemPos = 0;
  for (const auto& item : *m_vecItems)
  {
    if (chapterIdx == item->GetProperty("chapter").asInteger())
      break;
    itemPos++;
  }

  if (itemPos < m_vecItems->Size())
  {
    std::string time = StringUtils::Format("chapter://%s/%i", m_filePath.c_str(), chapterIdx);
    std::string cachefile = CTextureCache::GetInstance().GetCachedPath(CTextureCache::GetInstance().GetCacheFile(time) + ".jpg");
    if (XFILE::CFile::Exists(cachefile))
    {
      (*m_vecItems)[itemPos]->SetArt("thumb", cachefile);
    }
  }
}

void CGUIDialogVideoBookmarks::OnRefreshList()
{
  m_bookmarks.clear();
  std::vector<CFileItemPtr> items;

  // open the d/b and retrieve the bookmarks for the current movie
  m_filePath = g_application.CurrentFile();
  if (g_application.CurrentFileItem().HasProperty("original_listitem_url") &&
     !URIUtils::IsVideoDb(g_application.CurrentFileItem().GetProperty("original_listitem_url").asString()))
     m_filePath = g_application.CurrentFileItem().GetProperty("original_listitem_url").asString();

  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetBookMarksForFile(m_filePath, m_bookmarks);
  videoDatabase.GetBookMarksForFile(m_filePath, m_bookmarks, CBookmark::EPISODE, true);
  videoDatabase.Close();

  CSingleLock lock(m_refreshSection);
  m_vecItems->Clear();

  // cycle through each stored bookmark and add it to our list control
  for (unsigned int i = 0; i < m_bookmarks.size(); ++i)
  {
    std::string bookmarkTime;
    if (m_bookmarks[i].type == CBookmark::EPISODE)
      bookmarkTime = StringUtils::Format("%s %li %s %li", g_localizeStrings.Get(20373).c_str(), m_bookmarks[i].seasonNumber, g_localizeStrings.Get(20359).c_str(), m_bookmarks[i].episodeNumber);
    else
      bookmarkTime = StringUtils::SecondsToTimeString((long)m_bookmarks[i].timeInSeconds, TIME_FORMAT_HH_MM_SS);

    CFileItemPtr item(new CFileItem(StringUtils::Format(g_localizeStrings.Get(299).c_str(), i+1)));
    item->SetLabel2(bookmarkTime);
    item->SetArt("thumb", m_bookmarks[i].thumbNailImage);
    item->SetProperty("resumepoint", m_bookmarks[i].timeInSeconds);
    item->SetProperty("playerstate", m_bookmarks[i].playerState);
    item->SetProperty("isbookmark", "true");
    items.push_back(item);
  }

  // add chapters if around
  for (int i = 1; i <= g_application.GetAppPlayer().GetChapterCount(); ++i)
  {
    std::string chapterName;
    g_application.GetAppPlayer().GetChapterName(chapterName, i);

    int64_t pos = g_application.GetAppPlayer().GetChapterPos(i);
    std::string time = StringUtils::SecondsToTimeString((long) pos, TIME_FORMAT_HH_MM_SS);

    if (chapterName.empty() ||
        StringUtils::StartsWithNoCase(chapterName, time) ||
        StringUtils::IsNaturalNumber(chapterName))
      chapterName = StringUtils::Format(g_localizeStrings.Get(25010).c_str(), i);

    CFileItemPtr item(new CFileItem(chapterName));
    item->SetLabel2(time);

    std::string chapterPath = StringUtils::Format("chapter://%s/%i", m_filePath.c_str(), i);
    std::string cachefile = CTextureCache::GetInstance().GetCachedPath(CTextureCache::GetInstance().GetCacheFile(chapterPath)+".jpg");
    if (XFILE::CFile::Exists(cachefile))
      item->SetArt("thumb", cachefile);
    else if (i > m_jobsStarted && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS))
    {
      CFileItem item(m_filePath, false);
      CJob* job = new CThumbExtractor(item, m_filePath, true, chapterPath, pos * 1000, false);
      AddJob(job);
      m_mapJobsChapter[job] = i;
      m_jobsStarted++;
    }

    item->SetProperty("chapter", i);
    item->SetProperty("resumepoint", static_cast<double>(pos));
    item->SetProperty("ischapter", "true");
    items.push_back(item);
  }

  // sort items by resume point
  std::sort(items.begin(), items.end(), [](const CFileItemPtr &item1, const CFileItemPtr &item2) {
    return item1->GetProperty("resumepoint").asDouble() < item2->GetProperty("resumepoint").asDouble();
  });

  // add items to file list and mark the proper item as selected if the current playtime is above
  int selectedItemIndex = 0;
  double playTime = g_application.GetTime();
  for (auto& item : items)
  {
    m_vecItems->Add(item);
    if (playTime >= item->GetProperty("resumepoint").asDouble())
      selectedItemIndex = m_vecItems->Size() - 1;
  }

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(selectedItemIndex);
}

void CGUIDialogVideoBookmarks::Update()
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();

  if (g_application.CurrentFileItem().HasVideoInfoTag() && g_application.CurrentFileItem().GetVideoInfoTag()->m_iEpisode > -1)
  {
    std::vector<CVideoInfoTag> episodes;
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


  m_viewControl.SetCurrentView(DEFAULT_VIEW_ICONS);

  // empty the list ready for population
  Clear();

  OnRefreshList();

  videoDatabase.Close();
}

void CGUIDialogVideoBookmarks::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogVideoBookmarks::GotoBookmark(int item)
{
  if (item < 0 || item >= m_vecItems->Size() || !g_application.GetAppPlayer().HasPlayer())
    return;

  CFileItemPtr fileItem = m_vecItems->Get(item);
  int chapter = static_cast<int>(fileItem->GetProperty("chapter").asInteger());
  if (chapter <= 0)
  {
    g_application.GetAppPlayer().SetPlayerState(fileItem->GetProperty("playerstate").asString());
    g_application.SeekTime(fileItem->GetProperty("resumepoint").asDouble());
  }
  else
    g_application.GetAppPlayer().SeekChapter(chapter);

  Close();
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

  if( g_application.GetAppPlayer().HasPlayer() )
    bookmark.playerState = g_application.GetAppPlayer().GetPlayerState();
  else
    bookmark.playerState.clear();

  bookmark.player = g_application.GetCurrentPlayer();

  // create the thumbnail image
  float aspectRatio = g_application.GetAppPlayer().GetRenderAspectRatio();
  int width = BOOKMARK_THUMB_WIDTH;
  int height = (int)(BOOKMARK_THUMB_WIDTH / aspectRatio);
  if (height > (int)BOOKMARK_THUMB_WIDTH)
  {
    height = BOOKMARK_THUMB_WIDTH;
    width = (int)(BOOKMARK_THUMB_WIDTH * aspectRatio);
  }


  uint8_t *pixels = (uint8_t*)malloc(height * width * 4);
  unsigned int captureId = g_application.GetAppPlayer().RenderCaptureAlloc();

  g_application.GetAppPlayer().RenderCapture(captureId, width, height, CAPTUREFLAG_IMMEDIATELY);
  bool hasImage = g_application.GetAppPlayer().RenderCaptureGetPixels(captureId, 1000, pixels, height * width * 4);

  if (hasImage)
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    auto crc = Crc32::ComputeFromLowerCase(g_application.CurrentFile());
    bookmark.thumbNailImage = StringUtils::Format("%08x_%i.jpg", crc, (int)bookmark.timeInSeconds);
    bookmark.thumbNailImage = URIUtils::AddFileToFolder(profileManager->GetBookmarksThumbFolder(), bookmark.thumbNailImage);

    if (!CPicture::CreateThumbnailFromSurface(pixels, width, height, width * 4,
                                                         bookmark.thumbNailImage))
    {
      bookmark.thumbNailImage.clear();
    }
    else
      CLog::Log(LOGERROR,"CGUIDialogVideoBookmarks: failed to create thumbnail");

    g_application.GetAppPlayer().RenderCaptureRelease(captureId);
  }
  else
    CLog::Log(LOGERROR,"CGUIDialogVideoBookmarks: failed to create thumbnail 2");

  free(pixels);

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
  m_jobsStarted = 0;
  m_mapJobsChapter.clear();
  m_vecItems->Clear();
}

void CGUIDialogVideoBookmarks::OnWindowUnload()
{
  //stop running thumb extraction jobs
  CancelJobs();
  m_mapJobsChapter.clear();
  m_vecItems->Clear();
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
  std::vector<CVideoInfoTag> episodes;
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetEpisodesByFile(g_application.CurrentFile(), episodes);
  videoDatabase.Close();
  if (!episodes.empty())
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
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_REFRESH_LIST, 0, WINDOW_DIALOG_VIDEO_BOOKMARKS);
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(298),   // "Bookmarks"
                                          g_localizeStrings.Get(21362));// "Bookmark created"
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
    std::vector<CVideoInfoTag> episodes;
    videoDatabase.GetEpisodesByFile(g_application.CurrentFile(),episodes);
    if (episodes.size() > 1)
    {
      bReturn = CGUIDialogVideoBookmarks::AddEpisodeBookmark();
      if(bReturn)
      {
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_REFRESH_LIST, 0, WINDOW_DIALOG_VIDEO_BOOKMARKS);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                              g_localizeStrings.Get(298),   // "Bookmarks"
                                              g_localizeStrings.Get(21363));// "Episode Bookmark created"

      }
    }
    videoDatabase.Close();
  }
  return bReturn;
}

void CGUIDialogVideoBookmarks::OnJobComplete(unsigned int jobID,
                                             bool success, CJob* job)
{
  if (success && IsActive())
  {
    MAPJOBSCHAPS::iterator iter = m_mapJobsChapter.find(job);
    if (iter != m_mapJobsChapter.end())
    {
      unsigned int chapterIdx = (*iter).second;
      CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, 1, chapterIdx);
      CApplicationMessenger::GetInstance().SendGUIMessage(m);
      m_mapJobsChapter.erase(iter);
    }
  }
  CJobQueue::OnJobComplete(jobID, success, job);
}
