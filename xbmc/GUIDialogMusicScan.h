#pragma once
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
