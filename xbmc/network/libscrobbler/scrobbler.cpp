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

#include "PlatformDefs.h"
#include "scrobbler.h"
#include "utils/md5.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "music/tags/MusicInfoTag.h"
#include "errors.h"
#include "threads/Atomics.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/XMLUtils.h"
#include "Application.h"
#include "threads/SingleLock.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "URL.h"

#define SCROBBLER_CLIENT              "xbm"
//#define SCROBBLER_CLIENT              "tst"     // For testing ONLY!
#define SCROBBLER_PROTOCOL            "1.2.1"
#define SCROBBLER_CLIENT_VERSION      "0.2"
#define SCROBBLER_JOURNAL_VERSION     1
#define SCROBBLER_MAX_SUBMISSIONS     50        // API rule
#define SCROBBLER_MIN_DURATION        30        // seconds. API rule
#define SCROBBLER_ACTION_SUBMIT       1
#define SCROBBLER_ACTION_NOWPLAYING   2

CScrobbler::CScrobbler(const CStdString &strHandshakeURL, const CStdString &strLogPrefix)
  : CThread("CScrobbler")
{ 
  m_bBanned         = false;
  m_bBadAuth        = false;
  m_pHttp           = NULL;
  m_strHandshakeURL = strHandshakeURL;
  m_strLogPrefix    = strLogPrefix;
  ResetState();
}

CScrobbler::~CScrobbler()
{
}

void CScrobbler::Init()
{
  if (!CanScrobble())
    return;
  ResetState();
  LoadCredentials();
  LoadJournal();
  if (!IsRunning())
    Create();
}

void CScrobbler::Term()
{
  StopThread();
  SaveJournal();
}

