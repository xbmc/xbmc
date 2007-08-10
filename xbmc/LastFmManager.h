#pragma once

#include "utils/Thread.h"
#include "playlist.h"

class CLastFmManager : CThread
{
public:
  virtual ~CLastFmManager();
  static void RemoveInstance();
  static CLastFmManager* GetInstance();

  //radio
  bool ChangeStation(const CURL& stationUrl);
  void StopRadio(bool bKillSession = true);
  void OnSongChange(bool bNewSongIsLastFm);
  bool IsRadioEnabled() {return !m_RadioSession.IsEmpty();}

  //scrobbler
  bool CanScrobble(const CFileItem &fileitem);
protected:
  virtual void Process();
  virtual void OnStartup();

private:
  CLastFmManager();
  static CLastFmManager* m_pInstance;

  bool RadioHandShake();
  bool RequestRadioTracks();
  void CacheTrackThumb(const int nrInitialTracksToAdd);
  void Parameter(const CStdString& key, const CStdString& data, CStdString& value);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void Update();
  void AddToPlaylist(const int nrTracks);

  CStdString m_RadioUrl;
  CStdString m_RadioSession;
  CStdString m_RadioBaseUrl;
  CStdString m_RadioBasePath;
  CStdString m_RadioSubscriber;
  CStdString m_RadioBanned;

  PLAYLIST::CPlayList m_RadioTrackQueue;
  HANDLE m_hWorkerEvent;
  CCriticalSection m_lockCache;
  CCriticalSection m_lockPlaylist;

  CGUIDialogProgress* dlgProgress;
  void InitProgressDialog(const CStdString& strUrl);
  void UpdateProgressDialog(const int iStringID);
  void CloseProgressDialog();
};
