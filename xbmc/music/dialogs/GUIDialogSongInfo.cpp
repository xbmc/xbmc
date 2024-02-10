/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSongInfo.h"

#include "GUIDialogMusicInfo.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "Util.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "music/windows/GUIWindowMusicBase.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"

using namespace KODI;

#define CONTROL_BTN_REFRESH       6
#define CONTROL_USERRATING        7
#define CONTROL_BTN_PLAY 8
#define CONTROL_BTN_GET_THUMB     10
#define CONTROL_ALBUMINFO         12

#define CONTROL_LIST              50

#define TIME_TO_BUSY_DIALOG 500

class CGetSongInfoJob : public CJob
{
public:
  ~CGetSongInfoJob(void) override = default;

  // Fetch full song information including art types list
  bool DoWork() override
  {
    CGUIDialogSongInfo *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSongInfo>(WINDOW_DIALOG_SONG_INFO);
    if (!dialog)
      return false;
    if (dialog->IsCancelled())
      return false;
    CFileItemPtr m_song = dialog->GetCurrentListItem();

    // Fetch tag data from library using filename of item path, or scanning file
    // (if item does not already have this loaded)
    if (!m_song->LoadMusicTag())
    {
      // Stop SongInfoDialog waiting
      dialog->FetchComplete();
      return false;
    }
    if (dialog->IsCancelled())
      return false;
    // Fetch album and primary song artist data from library as properties
    // and lyrics by scanning tags from file
    MUSIC_INFO::CMusicInfoLoader::LoadAdditionalTagInfo(m_song.get());
    if (dialog->IsCancelled())
      return false;

    // Get album path (for use in browsing art selection)
    std::string albumpath;
    CMusicDatabase db;
    db.Open();
    db.GetAlbumPath(m_song->GetMusicInfoTag()->GetAlbumId(), albumpath);
    m_song->SetProperty("album_path", albumpath);
    db.Close();
    if (dialog->IsCancelled())
      return false;

    // Load song art.
    // For songs in library this includes related album and artist(s) art.
    // Also fetches artist art for non library songs when artist can be found
    // uniquely by name, otherwise just embedded or cached thumb is fetched.
    CMusicThumbLoader loader;
    loader.LoadItem(m_song.get());
    if (dialog->IsCancelled())
      return false;

    // For songs in library fill vector of possible art types, with current art when it exists
    // for display on the art type selection dialog
    CFileItemList artlist;
    MUSIC_UTILS::FillArtTypesList(*m_song, artlist);
    dialog->SetArtTypeList(artlist);
    if (dialog->IsCancelled())
      return false;

    // Tell waiting SongInfoDialog that job is complete
    dialog->FetchComplete();

    return true;
  }
};

CGUIDialogSongInfo::CGUIDialogSongInfo(void)
    : CGUIDialog(WINDOW_DIALOG_SONG_INFO, "DialogMusicInfo.xml")
    , m_song(new CFileItem)
{
  m_cancelled = false;
  m_hasUpdatedUserrating = false;
  m_startUserrating = -1;
  m_artTypeList.Clear();
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSongInfo::~CGUIDialogSongInfo(void) = default;

bool CGUIDialogSongInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_artTypeList.Clear();
      if (m_startUserrating != m_song->GetMusicInfoTag()->GetUserrating())
      {
        m_hasUpdatedUserrating = true;

        // Asynchronously update song userrating in library
        MUSIC_UTILS::UpdateSongRatingJob(m_song, m_song->GetMusicInfoTag()->GetUserrating());

        // Send a message to all windows to tell them to update the fileitem
        // This communicates the rating change to the music lib window, current playlist and OSD.
        // The music lib window item is updated to but changes to the rating when it is the sort
        // do not show on screen until refresh() that fetches the list from scratch, sorts etc.
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_song);
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      }
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg);
      break;
    }
  case GUI_MSG_WINDOW_INIT:
    CGUIDialog::OnMessage(message);
    Update();
    m_cancelled = false;
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_USERRATING)
      {
        OnSetUserrating();
      }
      else if (iControl == CONTROL_ALBUMINFO)
      {
        CGUIDialogMusicInfo::ShowForAlbum(m_albumId);
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetArt();
        return true;
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if ((ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction))
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= static_cast<int>(m_song->GetMusicInfoTag()->GetContributors().size()))
            break;
          int idArtist = m_song->GetMusicInfoTag()->GetContributors()[iItem].GetArtistId();
          if (idArtist > 0)
            CGUIDialogMusicInfo::ShowForArtist(idArtist);
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_PLAY)
      {
        OnPlaySong(m_song);
        return true;
      }
      return false;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSongInfo::OnAction(const CAction& action)
{
  int userrating = m_song->GetMusicInfoTag()->GetUserrating();
  if (action.GetID() == ACTION_INCREASE_RATING)
  {
    SetUserrating(userrating + 1);
    return true;
  }
  else if (action.GetID() == ACTION_DECREASE_RATING)
  {
    SetUserrating(userrating - 1);
    return true;
  }
  else if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSongInfo::OnBack(int actionID)
{
  m_cancelled = true;
  return CGUIDialog::OnBack(actionID);
}

void CGUIDialogSongInfo::FetchComplete()
{
  //Trigger the event to indicate data has been fetched
  m_event.Set();
}

void CGUIDialogSongInfo::OnInitWindow()
{
  // Enable album info button when we know album
  m_albumId = m_song->GetMusicInfoTag()->GetAlbumId();

  CONTROL_ENABLE_ON_CONDITION(CONTROL_ALBUMINFO, m_albumId > 0);

  // Disable music user rating button for plugins as they don't have tables to save this
  if (m_song->IsPlugin())
    CONTROL_DISABLE(CONTROL_USERRATING);
  else
    CONTROL_ENABLE(CONTROL_USERRATING);

  // Disable the Choose Art button if the user isn't allowed it
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB,
    profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);

  SET_CONTROL_HIDDEN(CONTROL_BTN_REFRESH);
  SET_CONTROL_LABEL(CONTROL_USERRATING, 38023);
  SET_CONTROL_LABEL(CONTROL_BTN_GET_THUMB, 13511);
  SET_CONTROL_LABEL(CONTROL_ALBUMINFO, 10523);
  SET_CONTROL_LABEL(CONTROL_BTN_PLAY, 208);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogSongInfo::Update()
{
  CFileItemList items;
  for (const auto& contributor : m_song->GetMusicInfoTag()->GetContributors())
  {
    auto item = std::make_shared<CFileItem>(contributor.GetRoleDesc());
    item->SetLabel2(contributor.GetArtist());
    item->GetMusicInfoTag()->SetDatabaseId(contributor.GetArtistId(), MediaTypeArtist);
    items.Add(std::move(item));
  }
  CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, &items);
  OnMessage(message);
}

