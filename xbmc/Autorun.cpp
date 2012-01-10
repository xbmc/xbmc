/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#ifdef HAS_DVD_DRIVE

#include "Autorun.h"
#include "Application.h"
#include "Util.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/FactoryDirectory.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "video/VideoDatabase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

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

  PlayDisc(path, bypassSettings, startFromBeginning);
}

bool CAutorun::PlayDisc(const CStdString& path, bool bypassSettings, bool startFromBeginning)
{
  if ( !bypassSettings && !g_guiSettings.GetBool("audiocds.autorun") && !g_guiSettings.GetBool("dvds.autorun"))
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

  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( mediaPath ));
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
            items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
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
  if (!bPlaying && (bypassSettings || g_guiSettings.GetBool("audiocds.autorun")) && bAllowMusic)
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
