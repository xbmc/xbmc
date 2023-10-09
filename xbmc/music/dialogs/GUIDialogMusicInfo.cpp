/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogMusicInfo.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/Directory.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicDatabase.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicThumbLoader.h"
#include "music/MusicUtils.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "music/windows/GUIWindowMusicBase.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/ProgressJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace MUSIC_INFO;
using namespace MUSICDATABASEDIRECTORY;
using namespace KODI::MESSAGING;

#define CONTROL_BTN_REFRESH      6
#define CONTROL_USERRATING       7
#define CONTROL_BTN_PLAY 8
#define CONTROL_BTN_GET_THUMB   10
#define CONTROL_ARTISTINFO      12

#define CONTROL_LIST            50

#define TIME_TO_BUSY_DIALOG 500

class CGetInfoJob : public CJob
{
public:
  ~CGetInfoJob(void) override = default;

  // Fetch full album/artist information including art types list
  bool DoWork() override
  {
    CGUIDialogMusicInfo *dialog = CServiceBroker::GetGUI()->GetWindowManager().
	  GetWindow<CGUIDialogMusicInfo>(WINDOW_DIALOG_MUSIC_INFO);
    if (!dialog)
      return false;
    if (dialog->IsCancelled())
      return false;
    CFileItemPtr m_item = dialog->GetCurrentListItem();
    CMusicInfoTag& tag = *m_item->GetMusicInfoTag();

    CMusicDatabase database;
    database.Open();
    // May only have partially populated music item, so fetch all artist or album data from db
    if (tag.GetType() == MediaTypeArtist)
    {
      int artistId = tag.GetDatabaseId();
      CArtist artist;
      if (!database.GetArtist(artistId, artist))
        return false;
      tag.SetArtist(artist);
      CMusicDatabase::SetPropertiesFromArtist(*m_item, artist);
      m_item->SetLabel(artist.strArtist);

      // Get artist folder where local art could be found
      // Get the *name* of the folder for this artist within the Artist Info folder (may not exist).
      // If there is no Artist Info folder specified in settings this will be blank
      database.GetArtistPath(artist, artist.strPath);
      // Get the old location for those album artists with a unique folder (local to music files)
      // If there is no folder for the artist and *only* the artist this will be blank
      std::string oldartistpath;
      bool oldpathfound = database.GetOldArtistPath(artist.idArtist, oldartistpath);

      // Set up path for *item folder when browsing for art, by default this is
      // in the Artist Info Folder (when it exists), but could end up blank
      std::string artistItemPath = artist.strPath;
      if (!CDirectory::Exists(artistItemPath))
      {
        // Fall back local to music files (historic location for those album artists with a unique folder)
        // although there may not be such a unique folder for the arist
        if (oldpathfound)
          artistItemPath = oldartistpath;
        else
          // Fall back further to browse the Artist Info Folder itself
          artistItemPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
      }
      m_item->SetPath(artistItemPath);

      // Store info as CArtist as well as item properties
      dialog->SetArtist(artist, oldartistpath);

      // Fetch artist discography as scraped from online sources, but always
      // include all the albums in the music library
      dialog->SetDiscography(database);
    }
    else
    {
      // tag.GetType == MediaTypeAlbum
      int albumId = tag.GetDatabaseId();
      CAlbum album;
      if (!database.GetAlbum(albumId, album))
        return false;
      tag.SetAlbum(album);
      CMusicDatabase::SetPropertiesFromAlbum(*m_item, album);

      // Get album folder where local art could be found
      database.GetAlbumPath(albumId, album.strPath);
      // Set up path for *item folder when browsing for art
      m_item->SetPath(album.strPath);
      // Store info as CAlbum as well as item properties
      dialog->SetAlbum(album, album.strPath);

      // Set the list of songs and related art
      dialog->SetSongs(album.songs);
    }
    database.Close();

    /*
      Load current art (to CGUIListItem.m_art)
      For albums this includes related artist(s) art and artist fanart set as
      fallback album fanart.
      Clear item art first to ensure fresh not cached/partial art
    */
    m_item->ClearArt();
    CMusicThumbLoader loader;
    loader.LoadItem(m_item.get());

    // Fill vector of possible art types with current art, when it exists,
    // for display on the art type selection dialog
    CFileItemList artlist;
    MUSIC_UTILS::FillArtTypesList(*m_item, artlist);
    dialog->SetArtTypeList(artlist);
    if (dialog->IsCancelled())
      return false;

    // Tell waiting MusicDialog that job is complete
    dialog->FetchComplete();

    return true;
  }
};

