#pragma once

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

#include "GUIDialog.h"
#include "MusicInfoScanner.h"
#include "utils/CriticalSection.h"

class CGUIDialogMusicScan: public CGUIDialog, public MUSIC_INFO::IMusicInfoScannerObserver
{
public:
  CGUIDialogMusicScan(void);
  virtual ~CGUIDialogMusicScan(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void StartScanning(const CStdString& strDirectory);
  void StartAlbumScan(const CStdString& strDirectory);
  void StartArtistScan(const CStdString& strDirectory);
  bool IsScanning();
  void StopScanning();

  void UpdateState();
protected:
  int GetStateString();
  virtual void OnDirectoryChanged(const CStdString& strDirectory);
  virtual void OnDirectoryScanned(const CStdString& strDirectory);
  virtual void OnFinished();
  virtual void OnStateChanged(MUSIC_INFO::SCAN_STATE state);
  virtual void OnSetProgress(int currentItem, int itemCount);

  MUSIC_INFO::CMusicInfoScanner m_musicInfoScanner;
  MUSIC_INFO::SCAN_STATE m_ScanState;
  CStdString m_strCurrentDir;

  CCriticalSection m_critical;

  float m_fPercentDone;
  int m_currentItem;
  int m_itemCount;
};