void CScrobbler::AddSong(const MUSIC_INFO::CMusicInfoTag &tag, bool lastfmradio)
{
  ClearSubmissionState();

  if (!CanScrobble() || !tag.Loaded())
    return;

  if (tag.GetArtist().empty() || tag.GetTitle().IsEmpty())
    return;

  // our tags are stored as UTF-8, so no conversion needed
  m_CurrentTrack.length           = tag.GetDuration();
  m_CurrentTrack.strArtist        = StringUtils::Join(tag.GetArtist(), g_advancedSettings.m_musicItemSeparator);
  m_CurrentTrack.strAlbum         = tag.GetAlbum();
  m_CurrentTrack.strTitle         = tag.GetTitle();
  m_CurrentTrack.strMusicBrainzID = tag.GetMusicBrainzTrackID();
  if (lastfmradio)  // TODO Set source more appropriately
    m_CurrentTrack.strSource        = "L" + tag.GetComment();
  else
    m_CurrentTrack.strSource        = "P";
  m_CurrentTrack.strRating        = "";
  m_CurrentTrack.strLength.Format("%d", m_CurrentTrack.length);
  m_CurrentTrack.strStartTime.Format("%d", time(NULL));
  m_CurrentTrack.strTrackNum.Format("%d",tag.GetTrackNumber());
  
  CURL::Encode(m_CurrentTrack.strArtist); 
  CURL::Encode(m_CurrentTrack.strTitle);
  CURL::Encode(m_CurrentTrack.strAlbum);
  CURL::Encode(m_CurrentTrack.strMusicBrainzID);

  m_bNotified = false;
  m_bSubmitted = !((lastfmradio && g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")) ||
      (!lastfmradio && g_guiSettings.GetBool("scrobbler.lastfmsubmit") && (m_CurrentTrack.length > SCROBBLER_MIN_DURATION || !m_CurrentTrack.strMusicBrainzID.IsEmpty())));
}

void CScrobbler::UpdateStatus()
{
  // Called from CApp::ProcessSlow() every ~500ms.
  if (!CanScrobble())
    return;
  if (g_application.IsPaused() || (g_application.GetPlaySpeed() != 1))
    return;

  m_submissionTimer++;

  // Try to notify Last.fm of our currently playing after ~5s of playback.
  // Don't try too hard, this is optional and doesn't affect the users library.
  if (!m_bNotified && m_submissionTimer >= 10)
  {
    m_bNotified = true; // Only try once
    {
      CSingleLock lock(m_actionLock);
      m_action = SCROBBLER_ACTION_NOWPLAYING;
    }
    m_hEvent.Set();
    return;
  }

  // Scrobble the track after 50% playback or 240s, whichever occurs first.
  // Just toss it in the queue here. We'll try to submit the queue at the
  // end of playback.
  if (!m_bSubmitted &&
      (m_submissionTimer > m_CurrentTrack.length || 
       m_submissionTimer >= 480))
  {
    CSingleLock lock(m_queueLock);
    m_bSubmitted = true;
    m_vecSubmissionQueue.push_back(m_CurrentTrack);
    lock.Leave(); 
    SaveJournal();
    CLog::Log(LOGDEBUG, "%s: Queued track for submission", m_strLogPrefix.c_str());
  }
}

void CScrobbler::SubmitQueue()
{
  if (CanScrobble())
  {
    {
      CSingleLock lock(m_actionLock);
      m_action = SCROBBLER_ACTION_SUBMIT;
    }
    m_hEvent.Set();
  }
}

void CScrobbler::SetUsername(const CStdString& strUser)
{
  if (strUser.IsEmpty())
    return;

  m_strUsername=strUser;
  CURL::Encode(m_strUsername);
  m_bBadAuth = false;
}

void CScrobbler::SetPassword(const CStdString& strPass)
{
  if (strPass.IsEmpty())
    return;
  m_strPasswordHash = strPass;
  m_strPasswordHash.ToLower();
  m_bBadAuth = false;
}

CStdString CScrobbler::GetConnectionState()
{
  if (!CanScrobble())
    return "";
  return (m_strSessionID.IsEmpty()) ?
    g_localizeStrings.Get(15208) : g_localizeStrings.Get(15207);
}

CStdString CScrobbler::GetSubmitInterval()
{
  CStdString strInterval;
  if (!CanScrobble())
    return strInterval;
  CStdString strFormat = g_localizeStrings.Get(15209);
  int seconds = m_CurrentTrack.length - m_submissionTimer/2;
  strInterval.Format(strFormat, std::max(seconds, m_failedHandshakeDelay));
  return strInterval;
}

CStdString CScrobbler::GetFilesCached()
{
  CStdString strCachedTracks;
  if (!CanScrobble())
    return strCachedTracks;
  CSingleLock lock(m_queueLock);
  CStdString strFormat = g_localizeStrings.Get(15210);
  strCachedTracks.Format(strFormat, m_vecSubmissionQueue.size());
  return strCachedTracks;
}

CStdString CScrobbler::GetSubmitState()
{
  CStdString strState;
  CStdString strFormat = g_localizeStrings.Get(15212);
  if (!CanScrobble())
    return strState;
  if (m_bSubmitting)
    strState = g_localizeStrings.Get(15211);
  else if (!g_application.IsPlayingAudio() || m_bBadAuth || m_bBanned)
    strState.Format(strFormat, 0);
  else if (m_strSessionID.IsEmpty())
    strState.Format(strFormat, m_failedHandshakeDelay);
  else
  {
    int seconds = m_CurrentTrack.length - m_submissionTimer/2;
    strState.Format(strFormat, std::max(0, seconds));
  }
  return strState;
}

void CScrobbler::ResetState()
{
  ClearSession();
  ClearSubmissionState();
  ClearErrorState();
}

void CScrobbler::ClearErrorState()
{
  m_hardErrorCount        = 0;
  m_lastFailedHandshake   = 0;
  m_failedHandshakeDelay  = 0;
}

void CScrobbler::ClearSubmissionState()
{
  m_bNotified             = true;  // Explicitly clear these when necessary
  m_bSubmitting           = false;
  m_bSubmitted            = true;
  m_submissionTimer       = 0;
  CSingleLock lock(m_actionLock);
  m_action                = 0;
}

void CScrobbler::ClearSession()
{
  CLog::Log(LOGDEBUG, "%s: Clearing session.", m_strLogPrefix.c_str());
  m_strSessionID.clear();
}

void CScrobbler::HandleHardError()
{
  CLog::Log(LOGDEBUG, "%s: A hard error has occurred.", m_strLogPrefix.c_str());
  if (++m_hardErrorCount == 3)
  {
    CLog::Log(LOGDEBUG, "%s: Three consecuetive hard errors have "\
        "occured. Forcing new handshake.", m_strLogPrefix.c_str());
    ClearSession();
  }
}

bool CScrobbler::LoadJournal()
{
  int                     journalVersion  = 0;
  SubmissionJournalEntry  entry;
  CXBMCTinyXML            xmlDoc;
  CStdString              JournalFileName = GetJournalFileName();
  CSingleLock             lock(m_queueLock);

  m_vecSubmissionQueue.clear();
  
  if (!xmlDoc.LoadFile(JournalFileName))
  {
    CLog::Log(LOGDEBUG, "%s: %s, Line %d (%s)", m_strLogPrefix.c_str(), 
        JournalFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRoot = xmlDoc.RootElement();
  if (strcmpi(pRoot->Value(), "asjournal") != 0)
  {
    CLog::Log(LOGDEBUG, "%s: %s missing <asjournal>", m_strLogPrefix.c_str(),
        JournalFileName.c_str());
    return false;
  }

  if (pRoot->Attribute("version"))
    journalVersion = atoi(pRoot->Attribute("version"));

  TiXmlNode *pNode = pRoot->FirstChild("entry");
  for (; pNode; pNode = pNode->NextSibling("entry"))
  {
    entry.Clear();
    XMLUtils::GetString(pNode, "artist", entry.strArtist);
    XMLUtils::GetString(pNode, "album", entry.strAlbum);
    XMLUtils::GetString(pNode, "title", entry.strTitle);
    XMLUtils::GetString(pNode, "length", entry.strLength);
    entry.length = atoi(entry.strLength.c_str());
    XMLUtils::GetString(pNode, "starttime", entry.strStartTime);
    XMLUtils::GetString(pNode, "musicbrainzid", entry.strMusicBrainzID);

    if (journalVersion > 0)
    {
      XMLUtils::GetString(pNode, "tracknum", entry.strTrackNum);
      XMLUtils::GetString(pNode, "source", entry.strSource);
      XMLUtils::GetString(pNode, "rating", entry.strRating);
    }
    else
    {
      // Update from journal v0
      // Convert start time stamp
      struct tm starttm;
      time_t startt;
      if (!strptime(entry.strStartTime.c_str(), "%Y-%m-%d %H:%M:%S", &starttm))
        continue;
      if ((startt = mktime(&starttm)) == -1)
        continue;
      entry.strStartTime.Format("%d", startt);
      // url encode entries
      CURL::Encode(entry.strArtist); 
      CURL::Encode(entry.strTitle);
      CURL::Encode(entry.strAlbum);
      CURL::Encode(entry.strMusicBrainzID);
    }
    m_vecSubmissionQueue.push_back(entry);
  }

  CLog::Log(LOGDEBUG, "%s: Journal loaded with %"PRIuS" entries.", m_strLogPrefix.c_str(),
      m_vecSubmissionQueue.size());
  return !m_vecSubmissionQueue.empty();
}

bool CScrobbler::SaveJournal()
{
  CSingleLock lock(m_queueLock);

  if (m_vecSubmissionQueue.size() == 0)
  {
    if (XFILE::CFile::Exists(GetJournalFileName()))
      XFILE::CFile::Delete(GetJournalFileName());
    return true;
  }
  CStdString        strJournalVersion;
  CXBMCTinyXML      xmlDoc;
  TiXmlDeclaration  decl("1.0", "utf-8", "yes");
  TiXmlElement      xmlRootElement("asjournal");
  xmlDoc.InsertEndChild(decl);
  strJournalVersion.Format("%d", SCROBBLER_JOURNAL_VERSION);
  xmlRootElement.SetAttribute("version", strJournalVersion.c_str());
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot)
    return false;

  int i = 0;
  SCROBBLERJOURNALITERATOR it = m_vecSubmissionQueue.begin();
  for (; it != m_vecSubmissionQueue.end(); it++, i++)
  {
    TiXmlElement entryNode("entry");
    TiXmlNode *pNode = pRoot->InsertEndChild(entryNode);
    if (!pNode)
      return false;
    XMLUtils::SetString(pNode, "artist", it->strArtist);
    XMLUtils::SetString(pNode, "album", it->strAlbum);
    XMLUtils::SetString(pNode, "title", it->strTitle);
    XMLUtils::SetString(pNode, "length", it->strLength);
    XMLUtils::SetString(pNode, "starttime", it->strStartTime);
    XMLUtils::SetString(pNode, "musicbrainzid", it->strMusicBrainzID);
    XMLUtils::SetString(pNode, "tracknum", it->strTrackNum);
    XMLUtils::SetString(pNode, "source", it->strSource);
    XMLUtils::SetString(pNode, "rating", it->strRating);
  }
  lock.Leave();

  CStdString FileName = GetJournalFileName();
  CLog::Log(LOGDEBUG, "%s: Journal with %d entries saved to %s",
      m_strLogPrefix.c_str(), i, FileName.c_str());
  return xmlDoc.SaveFile(FileName);
}

bool CScrobbler::DoHandshake(time_t now)
{
  XBMC::XBMC_MD5    authToken;
  CStdString        strAuthToken;
  CStdString        strTimeStamp;
  CStdString        strResponse;
  CStdString        strHandshakeRequest;

  // Create auth token. md5(md5(pass)+str(now))
  strTimeStamp.Format("%d", now);
  authToken.append(m_strPasswordHash + strTimeStamp);
  authToken.getDigest(strAuthToken);
  strAuthToken.ToLower();
  
  // Construct handshake URL.
  strHandshakeRequest.Format("http://%s/?hs=true"\
      "&p=%s&c=%s&v=%s&u=%s&t=%d&a=%s", m_strHandshakeURL.c_str(),
      SCROBBLER_PROTOCOL, SCROBBLER_CLIENT, SCROBBLER_CLIENT_VERSION,
      m_strUsername.c_str(), now, strAuthToken.c_str());
  
  // Make and handle request
  if (m_pHttp->Get(strHandshakeRequest, strResponse) &&
      HandleHandshake(strResponse))
    return true;
    
  m_failedHandshakeDelay = // 60, 120, 240, ... 7200s
    (m_failedHandshakeDelay) ? std::min(2*m_failedHandshakeDelay, 7200) : 60;
  m_lastFailedHandshake = now;
  if (!m_bBanned && !m_bBadAuth)
    CLog::Log(LOGDEBUG, "%s: A hard error has occurred during "\
        "handshake. Sleeping for %d minutes.",
        m_strLogPrefix.c_str(), m_failedHandshakeDelay/60);
  
  return false;
}

bool CScrobbler::HandleHandshake(CStdString &strResponse)
{
  if (strResponse.IsEmpty())
    return false;
  
  std::vector<CStdString> vecTokens;
  CUtil::Tokenize(strResponse, vecTokens, " \n\r");

  if (vecTokens[0] == "OK")
  {
    if (vecTokens.size() >= 4)
    {
      m_strSessionID      = vecTokens[1];
      m_strNowPlayingURL  = vecTokens[2];
      m_strSubmissionURL  = vecTokens[3];
      CLog::Log(LOGDEBUG, "%s: Handshake succeeded!", m_strLogPrefix.c_str());
      CLog::Log(LOGDEBUG, "%s: SessionID is %s", m_strLogPrefix.c_str(),
          m_strSessionID.c_str());
      CLog::Log(LOGDEBUG, "%s: NP URL is %s", m_strLogPrefix.c_str(),
          m_strNowPlayingURL.c_str());
      CLog::Log(LOGDEBUG, "%s: Submit URL is %s", m_strLogPrefix.c_str(),
          m_strSubmissionURL.c_str());
      ClearErrorState();
      return true;
    }
    CLog::Log(LOGERROR, "%s: Handshake failed! Received malformed "\
        "reply.", m_strLogPrefix.c_str());
  }
  else if (vecTokens[0] == "BANNED")
  {
    CLog::Log(LOGERROR, "%s: Handshake failed! Client is banned! "\
        "Disabling submissions. Subsequent scrobbles will be cached. "\
        "Please update your client to the newest version. ", m_strLogPrefix.c_str());
    if (m_failedHandshakeDelay == 0)
    {
      NotifyUser(SCROBBLER_USER_ERROR_BANNED);
    }
  }
  else if (vecTokens[0] == "BADAUTH")
  {
    CLog::Log(LOGERROR, "%s: Handshake failed! Authentication failed! "\
        "Disabling submissions. Subsequent scrobbles will be cached. "\
        "Please enter the correct credentials to re-enable scrobbling. ",
        m_strLogPrefix.c_str());
    if (m_failedHandshakeDelay == 0)
    {
      NotifyUser(SCROBBLER_USER_ERROR_BADAUTH);
    }
  }
  else if (vecTokens[0] == "BADTIME")
  {
    CLog::Log(LOGDEBUG, "%s: Handshake failed! Timestamp is invalid! "\
        "Disabling submissions. Subsequent scrobbles will be cached. "\
        "Please correct the system time and restart the application. ",
        m_strLogPrefix.c_str());
  }
  else if (vecTokens[0] == "FAILED")
  {
    CLog::Log(LOGDEBUG, "%s: Handshake failed! REASON: %s! ", m_strLogPrefix.c_str(), 
        strResponse.c_str());
  }
  else
    CLog::Log(LOGDEBUG, "%s: Handshake failed! REASON: Unspecified!", m_strLogPrefix.c_str());
  
  return false;
}

bool CScrobbler::DoNowPlayingNotification()
{
  CStdString        strNowPlayingRequest;
  CStdString        strResponse;

  // Construct now playing notification URL.
  strNowPlayingRequest.Format("s=%s&a=%s&t=%s&b=%s&l=%d&n=%s&m=%s",
      m_strSessionID.c_str(), m_CurrentTrack.strArtist.c_str(),
      m_CurrentTrack.strTitle.c_str(), m_CurrentTrack.strAlbum.c_str(),
      m_CurrentTrack.length, m_CurrentTrack.strTrackNum.c_str(),
      m_CurrentTrack.strMusicBrainzID.c_str());

  // Make and handle request
  if (m_pHttp->Post(m_strNowPlayingURL, strNowPlayingRequest, strResponse) &&
      HandleNowPlayingNotification(strResponse))
    return true;
  
  HandleHardError();
  return false;
}

bool CScrobbler::HandleNowPlayingNotification(CStdString &strResponse)
{
  if (strResponse.IsEmpty())
    return false;
 
  std::vector<CStdString> vecTokens;
  CUtil::Tokenize(strResponse, vecTokens, " \n\r");

  if (vecTokens[0] == "OK")
  {
    CLog::Log(LOGDEBUG, "%s: Now playing notification succeeded!", m_strLogPrefix.c_str());
    ClearErrorState();
    return true;
  }
  else if (vecTokens[0] == "BADSESSION")
  {
    CLog::Log(LOGDEBUG, "%s: Now playing notification failed! "\
        "REASON: Bad session ID. Forcing new handshake.", m_strLogPrefix.c_str());
    ClearSession();
  }
  else
    CLog::Log(LOGDEBUG, "%s: Now playing notification failed! "\
        "REASON: Unspecified.", m_strLogPrefix.c_str());

  return false;
}

bool CScrobbler::DoSubmission()
{
  int               i;
  int               numSubmissions;
  CStdString        strSubmissionRequest;
  CStdString        strSubmission;
  CStdString        strResponse;
  CSingleLock lock(m_queueLock);

  // Construct submission URL.
  numSubmissions = 
    std::min((size_t)SCROBBLER_MAX_SUBMISSIONS, m_vecSubmissionQueue.size());
  if (numSubmissions == 0)
    return true;
  strSubmissionRequest.Format("s=%s", m_strSessionID.c_str());
  SCROBBLERJOURNALITERATOR it = m_vecSubmissionQueue.begin();
  for (i = 0; it != m_vecSubmissionQueue.end() && i < numSubmissions; i++,it++)
  {
    strSubmission.Format("&a[%d]=%s&t[%d]=%s&i[%d]=%s&o[%d]=%s&r[%d]=%s",
        i, it->strArtist.c_str(),     i, it->strTitle.c_str(),
        i, it->strStartTime.c_str(),  i, it->strSource.c_str(),
        i, it->strRating.c_str());
    // Too many params, must be split (or hack CStdString)
    strSubmission.Format("%s&l[%d]=%s&b[%d]=%s&n[%d]=%s&m[%d]=%s",
        strSubmission.c_str(),        i, it->strLength.c_str(),
        i, it->strAlbum.c_str(),      i, it->strTrackNum.c_str(),
        i, it->strMusicBrainzID.c_str());
    strSubmissionRequest += strSubmission;
  }
  
  // Make and handle request
  lock.Leave();
  if (m_pHttp->Post(m_strSubmissionURL, strSubmissionRequest, strResponse) &&
      HandleSubmission(strResponse))
  {
    lock.Enter();
    SCROBBLERJOURNALITERATOR it = m_vecSubmissionQueue.begin();
    m_vecSubmissionQueue.erase(it, it + i); // Remove submitted entries
    lock.Leave();
    SaveJournal();
    return true;
  }

  HandleHardError();
  return false;
}

bool CScrobbler::HandleSubmission(CStdString &strResponse)
{
  if (strResponse.IsEmpty())
    return false;
  
  std::vector<CStdString> vecTokens;
  CUtil::Tokenize(strResponse, vecTokens, " \n\r");

  if (vecTokens[0] == "OK")
  {
    CLog::Log(LOGDEBUG, "%s: Submission succeeded!", m_strLogPrefix.c_str());
    ClearErrorState();
    return true;
  }
  else if (vecTokens[0] == "BADSESSION")
  {
    CLog::Log(LOGDEBUG, "%s: Submission failed! "\
        "REASON: Bad session. Forcing new handshake.", m_strLogPrefix.c_str());
    ClearSession();
  }
  else if (vecTokens[0] == "FAILED")
  {
    CLog::Log(LOGDEBUG, "%s: Submission failed! "\
        "REASON: %s", m_strLogPrefix.c_str(), strResponse.c_str());
  }
  else
    CLog::Log(LOGDEBUG, "%s: Submission failed! "\
        "REASON: Unspecified.", m_strLogPrefix.c_str());

  return false;
}

void CScrobbler::Process()
{
  CLog::Log(LOGDEBUG, "%s: Thread started.", m_strLogPrefix.c_str());
  if (!m_pHttp)
  {
    // Hack since CCurlFile isn't threadsafe
    if (!(m_pHttp = new XFILE::CCurlFile))
      return;
  }
  while (!m_bStop)
  {
    AbortableWait(m_hEvent);
    if (m_bStop)
      break;
    
    if (m_strSessionID.IsEmpty())
    {
      time_t now = time(NULL);
      // We need to handshake.
      if (m_bBanned || m_bBadAuth ||
          ((now - m_lastFailedHandshake) < m_failedHandshakeDelay))
        continue;
      if (!DoHandshake(now))
        continue;
    }
    int action = 0;
    {
      CSingleLock lock(m_actionLock);
      action = m_action;
      m_action = 0;
    }
    if (action == SCROBBLER_ACTION_NOWPLAYING)
      DoNowPlayingNotification();
    else if (action == SCROBBLER_ACTION_SUBMIT)
    {
      m_bSubmitting = true;
      DoSubmission();
      m_bSubmitting = false;
    }
  }
  delete m_pHttp; // More of aforementioned hack 
  m_pHttp = NULL;
  CLog::Log(LOGDEBUG, "%s: Thread ended.", m_strLogPrefix.c_str());
}

void CScrobbler::NotifyUser(int error)
{
}

bool CScrobbler::CanScrobble()
{
  return false;
}

void CScrobbler::LoadCredentials()
{
  SetUsername("");
  SetPassword("");
}

CStdString CScrobbler::GetJournalFileName()
{
  return "";
}

