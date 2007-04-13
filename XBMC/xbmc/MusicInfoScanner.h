#pragma once
#include "utils/Thread.h"
#include "musicDatabase.h"

namespace MUSIC_INFO
{
enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_MUSIC_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

class IMusicInfoScannerObserver
{
public:
  virtual void OnStateChanged(SCAN_STATE state) = 0;
  virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
  virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
  virtual void OnSetProgress(int currentItem, int itemCount)=0;
  virtual void OnFinished() = 0;
};

class CMusicInfoScanner : CThread, public IRunnable
{
public:
  CMusicInfoScanner();
  virtual ~CMusicInfoScanner();

  void Start(const CStdString& strDirectory, bool bUpdateAll);
  bool IsScanning();
  void Stop();
  void SetObserver(IMusicInfoScannerObserver* pObserver);

protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  void CheckForVariousArtists(VECSONGS &songs);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CStdString& strPath);

protected:
  IMusicInfoScannerObserver* m_pObserver;
  int m_currentItem;
  int m_itemCount;
  bool m_bRunning;
  bool m_bUpdateAll;
  bool m_bCanInterrupt;
  CStdString m_strStartDir;
  CMusicDatabase m_musicDatabase;
};
};
