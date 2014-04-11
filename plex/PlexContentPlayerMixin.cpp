#include "PlexContentPlayerMixin.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIWindowManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "guilib/LocalizeStrings.h"
#include "PlayListPlayer.h"
#include "FileSystem/PlexDirectory.h"
#include "StringUtils.h"
#include "PlexTypes.h"
#include "dialogs/GUIDialogOK.h"
#include "Client/PlexTranscoderClient.h"
#include "ApplicationMessenger.h"
#include "music/tags/MusicInfoTag.h"
#include "Client/PlexServerVersion.h"
#include "Client/PlexServerManager.h"
#include "Client/PlexServer.h"
#include "PlexApplication.h"
#include "Playlists/PlayQueueManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexContentPlayerMixin::PlayFileFromContainer(const CGUIControl* control)
{
  if (control == 0)
    return;

  // Let's see if we're asked to play something.
  if (control->IsContainer())
  {
    CGUIBaseContainer* container = (CGUIBaseContainer*)control;
    CGUIListItemPtr item = container->GetListItem(0);

    if (item)
      PlayPlexItem(boost::static_pointer_cast<CFileItem>(item), container);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr PlexContentPlayerMixin::GetNextUnwatched(const std::string& container)
{
  CFileItemList list;
  XFILE::CPlexDirectory dir;
  if (dir.GetDirectory(container, list))
  {
    for (int i = 0; i < list.Size(); i++)
    {
      CFileItemPtr n = list.Get(i);
      if (n->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_IN_PROGRESS ||
          n->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED)
        return n;
    }

    // if we didn't find any inprogress or unwatched just ... eh take the first one?
    return list.Get(0);
  }
  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexContentPlayerMixin::PlayLocalPlaylist(const CFileItemPtr& file)
{
  CFileItemList fileItems;
  int itemIndex = 0;
  EPlexDirectoryType type = file->GetPlexDirectoryType();

  // Get the most interesting container, for a track this means
  // the album, for a album it means... the album :) and for
  // a artist we just play everything by that artist.
  CStdString key;
  if (type == PLEX_DIR_TYPE_TRACK)
  {
    if (file->HasProperty("parentPath"))
      key = file->GetProperty("parentPath").asString();
    else if (file->HasProperty("parentKey"))
      key = file->GetProperty("parentKey").asString();
  }
  else if (type == PLEX_DIR_TYPE_ALBUM)
  {
    key = file->GetProperty("key").asString();
  }
  else if (type == PLEX_DIR_TYPE_ARTIST)
  {
    CURL p(file->GetPath());
    PlexUtils::AppendPathToURL(p, "allLeaves");
    key = p.Get();
  }

  XFILE::CPlexDirectory plexDir;
  plexDir.GetDirectory(key, fileItems);

  for (int i = 0; i < fileItems.Size(); ++i)
  {
    CFileItemPtr fileItem = fileItems[i];
    if (fileItem->GetProperty("unprocessed_key") == file->GetProperty("unprocessed_key"))
    {
      itemIndex = i;
      break;
    }
  }

  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  CApplicationMessenger::Get().PlayListPlayerClear(PLAYLIST_MUSIC);
  CApplicationMessenger::Get().PlayListPlayerAdd(PLAYLIST_MUSIC, fileItems);
  if (file->HasMusicInfoTag())
    CApplicationMessenger::Get().PlayListPlayerPlaySongId(file->GetMusicInfoTag()->GetDatabaseId());
  else
    CApplicationMessenger::Get().PlayListPlayerPlay();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexContentPlayerMixin::PlayMusicPlaylist(const CFileItemPtr& file)
{
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(file);
  if (server)
  {
    CPlexServerVersion version(server->GetVersion());
    if (!server->IsSecondary() && (version.isValid && version > CPlexServerVersion("0.9.9.6.0")))
      g_plexApplication.playQueueManager->createPlayQueueFromItem(server, file);
    else
      PlayLocalPlaylist(file);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexContentPlayerMixin::PlayPlexItem(const CFileItemPtr file, CGUIBaseContainer* container)
{
  /* something went wrong ... */
  if (file->HasProperty("unavailable") && file->GetProperty("unavailable").asBoolean())
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52010), "",
                                  "");
    return;
  }

  /* webkit can't be played by PHT */
  if (file->HasProperty("protocol") && file->GetProperty("protocol").asString() == "webkit")
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52011), "",
                                  "");
    return;
  }

  /* and we defintely not playing isos. */
  if (file->HasProperty("isdvd") && file->GetProperty("isdvd").asBoolean())
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52012), "",
                                  "");
    return;
  }

  // Now see what to do with it.
  EPlexDirectoryType type = file->GetPlexDirectoryType();
  if (type == PLEX_DIR_TYPE_TRACK || type == PLEX_DIR_TYPE_ARTIST || type == PLEX_DIR_TYPE_ALBUM)
    PlayMusicPlaylist(file);
  else if (type == PLEX_DIR_TYPE_PHOTO)
  {
    // Attempt to get the slideshow window
    CGUIWindowSlideShow* pSlideShow =
    (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return;

    // Stop playing video
    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    // Reset the window and add each item from the container
    pSlideShow->Reset();
    if (container)
    {
      BOOST_FOREACH(CGUIListItemPtr child, container->GetItems())
      pSlideShow->Add((CFileItem*)child.get());
    }

    // Set the currently selected photo
    pSlideShow->Select(file->GetPath());

    // Start the slideshow and show the window
    pSlideShow->StartSlideShow();
    g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
  else
  {
    CFileItemPtr rFile = file;

    g_playlistPlayer.Clear();

    if (type == PLEX_DIR_TYPE_SHOW)
    {
      CFileItemPtr season = GetNextUnwatched(file->GetPath());
      if (season)
        rFile = GetNextUnwatched(season->GetPath());
    }
    else if (type == PLEX_DIR_TYPE_SEASON)
    {
      rFile = GetNextUnwatched(file->GetPath());
    }

    // If there is more than one media item, allow picking which one.
    if (ProcessMediaChoice(rFile.get()) == false)
      return;

    // See if we're going to resume the playback or not.
    if (ProcessResumeChoice(rFile.get()) == false)
      return;

    // Play it.
    g_application.PlayFile(*rFile);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexContentPlayerMixin::ProcessResumeChoice(CFileItem* file)
{
  bool resumeItem = false;

  if (!file->m_bIsFolder && file->HasProperty("viewOffset") &&
      !file->GetProperty("viewOffset").asString().empty())
  {
    // Oh my god. Copy and paste code. We need a superclass which manages media.
    float seconds =
    boost::lexical_cast<float>(file->GetProperty("viewOffset").asString()) / 1000.0f;

    CContextButtons choices;
    CStdString resumeString;
    CStdString time = StringUtils::SecondsToTimeString(long(seconds));
    resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
    choices.Add(1, resumeString);
    choices.Add(2, g_localizeStrings.Get(12021));
    int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (retVal == -1)
      return false;

    resumeItem = (retVal == 1);
  }

  if (resumeItem)
    file->m_lStartOffset = STARTOFFSET_RESUME;
  else
    file->m_lStartOffset = 0;

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexContentPlayerMixin::ProcessMediaChoice(CFileItem* file)
{
  // If there is more than one media item, allow picking which one.
  if (file->m_mediaItems.size() > 1)
  {
    int onlineQuality = g_guiSettings.GetInt("plexmediaserver.onlinemediaquality");
    bool isLibraryItem = file->IsPlexMediaServerLibrary();

    // See if we're offering a choice.
    if (isLibraryItem || (!isLibraryItem && onlineQuality == PLEX_ONLINE_QUALITY_ALWAYS_ASK))
    {
      CContextButtons choices;

      for (size_t i = 0; i < file->m_mediaItems.size(); i++)
      {
        CFileItemPtr item = file->m_mediaItems[i];
        int mpartID = item->GetProperty("id").asInteger();
        if (mpartID == 0)
          mpartID = i;

        CStdString label;
        CStdString videoCodec =
        CStdString(item->GetProperty("mediaTag-videoCodec").asString()).ToUpper();
        CStdString videoRes =
        CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();

        CStdString audioCodec =
        CStdString(item->GetProperty("mediaTag-audioCodec").asString()).ToUpper();
        if (audioCodec.Equals("DCA"))
          audioCodec = "DTS";

        CStdString channelStr;
        int audioChannels = item->GetProperty("mediaTag-audioChannels").asInteger();

        if (audioChannels == 1)
          channelStr = "Mono";
        else if (audioChannels == 2)
          channelStr = "Stereo";
        else
          channelStr = boost::lexical_cast<std::string>(audioChannels - 1) + ".1";

        if (videoCodec.size() == 0 && videoRes.size() == 0)
        {
          label = "Unknown";
        }
        else
        {
          if (isdigit(videoRes[0]))
            videoRes += "p";

          label += videoRes;
          label += " " + videoCodec;
        }

        label += " - ";

        if (audioCodec.empty())
          label += "Unknown";
        else
          label += channelStr + " " + audioCodec;

        choices.Add(mpartID, label);
      }

      int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (choice >= 0)
        file->SetProperty("selectedMediaItem", choice);
      else
        return false;
    }
    else
    {
      if (isLibraryItem == false)
      {
        // Try to pick something that's equal or less than the preferred resolution.
        std::map<int, int> qualityMap;
        std::vector<int> qualities;
        int sd = PLEX_ONLINE_QUALITY_SD;

        for (size_t i = 0; i < file->m_mediaItems.size(); i++)
        {
          CFileItemPtr item = file->m_mediaItems[i];
          CStdString videoRes =
          CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();

          // Compute the quality, subsequent SDs get lesser values, assuming they're ordered
          // descending.
          int q = sd;
          if (videoRes != "SD" && videoRes.empty() == false)
            q = boost::lexical_cast<int>(videoRes);
          else
            sd -= 10;

          qualityMap[q] = i;
          qualities.push_back(q);
        }

        // Sort on quality descending.
        std::sort(qualities.begin(), qualities.end());
        std::reverse(qualities.begin(), qualities.end());

        int pickedIndex = qualities[qualities.size() - 1];
        BOOST_FOREACH(int q, qualities)
        {
          if (q <= onlineQuality)
          {
            pickedIndex = qualityMap[q];
            file->SetProperty("selectedMediaItem",
                              file->m_mediaItems[pickedIndex]->GetProperty("id").asInteger());
            break;
          }
        }
      }
    }
  }
  else
    file->SetProperty("selectedMediaItem", 0);

  return true;
}
