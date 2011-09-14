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

#include "GUIDialogMusicScan.h"
#include "guilib/GUIProgressControl.h"
#include "Application.h"
#include "Util.h"
#include "URL.h"
#include "guilib/GUIWindowManager.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

#define CONTROL_LABELSTATUS     401
#define CONTROL_LABELDIRECTORY  402
#define CONTROL_PROGRESS        403

CGUIDialogMusicScan::CGUIDialogMusicScan(void)
: CGUIDialog(WINDOW_DIALOG_MUSIC_SCAN, "DialogMusicScan.xml")
{
  m_musicInfoScanner.SetObserver(this);
}

CGUIDialogMusicScan::~CGUIDialogMusicScan(void)
{
}

bool CGUIDialogMusicScan::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_strCurrentDir.Empty();

      m_fPercentDone=-1.0F;

      UpdateState();
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogMusicScan::FrameMove()
{
  if (m_active)
    UpdateState();

  CGUIDialog::FrameMove();
}

void CGUIDialogMusicScan::OnDirectoryChanged(const CStdString& strDirectory)
{
  CSingleLock lock (m_critical);

  m_strCurrentDir = strDirectory;
}

void CGUIDialogMusicScan::OnStateChanged(SCAN_STATE state)
{
  CSingleLock lock (m_critical);

  m_ScanState = state;
}

void CGUIDialogMusicScan::OnSetProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fPercentDone=(float)((currentItem*100)/itemCount);
  if (m_fPercentDone>100.0F) m_fPercentDone=100.0F;
}

void CGUIDialogMusicScan::StartScanning(const CStdString& strDirectory)
{
  m_ScanState = PREPARING;

  if (!g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
  {
    Show();
  }

  // save settings
  g_application.SaveMusicScanSettings();

  m_musicInfoScanner.Start(strDirectory);
}

void CGUIDialogMusicScan::StartAlbumScan(const CStdString& strDirectory)
{
  m_ScanState = PREPARING;

  if (!g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
  {
    Show();
  }

  // save settings
  g_application.SaveMusicScanSettings();

  m_musicInfoScanner.FetchAlbumInfo(strDirectory);
}

void CGUIDialogMusicScan::StartArtistScan(const CStdString& strDirectory)
{
  m_ScanState = PREPARING;

  if (!g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
  {
    Show();
  }

  // save settings
  g_application.SaveMusicScanSettings();

  m_musicInfoScanner.FetchArtistInfo(strDirectory);
}

void CGUIDialogMusicScan::StopScanning()
{
  if (m_musicInfoScanner.IsScanning())
    m_musicInfoScanner.Stop();
}

bool CGUIDialogMusicScan::IsScanning()
{
  return m_musicInfoScanner.IsScanning();
}

void CGUIDialogMusicScan::OnDirectoryScanned(const CStdString& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  g_windowManager.SendThreadMessage(msg);
}

void CGUIDialogMusicScan::OnFinished()
{
  // clear cache
  CUtil::DeleteMusicDatabaseDirectoryCache();

  // send message
  CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
  g_windowManager.SendThreadMessage(msg);

  // be sure to restore the settings
  CLog::Log(LOGINFO,"Music scan was stopped or finished ... restoring FindRemoteThumbs");
  g_application.RestoreMusicScanSettings();

  if (!g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
  {
    g_application.getApplicationMessenger().Close(this, false, false);
  }
}

void CGUIDialogMusicScan::UpdateState()
{
  CSingleLock lock (m_critical);

  SET_CONTROL_LABEL(CONTROL_LABELSTATUS, GetStateString());

  if (m_ScanState == READING_MUSIC_INFO)
  {
    CURL url(m_strCurrentDir);
    CStdString strStrippedPath = url.GetWithoutUserDetails();
    CURL::Decode(strStrippedPath);

    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, strStrippedPath);

    if (m_fPercentDone>-1.0F)
    {
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
    }
  }
  else if (m_ScanState == DOWNLOADING_ALBUM_INFO || m_ScanState == DOWNLOADING_ARTIST_INFO)
  {
    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, m_strCurrentDir);
    if (m_fPercentDone>-1.0F)
    {
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
    }
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, "");
    SET_CONTROL_HIDDEN(CONTROL_PROGRESS);
  }
}

int CGUIDialogMusicScan::GetStateString()
{
  if (m_ScanState == PREPARING)
    return 314;
  else if (m_ScanState == REMOVING_OLD)
    return 701;
  else if (m_ScanState == CLEANING_UP_DATABASE)
    return 700;
  else if (m_ScanState == READING_MUSIC_INFO)
    return 505;
  else if (m_ScanState == COMPRESSING_DATABASE)
    return 331;
  else if (m_ScanState == WRITING_CHANGES)
    return 328;
  else if (m_ScanState == DOWNLOADING_ALBUM_INFO)
    return 21885;
  else if (m_ScanState == DOWNLOADING_ARTIST_INFO)
    return 21886;

  return -1;
}
