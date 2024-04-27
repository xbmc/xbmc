/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Autorun.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayList.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "video/VideoFileItemClassify.h"

#include <stdlib.h>
#ifndef TARGET_WINDOWS
#include "storage/DetectDVDType.h"
#endif
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

using namespace XFILE;
using namespace MEDIA_DETECT;
using namespace KODI;
using namespace KODI::MESSAGING;
using namespace KODI::VIDEO;
using namespace std::chrono_literals;

using KODI::MESSAGING::HELPERS::DialogResponse;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun() = default;

bool CAutorun::ExecuteAutorun(const std::string& path)
{
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return false;

  CCdInfo* pInfo = CServiceBroker::GetMediaManager().GetCdInfo(path);

  if ( pInfo == NULL )
    return false;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS(); // turn off the screensaver if it's active

  bool success = false;

#ifdef HAS_CDDA_RIPPER
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION) == AUTOCD_RIP &&
      pInfo->IsAudio(1) && !CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().musicLocked())
  {
    success = KODI::CDRIP::CCDDARipper::GetInstance().RipCD();
  }
  else
#endif

    success = PlayDisc(path, false, false);
  return success;
}

bool CAutorun::PlayDisc(const std::string& path, bool bypassSettings, bool startFromBeginning)
{
  if ( !bypassSettings && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION) != AUTOCD_PLAY && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_DVDS_AUTORUN))
    return false;

  int nSize = CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC).size();
  int nAddedToPlaylist = 0;

  std::string mediaPath;

  CCdInfo* pInfo = CServiceBroker::GetMediaManager().GetCdInfo(path);
  if (pInfo == NULL)
    return false;

  if (pInfo->IsAudio(1))
    mediaPath = "cdda://local/";

  if (mediaPath.empty() && (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)))
    mediaPath = "iso9660://";

  if (mediaPath.empty())
    mediaPath = path;

  if (mediaPath.empty() || mediaPath == "iso9660://")
    mediaPath = CServiceBroker::GetMediaManager().GetDiscPath();

  const CURL pathToUrl(mediaPath);
  std::unique_ptr<IDirectory> pDir ( CDirectoryFactory::Create( pathToUrl ));
  bool bPlaying = RunDisc(pDir.get(), mediaPath, nAddedToPlaylist, true, bypassSettings, startFromBeginning);

  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
    // Start playing the items we inserted
    return CServiceBroker::GetPlaylistPlayer().Play(nSize, "");
  }

  return bPlaying;
}

/**
 * This method tries to determine what type of disc is located in the given drive and starts to play the content appropriately.
 */
