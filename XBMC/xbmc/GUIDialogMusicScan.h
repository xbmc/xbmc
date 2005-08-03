#pragma once
#include "GUIDialog.h"
#include "MusicInfoScanner.h"
#include "utils/CriticalSection.h"

using namespace MUSIC_INFO;

class CGUIDialogMusicScan: public CGUIDialog, public IMusicInfoScannerObserver
{
public:
  CGUIDialogMusicScan(void);
  virtual ~CGUIDialogMusicScan(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void StartScanning(const CStdString& strDirectory, bool bUpdateAll);
  bool IsScanning();
  void StopScanning();

  void UpdateState();
protected:
  int GetStateString();
  virtual void OnDirectoryChanged(const CStdString& strDirectory);
  virtual void OnDirectoryScanned(const CStdString& strDirectory);
  virtual void OnFinished();
  virtual void OnStateChanged(SCAN_STATE state);

  CMusicInfoScanner m_musicInfoScanner;
  SCAN_STATE m_ScanState;
  CStdString m_strCurrentDir;

  CCriticalSection m_critical;
};