class CSetUserratingJob : public CJob
{
  int idAlbum;
  int iUserrating;
public:
  CSetUserratingJob(int albumId, int userrating) :
    idAlbum(albumId),
    iUserrating(userrating)
  { }

  ~CSetUserratingJob(void) override = default;

  bool DoWork(void) override
  {
    // Asynchronously update userrating in library
    CMusicDatabase db;
    if (db.Open())
    {
      db.SetAlbumUserrating(idAlbum, iUserrating);
      db.Close();
    }

    return true;
  }
};

class CRefreshInfoJob : public CProgressJob
{
public:
  CRefreshInfoJob(CGUIDialogProgress* progressDialog)
    : CProgressJob(nullptr)
  {
    if (progressDialog)
      SetProgressIndicators(nullptr, progressDialog);
    SetAutoClose(true);
  }

  ~CRefreshInfoJob(void) override = default;

  // Refresh album/artist information including art types list
  bool DoWork() override
  {
    CGUIDialogMusicInfo *dialog = CServiceBroker::GetGUI()->GetWindowManager().
	  GetWindow<CGUIDialogMusicInfo>(WINDOW_DIALOG_MUSIC_INFO);
    if (!dialog)
      return false;
    if (dialog->IsCancelled())
      return false;
    CFileItemPtr m_item = dialog->GetCurrentListItem();
    CMusicInfoTag& tag = *m_item->GetMusicInfoTag();
    CArtist& m_artist = dialog->GetArtist();
    CAlbum& m_album = dialog->GetAlbum();

    CGUIDialogProgress* dlgProgress = GetProgressDialog();
    CMusicDatabase database;
    database.Open();
    if (tag.GetType() == MediaTypeArtist)
    {
      ADDON::ScraperPtr scraper;
      if (!database.GetScraper(m_artist.idArtist, CONTENT_ARTISTS, scraper))
        return false;

      if (dlgProgress->IsCanceled())
        return false;
      database.ClearArtistLastScrapedTime(m_artist.idArtist);

      if (dlgProgress->IsCanceled())
        return false;
      CMusicInfoScanner scanner;
      if (scanner.UpdateArtistInfo(m_artist, scraper, true, dlgProgress) != CInfoScanner::INFO_ADDED)
        return false;
      else
        // Tell info dialog, so can show message
        dialog->SetScrapedInfo(true);

      if (dlgProgress->IsCanceled())
        return false;
      //That changed DB and m_artist, now update dialog item with new info and art
      tag.SetArtist(m_artist);
      CMusicDatabase::SetPropertiesFromArtist(*m_item, m_artist);

      // Fetch artist discography as scraped from online sources, but always
      // include all the albums in the music library
      dialog->SetDiscography(database);
    }
    else
    {
      // tag.GetType == MediaTypeAlbum
      ADDON::ScraperPtr scraper;
      if (!database.GetScraper(m_album.idAlbum, CONTENT_ALBUMS, scraper))
        return false;

      if (dlgProgress->IsCanceled())
        return false;
      database.ClearAlbumLastScrapedTime(m_album.idAlbum);

      if (dlgProgress->IsCanceled())
        return false;
      CMusicInfoScanner scanner;
      if (scanner.UpdateAlbumInfo(m_album, scraper, true, GetProgressDialog()) != CInfoScanner::INFO_ADDED)
        return false;
      else
        // Tell info dialog, so can show message
        dialog->SetScrapedInfo(true);

      if (dlgProgress->IsCanceled())
        return false;
      //That changed DB and m_album, now update dialog item with new info and art
      // Album songs are unchanged by refresh (even with Musicbrainz sync?)
      tag.SetAlbum(m_album);
      CMusicDatabase::SetPropertiesFromAlbum(*m_item, m_album);

      // Set the list of songs and related art
      dialog->SetSongs(m_album.songs);
    }
    database.Close();

    if (dlgProgress->IsCanceled())
      return false;
    /*
    Load current art (to CGUIListItem.m_art)
    For albums this includes related artist(s) art and artist fanart set as
    fallback album fanart.
    Clear item art first to ensure fresh not cached/partial art
    */
    m_item->ClearArt();
    CMusicThumbLoader loader;
    loader.LoadItem(m_item.get());

    if (dlgProgress->IsCanceled())
      return false;
    // Fill vector of possible art types with current art, when it exists,
    // for display on the art type selection dialog
    CFileItemList artlist;
    MUSIC_UTILS::FillArtTypesList(*m_item, artlist);
    dialog->SetArtTypeList(artlist);
    if (dialog->IsCancelled())
      return false;

    // Tell waiting MusicDialog that job is complete
    MarkFinished();
    return true;
  }
};