bool CAutorun::RunDisc(IDirectory* pDir, const std::string& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */, bool startFromBeginning /* = false */)
{
  if (!pDir)
  {
    CLog::Log(LOGDEBUG, "CAutorun::{}: cannot run disc. is it properly mounted?", __FUNCTION__);
    return false;
  }

  bool bPlaying(false);
  CFileItemList vecItems;

  CURL pathToUrl{strDrive};
  // if the url being requested is a generic "iso9660://" we need to enrich it with the actual drive.
  // use the hostname section to prepend the drive path
  if (pathToUrl.GetRedacted() == "iso9660://")
  {
    pathToUrl.Reset();
    pathToUrl.SetProtocol("iso9660");
    pathToUrl.SetHostName(CServiceBroker::GetMediaManager().TranslateDevicePath(""));
  }

  if ( !pDir->GetDirectory( pathToUrl, vecItems ) )
  {
    return false;
  }

  // Sorting necessary for easier HDDVD handling
  vecItems.Sort(SortByLabel, SortOrderAscending);

  bool bAllowVideo = true;
//  bool bAllowPictures = true;
  bool bAllowMusic = true;
  if (!g_passwordManager.IsMasterLockUnlocked(false))
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    bAllowVideo = !profileManager->GetCurrentProfile().videoLocked();
//    bAllowPictures = !profileManager->GetCurrentProfile().picturesLocked();
    bAllowMusic = !profileManager->GetCurrentProfile().musicLocked();
  }

  // is this a root folder we have to check the content to determine a disc type
  if (bRoot)
  {
    std::string hddvdname = "";
    CFileItemPtr phddvdItem;
    bool bAutorunDVDs = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_DVDS_AUTORUN);

    // check root folders next, for normal structured dvd's
    for (const auto& pItem : vecItems)
    {
      // is the current item a (non system) folder?
      if (pItem->m_bIsFolder && pItem->GetPath() != "." && pItem->GetPath() != "..")
      {
        std::string name = pItem->GetPath();
        URIUtils::RemoveSlashAtEnd(name);
        name = URIUtils::GetFileName(name);

        // Check if the current foldername indicates a DVD structure (name is "VIDEO_TS")
        if (StringUtils::EqualsNoCase(name, "VIDEO_TS") && bAllowVideo
        && (bypassSettings || bAutorunDVDs))
        {
          std::string path = URIUtils::AddFileToFolder(pItem->GetPath(), "VIDEO_TS.IFO");
          if (!CFileUtils::Exists(path))
            path = URIUtils::AddFileToFolder(pItem->GetPath(), "video_ts.ifo");
          CFileItemPtr item(new CFileItem(path, false));
          item->SetLabel(CServiceBroker::GetMediaManager().GetDiskLabel(strDrive));
          item->GetVideoInfoTag()->m_strFileNameAndPath =
              CServiceBroker::GetMediaManager().GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item->GetVideoInfoTag()->m_strFileNameAndPath.empty())
            item->SetStartOffset(STARTOFFSET_RESUME);

          CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::TYPE_VIDEO, false);
          CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_VIDEO, item);
          CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().Play(0, "");
          return true;
        }

        // Check if the current foldername indicates a Blu-Ray structure (default is "BDMV").
        // A BR should also include an "AACS" folder for encryption, Sony-BRs can also include update folders for PS3 (PS3_UPDATE / PS3_VPRM).
        //! @todo for the time being, the DVD autorun settings are used to determine if the BR should be started automatically.
        if (StringUtils::EqualsNoCase(name, "BDMV") && bAllowVideo
        && (bypassSettings || bAutorunDVDs))
        {
          CFileItemPtr item(new CFileItem(URIUtils::AddFileToFolder(pItem->GetPath(), "index.bdmv"), false));
          item->SetLabel(CServiceBroker::GetMediaManager().GetDiskLabel(strDrive));
          item->GetVideoInfoTag()->m_strFileNameAndPath =
              CServiceBroker::GetMediaManager().GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item->GetVideoInfoTag()->m_strFileNameAndPath.empty())
            item->SetStartOffset(STARTOFFSET_RESUME);

          CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::TYPE_VIDEO, false);
          CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_VIDEO, item);
          CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().Play(0, "");
          return true;
        }

        // Check if the current foldername indicates a HD DVD structure (default is "HVDVD_TS").
        // Most HD DVD will also include an "ADV_OBJ" folder for advanced content. This folder should be handled first.
        //! @todo for the time being, the DVD autorun settings are used to determine if the HD DVD should be started automatically.
        CFileItemList items, sitems;

        // Advanced Content HD DVD (most discs?)
        if (StringUtils::EqualsNoCase(name, "ADV_OBJ"))
        {
          CLog::Log(LOGINFO,"HD DVD: Checking for playlist.");
          // find playlist file
          CDirectory::GetDirectory(pItem->GetPath(), items, "*.xpl", DIR_FLAG_DEFAULTS);
          if (items.Size())
          {
            // HD DVD Standard says the highest numbered playlist has to be handled first.
            CLog::Log(LOGINFO,"HD DVD: Playlist found. Set filetypes to *.xpl for external player.");
            items.Sort(SortByLabel, SortOrderDescending);
            phddvdItem = pItem;
            hddvdname = URIUtils::GetFileName(items[0]->GetPath());
            CLog::Log(LOGINFO, "HD DVD: {}", items[0]->GetPath());
          }
        }

        // Standard Content HD DVD (few discs?)
        if (StringUtils::EqualsNoCase(name, "HVDVD_TS") && bAllowVideo
        && (bypassSettings || bAutorunDVDs))
        {
          if (hddvdname == "")
          {
            CLog::Log(LOGINFO,"HD DVD: Checking for ifo.");
            // find Video Manager or Title Set Information
            CDirectory::GetDirectory(pItem->GetPath(), items, "HV*.ifo", DIR_FLAG_DEFAULTS);
            if (items.Size())
            {
              // HD DVD Standard says the lowest numbered ifo has to be handled first.
              CLog::Log(LOGINFO,"HD DVD: IFO found. Set filename to HV* and filetypes to *.ifo for external player.");
              items.Sort(SortByLabel, SortOrderAscending);
              phddvdItem = pItem;
              hddvdname = URIUtils::GetFileName(items[0]->GetPath());
              CLog::Log(LOGINFO, "HD DVD: {}", items[0]->GetPath());
            }
          }
          // Find and sort *.evo files for internal playback.
          // While this algorithm works for all of my HD DVDs, it may fail on other discs. If there are very large extras which are
          // alphabetically before the main movie they will be sorted to the top of the playlist and get played first.
          CDirectory::GetDirectory(pItem->GetPath(), items, "*.evo", DIR_FLAG_DEFAULTS);
          if (items.Size())
          {
            // Sort *.evo files in alphabetical order.
            items.Sort(SortByLabel, SortOrderAscending);
            int64_t asize = 0;
            int ecount = 0;
            // calculate average size of elements above 1gb
            for (int j = 0; j < items.Size(); j++)
              if (items[j]->m_dwSize > 1000000000)
              {
                ecount++;
                asize = asize + items[j]->m_dwSize;
              }
            if (ecount > 0)
              asize = asize / ecount;
            // Put largest files in alphabetical order to top of new list.
            for (int j = 0; j < items.Size(); j++)
              if (items[j]->m_dwSize >= asize)
                sitems.Add (items[j]);
            // Sort *.evo files by size.
            items.Sort(SortBySize, SortOrderDescending);
            // Add other files with descending size to bottom of new list.
            for (int j = 0; j < items.Size(); j++)
              if (items[j]->m_dwSize < asize)
                sitems.Add (items[j]);
            // Replace list with optimized list.
            items.Clear();
            items.Copy (sitems);
            sitems.Clear();
          }
          if (hddvdname != "")
          {
            CFileItem item(URIUtils::AddFileToFolder(phddvdItem->GetPath(), hddvdname), false);
            item.SetLabel(CServiceBroker::GetMediaManager().GetDiskLabel(strDrive));
            item.GetVideoInfoTag()->m_strFileNameAndPath =
                CServiceBroker::GetMediaManager().GetDiskUniqueId(strDrive);

            if (!startFromBeginning && !item.GetVideoInfoTag()->m_strFileNameAndPath.empty())
              item.SetStartOffset(STARTOFFSET_RESUME);

            // get playername
            std::string hdVideoPlayer = CServiceBroker::GetPlayerCoreFactory().GetDefaultPlayer(item);

            // Single *.xpl or *.ifo files require an external player to handle playback.
            // If no matching rule was found, VideoPlayer will be default player.
            if (hdVideoPlayer != "VideoPlayer")
            {
              CLog::Log(LOGINFO, "HD DVD: External singlefile playback initiated: {}", hddvdname);
              g_application.PlayFile(item, hdVideoPlayer, false);
              return true;
            } else
              CLog::Log(LOGINFO,"HD DVD: No external player found. Fallback to internal one.");
          }

          //  internal *.evo playback.
          CLog::Log(LOGINFO,"HD DVD: Internal multifile playback initiated.");
          CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().SetShuffle(PLAYLIST::TYPE_VIDEO, false);
          CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_VIDEO, items);
          CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
          CServiceBroker::GetPlaylistPlayer().Play(0, "");
          return true;
        }

        // Video CDs can have multiple file formats. First we need to determine which one is used on the CD
        std::string strExt;
        if (StringUtils::EqualsNoCase(name, "MPEGAV"))
          strExt = ".dat";
        if (StringUtils::EqualsNoCase(name, "MPEG2"))
          strExt = ".mpg";

        // If a file format was extracted we are sure this is a VCD. Autoplay if settings indicate we should.
        if (!strExt.empty() && bAllowVideo
             && (bypassSettings || bAutorunDVDs))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, strExt, DIR_FLAG_DEFAULTS);
          if (items.Size())
          {
            items.Sort(SortByLabel, SortOrderAscending);
            CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
            CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_VIDEO, items);
            CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
            CServiceBroker::GetPlaylistPlayer().Play(0, "");
            return true;
          }
        }
        /* Probably want this if/when we add some automedia action dialog...
        else if (pItem->GetPath().Find("PICTURES") != -1 && bAllowPictures
              && (bypassSettings))
        {
          bPlaying = true;
          std::string strExec = StringUtils::Format("RecursiveSlideShow({})", pItem->GetPath());
          CBuiltins::Execute(strExec);
          return true;
        }
        */
      }
    }
  }

  // check video first
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_DVDS_AUTORUN)))
  {
    // stack video files
    CFileItemList tempItems;
    tempItems.Append(vecItems);
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_STACKVIDEOS))
      tempItems.Stack();
    CFileItemList itemlist;

    for (int i = 0; i < tempItems.Size(); i++)
    {
      CFileItemPtr pItem = tempItems[i];
      if (!pItem->m_bIsFolder && IsVideo(*pItem))
      {
        bPlaying = true;
        if (pItem->IsStack())
        {
          //! @todo remove this once the app/player is capable of handling stacks immediately
          CStackDirectory dir;
          CFileItemList items;
          dir.GetDirectory(pItem->GetURL(), items);
          itemlist.Append(items);
        }
        else
          itemlist.Add(pItem);
      }
    }
    if (itemlist.Size())
    {
      if (!bAllowVideo)
      {
        if (!bypassSettings)
          return false;

        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;
      }
      CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_VIDEO);
      CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_VIDEO, itemlist);
      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
      CServiceBroker::GetPlaylistPlayer().Play(0, "");
    }
  }
  // then music
  if (!bPlaying && (bypassSettings || CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION) == AUTOCD_PLAY) && bAllowMusic)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && MUSIC::IsAudio(*pItem))
      {
        nAddedToPlaylist++;
        CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_MUSIC, pItem);
      }
    }
  }
  /* Probably want this if/when we add some automedia action dialog...
  // and finally pictures
  if (!nAddedToPlaylist && !bPlaying && bypassSettings && bAllowPictures)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsPicture())
      {
        bPlaying = true;
        std::string strExec = StringUtils::Format("RecursiveSlideShow({})", strDrive);
        CBuiltins::Execute(strExec);
        break;
      }
    }
  }
  */

  // check subdirs if we are not playing yet
  if (!bPlaying)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr  pItem = vecItems[i];
      if (pItem->m_bIsFolder)
      {
        if (pItem->GetPath() != "." && pItem->GetPath() != ".." )
        {
          if (RunDisc(pDir, pItem->GetPath(), nAddedToPlaylist, false, bypassSettings, startFromBeginning))
          {
            bPlaying = true;
            break;
          }
        }
      } // if (non system) folder
    } // for all items in directory
  } // if root folder

  return bPlaying;
}