void CGUIDialogSongInfo::SetUserrating(int userrating)
{
  userrating = std::max(userrating, 0);
  userrating = std::min(userrating, 10);
  if (userrating != m_song->GetMusicInfoTag()->GetUserrating())
  {
    m_song->GetMusicInfoTag()->SetUserrating(userrating);
  }
}

bool CGUIDialogSongInfo::SetSong(CFileItem* item)
{
  *m_song = *item;
  m_event.Reset();
  m_cancelled = false;  // SetSong happens before win_init
  // In a separate job fetch song info and fill list of art types.
  int jobid =
      CServiceBroker::GetJobManager()->AddJob(new CGetSongInfoJob(), nullptr, CJob::PRIORITY_LOW);

  // Wait to get all data before show, allowing user to cancel if fetch is slow
  if (!CGUIDialogBusy::WaitOnEvent(m_event, TIME_TO_BUSY_DIALOG))
  {
    // Cancel job still waiting in queue (unlikely)
    CServiceBroker::GetJobManager()->CancelJob(jobid);
    // Flag to stop job already in progress
    m_cancelled = true;
    return false;
  }

  // Store initial userrating
  m_startUserrating = m_song->GetMusicInfoTag()->GetUserrating();
  m_hasUpdatedUserrating = false;
  return true;
}

void CGUIDialogSongInfo::SetArtTypeList(CFileItemList& artlist)
{
  m_artTypeList.Copy(artlist);
}

CFileItemPtr CGUIDialogSongInfo::GetCurrentListItem(int offset)
{
  return m_song;
}

std::string CGUIDialogSongInfo::GetContent()
{
  return "songs";
}

