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

#ifdef HAS_DVD_DRIVE
#include <stdlib.h>

#include "Autorun.h"
#include "Application.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "video/VideoDatabase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

using namespace XFILE;
using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun()
{}

void CAutorun::ExecuteAutorun(const std::string& path, bool bypassSettings, bool ignoreplaying, bool startFromBeginning )
{
  if ((!ignoreplaying && (g_application.m_pPlayer->IsPlayingAudio() || g_application.m_pPlayer->IsPlayingVideo() || g_windowManager.HasModalDialog())) || g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return ;

  CCdInfo* pInfo = g_mediaManager.GetCdInfo(path);

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();  // turn off the screensaver if it's active
#ifdef HAS_CDDA_RIPPER
  if (CSettings::Get().GetInt("audiocds.autoaction") == AUTOCD_RIP && 
      pInfo->IsAudio(1) && !CProfilesManager::Get().GetCurrentProfile().musicLocked())
  {
    CCDDARipper::GetInstance().RipCD();
  }
  else
#endif
  PlayDisc(path, bypassSettings, startFromBeginning);
}

bool CAutorun::PlayDisc(const std::string& path, bool bypassSettings, bool startFromBeginning)
{
  if ( !bypassSettings && CSettings::Get().GetInt("audiocds.autoaction") != AUTOCD_PLAY && !CSettings::Get().GetBool("dvds.autorun"))
    return false;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;

  std::string mediaPath;

  CCdInfo* pInfo = g_mediaManager.GetCdInfo(path);
  if (pInfo == NULL)
    return false;

  if (mediaPath.empty() && pInfo->IsAudio(1))
    mediaPath = "cdda://local/";

  if (mediaPath.empty() && (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)))
    mediaPath = "iso9660://";

  if (mediaPath.empty())
    mediaPath = path;

  if (mediaPath.empty() || mediaPath == "iso9660://")
    mediaPath = g_mediaManager.GetDiscPath();

  const CURL pathToUrl(mediaPath);
  std::unique_ptr<IDirectory> pDir ( CDirectoryFactory::Create( pathToUrl ));
  bool bPlaying = RunDisc(pDir.get(), mediaPath, nAddedToPlaylist, true, bypassSettings, startFromBeginning);

  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
    g_windowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    return g_playlistPlayer.Play(nSize);
  }

  return bPlaying;
}

/**
 * This method tries to determine what type of disc is located in the given drive and starts to play the content appropriately.
 */