CGUIDialogMusicInfo::CGUIDialogMusicInfo(void)
  : CGUIDialog(WINDOW_DIALOG_MUSIC_INFO, "DialogMusicInfo.xml"),
    m_albumSongs(new CFileItemList),
    m_item(new CFileItem),
    m_artTypeList(new CFileItemList)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMusicInfo::~CGUIDialogMusicInfo(void)
{
}

bool CGUIDialogMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_artTypeList->Clear();
      // For albums update user rating if it has changed
      if (!m_bArtistInfo && m_startUserrating != m_item->GetMusicInfoTag()->GetUserrating())
      {
        m_hasUpdatedUserrating = true;

        // Asynchronously update song userrating in library
        CSetUserratingJob *job = new CSetUserratingJob(m_item->GetMusicInfoTag()->GetAlbumId(),
                                                       m_item->GetMusicInfoTag()->GetUserrating());
        CServiceBroker::GetJobManager()->AddJob(job, nullptr);
      }
      if (m_hasRefreshed || m_hasUpdatedUserrating)
      {
        // Send a message to all windows to tell them to update the item.
        // This communicates changes to the music lib window.
        // The music lib window item is updated to but changes to the rating when it is the sort
        // do not show on screen until refresh() that fetches the list from scratch, sorts etc.
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_item);
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      }

      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg);
      m_albumSongs->Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      Update();
      m_cancelled = false;
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_USERRATING)
      {
        OnSetUserrating();
      }
      else if (iControl == CONTROL_BTN_REFRESH)
      {
        RefreshInfo();
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetArt();
        return true;
      }
      else if (iControl == CONTROL_ARTISTINFO)
      {
        if (!m_bArtistInfo)
          OnArtistInfo(m_album.artistCredits[0].GetArtistId());
        return true;
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (m_bArtistInfo && (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction))
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
          int iItem = msg.GetParam1();
          int id = -1;
          if (iItem >= 0 && iItem < m_albumSongs->Size())
              id = m_albumSongs->Get(iItem)->GetMusicInfoTag()->GetDatabaseId();
          if (id > 0)
          {
            OnAlbumInfo(id);
            return true;
          }
        }
      }
      else if (iControl == CONTROL_BTN_PLAY)
      {
        if (m_album.idAlbum >= 0)
        {
          // Play album
          const std::string path = StringUtils::Format("musicdb://albums/{}", m_album.idAlbum);
          OnPlayItem(std::make_shared<CFileItem>(path, m_album));
          return true;
        }
        else
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
          const int iItem = msg.GetParam1();
          if (iItem >= 0 && iItem < m_albumSongs->Size())
          {
            // Play selected song
            OnPlayItem(m_albumSongs->Get(iItem));
            return true;
          }
        }
        return false;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogMusicInfo::OnAction(const CAction &action)
{
  int userrating = m_item->GetMusicInfoTag()->GetUserrating();
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

bool CGUIDialogMusicInfo::SetItem(CFileItem* item)
{
  *m_item = *item;
  m_event.Reset();
  m_cancelled = false;  // Happens before win_init

  // In a separate job fetch info and fill list of art types.
  int jobid =
      CServiceBroker::GetJobManager()->AddJob(new CGetInfoJob(), nullptr, CJob::PRIORITY_LOW);

  // Wait to get all data before show, allowing user to cancel if fetch is slow
  if (!CGUIDialogBusy::WaitOnEvent(m_event, TIME_TO_BUSY_DIALOG))
  {
    // Cancel job still waiting in queue (unlikely)
    CServiceBroker::GetJobManager()->CancelJob(jobid);
    // Flag to stop job already in progress
    m_cancelled = true;
    return false;
  }

  return true;
}

void CGUIDialogMusicInfo::SetAlbum(const CAlbum& album, const std::string &path)
{
  m_album = album;
  m_item->SetPath(album.strPath);

  m_startUserrating = m_album.iUserrating;
  m_fallbackartpath.clear();
  m_bArtistInfo = false;
  m_hasUpdatedUserrating = false;
  m_hasRefreshed = false;
}

void CGUIDialogMusicInfo::SetArtist(const CArtist& artist, const std::string &path)
{
  m_artist = artist;
  m_fallbackartpath = path;
  m_bArtistInfo = true;
  m_hasRefreshed = false;
}

void CGUIDialogMusicInfo::SetSongs(const VECSONGS &songs) const
{
  m_albumSongs->Clear();
  CMusicThumbLoader loader;
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItemPtr item(new CFileItem(song));
    // Load the song art and related artist(s) (that may be different from album artist) art
    loader.LoadItem(item.get());
    m_albumSongs->Add(item);
  }
}