/*
  Allow user to choose artwork for the song
  For each type of art the options are:
  1.  Current art
  2.  Local art (thumb found by filename)
  3.  Embedded art (@todo)
  4.  None
  Note that songs are not scraped, hence there is no list of urls for possible remote art
*/
void CGUIDialogSongInfo::OnGetArt()
{
  std::string type = MUSIC_UTILS::ShowSelectArtTypeDialog(m_artTypeList);
  if (type.empty())
    return; // Cancelled

  CFileItemList items;
  CGUIListItem::ArtMap primeArt = m_song->GetArt(); // Song art without fallbacks
  bool bHasArt = m_song->HasArt(type);
  bool bFallback(false);
  if (bHasArt)
  {
    // Check if that type of art is actually a fallback, e.g. album thumb or artist fanart
    CGUIListItem::ArtMap::const_iterator i = primeArt.find(type);
    bFallback = (i == primeArt.end());
  }

  // Build list of possible images of that art type
  if (bHasArt)
  {
    // Add item for current artwork, could a fallback from album/artist
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", m_song->GetArt(type));
    item->SetArt("icon", "DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(13512));  //! @todo: label fallback art so user knows?
    items.Add(item);
  }
  else if (m_song->HasArt("thumb"))
  { // For missing art of that type add the thumb (when it exists and not a fallback)
    CGUIListItem::ArtMap::const_iterator i = primeArt.find("thumb");
    if (i != primeArt.end())
    {
      CFileItemPtr item(new CFileItem("thumb://Thumb", false));
      item->SetArt("thumb", m_song->GetArt("thumb"));
      item->SetArt("icon", "DefaultAlbumCover.png");
      item->SetLabel(g_localizeStrings.Get(21371));
      items.Add(item);
    }
  }

  std::string localThumb;
  if (type == "thumb")
  { // Local thumb type art held in <filename>.tbn (for non-library items)
    localThumb = m_song->GetUserMusicThumb(true);
    if (MUSIC::IsMusicDb(*m_song))
    {
      CFileItem item(m_song->GetMusicInfoTag()->GetURL(), false);
      localThumb = item.GetUserMusicThumb(true);
    }
    if (CFileUtils::Exists(localThumb))
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetArt("thumb", localThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }

  // Clear these local images from cache so user will see any recent
  // local file changes immediately
  for (auto& item : items)
  {
    std::string thumb(item->GetArt("thumb"));
    if (thumb.empty())
      continue;
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
    // Remove any thumbnail of local image too (created when browsing files)
    std::string thumbthumb(CTextureUtils::GetWrappedThumbURL(thumb));
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumbthumb);
  }

  if (bHasArt && !bFallback)
  { // Actually has this type of art (not a fallback) so
    // allow the user to delete it by selecting "no art".
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetArt("thumb", "DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(13515));
    items.Add(item);
  }

  //! @todo: Add support for extracting embedded art

  // Show list of possible art for user selection
  std::string result;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  // Add album folder as source (could be disc set)
  std::string albumpath = m_song->GetProperty("album_path").asString();
  if (!albumpath.empty())
  {
    CFileItem pathItem(albumpath, true);
    CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(sources, pathItem);
  }
  else  // Add parent folder of song
    CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(sources, *m_song);
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  if (CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(13511), result) &&
    result != "thumb://Current")
  {
    // User didn't choose the one they have, or the fallback image.
    // Overwrite with the new art or clear it
    std::string newArt;
    if (result == "thumb://Thumb")
      newArt = m_song->GetArt("thumb");
    else if (result == "thumb://Local")
      newArt = localThumb;
//    else if (result == "thumb://Embedded")
//      newArt = embeddedArt;
    else if (CFileUtils::Exists(result))
      newArt = result;
    else // none
      newArt.clear();

    // Asynchronously update that type of art in the database
    MUSIC_UTILS::UpdateArtJob(m_song, type, newArt);

    // Update local song with current art
    if (newArt.empty())
    {
      // Remove that type of art from the song
      primeArt.erase(type);
      m_song->SetArt(primeArt);
    }
    else
      // Add or modify the type of art
      m_song->SetArt(type, newArt);

    // Update local art list with current art
    // Show any fallback art when song art removed
    if (newArt.empty() && m_song->HasArt(type))
      newArt = m_song->GetArt(type);
    for (const auto& artitem : m_artTypeList)
    {
      if (artitem->GetProperty("artType") == type)
      {
        artitem->SetArt("thumb", newArt);
        break;
      }
    }

    // Get new artwork to show in other places e.g. on music lib window,
    // current playlist and player OSD.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_song);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  }

  // Re-open the art type selection dialog as we come back from
  // the image selection dialog
  OnGetArt();
}

void CGUIDialogSongInfo::OnSetUserrating()
{
  int userrating = MUSIC_UTILS::ShowSelectRatingDialog(m_song->GetMusicInfoTag()->GetUserrating());
  if (userrating < 0) // Nothing selected, so rating unchanged
    return;

  SetUserrating(userrating);
}

void CGUIDialogSongInfo::ShowFor(CFileItem* pItem)
{
  if (pItem->m_bIsFolder)
    return;
  if (!MUSIC::IsMusicDb(*pItem))
    pItem->LoadMusicTag();
  if (!pItem->HasMusicInfoTag())
    return;

  CGUIDialogSongInfo *dialog = CServiceBroker::GetGUI()->GetWindowManager().
    GetWindow<CGUIDialogSongInfo>(WINDOW_DIALOG_SONG_INFO);
  if (dialog)
  {
    if (dialog->SetSong(pItem))  // Fetch full song info asynchronously
    {
      dialog->Open();
      if (dialog->HasUpdatedUserrating())
      {
        auto window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowMusicBase>(WINDOW_MUSIC_NAV);
        if (window)
          window->RefreshContent("songs");
      }
    }
  }
}

void CGUIDialogSongInfo::OnPlaySong(const std::shared_ptr<CFileItem>& item)
{
  Close(true);
  MUSIC_UTILS::PlayItem(item, "");
}
