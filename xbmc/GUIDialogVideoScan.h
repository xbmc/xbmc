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
#include "VideoInfoScanner.h"
#include "utils/CriticalSection.h"

class CGUIDialogVideoScan: public CGUIDialog, public VIDEO::IVideoInfoScannerObserver
{
public:
  CGUIDialogVideoScan(void);
  virtual ~CGUIDialogVideoScan(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();

  void StartScanning(const CStdString& strDirectory, bool scanAll = false);
  bool IsScanning();
  void StopScanning();

  void UpdateState();
protected:
  int GetStateString();
  virtual void OnDirectoryChanged(const CStdString& strDirectory);
  virtual void OnDirectoryScanned(const CStdString& strDirectory);
  virtual void OnFinished();
  virtual void OnStateChanged(VIDEO::SCAN_STATE state);
  virtual void OnSetProgress(int currentItem, int itemCount);
  virtual void OnSetCurrentProgress(int currentItem, int itemCount);
  virtual void OnSetTitle(const CStdString& strTitle);

  VIDEO::CVideoInfoScanner m_videoInfoScanner;
  VIDEO::SCAN_STATE m_ScanState;
  CStdString m_strCurrentDir;
  CStdString m_strTitle;

  CCriticalSection m_critical;

  float m_fPercentDone;
  float m_fCurrentPercentDone;
  int m_currentItem;
  int m_itemCount;
};