void CGUIDialogMusicInfo::SetDiscography(CMusicDatabase& database) const
{
  m_albumSongs->Clear();
  database.GetArtistDiscography(m_artist.idArtist, *m_albumSongs);
  CMusicThumbLoader loader;
  for (const auto& item : *m_albumSongs)
  {
    // Load all the album art and related artist(s) art (could be other collaborating artists)
    loader.LoadItem(item.get());
    if (item->GetMusicInfoTag()->GetDatabaseId() == -1)
      item->SetArt("thumb", "DefaultAlbumCover.png");
  }
}

void CGUIDialogMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    SET_CONTROL_HIDDEN(CONTROL_ARTISTINFO);
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);

    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs.get());
    OnMessage(message);

  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_ARTISTINFO);
    SET_CONTROL_VISIBLE(CONTROL_USERRATING);

    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs.get());
    OnMessage(message);

  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // Disable the Choose Art button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB,
    profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
}

void CGUIDialogMusicInfo::SetLabel(int iControl, const std::string& strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIDialogMusicInfo::OnInitWindow()
{
  SET_CONTROL_LABEL(CONTROL_BTN_REFRESH, 184);
  SET_CONTROL_LABEL(CONTROL_USERRATING, 38023);
  SET_CONTROL_LABEL(CONTROL_BTN_GET_THUMB, 13511);
  SET_CONTROL_LABEL(CONTROL_ARTISTINFO, 21891);
  SET_CONTROL_LABEL(CONTROL_BTN_PLAY, 208);

  if (m_bArtistInfo)
  {
    SET_CONTROL_HIDDEN(CONTROL_ARTISTINFO);
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);
    SET_CONTROL_HIDDEN(CONTROL_BTN_PLAY);
  }
  CGUIDialog::OnInitWindow();
}

void CGUIDialogMusicInfo::FetchComplete()
{
  //Trigger the event to indicate data has been fetched
  m_event.Set();
}


