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

#include "ProgramDatabase.h"
#include "utils/Thread.h"
//#include "ProgramInfo.h"
//#include "Program.h"

namespace PROGRAM_INFO
{
  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_PROGRAM_INFO, DOWNLOADING_PROGRAM_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };
  class IProgramInfoScannerObserver
  {
  public:
    virtual ~IProgramInfoScannerObserver() {}
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnFinished() = 0;
  };

  class CProgramInfoScanner : CThread, public IRunnable
  {
    public:
      CProgramInfoScanner();
      virtual ~CProgramInfoScanner();
      void SetObserver(IProgramInfoScannerObserver* pObserver);
  //      void Start(const CStdString& strDirectory);
  //    void FetchProgramInfo(const CStdString& strDirectory);
  //    bool IsScanning();
  //    void Stop();
  //    bool DownloadProgramInfo(const CStdString& strPath, const CStdString& strName, bool& bCanceled, CProgramInfo& program, CGUIDialogProgress* pDialog=NULL);

  protected:
  //  virtual void Process();
  //  int RetrieveProgramInfo(CFileItemList& items, const CStdString& strDirectory);
  //  bool DoScan(const CStdString& strDirectory);
  //  virtual void Run();
  //  int CountFiles(const CFileItemList& items, bool recursive);
  //  int CountFilesRecursively(const CStdString& strPath);

  protected:
    IProgramInfoScannerObserver* m_pObserver;
  //  int m_currentItem;
  //  int m_itemCount;
  //  bool m_bRunning;
  //  bool m_bCanInterrupt;
  //  bool m_needsCleanup;
  //  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
    CProgramDatabase m_programDatabase;

  //  std::set<CStdString> m_pathsToScan;
  //  std::set<CProgram> m_programsToScan;
  //  std::set<CStdString> m_pathsToCount;
  //  std::vector<long> m_programsScanned;
  };
}
