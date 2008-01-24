#pragma once

#include "utils/Thread.h"
#include "PlayList.h"

class CLastFmManager : CThread
{
public:
  virtual ~CLastFmManager();
  static void RemoveInstance();
  static CLastFmManager* GetInstance();

  //radio
  bool ChangeStation(const CURL& stationUrl);
  void StopRadio(bool bKillSession = true);
  void OnSongChange(CFileItem& newSong);
  bool IsRadioEnabled() {return !m_RadioSession.IsEmpty();}
  bool IsLastFmEnabled();
  bool Love(bool askConfirmation = true);
  bool Ban(bool askConfirmation = true);
  bool CanLove();
  bool CanBan();
  bool Unlove(const CMusicInfoTag& musicinfotag, bool askConfirmation = true);
  bool Unban(const CMusicInfoTag& musicinfotag, bool askConfirmation = true);

  //scrobbler
  bool CanScrobble(const CFileItem &fileitem);
protected:
  virtual void Process();
  virtual void OnStartup();

private:
  typedef struct
  {
    CFileItem* CurrentSong;
    bool IsLoved;
    bool IsBanned;
  } LastFmManagerSong;

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
  bool CallXmlRpc(const CStdString& action, const CStdString& artist, const CStdString& title);
  void CreateMD5Hash(const CStdString& bufferToHash, CStdString& hash);
  bool Love(const CMusicInfoTag& musicinfotag);
  bool Ban(const CMusicInfoTag& musicinfotag);

  CStdString m_RadioUrl;
  CStdString m_RadioSession;
  CStdString m_RadioBaseUrl;
  CStdString m_RadioBasePath;
  CStdString m_RadioSubscriber;
  CStdString m_RadioBanned;

  LastFmManagerSong m_CurrentSong;

  PLAYLIST::CPlayList m_RadioTrackQueue;
  HANDLE m_hWorkerEvent;
  CCriticalSection m_lockCache;
  CCriticalSection m_lockPlaylist;

  CGUIDialogProgress* dlgProgress;
  void InitProgressDialog(const CStdString& strUrl);
  void UpdateProgressDialog(const int iStringID);
  void CloseProgressDialog();
};
