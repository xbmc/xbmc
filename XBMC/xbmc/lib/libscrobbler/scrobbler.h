/*
    This file is part of libscrobbler. Modified for XBMC by Bobbin007

    libscrobbler is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libscrobbler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libscrobbler; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright © 2003 Russell Garrett (russ-scrobbler@garrett.co.uk)
*/
#ifndef _SCROBBLER_H
#define _SCROBBLER_H

#include <vector>
#include "utils\Thread.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

/**
  An enumeration of the status messages so that clients can
  do things with them, and not have to parse the text
  to work out what's going on.
  If you don't care about these, then you can continue to
  just override the char* only version of statusUpdate
*/
enum ScrobbleStatus
{
  S_SUBMITTING = 0,

  S_NOT_SUBMITTING,

  S_CONNECT_ERROR,
  
  S_HANDSHAKE_SUCCESS,
  S_HANDSHAKE_UP_TO_DATE,
  S_HANDSHAKE_OLD_CLIENT,
  S_HANDSHAKE_INVALID_RESPONSE,
  S_HANDSHAKE_BAD_USERNAME,
  S_HANDSHAKE_ERROR,
  S_HANDHAKE_NOTREADY,

  S_SUBMIT_SUCCESS,
  S_SUBMIT_INVALID_RESPONSE,
  S_SUBMIT_INTERVAL,
  S_SUBMIT_BAD_PASSWORD,
  S_SUBMIT_FAILED,
  S_SUBMIT_BADAUTH,

  S_DEBUG

};

typedef struct SubmissionJournalEntry_s {
  CStdString strArtist;
  CStdString strAlbum;
  CStdString strTitle;
  CStdString strLength;
  CStdString strStartTime;
  CStdString strMusicBrainzID;
  SubmissionJournalEntry_s() {};
  SubmissionJournalEntry_s(const struct SubmissionJournalEntry_s& j) {
    strArtist = j.strArtist;
    strAlbum = j.strAlbum;
    strTitle = j.strTitle;
    strLength = j.strLength;
    strStartTime = j.strStartTime;
    strMusicBrainzID = j.strMusicBrainzID;
  };
} SubmissionJournalEntry;

/**
  Audioscrobbler client class.
  $Id$

  @version  1.1
  @author   Russ Garrett (russ-scrobbler@garrett.co.uk)
*/

class CScrobbler : public CThread
{
private:
  /**
    Use this to initially set up the username and password. This sends a 
    handshake request to the server to determine the submit URL and 
    initial submit interval, and submissions will only be sent when 
    it recieves a response to this.
    
    @param user   The user's Audioscrobbler username.
    @param pass   The user's Audioscrobbler password.
    @see      setPassword()
    @see      setUsername()
  */
  CScrobbler();
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

public:
  virtual ~CScrobbler();

  static void RemoveInstance();
  static CScrobbler* GetInstance();
  /**
    Call this to add a song to the submission queue. The submission will get
    sent immediately unless the server has told it to cache, in which case it
    will get sent with the first song submission after the cache period has 
    expired. 
    
    @note Submission is not synchronous, so song submission status is only available through the statusUpdate callback.
    @note There's no way to tell the class to submit explicitly, it'll try to submit when you addSong.

    @param artist The artist name.
    @param title  The title of the song.
    @param length The length of the song in seconds.
    @param ltime  The time (unix time) in seconds when playback of the song started.
    @param trackid  The MusicBrainz TrackID of the song (in guid format).
    @see      statusUpdate()
    @retval 0   Failure. This could be because the song submitted was too short.
    @retval 1   This tripped song submission, Scrobbler is now connecting.
    @retval 2   The submission was cached.
  */
  int AddSong(const MUSIC_INFO::CMusicInfoTag& tag);

  /**
    Set the user's password if it's changed since the constructor.

    @param pass   The user's new password.
    @see      Scrobbler()
    @see      setUsername()
  */
  void SetPassword(const CStdString& strPass);

  //  should call just after contruction
  void Init();

  // should call juts before deletion
  void Term();


  /**
    Set the user's user name if it's changed since the constructor.

    @param user   The user's new user name.
    @see      Scrobbler()
    @see      setUsername()
  */
  void SetUsername(const CStdString& strUser);

  void SetSubmitSong(bool bSubmit);
  bool ShouldSubmit();
  void SetSongStartTime();

  CStdString GetConnectionState();
  CStdString GetSubmitInterval();
  CStdString GetFilesCached();
  void  SetSecsTillSubmit(int iSecs);
  CStdString GetSubmitState();
private:

  /**
    Override this to receive updates on the status of the scrobbler 
    client to display to the user. You don't have to override this,
    but it helps.
  
    If you do ovveride it, the old text only version won't
    work any more.

    @param status The status code.
    @param text   The text of the status update.
  */
  virtual void StatusUpdate(ScrobbleStatus status, const CStdString& strText);

  /**
    old status update - text only
    
    Override this to receive updates on the status of the scrobbler 
    client to display to the user. You don't have to override this,
    but it helps.

    @param text   The text of the status update.
  */
  virtual void StatusUpdate(const CStdString& strText);

  virtual void SetInterval(int in);

  /**
    Override this to save the cache - called from the destructor.

    @see        LoadJournal()
    @retval 1   You saved some cache.
    @retval 0   You didn't save cache, or can't.
  */
  virtual int SaveJournal();
  
  /**
    This is called when you should load the cache. Use setCache.

    @see      SaveJournal()
    @retval 1   You loaded some cache.
    @retval 0   There was no cache to load, or you can't.
  */
  virtual int LoadCache();
  virtual int LoadJournal();

  virtual void GenSessionKey();

  void DoSubmit();
  void DoHandshake();

   // call back functions from curl
  void HandleHandshake(char *handshake);
  void HandleSubmit(char *data);

  CStdString GetTempFileName();
  CStdString GetJournalFileName();

  CStdString m_strUserName;
  CStdString m_strPassword; // MD5 hash
  CStdString m_strChallenge;
  CStdString m_strSessionKey;
  CStdString m_strHsString;
  CStdString m_strSubmitUrl;
  CStdString m_strSubmit;
  CStdString m_strClientId;
  CStdString m_strClientVer;
  CStdString m_strPostString;
  
  bool m_bReadyToSubmit;

  bool m_bSubmitInProgress;

  bool m_bCloseThread;
  HANDLE m_hWorkerEvent;

  int m_iSongNum;

  time_t m_Interval;
  time_t m_LastConnect;

  time_t m_SongStartTime;

  bool m_bUpdateWarningDone;
  bool m_bConnectionWarningDone;
  bool m_bAuthWarningDone;
  bool m_bBadPassWarningDone;

  int m_iSecsTillSubmit;
  bool m_bShouldSubmit;
  bool m_bReHandShaking;
  
  std::vector<SubmissionJournalEntry> m_vecSubmissionJournal;

  static CScrobbler* m_pInstance;
};

#endif
