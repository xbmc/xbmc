/*
 *      Copyright (C) 2005-2018 Team Kodi
 *      http://kodi.tv
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

#include "GUIDialogMusicInfo.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "input/Key.h"
#include "music/MusicDatabase.h"
#include "music/MusicThumbLoader.h"
#include "music/MusicUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "profiles/ProfilesManager.h"
#include "ServiceBroker.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "settings/MediaSourceSettings.h"
#include "TextureCache.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace MUSIC_INFO;

#define CONTROL_BTN_REFRESH      6
#define CONTROL_USERRATING       7
#define CONTROL_BTN_GET_THUMB   10

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
    // May only have partially populated item, so fetch all artist or album data from db
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
          artistItemPath = CServiceBroker::GetSettings().GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
      }        
      m_item->SetPath(artistItemPath);
      
      // Store info as CArtist as well as item properties
      dialog->SetArtist(artist, oldartistpath);

      // Fetch artist discography as scraped from online sources
      //! @todo: make links to library albums more efficient, always include those albums
      dialog->SetDiscography();
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

CGUIDialogMusicInfo::CGUIDialogMusicInfo(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_INFO, "DialogMusicInfo.xml")
    , m_item(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
  m_startUserrating = -1;
  m_hasUpdatedUserrating = false;
  m_bViewReview = false;
  m_bArtistInfo = false;
  m_cancelled = false;
  m_artTypeList.Clear();
}

CGUIDialogMusicInfo::~CGUIDialogMusicInfo(void)
{
  delete m_albumSongs;
}

bool CGUIDialogMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_artTypeList.Clear();
      // For albums update user rating if it has changed
      if(!m_bArtistInfo && m_startUserrating != m_item->GetMusicInfoTag()->GetUserrating())
      {
        m_hasUpdatedUserrating = true;

        // Asynchronously update song userrating in library
        CSetUserratingJob *job = new CSetUserratingJob(m_item->GetMusicInfoTag()->GetAlbumId(), 
                                                       m_item->GetMusicInfoTag()->GetUserrating());
        CJobManager::GetInstance().AddJob(job, NULL);

        // Send a message to all windows to tell them to update the album fileitem
        // This communicates the rating change to the music lib window.
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
      m_bViewReview = true;
      m_bRefresh = false;
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
        m_bRefresh = true;
        Close();
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
        if (m_bArtistInfo && (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction))
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= static_cast<int>(m_albumSongs->Size()))
            break;
          CFileItemPtr item = m_albumSongs->Get(iItem);
          OnSearch(item.get());
          return true;
        }
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
  int jobid = CJobManager::GetInstance().AddJob(new CGetInfoJob(), nullptr, CJob::PRIORITY_LOW);

  // Wait to get all data before show, allowing user to to cancel if fetch is slow
  if (!CGUIDialogBusy::WaitOnEvent(m_event, TIME_TO_BUSY_DIALOG))
  {
    // Cancel job still waiting in queue (unlikely)
    CJobManager::GetInstance().CancelJob(jobid);
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
  m_bArtistInfo = false;
  m_hasUpdatedUserrating = false;

  // CurrentDirectory() returns m_albumSongs (a convenient CFileItemList)
  // Set content so can return dialog CONTAINER_CONTENT as "albums"
  m_albumSongs->SetContent("albums");
  // Copy art from ListItem so CONTAINER_ART returns album art
  m_albumSongs->SetArt(m_item->GetArt());
}

void CGUIDialogMusicInfo::SetArtist(const CArtist& artist, const std::string &path)
{
  m_artist = artist;
  m_fallbackartpath = path;
  m_bArtistInfo = true;

  // CurrentDirectory() returns m_albumSongs (a convenient CFileItemList)
  // Set content so can return dialog CONTAINER_CONTENT as "artists"
  m_albumSongs->SetContent("artists"); 
  // Copy art from ListItem so CONTAINER_ART returns artist art
  m_albumSongs->SetArt(m_item->GetArt());
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

void CGUIDialogMusicInfo::SetDiscography() const
{
  m_albumSongs->Clear();
  CMusicDatabase database;
  database.Open();

  std::vector<int> albumsByArtist;
  database.GetAlbumsByArtist(m_artist.idArtist, albumsByArtist);

  // Sort the discography by year
  auto discography = m_artist.discography;
  std::sort(discography.begin(), discography.end(), [](const std::pair<std::string, std::string> &left, const std::pair<std::string, std::string> &right) {
    return left.second < right.second;
  });

  for (unsigned int i=0; i < discography.size(); ++i)
  {
    CFileItemPtr item(new CFileItem(discography[i].first));
    item->SetLabel2(discography[i].second);

    CMusicThumbLoader loader;
    int idAlbum = -1;
    for (std::vector<int>::const_iterator album = albumsByArtist.begin(); album != albumsByArtist.end(); ++album)
    {
      if (StringUtils::EqualsNoCase(database.GetAlbumById(*album), item->GetLabel()))
      {
        idAlbum = *album;
        item->GetMusicInfoTag()->SetDatabaseId(idAlbum, "album");
        // Load all the album art and related artist(s) art (could be other collaborating artists)
        loader.LoadItem(item.get());
        break;
      }
    }
    if (idAlbum == -1) 
      item->SetArt("thumb", "DefaultAlbumCover.png");

    m_albumSongs->Add(item);
  }
}

void CGUIDialogMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);

    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);

  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_USERRATING);

    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);

  }

  const CProfilesManager &profileManager = CServiceBroker::GetProfileManager();

  // Disable the Choose Art button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, 
    profileManager.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
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

  if (m_bArtistInfo)
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogMusicInfo::FetchComplete()
{
  //Trigger the event to indicate data has been fetched
  m_event.Set();
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

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

//! @todo Currently no support for "embedded thumb" as there is no easy way to grab it
//!       without sending a file that has this as it's album to this class
void CGUIDialogMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_item->GetArt("thumb")))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", m_item->GetArt("thumb"));
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  std::vector<std::string> thumbs;
  if (m_bArtistInfo)
    m_artist.thumbURL.GetThumbURLs(thumbs);
  else
    m_album.thumbURL.GetThumbURLs(thumbs);

  for (unsigned int i = 0; i < thumbs.size(); ++i)
  {
    std::string strItemPath;
    strItemPath = StringUtils::Format("thumb://Remote%i", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetArt("thumb", thumbs[i]);
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20015));
    
    //! @todo Do we need to clear the cached image?
    //    CTextureCache::GetInstance().ClearCachedImage(thumb);
    items.Add(item);
  }

  // local thumb
  std::string localThumb;
  bool existsThumb = false;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    // First look for thumb in the artists folder, the primary location
    std::string strArtistPath = m_artist.strPath;
    // Get path when don't already have it.
    bool artistpathfound = !strArtistPath.empty();
    if (!artistpathfound)
      artistpathfound = database.GetArtistPath(m_artist, strArtistPath);
    if (artistpathfound)
    {
      localThumb = URIUtils::AddFileToFolder(strArtistPath, "folder.jpg");
      existsThumb = CFile::Exists(localThumb);
    }
    // If not there fall back local to music files (historic location for those album artists with a unique folder)
    if (!existsThumb)
    {
      artistpathfound = database.GetOldArtistPath(m_artist.idArtist, strArtistPath);
      if (artistpathfound)
      {
        localThumb = URIUtils::AddFileToFolder(strArtistPath, "folder.jpg");
        existsThumb = CFile::Exists(localThumb);
      }
    }
  }
  else
  {
    localThumb = m_item->GetUserMusicThumb();
    existsThumb = CFile::Exists(localThumb);
  }
  if (existsThumb)
  {
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetArt("thumb", localThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  {
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetIconImage("DefaultArtist.png");
    else
      item->SetIconImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  std::string result;
  bool flip=false;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  AddItemPathToFileBrowserSources(sources, *m_item);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), result, &flip))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  std::string newThumb;
  if (StringUtils::StartsWith(result, "thumb://Remote"))
  {
    int number = atoi(result.substr(14).c_str());
    newThumb = thumbs[number];
  }
  else if (result == "thumb://Local")
    newThumb = localThumb;
  else if (CFile::Exists(result))
    newThumb = result;

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_item->GetMusicInfoTag()->GetDatabaseId(), m_item->GetMusicInfoTag()->GetType(), "thumb", newThumb);
    db.Close();
  }

  m_item->SetArt("thumb", newThumb);


  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  // Update our screen
  Update();
}


// Allow user to select a Fanart
void CGUIDialogMusicInfo::OnGetFanart()
{
  CFileItemList items;

  if (m_item->HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetArt("thumb", m_item->GetArt("fanart"));
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    std::string strItemPath = StringUtils::Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    std::string thumb = m_artist.fanart.GetPreviewURL(i);
    item->SetArt("thumb", CTextureUtils::GetWrappedThumbURL(thumb));
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    //! @todo Do we need to clear the cached image?
    //    CTextureCache::GetInstance().ClearCachedImage(thumb);
    items.Add(item);
  }

  // Grab a local fanart 
  std::string strLocal;
  CMusicDatabase database;
  database.Open();
  // First look for fanart in the artists folder, the primary location
  std::string strArtistPath = m_artist.strPath;
  // Get path when don't already have it.
  bool artistpathfound = !strArtistPath.empty();
  if (!artistpathfound)
    artistpathfound = database.GetArtistPath(m_artist, strArtistPath);
  if (artistpathfound)
  {
    CFileItem item(strArtistPath, true);
    strLocal = item.GetLocalFanart();
  }
  // If not there fall back local to music files (historic location for those album artists with a unique folder)
  if (strLocal.empty())
  {
    artistpathfound = database.GetOldArtistPath(m_artist.idArtist, strArtistPath);
    if (artistpathfound)
    {
      CFileItem item(strArtistPath, true);
      strLocal = item.GetLocalFanart();
    }
  }

  if (!strLocal.empty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetArt("thumb", strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));

    //! @todo Do we need to clear the cached image?
    CTextureCache::GetInstance().ClearCachedImage(strLocal);
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultArtist.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  std::string result;
  bool flip = false;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  AddItemPathToFileBrowserSources(sources, *m_item);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (StringUtils::EqualsNoCase(result, "fanart://Current"))
   return;

  if (StringUtils::EqualsNoCase(result, "fanart://Local"))
    result = strLocal;

  if (StringUtils::StartsWith(result, "fanart://Remote"))
  {
    int iFanart = atoi(result.substr(15).c_str());
    m_artist.fanart.SetPrimaryFanart(iFanart);
    result = m_artist.fanart.GetImageURL();
  }
  else if (StringUtils::EqualsNoCase(result, "fanart://None") || !CFile::Exists(result))
    result.clear();

  if (flip && !result.empty())
    result = CTextureUtils::GetWrappedImageURL(result, "", "flipped");

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_item->GetMusicInfoTag()->GetDatabaseId(), m_item->GetMusicInfoTag()->GetType(), "fanart", result);
    db.Close();
  }

  m_item->SetArt("fanart", result);

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  // Update our screen
  Update();
}

void CGUIDialogMusicInfo::OnSearch(const CFileItem* pItem)
{
  CMusicDatabase database;
  database.Open();
  if (pItem->HasMusicInfoTag() &&
      pItem->GetMusicInfoTag()->GetDatabaseId() > 0)
  {
    CAlbum album;
    if (database.GetAlbum(pItem->GetMusicInfoTag()->GetDatabaseId(), album))
    {
      std::string strPath;
      database.GetAlbumPath(pItem->GetMusicInfoTag()->GetDatabaseId(), strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
}

CFileItemPtr CGUIDialogMusicInfo::GetCurrentListItem(int offset)
{
  return m_item;
}

void CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item)
{
  std::string itemDir;

  if (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == MediaTypeSong)
    itemDir = URIUtils::GetParentPath(item.GetMusicInfoTag()->GetURL());
  else
    itemDir = item.GetPath();

  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = g_localizeStrings.Get(36041);
    itemSource.strPath = itemDir;
    sources.push_back(itemSource);
  }
}

void CGUIDialogMusicInfo::SetArtTypeList(CFileItemList& artlist)
{
  m_artTypeList.Copy(artlist);
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
  std::string type = MUSIC_UTILS::ShowSelectArtTypeDialog(m_artTypeList);
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
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(13512));
    items.Add(item);
  }
  else if (m_item->HasArt("thumb"))
  { 
    // For missing art of that type add the thumb (when it exists and not a fallback)
    CGUIListItem::ArtMap::const_iterator i = primeArt.find("thumb");
    if (i != primeArt.end())
    {
      CFileItemPtr item(new CFileItem("thumb://Thumb", false));
      item->SetArt("thumb", m_item->GetArt("thumb"));
      if (m_bArtistInfo)
        item->SetIconImage("DefaultArtistCover.png");
      else
        item->SetIconImage("DefaultAlbumCover.png");
      item->SetLabel(g_localizeStrings.Get(21371));
      items.Add(item);
    }
  }

 // Grab the thumbnails of this art type scraped from the web
  std::vector<std::string> remotethumbs;
  if (type == "fanart" && m_bArtistInfo)
  {
    // Scraped artist fanart URLs are held separately from other art types
    //! @todo Change once scraping all art types is unified
    for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
    {
      std::string strItemPath;
      strItemPath = StringUtils::Format("fanart://Remote%i", i);
      CFileItemPtr item(new CFileItem(strItemPath, false));
      std::string thumb = m_artist.fanart.GetPreviewURL(i);
      std::string wrappedthumb = CTextureUtils::GetWrappedThumbURL(thumb);
      remotethumbs.push_back(wrappedthumb); // Store for lookup after selection
      item->SetArt("thumb", wrappedthumb);
      item->SetIconImage("DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(20441));

      items.Add(item);
    }
  }
  else
  {
    if (m_bArtistInfo)
      m_artist.thumbURL.GetThumbURLs(remotethumbs, type);
    else
      m_album.thumbURL.GetThumbURLs(remotethumbs, type);

    for (unsigned int i = 0; i < remotethumbs.size(); ++i)
    {
      std::string strItemPath;
      strItemPath = StringUtils::Format("thumb://Remote%i", i);
      CFileItemPtr item(new CFileItem(strItemPath, false));
      item->SetArt("thumb", remotethumbs[i]);
      item->SetIconImage("DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13513));

      items.Add(item);
    }
  }

  // Local art
  // Album and artist thumbs can be found in local files
  //! @todo: additional art types could also be found locally e.g "poster.jpg"
  std::string localThumb;
  bool existsThumb = false;
  if (type == "thumb")
  { 
    if (m_bArtistInfo)
    {
      // First look for thumb in the artists folder, the primary location     
      if (!m_artist.strPath.empty())
      {
        localThumb = URIUtils::AddFileToFolder(m_artist.strPath, "folder.jpg");
        existsThumb = CFile::Exists(localThumb);
      }
      // If not there fall back local to music files when there is a unique artist folder
      if (!existsThumb && !m_fallbackartpath.empty())
      {
        localThumb = URIUtils::AddFileToFolder(m_fallbackartpath, "folder.jpg");
        existsThumb = CFile::Exists(localThumb);
      }
    }
    else
    {
      localThumb = m_item->GetUserMusicThumb(true);
      if (m_item->IsMusicDb())
      {
        CFileItem item(m_item->GetMusicInfoTag()->GetURL(), false);
        localThumb = item.GetUserMusicThumb(true);
      }
    }

    if (CFile::Exists(localThumb))
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetArt("thumb", localThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }
  //! @todo Artist fanart can also be in local file

  if (bHasArt && !bFallback)
  { // Actually has this type of art (not a fallback) so 
    // allow the user to delete it by selecting "no art".
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetIconImage("DefaultArtist.png");
    else
      item->SetIconImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(13515));
    items.Add(item);
  }

  //! @todo: Add support for extracting embedded art from song files to use for album

  // Show list of possible art for user selection
  // Note that during selection thumbs of *all* images shown are cached. When 
  // browsing folders there could be many irrelevant thumbs cached that are
  // never used by Kodi again, but there is no obvious way to clear these 
  // thumbs from the cache automatically.
  std::string result;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(sources, *m_item);
  g_mediaManager.GetLocalDrives(sources);
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
    else if (StringUtils::StartsWith(result, "fanart://Remote"))
    {
      int number = atoi(result.substr(15).c_str());
      newArt = remotethumbs[number];
    }
    else if (result == "thumb://Thumb")
      newArt = m_item->GetArt("thumb");
    else if (result == "thumb://Local")
      newArt = localThumb;
    else if (CFile::Exists(result))
      newArt = result;
    else // none
      newArt.clear();

    // Asynchronously update that type of art in the database and then
    // refresh artist, album and fallback art of currently playing song
    MUSIC_UTILS::UpdateArtJob(m_item, type, newArt);

    // Update local item and art list with current art
    m_item->SetArt(type, newArt);
    for (const auto artitem : m_artTypeList)
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

void CGUIDialogMusicInfo::ShowFor(CFileItem item)
{
  auto window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowMusicNav>(WINDOW_MUSIC_NAV);
  if (window)
    window->OnItemInfo(&item);
}
