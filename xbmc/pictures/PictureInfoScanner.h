#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
#include "threads/Thread.h"
#include "PictureDatabase.h"

class CGUIDialogProgressBarHandle;

namespace PICTURE_INFO
{
class CPictureInfoScanner : CThread, public IRunnable
{
public:
  CPictureInfoScanner();
  virtual ~CPictureInfoScanner() { }

  void Start(const CStdString &strDirectory);
  bool IsScanning() { return m_bRunning; }
  void Stop();

  // Set whether or not to show a progress dialog
  void ShowDialog(bool show) { m_showDialog = show; }

protected:
  virtual void Process();
  virtual void Run();
  int  CountFilesRecursively(const std::string &strPath);
  int  CountFiles(const CFileItemList &items, bool recursive);
  bool DoScan(const std::string &strDirectory);
  int  GetPathHash(const CFileItemList &items, std::string &hash);
  int  RetrievePictureInfo(CFileItemList &items, const std::string &strDirectory);
  
protected:
  bool m_showDialog;
  CGUIDialogProgressBarHandle* m_handle;

  int  m_currentItem;
  int  m_itemCount;
  bool m_bRunning;
  bool m_bCanInterrupt;

  CPictureDatabase      m_pictureDatabase;
  std::set<std::string> m_pathsToScan;
  std::set<std::string> m_pathsToCount;
};
}