void CAutorun::HandleAutorun()
{
#if !defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  const CDetectDVDMedia& mediadetect = CServiceBroker::GetDetectDVDMedia();

  if (!m_bEnable)
  {
    mediadetect.m_evAutorun.Reset();
    return ;
  }

  if (mediadetect.m_evAutorun.Wait(0ms))
  {
    if (!ExecuteAutorun(""))
      CLog::Log(LOGDEBUG, "{}: Could not execute autorun", __func__);
    mediadetect.m_evAutorun.Reset();
  }
#endif
}

void CAutorun::Enable()
{
  m_bEnable = true;
}

void CAutorun::Disable()
{
  m_bEnable = false;
}

bool CAutorun::IsEnabled() const
{
  return m_bEnable;
}

bool CAutorun::PlayDiscAskResume(const std::string& path)
{
  return PlayDisc(path, true,
                  !CanResumePlayDVD(path) ||
                      HELPERS::ShowYesNoDialogText(CVariant{341}, CVariant{""}, CVariant{13404},
                                                   CVariant{12021}) == DialogResponse::CHOICE_YES);
}

bool CAutorun::CanResumePlayDVD(const std::string& path)
{
  std::string strUniqueId = CServiceBroker::GetMediaManager().GetDiskUniqueId(path);
  if (!strUniqueId.empty())
  {
    CVideoDatabase dbs;
    dbs.Open();
    CBookmark bookmark;
    if (dbs.GetResumeBookMark(strUniqueId, bookmark))
      return true;
  }
  return false;
}

void CAutorun::SettingOptionAudioCdActionsFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data)
{
  list.emplace_back(g_localizeStrings.Get(16018), AUTOCD_NONE);
  list.emplace_back(g_localizeStrings.Get(14098), AUTOCD_PLAY);
#ifdef HAS_CDDA_RIPPER
  list.emplace_back(g_localizeStrings.Get(14096), AUTOCD_RIP);
#endif
}
