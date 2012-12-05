/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "Autorun.h"
#include "Application.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "video/VideoDatabase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun()
{}

void CAutorun::ExecuteAutorun(const CStdString& path, bool bypassSettings, bool ignoreplaying, bool startFromBeginning )
{
  if ((!ignoreplaying && (g_application.IsPlayingAudio() || g_application.IsPlayingVideo() || g_windowManager.HasModalDialog())) || g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return ;

  CCdInfo* pInfo = g_mediaManager.GetCdInfo(path);

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();  // turn off the screensaver if it's active
#ifdef HAS_CDDA_RIPPER
  if (g_guiSettings.GetInt("audiocds.autoaction") == AUTOCD_RIP && 
      pInfo->IsAudio(1) && !g_settings.GetCurrentProfile().musicLocked())
  {
    CCDDARipper::GetInstance().RipCD();
  }
  else
#endif
  PlayDisc(path, bypassSettings, startFromBeginning);
}

bool CAutorun::PlayDisc(const CStdString& path, bool bypassSettings, bool startFromBeginning)
{
  if ( !bypassSettings && !g_guiSettings.GetInt("audiocds.autoaction") == AUTOCD_PLAY && !g_guiSettings.GetBool("dvds.autorun"))
    return false;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;

  CStdString mediaPath;

  CCdInfo* pInfo = g_mediaManager.GetCdInfo(path);
  if (pInfo == NULL)
    return false;

  if (mediaPath.IsEmpty() && pInfo->IsAudio(1))
    mediaPath = "cdda://local/";

  if (mediaPath.IsEmpty() && (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)))
    mediaPath = "iso9660://";

  if (mediaPath.IsEmpty())
    mediaPath = path;

#ifdef _WIN32
  if (mediaPath.IsEmpty() || mediaPath.CompareNoCase("iso9660://") == 0)
    mediaPath = g_mediaManager.TranslateDevicePath("");
#endif

  auto_ptr<IDirectory> pDir ( CDirectoryFactory::Create( mediaPath ));
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
bool CAutorun::RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */, bool startFromBeginning /* = false */)
{
  bool bPlaying(false);
  CFileItemList vecItems;

  if ( !pDir->GetDirectory( strDrive, vecItems ) )
  {
    return false;
  }

  // Sorting necessary for easier HDDVD handling
  vecItems.Sort(SORT_METHOD_LABEL, SortOrderAscending);

  bool bAllowVideo = true;
//  bool bAllowPictures = true;
  bool bAllowMusic = true;
  if (!g_passwordManager.IsMasterLockUnlocked(false))
  {
    bAllowVideo = !g_settings.GetCurrentProfile().videoLocked();
//    bAllowPictures = !g_settings.GetCurrentProfile().picturesLocked();
    bAllowMusic = !g_settings.GetCurrentProfile().musicLocked();
  }

  // is this a root folder we have to check the content to determine a disc type
  if( bRoot )
  {
    CStdString hddvdname = "";
    CFileItemPtr phddvdItem;

    // check root folders next, for normal structured dvd's
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];

      // is the current item a (non system) folder?
      if (pItem->m_bIsFolder && pItem->GetPath() != "." && pItem->GetPath() != "..")
      {
        CStdString name = pItem->GetPath();
        URIUtils::RemoveSlashAtEnd(name);
        name = URIUtils::GetFileName(name);

        // Check if the current foldername indicates a DVD structure (name is "VIDEO_TS")
        if (name.Equals("VIDEO_TS") && bAllowVideo
        && (bypassSettings || g_guiSettings.GetBool("dvds.autorun")))
        {
          CStdString path = URIUtils::AddFileToFolder(pItem->GetPath(), "VIDEO_TS.IFO");
          if(!CFile::Exists(path))
            path = URIUtils::AddFileToFolder(pItem->GetPath(), "video_ts.ifo");
          CFileItem item(path, false);
          item.SetLabel(g_mediaManager.GetDiskLabel(strDrive));
          item.GetVideoInfoTag()->m_strFileNameAndPath = g_mediaManager.GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
            item.m_lStartOffset = STARTOFFSET_RESUME;

          g_application.PlayFile(item, false);
          bPlaying = true;
          return true;
        }

        // Check if the current foldername indicates a Blu-Ray structure (default is "BDMV").
        // A BR should also include an "AACS" folder for encryption, Sony-BRs can also include update folders for PS3 (PS3_UPDATE / PS3_VPRM).
        // ToDo: for the time beeing, the DVD autorun settings are used to determine if the BR should be started automatically.
        if (name.Equals("BDMV") && bAllowVideo
        && (bypassSettings || g_guiSettings.GetBool("dvds.autorun")))
        {
          CFileItem item(URIUtils::AddFileToFolder(pItem->GetPath(), "index.bdmv"), false);
          item.SetLabel(g_mediaManager.GetDiskLabel(strDrive));
          item.GetVideoInfoTag()->m_strFileNameAndPath = g_mediaManager.GetDiskUniqueId(strDrive);

          if (!startFromBeginning && !item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
            item.m_lStartOffset = STARTOFFSET_RESUME;

          g_application.PlayFile(item, false);
          bPlaying = true;
          return true;
        }

        // Check if the current foldername indicates a HD DVD structure (default is "HVDVD_TS").
        // Most HD DVD will also include an "ADV_OBJ" folder for advanced content. This folder should be handled first.
        // ToDo: for the time beeing, the DVD autorun settings are used to determine if the HD DVD should be started automatically.
        CFileItemList items, sitems;
        
        // Advanced Content HD DVD (most discs?)
        if (name.Equals("ADV_OBJ"))
        {
          CLog::Log(LOGINFO,"HD DVD: Checking for playlist.");
          // find playlist file
          CDirectory::GetDirectory(pItem->GetPath(), items, "*.xpl");
          if (items.Size())
          {
            // HD DVD Standard says the highest numbered playlist has to be handled first.
            CLog::Log(LOGINFO,"HD DVD: Playlist found. Set filetypes to *.xpl for external player.");
            items.Sort(SORT_METHOD_LABEL, SortOrderDescending);
            phddvdItem = pItem; 
            hddvdname = URIUtils::GetFileName(items[0]->GetPath());
            CLog::Log(LOGINFO,"HD DVD: %s", items[0]->GetPath().c_str());
          }
        }

        // Standard Content HD DVD (few discs?)
        if (name.Equals("HVDVD_TS") && bAllowVideo
        && (bypassSettings || g_guiSettings.GetBool("dvds.autorun")))
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
              items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
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
            items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
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
            items.Sort(SORT_METHOD_SIZE, SortOrderDescending);
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

            if (!startFromBeginning && !item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
            item.m_lStartOffset = STARTOFFSET_RESUME;

            // get playername
            CStdString hddvdplayer = CPlayerCoreFactory::GetPlayerName(CPlayerCoreFactory::GetDefaultPlayer(item));
            
            // Single *.xpl or *.ifo files require an external player to handle playback.
            // If no matching rule was found, DVDPlayer will be default player.
            if (hddvdplayer != "DVDPlayer")
            {
              CLog::Log(LOGINFO,"HD DVD: External singlefile playback initiated: %s",hddvdname.c_str());
              g_application.PlayFile(item, false);
              bPlaying = true;
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
          bPlaying = true;
          return true;
        }
				
        // Video CDs can have multiple file formats. First we need to determine which one is used on the CD
        CStdString strExt;
        if (name.Equals("MPEGAV"))
          strExt = ".dat";
        if (name.Equals("MPEG2"))
          strExt = ".mpg";

        // If a file format was extracted we are sure this is a VCD. Autoplay if settings indicate we should.
        if (!strExt.IsEmpty() && bAllowVideo
             && (bypassSettings || g_guiSettings.GetBool("dvds.autorun")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, strExt);
          if (items.Size())
          {
            items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
            g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Play(0);
            bPlaying = true;
            return true;
          }
        }
        /* Probably want this if/when we add some automedia action dialog...
        else if (pItem->GetPath().Find("PICTURES") != -1 && bAllowPictures
              && (bypassSettings))
        {
          bPlaying = true;
          CStdString strExec;
          strExec.Format("XBMC.RecursiveSlideShow(%s)", pItem->GetPath().c_str());
          CBuiltins::Execute(strExec);
          return true;
        }
        */
      }
    }
  }

  // check video first
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || g_guiSettings.GetBool("dvds.autorun")))
  {
    // stack video files
    CFileItemList tempItems;
    tempItems.Append(vecItems);
    if (g_settings.m_videoStacking)
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
          dir.GetDirectory(pItem->GetPath(), items);
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
  if (!bPlaying && (bypassSettings || g_guiSettings.GetInt("audiocds.autoaction") == AUTOCD_PLAY) && bAllowMusic)
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
        CStdString strExec;
        strExec.Format("XBMC.RecursiveSlideShow(%s)", strDrive.c_str());
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
#ifndef _WIN32
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

bool CAutorun::PlayDiscAskResume(const CStdString& path)
{
  return PlayDisc(path, true, !CanResumePlayDVD(path) || CGUIDialogYesNo::ShowAndGetInput(341, -1, -1, -1, 13404, 12021));
}

bool CAutorun::CanResumePlayDVD(const CStdString& path)
{
  CStdString strUniqueId = g_mediaManager.GetDiskUniqueId(path);
  if (!strUniqueId.IsEmpty())
  {
    CVideoDatabase dbs;
    dbs.Open();
    CBookmark bookmark;
    if (dbs.GetResumeBookMark(strUniqueId, bookmark))
      return true;
  }
  return false;
}

#endif