void CGUIDialogMusicInfo::RefreshInfo()
{
  // Double check we have permission (button should be hidden when not)
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
  if (!profileManager->GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return;

  // Check if scanning
  if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
  {
    HELPERS::ShowOKDialogText(CVariant{ 189 }, CVariant{ 14057 });
    return;
  }

  CGUIDialogProgress* dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().
    GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
  if (!dlgProgress)
    return;

  if (m_bArtistInfo)
  { // Show dialog box indicating we're searching for the artist
    dlgProgress->SetHeading(CVariant{ 21889 });
    dlgProgress->SetLine(0, CVariant{ m_artist.strArtist });
    dlgProgress->SetLine(1, CVariant{ "" });
    dlgProgress->SetLine(2, CVariant{ "" });
  }
  else
  { // Show dialog box indicating we're searching for the album
    dlgProgress->SetHeading(CVariant{ 185 });
    dlgProgress->SetLine(0, CVariant{ m_album.strAlbum });
    dlgProgress->SetLine(1, CVariant{ m_album.strArtistDesc });
    dlgProgress->SetLine(2, CVariant{ "" });
  }
  dlgProgress->Open();

  SetScrapedInfo(false);
  // Start separate job to scrape info and fill list of art types.
  CServiceBroker::GetJobManager()->AddJob(new CRefreshInfoJob(dlgProgress), nullptr,
                                          CJob::PRIORITY_HIGH);

  // Wait for refresh to complete or be canceled, but render every 10ms so that the
  // pointer movements works on dialog even when job is reporting progress infrequently
  if (dlgProgress)
    dlgProgress->Wait(10);

  if (dlgProgress->IsCanceled())
  {
    return;
  }

  // Show message when scraper was unsuccessful
  if (!HasScrapedInfo())
  {
    if (m_bArtistInfo)
      HELPERS::ShowOKDialogText(CVariant{ 21889 }, CVariant{ 20199 });
    else
      HELPERS::ShowOKDialogText(CVariant{ 185 }, CVariant{ 500 });
    return;
  }

  //  Show new values on screen
  Update();
  m_hasRefreshed = true;

  if (dlgProgress)
    dlgProgress->Close();
}

void CGUIDialogMusicInfo::SetUserrating(int userrating) const
{
  userrating = std::max(userrating, 0);
  userrating = std::min(userrating, 10);
  if (userrating != m_item->GetMusicInfoTag()->GetUserrating())
  {
    m_item->GetMusicInfoTag()->SetUserrating(userrating);
  }
}

void CGUIDialogMusicInfo::OnAlbumInfo(int id)
{
  // Switch to show album info for given album ID
  // Close current (artist) dialog to save art changes
  Close(true);
  ShowForAlbum(id);
}

void CGUIDialogMusicInfo::OnArtistInfo(int id)
{
  // Switch to show artist info for given artist ID
  // Close current (album) dialog to save art and rating changes
  Close(true);
  ShowForArtist(id);
}

CFileItemPtr CGUIDialogMusicInfo::GetCurrentListItem(int offset)
{
  return m_item;
}

std::string CGUIDialogMusicInfo::GetContent()
{
  if (m_item->GetMusicInfoTag()->GetType() == MediaTypeArtist)
    return "artists";
  else
    return "albums";
}

void CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item)
{
  std::string itemDir;
  std::string artistFolder;

  itemDir = item.GetPath();
  if (item.HasMusicInfoTag())
  {
    if (item.GetMusicInfoTag()->GetType() == MediaTypeSong)
      itemDir = URIUtils::GetParentPath(item.GetMusicInfoTag()->GetURL());

    // For artist add Artist Info Folder path to browser sources
    if (item.GetMusicInfoTag()->GetType() == MediaTypeArtist)
    {
      artistFolder = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
      if (!artistFolder.empty() && artistFolder.compare(itemDir) == 0)
        itemDir.clear();  // skip *item when artist not have a unique path
    }
  }
  // Add "*Item folder" path to file browser sources
  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = g_localizeStrings.Get(36041);
    itemSource.strPath = itemDir;
    sources.push_back(itemSource);
  }

  // For artist add Artist Info Folder path to browser sources
  if (!artistFolder.empty() && CDirectory::Exists(artistFolder))
  {
    CMediaSource itemSource;
    itemSource.strName = "* " + g_localizeStrings.Get(20223);
    itemSource.strPath = artistFolder;
    sources.push_back(itemSource);
  }
}

void CGUIDialogMusicInfo::SetArtTypeList(CFileItemList& artlist)
{
  m_artTypeList->Clear();
  m_artTypeList->Copy(artlist);
}