bool CAutorun::RunDisc(IDirectory* pDir, const std::string& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */, bool startFromBeginning /* = false */)
{
  bool bPlaying(false);
  CFileItemList vecItems;

  const CURL pathToUrl(strDrive);
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
    bAllowVideo = !CProfilesManager::Get().GetCurrentProfile().videoLocked();
//    bAllowPictures = !CProfilesManager::Get().GetCurrentProfile().picturesLocked();
    bAllowMusic = !CProfilesManager::Get().GetCurrentProfile().musicLocked();
  }

  // is this a root folder we have to check the content to determine a disc type
  if( bRoot )
  {
    std::string hddvdname = "";
    CFileItemPtr phddvdItem;

    // check root folders next, for normal structured dvd's
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];

      // is the current item a (non system) folder?
      if (pItem->m_bIsFolder && pItem->GetPath() != "." && pItem->GetPath() != "..")
      {
        std::string name = pItem->GetPath();
        URIUtils::RemoveSlashAtEnd(name);
        name = URIUtils::GetFileName(name);

        // Check if the current foldername indicates a DVD structure (name is "VIDEO_TS")
        if (StringUtils::EqualsNoCase(name, "VIDEO_TS") && bAllowVideo
        && (bypassSettings || CSettings::Get().GetBool("dvds.autorun")))
        {
          std::string path = URIUtils::AddFileToFolder(pItem->GetPath(), "VIDEO_TS.IFO");
          if(!CFile::Exists(path))
            path = URIUtils::AddFileToFolder(pItem->GetPath(), "video_ts.ifo");
          CFileItemPtr item(new CFileItem(path, false));
          item->SetLabel(g_mediaManager.GetDiskLabel(strDrive));
          item->GetVideoInfoTag()->m_strFileNameAndPath = g_mediaManager.GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item->GetVideoInfoTag()->m_strFileNameAndPath.empty())
            item->m_lStartOffset = STARTOFFSET_RESUME;

          g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.SetShuffle (PLAYLIST_VIDEO, false);
          g_playlistPlayer.Add(PLAYLIST_VIDEO, item);
          g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.Play(0);
          return true;
        }

        // Check if the current foldername indicates a Blu-Ray structure (default is "BDMV").
        // A BR should also include an "AACS" folder for encryption, Sony-BRs can also include update folders for PS3 (PS3_UPDATE / PS3_VPRM).
        // ToDo: for the time beeing, the DVD autorun settings are used to determine if the BR should be started automatically.
        if (StringUtils::EqualsNoCase(name, "BDMV") && bAllowVideo
        && (bypassSettings || CSettings::Get().GetBool("dvds.autorun")))
        {
          CFileItemPtr item(new CFileItem(URIUtils::AddFileToFolder(pItem->GetPath(), "index.bdmv"), false));
          item->SetLabel(g_mediaManager.GetDiskLabel(strDrive));
          item->GetVideoInfoTag()->m_strFileNameAndPath = g_mediaManager.GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item->GetVideoInfoTag()->m_strFileNameAndPath.empty())
            item->m_lStartOffset = STARTOFFSET_RESUME;

          g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.SetShuffle (PLAYLIST_VIDEO, false);
          g_playlistPlayer.Add(PLAYLIST_VIDEO, item);
          g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.Play(0);
          return true;
        }

        // Check if the current foldername indicates a HD DVD structure (default is "HVDVD_TS").
        // Most HD DVD will also include an "ADV_OBJ" folder for advanced content. This folder should be handled first.
        // ToDo: for the time beeing, the DVD autorun settings are used to determine if the HD DVD should be started automatically.
        CFileItemList items, sitems;
        
        // Advanced Content HD DVD (most discs?)
        if (StringUtils::EqualsNoCase(name, "ADV_OBJ"))
        {
          CLog::Log(LOGINFO,"HD DVD: Checking for playlist.");
          // find playlist file
          CDirectory::GetDirectory(pItem->GetPath(), items, "*.xpl");
          if (items.Size())
          {
            // HD DVD Standard says the highest numbered playlist has to be handled first.
            CLog::Log(LOGINFO,"HD DVD: Playlist found. Set filetypes to *.xpl for external player.");
            items.Sort(SortByLabel, SortOrderDescending);
            phddvdItem = pItem; 
            hddvdname = URIUtils::GetFileName(items[0]->GetPath());
            CLog::Log(LOGINFO,"HD DVD: %s", items[0]->GetPath().c_str());
          }
        }

        // Standard Content HD DVD (few discs?)
        if (StringUtils::EqualsNoCase(name, "HVDVD_TS") && bAllowVideo
        && (bypassSettings || CSettings::Get().GetBool("dvds.autorun")))
        {
          if (hddvdname == "")
          {
            CLog::Log(LOGINFO,"HD DVD: Checking for ifo.");
            // find Video Manager or Title Set Information
            CDirectory::GetDirectory(pItem->GetPath(), items, "HV*.ifo");
            if (items.Size())
            {
              // HD DVD Standard says the lowest numbered ifo has to be handled first.
              CLog::Log(LOGINFO,"HD DVD: IFO found. Set filename to HV* and filetypes to *.ifo for external player.");
              items.Sort(SortByLabel, SortOrderAscending);
              phddvdItem = pItem; 
              hddvdname = URIUtils::GetFileName(items[0]->GetPath());
              CLog::Log(LOGINFO,"HD DVD: %s",items[0]->GetPath().c_str());
            }
          }
          // Find and sort *.evo files for internal playback.
          // While this algorithm works for all of my HD DVDs, it may fail on other discs. If there are very large extras which are
          // alphabetically before the main movie they will be sorted to the top of the playlist and get played first.
          CDirectory::GetDirectory(pItem->GetPath(), items, "*.evo");
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
            item.SetLabel(g_mediaManager.GetDiskLabel(strDrive));
            item.GetVideoInfoTag()->m_strFileNameAndPath = g_mediaManager.GetDiskUniqueId(strDrive);

            if (!startFromBeginning && !item.GetVideoInfoTag()->m_strFileNameAndPath.empty())
            item.m_lStartOffset = STARTOFFSET_RESUME;

            // get playername
            std::string hddvdplayer = CPlayerCoreFactory::Get().GetPlayerName(CPlayerCoreFactory::Get().GetDefaultPlayer(item));
            
            // Single *.xpl or *.ifo files require an external player to handle playback.
            // If no matching rule was found, DVDPlayer will be default player.
            if (hddvdplayer != "DVDPlayer")
            {
              CLog::Log(LOGINFO,"HD DVD: External singlefile playback initiated: %s",hddvdname.c_str());
              g_application.PlayFile(item, false);
              return true;
            } else
              CLog::Log(LOGINFO,"HD DVD: No external player found. Fallback to internal one.");
          }

          //  internal *.evo playback.
          CLog::Log(LOGINFO,"HD DVD: Internal multifile playback initiated.");
          g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.SetShuffle (PLAYLIST_VIDEO, false);
          g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
          g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
          g_playlistPlayer.Play(0);
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
             && (bypassSettings || CSettings::Get().GetBool("dvds.autorun")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, strExt);
          if (items.Size())
          {
            items.Sort(SortByLabel, SortOrderAscending);
            g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Play(0);
            return true;
          }
        }
        /* Probably want this if/when we add some automedia action dialog...
        else if (pItem->GetPath().Find("PICTURES") != -1 && bAllowPictures
              && (bypassSettings))
        {
          bPlaying = true;
          std::string strExec = StringUtils::Format("RecursiveSlideShow(%s)", pItem->GetPath().c_str());
          CBuiltins::Execute(strExec);
          return true;
        }
        */
      }
    }
  }

  // check video first
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CSettings::Get().GetBool("dvds.autorun")))
  {
    // stack video files
    CFileItemList tempItems;
    tempItems.Append(vecItems);
    if (CSettings::Get().GetBool("myvideos.stackvideos"))
      tempItems.Stack();
    CFileItemList itemlist;

    for (int i = 0; i < tempItems.Size(); i++)
    {
      CFileItemPtr pItem = tempItems[i];
      if (!pItem->m_bIsFolder && pItem->IsVideo())
      {
        bPlaying = true;
        if (pItem->IsStack())
        {
          // TODO: remove this once the app/player is capable of handling stacks immediately
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

        if (g_windowManager.GetActiveWindow() != WINDOW_VIDEO_FILES)
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;
      }
      g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
      g_playlistPlayer.Add(PLAYLIST_VIDEO, itemlist);
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
      g_playlistPlayer.Play(0);
    }
  }
  // then music
  if (!bPlaying && (bypassSettings || CSettings::Get().GetInt("audiocds.autoaction") == AUTOCD_PLAY) && bAllowMusic)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsAudio())
      {
        nAddedToPlaylist++;
        g_playlistPlayer.Add(PLAYLIST_MUSIC, pItem);
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
        std::string strExec = StringUtils::Format("RecursiveSlideShow(%s)", strDrive.c_str());
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
#ifndef TARGET_WINDOWS
  if (!m_bEnable)
  {
    CDetectDVDMedia::m_evAutorun.Reset();
    return ;
  }

  if (CDetectDVDMedia::m_evAutorun.WaitMSec(0))
  {
    ExecuteAutorun();
    CDetectDVDMedia::m_evAutorun.Reset();
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
  return PlayDisc(path, true, !CanResumePlayDVD(path) || CGUIDialogYesNo::ShowAndGetInput(341, "", "", "", 13404, 12021));
}

bool CAutorun::CanResumePlayDVD(const std::string& path)
{
  std::string strUniqueId = g_mediaManager.GetDiskUniqueId(path);
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

void CAutorun::SettingOptionAudioCdActionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(16018), AUTOCD_NONE));
  list.push_back(make_pair(g_localizeStrings.Get(14098), AUTOCD_PLAY));
#ifdef HAS_CDDA_RIPPER
  list.push_back(make_pair(g_localizeStrings.Get(14096), AUTOCD_RIP));
#endif
}

#endif
