/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
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

#ifndef LIBSCROBBLER_H__
#define LIBSCROBBLER_H__

#include <vector>
#include "utils/StdString.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#define SCROBBLER_USER_ERROR_BADAUTH  1
#define SCROBBLER_USER_ERROR_BANNED   2

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

namespace XFILE
{
  class CCurlFile;
}

/* The following structure describes an entry in the scrobbler submission
 * journal.  Declare members added purely for convenience at the top and
 * members which will actually be read/written from/to disk under a
 * version number comment.  Also, remember to bump the version macro in
 * scrobbler.cpp.
 */
typedef struct SubmissionJournalEntry_s
{
  int        length;
  // v0
  CStdString strArtist;
  CStdString strAlbum;
  CStdString strTitle;
  CStdString strLength;  // Required if strSource below is "P"
  CStdString strStartTime;
  CStdString strMusicBrainzID;
  // v1
  CStdString strTrackNum;
  CStdString strSource;
  /* strSource must be one of the following.
   *  P: Chosen by the user (the most common value, unless you have a reason
   *     for choosing otherwise, use this).
   *  R: Non-personalised broadcast (e.g. Shoutcast, BBC Radio 1).
   *  E: Personalised recommendation except Last.fm (e.g. Pandora, Launchcast).
   *  L: Last.fm (any mode). In this case, the 5-digit Last.fm recommendation
   *     key must be appended to this source ID to prove the validity of the
   *     submission (for example, "o[0]=L1b48a").
   *  U: Source unknown.
   */
  CStdString strRating;
  /* strRating must be one of the following or empty.
   *  L: Love (on any mode if the user has manually loved the track). This
   *     implies a listen.
   *  B: Ban (only if source=L). This implies a skip, and the client should
   *     skip to the next track when a ban happens.
   *  S: Skip (only if source=L)
   *
   *  NOTE: This will eventually replace the love/ban web service.
   */
  SubmissionJournalEntry_s() {}
  SubmissionJournalEntry_s(const struct SubmissionJournalEntry_s& j)
  {
    strArtist         = j.strArtist;
    strAlbum          = j.strAlbum;
    strTitle          = j.strTitle;
    strLength         = j.strLength;
    strStartTime      = j.strStartTime;
    strMusicBrainzID  = j.strMusicBrainzID;
    strTrackNum       = j.strTrackNum;
    strSource         = j.strSource;
    strRating         = j.strRating;
    length            = j.length;
  }
  void Clear()
  {
    strArtist.clear();
    strAlbum.clear();
    strTitle.clear();
    strLength.clear();
    strStartTime.clear();
    strMusicBrainzID.clear();
    strTrackNum.clear();
    strSource = "P";
    strRating.clear();
    length = 0;
  }
} SubmissionJournalEntry;

typedef std::vector<SubmissionJournalEntry>::iterator SCROBBLERJOURNALITERATOR;

class CScrobbler : public CThread
{
protected:
  bool m_bNotified;
  bool m_bSubmitting;
  bool m_bSubmitted;
  bool m_bBanned;
  bool m_bBadAuth;
  int m_submissionTimer;
  int m_hardErrorCount;
  int m_failedHandshakeDelay;
  int m_action;
  time_t m_lastFailedHandshake;
  CStdString m_strLogPrefix;
  CStdString m_strUsername;
  CStdString m_strPasswordHash;
  CStdString m_strSessionID;
  CStdString m_strHandshakeURL;
  CStdString m_strNowPlayingURL;
  CStdString m_strSubmissionURL;
  CStdString m_strHandshakeTimeStamp;
  SubmissionJournalEntry m_CurrentTrack;
  CEvent m_hEvent;
  XFILE::CCurlFile  *m_pHttp;
  CCriticalSection  m_queueLock;
  CCriticalSection  m_actionLock;
  std::vector<SubmissionJournalEntry> m_vecSubmissionQueue;
private:
  void ResetState();
  void ClearErrorState();
  void ClearSubmissionState();
  void ClearSession();
  void HandleHardError();
  bool LoadJournal();
  bool SaveJournal();
  bool DoHandshake(time_t now);
  bool HandleHandshake(CStdString &strResponse);
  bool DoNowPlayingNotification();
  bool HandleNowPlayingNotification(CStdString &strResponse);
  bool DoSubmission();
  bool HandleSubmission(CStdString &strResponse);
  virtual void Process();  // Shouldn't need over ridden by inheriting CScrobblers
protected:
  virtual void NotifyUser(int error);
  virtual bool CanScrobble();
  virtual void LoadCredentials();
  virtual CStdString GetJournalFileName();

public:
  CScrobbler(const CStdString &strHandshakeURL, const CStdString &strLogPrefix = "CScrobbler");
  virtual ~CScrobbler();
  void Init();
  void Term();
  void AddSong(const MUSIC_INFO::CMusicInfoTag &tag, bool lastfmradio);
  void UpdateStatus();
  void SubmitQueue();
  void SetUsername(const CStdString &strUser);
  void SetPassword(const CStdString &strPass);
  CStdString GetConnectionState();
  CStdString GetSubmitInterval();
  CStdString GetFilesCached();
  CStdString GetSubmitState();
};

#endif // LIBSCROBBLER_H__