/*
Allow user to choose artwork for the artist or album
For each type of art the options are:
1.  Current art
2.  Local art (thumb found by filename)
3.  Remote art (scraped list of urls from online sources e.g. fanart.tv)
5.  Embedded art (@todo)
6.  None
*/
void CGUIDialogMusicInfo::OnGetArt()
{
  std::string type = MUSIC_UTILS::ShowSelectArtTypeDialog(*m_artTypeList);
  if (type.empty())
    return; // Cancelled

  CFileItemList items;
  CGUIListItem::ArtMap primeArt = m_item->GetArt(); // art without fallbacks
  bool bHasArt = m_item->HasArt(type);
  bool bFallback(false);
  if (bHasArt)
  {
    // Check if that type of art is actually a fallback, e.g. artist fanart
    CGUIListItem::ArtMap::const_iterator i = primeArt.find(type);
    bFallback = (i == primeArt.end());
  }

  // Build list of possible images of that art type
  if (bHasArt)
  {
    // Add item for current artwork
    // For album it could be a fallback from artist
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", m_item->GetArt(type));
    item->SetArt("icon", "DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(13512));
    items.Add(item);
  }

 // Grab the thumbnails of this art type scraped from the web
  std::vector<std::string> remotethumbs;
  // Art type is encoded into the scraper XML as optional "aspect=" field
  // Type "thumb" returns URLs for all types of art including those without aspect.
  // Those URL without aspect are also returned for all other type values.
  if (m_bArtistInfo)
    m_artist.thumbURL.GetThumbUrls(remotethumbs, type);
  else
    m_album.thumbURL.GetThumbUrls(remotethumbs, type);

  for (unsigned int i = 0; i < remotethumbs.size(); ++i)
  {
    std::string strItemPath;
    strItemPath = StringUtils::Format("thumb://Remote{}", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetArt("thumb", remotethumbs[i]);
    item->SetArt("icon", "DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(13513));

    items.Add(item);
  }

  // Local art
  std::string localArt;
  std::vector<std::string> paths;
  if (m_bArtistInfo)
  {
    // Individual artist subfolder within the Artist Information Folder
    paths.emplace_back(m_artist.strPath);
    // Fallback local to music files (when there is a unique folder)
    paths.emplace_back(m_fallbackartpath);
  }
  else
    // Album folder, when a unique one exists, no fallback
    paths.emplace_back(m_album.strPath);
  for (const auto& path : paths)
  {
    if (!localArt.empty() && CFileUtils::Exists(localArt))
      break;
    if (!path.empty())
    {
      CFileItem item(path, true);
      if (type == "thumb")
        // Local music thumbnail images named by <musicthumbs>
        localArt = item.GetUserMusicThumb(true);
      else
      { // Check case and ext insenitively for local images with type as name
        // e.g. <arttype>.jpg
        CFileItemList items;
        CDirectory::GetDirectory(path, items,
            CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
            DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);

        for (int j = 0; j < items.Size(); j++)
        {
          std::string strCandidate = URIUtils::GetFileName(items[j]->GetPath());
          URIUtils::RemoveExtension(strCandidate);
          if (StringUtils::EqualsNoCase(strCandidate, type))
          {
            localArt = items[j]->GetPath();
            break;
          }
        }
      }
    }
  }
  if (!localArt.empty() && CFileUtils::Exists(localArt))
  {
    CFileItemPtr item(new CFileItem("Local Art: " + localArt, false));
    item->SetArt("thumb", localArt);
    item->SetLabel(g_localizeStrings.Get(13514)); // "Local art"
    items.Add(item);
  }

  // No art
  if (bHasArt && !bFallback)
  { // Actually has this type of art (not a fallback) so
    // allow the user to delete it by selecting "no art".
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetArt("icon", "DefaultArtist.png");
    else
      item->SetArt("icon", "DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(13515));
    items.Add(item);
  }

  //! @todo: Add support for extracting embedded art from song files to use for album

  // Clear local images of this type from cache so user will see any recent
  // local file changes immediately
  for (auto& item : items)
  {
    // Skip images from remote sources, recache done by refresh (could be slow)
    if (StringUtils::StartsWith(item->GetPath(), "thumb://Remote"))
      continue;
    std::string thumb(item->GetArt("thumb"));
    if (thumb.empty())
      continue;
    CURL url(CTextureUtils::UnwrapImageURL(thumb));
    // Skip images from remote sources (current thumb could be remote)
    if (url.IsProtocol("http") || url.IsProtocol("https"))
      continue;
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
    // Remove any thumbnail of local image too (created when browsing files)
    std::string thumbthumb(CTextureUtils::GetWrappedThumbURL(thumb));
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumbthumb);
  }

  // Show list of possible art for user selection
  // Note that during selection thumbs of *all* images shown are cached. When
  // browsing folders there could be many irrelevant thumbs cached that are
  // never used by Kodi again, but there is no obvious way to clear these
  // thumbs from the cache automatically.
  std::string result;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(sources, *m_item);
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  if (CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(13511), result) &&
    result != "thumb://Current")
  {
    // User didn't choose the one they have.
    // Overwrite with the new art or clear it
    std::string newArt;
    if (StringUtils::StartsWith(result, "thumb://Remote"))
    {
      int number = atoi(result.substr(14).c_str());
      newArt = remotethumbs[number];
    }
    else if (result == "thumb://Thumb")
      newArt = m_item->GetArt("thumb");
    else if (StringUtils::StartsWith(result, "Local Art: "))
      newArt = localArt;
    else if (CFileUtils::Exists(result))
      newArt = result;
    else // none
      newArt.clear();

    // Asynchronously update that type of art in the database and then
    // refresh artist, album and fallback art of currently playing song
    MUSIC_UTILS::UpdateArtJob(m_item, type, newArt);

    // Update local item and art list with current art
    m_item->SetArt(type, newArt);
    for (const auto& artitem : *m_artTypeList)
    {
      if (artitem->GetProperty("artType") == type)
      {
        artitem->SetArt("thumb", newArt);
        break;
      }
    }

    // Get new artwork to show in other places e.g. on music lib window, but this does
    // not update artist, album or fallback art for the currently playing song as it
    // is a different item with different ID and media type
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_item);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

  // Re-open the art type selection dialog as we come back from
  // the image selection dialog
  OnGetArt();
}

void CGUIDialogMusicInfo::OnSetUserrating() const
{
  int userrating = MUSIC_UTILS::ShowSelectRatingDialog(m_item->GetMusicInfoTag()->GetUserrating());
  if (userrating < 0) // Nothing selected, so rating unchanged
    return;

  SetUserrating(userrating);
}


void CGUIDialogMusicInfo::ShowForAlbum(int idAlbum)
{
  std::string path = StringUtils::Format("musicdb://albums/{}", idAlbum);
  CFileItem item(path, true); // An album, but IsAlbum() not set as didn't use SetAlbum()
  ShowFor(&item);
}

void CGUIDialogMusicInfo::ShowForArtist(int idArtist)
{
  std::string path = StringUtils::Format("musicdb://artists/{}", idArtist);
  CFileItem item(path, true);
  ShowFor(&item);
}

void CGUIDialogMusicInfo::ShowFor(CFileItem* pItem)
{
  if (pItem->IsParentFolder() || URIUtils::IsSpecial(pItem->GetPath()) ||
    StringUtils::StartsWithNoCase(pItem->GetPath(), "musicsearch://"))
    return; // nothing to do

  if (!pItem->m_bIsFolder)
  { // Show Song information dialog
    CGUIDialogSongInfo::ShowFor(pItem);
    return;
  }

  CFileItem musicitem("musicdb://", true);

  // We have a folder album/artist info dialog only shown for db items
  // or for music video with artist/album in music library
  if (pItem->IsMusicDb())
  {
    if (!pItem->HasMusicInfoTag() || pItem->GetMusicInfoTag()->GetDatabaseId() < 1)
    {
      // Maybe only path is set, then set MusicInfoTag
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(pItem->GetPath(), params);
      if (params.GetArtistId() > 0)
        pItem->GetMusicInfoTag()->SetDatabaseId(params.GetArtistId(), MediaTypeArtist);
      else if (params.GetAlbumId() > 0)
        pItem->GetMusicInfoTag()->SetDatabaseId(params.GetAlbumId(), MediaTypeAlbum);
      else
        return; // nothing to do
    }
    musicitem.SetFromMusicInfoTag(*pItem->GetMusicInfoTag());
  }
  else if (pItem->HasProperty("artist_musicid"))
  {
    musicitem.GetMusicInfoTag()->SetDatabaseId(pItem->GetProperty("artist_musicid").asInteger32(),
                                               MediaTypeArtist);
  }
  else if (pItem->HasProperty("album_musicid"))
  {
    musicitem.GetMusicInfoTag()->SetDatabaseId(pItem->GetProperty("album_musicid").asInteger32(),
                                               MediaTypeAlbum);
  }
  else
    return; // nothing to do


  CGUIDialogMusicInfo *pDlgMusicInfo = CServiceBroker::GetGUI()->GetWindowManager().
	  GetWindow<CGUIDialogMusicInfo>(WINDOW_DIALOG_MUSIC_INFO);
    if (pDlgMusicInfo)
    {
      if (pDlgMusicInfo->SetItem(&musicitem))
      {
        pDlgMusicInfo->Open();
        if (pItem->GetMusicInfoTag()->GetType() == MediaTypeAlbum &&
          pDlgMusicInfo->HasUpdatedUserrating())
        {
          auto window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowMusicBase>(WINDOW_MUSIC_NAV);
          if (window)
            window->RefreshContent("albums");
        }
      }
    }
}

void CGUIDialogMusicInfo::OnPlayItem(const std::shared_ptr<CFileItem>& item)
{
  Close(true);
  MUSIC_UTILS::PlayItem(item, "");
}
