/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "system.h"

#if defined (HAS_OMXPLAYER)

#include <sstream>
#include <iomanip>

#include "OMXPlayerAudio.h"
#include "OMXPlayerVideo.h"
#include "OMXPlayer.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "windowing/WindowingFactory.h"

#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/StreamDetails.h"
#include "xbmc/playlists/PlayListM3U.h"

#include "utils/LangCodeExpander.h"
#include "guilib/LocalizeStrings.h"

#include "storage/MediaManager.h"
#include "GUIUserMessages.h"
#include "utils/StreamUtils.h"

#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/DVDInputStreamTV.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"

#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"

#include "DVDFileInfo.h"

#include "Util.h"
#include "LangInfo.h"

#include "utils/JobManager.h"
//#include "cores/AudioEngine/AEFactory.h"
//#include "cores/AudioEngine/Utils/AEUtil.h"
#include "video/VideoThumbLoader.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "filesystem/PVRFile.h"

#include "utils/StringUtils.h"

using namespace XFILE;
using namespace PVR;

// ****************************************************************
void COMXSelectionStreams::Clear(StreamType type, StreamSource source)
{
  CSingleLock lock(m_section);
  for(int i=m_Streams.size()-1;i>=0;i--)
  {
    if(type && m_Streams[i].type != type)
      continue;

    if(source && m_Streams[i].source != source)
      continue;

    m_Streams.erase(m_Streams.begin() + i);
  }
}

void COMXPlayer::GetAudioStreamLanguage(int iStream, CStdString &strLanguage)
{
  strLanguage = "";
  OMXSelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, iStream);
  if(s.language.length() > 0)
    strLanguage = s.language;
}

OMXSelectionStream& COMXSelectionStreams::Get(StreamType type, int index)
{
  CSingleLock lock(m_section);
  int count = -1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    if(m_Streams[i].type != type)
      continue;
    count++;
    if(count == index)
      return m_Streams[i];
  }
  CLog::Log(LOGERROR, "%s - failed to get stream", __FUNCTION__);
  return m_invalid;
}

std::vector<OMXSelectionStream> COMXSelectionStreams::Get(StreamType type)
{
  std::vector<OMXSelectionStream> streams;
  int count = Count(type);
  for(int index = 0; index < count; ++index){
    streams.push_back(Get(type, index));
  }
  return streams;
}

#define PREDICATE_RETURN(lh, rh) \
  do { \
    if((lh) != (rh)) \
      return (lh) > (rh); \
  } while(0)

static bool PredicateAudioPriority(const OMXSelectionStream& lh, const OMXSelectionStream& rh)
{
  PREDICATE_RETURN(lh.type_index == g_settings.m_currentVideoSettings.m_AudioStream
                 , rh.type_index == g_settings.m_currentVideoSettings.m_AudioStream);

  if(!g_guiSettings.GetString("locale.audiolanguage").Equals("original"))
  {
    CStdString audio_language = g_langInfo.GetAudioLanguage();
    PREDICATE_RETURN(audio_language.Equals(lh.language.c_str())
                   , audio_language.Equals(rh.language.c_str()));
  }

  PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                 , rh.flags & CDemuxStream::FLAG_DEFAULT);

  PREDICATE_RETURN(lh.channels
                 , rh.channels);

  PREDICATE_RETURN(StreamUtils::GetCodecPriority(lh.codec)
                 , StreamUtils::GetCodecPriority(rh.codec));
  return false;
}

static bool PredicateSubtitlePriority(const OMXSelectionStream& lh, const OMXSelectionStream& rh)
{
  if(!g_settings.m_currentVideoSettings.m_SubtitleOn)
  {
    PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_FORCED
                   , rh.flags & CDemuxStream::FLAG_FORCED);
  }

  PREDICATE_RETURN(lh.type_index == g_settings.m_currentVideoSettings.m_SubtitleStream
                 , rh.type_index == g_settings.m_currentVideoSettings.m_SubtitleStream);

  CStdString subtitle_language = g_langInfo.GetSubtitleLanguage();
  if(!g_guiSettings.GetString("locale.subtitlelanguage").Equals("original"))
  {
    PREDICATE_RETURN((lh.source == STREAM_SOURCE_DEMUX_SUB || lh.source == STREAM_SOURCE_TEXT) && subtitle_language.Equals(lh.language.c_str())
                   , (rh.source == STREAM_SOURCE_DEMUX_SUB || rh.source == STREAM_SOURCE_TEXT) && subtitle_language.Equals(rh.language.c_str()));
  }

  PREDICATE_RETURN(lh.source == STREAM_SOURCE_DEMUX_SUB
                 , rh.source == STREAM_SOURCE_DEMUX_SUB);

  PREDICATE_RETURN(lh.source == STREAM_SOURCE_TEXT
                 , rh.source == STREAM_SOURCE_TEXT);

  if(!g_guiSettings.GetString("locale.subtitlelanguage").Equals("original"))
  {
    PREDICATE_RETURN(subtitle_language.Equals(lh.language.c_str())
                   , subtitle_language.Equals(rh.language.c_str()));
  }

  PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                 , rh.flags & CDemuxStream::FLAG_DEFAULT);

  return false;
}

static bool PredicateVideoPriority(const OMXSelectionStream& lh, const OMXSelectionStream& rh)
{
  PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                 , rh.flags & CDemuxStream::FLAG_DEFAULT);
  return false;
}

bool COMXSelectionStreams::Get(StreamType type, CDemuxStream::EFlags flag, OMXSelectionStream& out)
{
  CSingleLock lock(m_section);
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    if(m_Streams[i].type != type)
      continue;
    if((m_Streams[i].flags & flag) != flag)
      continue;
    out = m_Streams[i];
    return true;
  }
  return false;
}

int COMXSelectionStreams::IndexOf(StreamType type, int source, int id) const
{
  CSingleLock lock(m_section);
  int count = -1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    if(type && m_Streams[i].type != type)
      continue;
    count++;
    if(source && m_Streams[i].source != source)
      continue;
    if(id < 0)
      continue;
    if(m_Streams[i].id == id)
      return count;
  }
  if(id < 0)
    return count;
  else
    return -1;
}

int COMXSelectionStreams::IndexOf(StreamType type, COMXPlayer& p) const
{
  if (p.m_pInputStream && p.m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    int id = -1;
    if(type == STREAM_AUDIO)
      id = ((CDVDInputStreamNavigator*)p.m_pInputStream)->GetActiveAudioStream();
    else if(type == STREAM_VIDEO)
      id = p.m_CurrentVideo.id;
    else if(type == STREAM_SUBTITLE)
      id = ((CDVDInputStreamNavigator*)p.m_pInputStream)->GetActiveSubtitleStream();

    return IndexOf(type, STREAM_SOURCE_NAV, id);
  }

  if(type == STREAM_AUDIO)
    return IndexOf(type, p.m_CurrentAudio.source, p.m_CurrentAudio.id);
  else if(type == STREAM_VIDEO)
    return IndexOf(type, p.m_CurrentVideo.source, p.m_CurrentVideo.id);
  else if(type == STREAM_SUBTITLE)
    return IndexOf(type, p.m_CurrentSubtitle.source, p.m_CurrentSubtitle.id);
  else if(type == STREAM_TELETEXT)
    return IndexOf(type, p.m_CurrentTeletext.source, p.m_CurrentTeletext.id);

  return -1;
}

int COMXSelectionStreams::Source(StreamSource source, std::string filename)
{
  CSingleLock lock(m_section);
  int index = source - 1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    OMXSelectionStream &s = m_Streams[i];
    if(STREAM_SOURCE_MASK(s.source) != source)
      continue;
    // if it already exists, return same
    if(s.filename == filename)
      return s.source;
    if(index < s.source)
      index = s.source;
  }
  // return next index
  return index + 1;
}

void COMXSelectionStreams::Update(OMXSelectionStream& s)
{
  CSingleLock lock(m_section);
  int index = IndexOf(s.type, s.source, s.id);
  if(index >= 0)
  {
    OMXSelectionStream& o = Get(s.type, index);
    s.type_index = o.type_index;
    o = s;
  }
  else
  {
    s.type_index = Count(s.type);
    m_Streams.push_back(s);
  }
}

void COMXSelectionStreams::Update(CDVDInputStream* input, CDVDDemux* demuxer)
{
  if(input && input->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* nav = (CDVDInputStreamNavigator*)input;
    string filename = nav->GetFileName();
    int source = Source(STREAM_SOURCE_NAV, filename);

    int count;
    count = nav->GetAudioStreamCount();
    for(int i=0;i<count;i++)
    {
      OMXSelectionStream s;
      s.source   = source;
      s.type     = STREAM_AUDIO;
      s.id       = i;
      s.name     = nav->GetAudioStreamLanguage(i);
      s.flags    = CDemuxStream::FLAG_NONE;
      s.filename = filename;
      s.channels = 0;
      Update(s);
    }

    count = nav->GetSubTitleStreamCount();
    for(int i=0;i<count;i++)
    {
      OMXSelectionStream s;
      s.source   = source;
      s.type     = STREAM_SUBTITLE;
      s.id       = i;
      s.name     = nav->GetSubtitleStreamLanguage(i);
      s.flags    = CDemuxStream::FLAG_NONE;
      s.filename = filename;
      s.channels = 0;
      Update(s);
    }
  }
  else if(demuxer)
  {
    string filename = demuxer->GetFileName();
    int count = demuxer->GetNrOfStreams();
    int source;
    if(input) /* hack to know this is sub decoder */
      source = Source(STREAM_SOURCE_DEMUX, filename);
    else
      source = Source(STREAM_SOURCE_DEMUX_SUB, filename);


    for(int i=0;i<count;i++)
    {
      CDemuxStream* stream = demuxer->GetStream(i);
      /* make sure stream is marked with right source */
      stream->source = source;

      OMXSelectionStream s;
      s.source   = source;
      s.type     = stream->type;
      s.id       = stream->iId;
      s.language = stream->language;
      s.flags    = stream->flags;
      s.filename = demuxer->GetFileName();
      stream->GetStreamName(s.name);
      CStdString codec;
      demuxer->GetStreamCodecName(stream->iId, codec);
      s.codec    = codec;
      s.channels = 0; // Default to 0. Overwrite if STREAM_AUDIO below.
      if(stream->type == STREAM_AUDIO)
      {
        std::string type;
        ((CDemuxStreamAudio*)stream)->GetStreamType(type);
        if(type.length() > 0)
        {
          if(s.name.length() > 0)
            s.name += " - ";
          s.name += type;
        }
        s.channels = ((CDemuxStreamAudio*)stream)->iChannels;
      }
      Update(s);
    }
  }
}

// ****************************************************************
COMXPlayer::COMXPlayer(IPlayerCallback &callback) 
    : IPlayer(callback),
      CThread("COMXPlayer"),
      m_ready(true),
      m_CurrentAudio(STREAM_AUDIO, DVDPLAYER_AUDIO),
      m_CurrentVideo(STREAM_VIDEO, DVDPLAYER_VIDEO),
      m_CurrentSubtitle(STREAM_SUBTITLE, DVDPLAYER_SUBTITLE),
      m_CurrentTeletext(STREAM_TELETEXT, DVDPLAYER_TELETEXT),
      m_player_video(&m_av_clock, &m_overlayContainer, m_messenger),
      m_player_audio(&m_av_clock, m_messenger),
      m_player_subtitle(&m_overlayContainer),
      m_messenger("player")
{
  m_bAbortRequest     = false;
  m_pDemuxer          = NULL;
  m_pSubtitleDemuxer  = NULL;
  m_pInputStream      = NULL;
  m_UpdateApplication = 0;
  m_caching           = CACHESTATE_DONE;
  m_playSpeed         = DVD_PLAYSPEED_NORMAL;

  m_State.Clear();
  m_dvd.Clear();
  m_EdlAutoSkipMarkers.Clear();

  memset(&m_SpeedState, 0, sizeof(m_SpeedState));
}

COMXPlayer::~COMXPlayer()
{
  CloseFile();
}

bool COMXPlayer::OpenFile(const CFileItem &file, const CPlayerOptions &options)
{
  try
  {
    CLog::Log(LOGNOTICE, "COMXPlayer: Opening: %s", file.GetPath().c_str());
    // if playing a file close it first
    // this has to be changed so we won't have to close it.
    if(IsRunning())
      CloseFile();

    m_playSpeed = DVD_PLAYSPEED_NORMAL;
    SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    m_PlayerOptions     = options;
    m_bAbortRequest     = false;

    m_UpdateApplication = 0;
    m_offset_pts        = 0;
    m_current_volume    = 0;
    m_change_volume     = true;

    m_item              = file;
    m_mimetype          = file.GetMimeType();
    m_filename          = file.GetPath();

    m_State.Clear();

    m_ready.Reset();

    g_renderManager.PreInit();

    Create();

    if(!m_ready.WaitMSec(100))
    {
      CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
      if(dialog)
      {
        dialog->Show();
        while(!m_ready.WaitMSec(1))
          g_windowManager.ProcessRenderLoop(false);
        dialog->Close();
      }
    }

    // Playback might have been stopped due to some error
    if (m_bStop || m_bAbortRequest)
      return false;

    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown on open", __FUNCTION__);
    return false;
  }
}

bool COMXPlayer::CloseFile()
{
  CLog::Log(LOGDEBUG, "COMXPlayer::CloseFile");

  // unpause the player
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  if(m_pSubtitleDemuxer)
    m_pSubtitleDemuxer->Abort();

  CLog::Log(LOGDEBUG, "COMXPlayer: waiting for threads to exit");
  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();
  
  CLog::Log(LOGDEBUG, "COMXPlayer: finished waiting");

  m_Edl.Clear();
  m_EdlAutoSkipMarkers.Clear();

  g_renderManager.UnInit();
  return true;
}

bool COMXPlayer::IsPlaying() const
{
  return !m_bStop;
}

void COMXPlayer::OnStartup()
{
  m_CurrentVideo.Clear();
  m_CurrentAudio.Clear();
  m_CurrentSubtitle.Clear();

  m_messenger.Init();

  CUtil::ClearTempFonts();
}

bool COMXPlayer::OpenInputStream()
{
  if(m_pInputStream)
    SAFE_DELETE(m_pInputStream);

  CLog::Log(LOGNOTICE, "Creating InputStream");

  // correct the filename if needed
  CStdString filename(m_filename);
  if (filename.Find("dvd://") == 0
  ||  filename.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
  {
    m_filename = g_mediaManager.TranslateDevicePath("");
  }
retry:
  // before creating the input stream, if this is an HLS playlist then get the
  // most appropriate bitrate based on our network settings
  if (filename.Left(7) == "http://" && filename.Right(5) == ".m3u8")
  {
    // get the available bandwidth (as per user settings)
    int maxrate = g_guiSettings.GetInt("network.bandwidth");
    if(maxrate <= 0)
      maxrate = INT_MAX;

    // determine the most appropriate stream
    m_filename = PLAYLIST::CPlayListM3U::GetBestBandwidthStream(m_filename, (size_t)maxrate);
  }

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename, m_mimetype);
  if(m_pInputStream == NULL)
  {
    CLog::Log(LOGERROR, "COMXPlayer::OpenInputStream - unable to create input stream for [%s]", m_filename.c_str());
    return false;
  }
  else
    m_pInputStream->SetFileItem(m_item);

  if (!m_pInputStream->Open(m_filename.c_str(), m_mimetype))
  {
    if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      CLog::Log(LOGERROR, "COMXPlayer::OpenInputStream - failed to open [%s] as DVD ISO, trying Bluray", m_filename.c_str());
      m_mimetype = "bluray/iso";
      filename = m_filename;
      filename = filename + "/BDMV/index.bdmv";
      int title = (int)m_item.GetProperty("BlurayStartingTitle").asInteger();
      if( title )
        filename.AppendFormat("?title=%d",title);

      m_filename = filename;
      goto retry;
    }
    CLog::Log(LOGERROR, "COMXPlayer::OpenInputStream - error opening [%s]", m_filename.c_str());
    return false;
  }

  if (m_pInputStream && ( m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
                       || m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY) ) )
  {
    CLog::Log(LOGINFO, "COMXPlayer::OpenInputStream - DVD/BD not supported - Will try...");
    // return false;
  }

  // find any available external subtitles for non dvd files
  if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_HTSP))
  {
    // find any available external subtitles
    std::vector<CStdString> filenames;
    CUtil::ScanForExternalSubtitles( m_filename, filenames );

    // find any upnp subtitles
    CStdString key("upnp:subtitle:1");
    for(unsigned s = 1; m_item.HasProperty(key); key.Format("upnp:subtitle:%u", ++s))
      filenames.push_back(m_item.GetProperty(key).asString());

    for(unsigned int i=0;i<filenames.size();i++)
    {
      // if vobsub subtitle:    
      if (URIUtils::GetExtension(filenames[i]) == ".idx")
      {
        CStdString strSubFile;
        if ( CUtil::FindVobSubPair( filenames, filenames[i], strSubFile ) )
          AddSubtitleFile(filenames[i], strSubFile);
      }
      else
      {
        if ( !CUtil::IsVobSub(filenames, filenames[i] ) )
        {
          AddSubtitleFile(filenames[i]);
        }
      }
    } // end loop over all subtitle files    

    g_settings.m_currentVideoSettings.m_SubtitleCached = true;
  }

  SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
  SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
  m_av_clock.Reset();
  m_av_clock.OMXReset();
  m_dvd.Clear();
  m_iChannelEntryTimeOut = 0;

  return true;
}

bool COMXPlayer::OpenDemuxStream()
{
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);

  CLog::Log(LOGNOTICE, "Creating Demuxer");

  try
  {
    int attempts = 10;
    while(!m_bStop && attempts-- > 0)
    {
      m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
      if(!m_pDemuxer && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
      {
        continue;
      }
      else if(!m_pDemuxer && m_pInputStream->NextStream() != CDVDInputStream::NEXTSTREAM_NONE)
      {
        CLog::Log(LOGDEBUG, "%s - New stream available from input, retry open", __FUNCTION__);
        continue;
      }
      break;
    }

    if(!m_pDemuxer)
    {
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }

  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    return false;
  }

  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

  int64_t len = m_pInputStream->GetLength();
  int64_t tim = m_pDemuxer->GetStreamLength();
  if(len > 0 && tim > 0)
    m_pInputStream->SetReadRate(len * 1000 / tim);

  return true;
}

void COMXPlayer::OpenDefaultStreams()
{
  // bypass for DVDs. The DVD Navigator has already dictated which streams to open.
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return;

  OMXSelectionStreams streams;
  bool valid;

  // open video stream
  streams = m_SelectionStreams.Get(STREAM_VIDEO, PredicateVideoPriority);
  valid   = false;
  for(OMXSelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenVideoStream(it->id, it->source))
      valid = true;;
  }
  if(!valid)
    CloseVideoStream(true);

  // open audio stream
  if(m_PlayerOptions.video_only)
    streams.clear();
  else
    streams = m_SelectionStreams.Get(STREAM_AUDIO, PredicateAudioPriority);
  valid   = false;

  for(OMXSelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenAudioStream(it->id, it->source))
      valid = true;
  }
  if(!valid)
    CloseAudioStream(true);

  // enable subtitles
  m_player_video.EnableSubtitle(g_settings.m_currentVideoSettings.m_SubtitleOn);

  // open subtitle stream
  streams = m_SelectionStreams.Get(STREAM_SUBTITLE, PredicateSubtitlePriority);
  valid   = false;
  for(OMXSelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenSubtitleStream(it->id, it->source))
    {
      valid = true;
      if(it->flags & CDemuxStream::FLAG_FORCED)
        m_player_video.EnableSubtitle(true);
    }
  }
  if(!valid)
    CloseSubtitleStream(true);

  // open teletext stream
  /*
  streams = m_SelectionStreams.Get(STREAM_TELETEXT);
  valid   = false;
  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenTeletextStream(it->id, it->source))
      valid = true;
  }
  if(!valid)
    CloseTeletextStream(true);
  */

  m_av_clock.OMXStop();
  m_av_clock.OMXReset();
}

bool COMXPlayer::ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream)
{

  // check if we should read from subtitle demuxer
  if(m_player_subtitle.AcceptsData() &&  m_pSubtitleDemuxer)
  {
    if(m_pSubtitleDemuxer)
      packet = m_pSubtitleDemuxer->Read();

    if(packet)
    {
      UpdateCorrection(packet, m_offset_pts);
      if(packet->iStreamId < 0)
        return true;

      stream = m_pSubtitleDemuxer->GetStream(packet->iStreamId);
      if (!stream)
      {
        CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
        return false;
      }
      if(stream->source == STREAM_SOURCE_NONE)
      {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX_SUB);
        m_SelectionStreams.Update(NULL, m_pSubtitleDemuxer);
      }
      return true;
    }
  }

  // read a data frame from stream.
  if(m_pDemuxer)
    packet = m_pDemuxer->Read();

  if(packet)
  {
    // stream changed, update and open defaults
    if(packet->iStreamId == DMX_SPECIALID_STREAMCHANGE)
    {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
        OpenDefaultStreams();
        return true;
    }

    UpdateCorrection(packet, m_offset_pts);
    // this groupId stuff is getting a bit messy, need to find a better way
    // currently it is used to determine if a menu overlay is associated with a picture
    // for dvd's we use as a group id, the current cell and the current title
    // to be a bit more precise we alse count the number of disc's in case of a pts wrap back in the same cell / title
    packet->iGroupId = m_pInputStream->GetCurrentGroupId();

    if(packet->iStreamId < 0)
      return true;

    if(m_pDemuxer)
    {
      stream = m_pDemuxer->GetStream(packet->iStreamId);
      if (!stream)
      {
        CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
        return false;
      }
      if(stream->source == STREAM_SOURCE_NONE)
      {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
      }
    }
    return true;
  }
  return false;
}

bool COMXPlayer::IsValidStream(COMXCurrentStream& stream)
{
  if(stream.id<0)
    return true; // we consider non selected as valid

  int source = STREAM_SOURCE_MASK(stream.source);
  if(source == STREAM_SOURCE_TEXT)
    return true;
  if(source == STREAM_SOURCE_DEMUX_SUB)
  {
    CDemuxStream* st = m_pSubtitleDemuxer->GetStream(stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;
    return true;
  }
  if(source == STREAM_SOURCE_DEMUX)
  {
    CDemuxStream* st = m_pDemuxer->GetStream(stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;

    if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if(stream.type == STREAM_AUDIO    && st->iPhysicalId != m_dvd.iSelectedAudioStream)
        return false;
      if(stream.type == STREAM_SUBTITLE && st->iPhysicalId != m_dvd.iSelectedSPUStream)
        return false;
    }

    return true;
  }

  return false;
}

bool COMXPlayer::IsBetterStream(COMXCurrentStream& current, CDemuxStream* stream)
{
  // Do not reopen non-video streams if we're in video-only mode
  if(m_PlayerOptions.video_only && current.type != STREAM_VIDEO)
    return false;

  if (m_pInputStream && ( m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
                       || m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY) ) )
  {
    int source_type;

    source_type = STREAM_SOURCE_MASK(current.source);
    if(source_type != STREAM_SOURCE_DEMUX
    && source_type != STREAM_SOURCE_NONE)
      return false;

    source_type = STREAM_SOURCE_MASK(stream->source);
    if(source_type  != STREAM_SOURCE_DEMUX
    || stream->type != current.type
    || stream->iId  == current.id)
      return false;

    if(current.type == STREAM_AUDIO    && stream->iPhysicalId == m_dvd.iSelectedAudioStream)
      return true;
    if(current.type == STREAM_SUBTITLE && stream->iPhysicalId == m_dvd.iSelectedSPUStream)
      return true;
    if(current.type == STREAM_VIDEO    && current.id < 0)
      return true;
  }
  else
  {
    if(stream->source == current.source
    && stream->iId    == current.id)
      return false;

    if(stream->disabled)
      return false;

    if(stream->type != current.type)
      return false;

    if(current.type == STREAM_SUBTITLE)
      return false;

    if(current.id < 0)
      return true;
  }
  return false;
}

bool COMXPlayer::WaitForPausedThumbJobs(int timeout_ms)
{
  // use m_bStop and Sleep so we can get canceled.
  while (!m_bStop && (timeout_ms > 0))
  {
    if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
    {
      Sleep(100);
      timeout_ms -= 100;
    }
    else
      return true;
  }

  return false;
}

void COMXPlayer::Process()
{
  //bool bAEStopped = false;

  if(!m_av_clock.OMXInitialize(false, false))
  {
    m_bAbortRequest = true;
    return;
  }
  if(g_guiSettings.GetBool("videoplayer.adjustrefreshrate"))
    m_av_clock.HDMIClockSync();

  m_av_clock.OMXStateExecute();
  m_av_clock.OMXStart();

  //CLog::Log(LOGDEBUG, "COMXPlayer: Thread started");

  try
  {
    m_stats             = false;

    if(!OpenInputStream())
    {
      m_bAbortRequest = true;
      return;
    }

    if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      CLog::Log(LOGNOTICE, "OMXPlayer: playing a dvd with menu's");
      m_PlayerOptions.starttime = 0;

      if(m_PlayerOptions.state.size() > 0)
        ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(m_PlayerOptions.state);
      else
        ((CDVDInputStreamNavigator*)m_pInputStream)->EnableSubtitleStream(g_settings.m_currentVideoSettings.m_SubtitleOn);

      g_settings.m_currentVideoSettings.m_SubtitleCached = true;
    }

    if(!OpenDemuxStream())
    {
      m_bAbortRequest = true;
      return;
    }

    m_player_video.EnableFullscreen(true);

    OpenDefaultStreams();

    // look for any EDL files
    m_Edl.Clear();
    m_EdlAutoSkipMarkers.Clear();
    float fFramesPerSecond;
    if (m_CurrentVideo.id >= 0 && m_CurrentVideo.hint.fpsrate > 0 && m_CurrentVideo.hint.fpsscale > 0)
    {
      fFramesPerSecond = (float)m_CurrentVideo.hint.fpsrate / (float)m_CurrentVideo.hint.fpsscale;
      m_Edl.ReadEditDecisionLists(m_filename, fFramesPerSecond, m_CurrentVideo.hint.height);
    }

    /*
     * Check to see if the demuxer should start at something other than time 0. This will be the case
     * if there was a start time specified as part of the "Start from where last stopped" (aka
     * auto-resume) feature or if there is an EDL cut or commercial break that starts at time 0.
     */
    CEdl::Cut cut;
    int starttime = 0;
    if(m_PlayerOptions.starttime > 0 || m_PlayerOptions.startpercent > 0)
    {
      if (m_PlayerOptions.startpercent > 0 && m_pDemuxer)
      {
        int64_t playerStartTime = (int64_t) ( ( (float) m_pDemuxer->GetStreamLength() ) * ( m_PlayerOptions.startpercent/(float)100 ) );
        starttime = m_Edl.RestoreCutTime(playerStartTime);
      }
      else
      {
        starttime = m_Edl.RestoreCutTime((int64_t)m_PlayerOptions.starttime * 1000); // s to ms
      }
      CLog::Log(LOGDEBUG, "%s - Start position set to last stopped position: %d", __FUNCTION__, starttime);
    }
    else if(m_Edl.InCut(0, &cut)
        && (cut.action == CEdl::CUT || cut.action == CEdl::COMM_BREAK))
    {
      starttime = cut.end;
      CLog::Log(LOGDEBUG, "%s - Start position set to end of first cut or commercial break: %d", __FUNCTION__, starttime);
      if(cut.action == CEdl::COMM_BREAK)
      {
        /*
         * Setup auto skip markers as if the commercial break had been skipped using standard
         * detection.
         */
        m_EdlAutoSkipMarkers.commbreak_start = cut.start;
        m_EdlAutoSkipMarkers.commbreak_end   = cut.end;
        m_EdlAutoSkipMarkers.seek_to_start   = true;
      }
    }
    if(starttime > 0)
    {
      double startpts = DVD_NOPTS_VALUE;
      if(m_pDemuxer)
      {
        if (m_pDemuxer->SeekTime(starttime, false, &startpts))
          CLog::Log(LOGDEBUG, "%s - starting demuxer from: %d", __FUNCTION__, starttime);
        else
          CLog::Log(LOGDEBUG, "%s - failed to start demuxing from: %d", __FUNCTION__, starttime);
      }

      if(m_pSubtitleDemuxer)
      {
        if(m_pSubtitleDemuxer->SeekTime(starttime, false, &startpts))
          CLog::Log(LOGDEBUG, "%s - starting subtitle demuxer from: %d", __FUNCTION__, starttime);
        else
          CLog::Log(LOGDEBUG, "%s - failed to start subtitle demuxing from: %d", __FUNCTION__, starttime);
      }
    }

    // make sure all selected stream have data on startup
    if (CachePVRStream())
      SetCaching(CACHESTATE_PVR);

    // make sure application know our info
    UpdateApplication(0);
    UpdatePlayState(0);

    if (m_PlayerOptions.identify == false)
      m_callback.OnPlayBackStarted();

    // we are done initializing now, set the readyevent
    m_ready.Set();

    if (!CachePVRStream())
      SetCaching(CACHESTATE_FLUSH);

    // shutdown AE
    /*
    CAEFactory::Shutdown();
    bAEStopped = true;
    */

    // stop thumb jobs
    CJobManager::GetInstance().Pause(kJobTypeMediaFlags);

    /*
    if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
    {
      if (!WaitForPausedThumbJobs(20000))
      {
        CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);
        CLog::Log(LOGINFO, "COMXPlayer::Process:thumbgen jobs still running !!!");
      }
    }
    */

    while (!m_bAbortRequest)
    {
      // handle messages send to this thread, like seek or demuxer reset requests
      HandleMessages();

      if(m_bAbortRequest)
        break;

      // should we open a new input stream?
      if(!m_pInputStream)
      {
        if (OpenInputStream() == false)
        {
          m_bAbortRequest = true;
          break;
        }
      }

      // should we open a new demuxer?
      if(!m_pDemuxer)
      {
        if (m_pInputStream->NextStream() == CDVDInputStream::NEXTSTREAM_NONE)
          break;

        if (m_pInputStream->IsEOF())
          break;

        if (OpenDemuxStream() == false)
        {
          m_bAbortRequest = true;
          break;
        }

        OpenDefaultStreams();

        if (CachePVRStream())
          SetCaching(CACHESTATE_PVR);

        UpdateApplication(0);
        UpdatePlayState(0);
      }

      // handle eventual seeks due to playspeed
      HandlePlaySpeed();

      // update player state
      UpdatePlayState(200);

      // update application with our state
      UpdateApplication(1000);

      if (CheckDelayedChannelEntry())
        continue;

      // if the queues are full, no need to read more
      if ((!m_player_audio.AcceptsData() && m_CurrentAudio.id >= 0)
      ||  (!m_player_video.AcceptsData() && m_CurrentVideo.id >= 0))
      {
        Sleep(10);
        continue;
      }

      // always yield to players if they have data
      if((m_player_audio.HasData() || m_CurrentAudio.id < 0)
      && (m_player_video.HasData() || m_CurrentVideo.id < 0))
        Sleep(0);

      DemuxPacket* pPacket = NULL;
      CDemuxStream *pStream = NULL;
      ReadPacket(pPacket, pStream);
      if (pPacket && !pStream)
      {
        /* probably a empty packet, just free it and move on */
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
        continue;
      }

      if (!pPacket)
      {
        // when paused, demuxer could be be returning empty
        if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
          continue;

        // check for a still frame state
        if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

          // stills will be skipped
          if(m_dvd.state == DVDSTATE_STILL)
          {
            if (m_dvd.iDVDStillTime > 0)
            {
              if ((XbmcThreads::SystemClockMillis() - m_dvd.iDVDStillStartTime) >= m_dvd.iDVDStillTime)
              {
                m_dvd.iDVDStillTime = 0;
                m_dvd.iDVDStillStartTime = 0;
                m_dvd.state = DVDSTATE_NORMAL;
                pStream->SkipStill();
                continue;
              }
            }
          }
        }

        // if there is another stream available, reopen demuxer
        CDVDInputStream::ENextStream next = m_pInputStream->NextStream();
        if(next == CDVDInputStream::NEXTSTREAM_OPEN)
        {
          SAFE_DELETE(m_pDemuxer);
          m_CurrentAudio.stream = NULL;
          m_CurrentVideo.stream = NULL;
          m_CurrentSubtitle.stream = NULL;
          continue;
        }

        // input stream asked us to just retry
        if(next == CDVDInputStream::NEXTSTREAM_RETRY)
        {
          Sleep(100);
          continue;
        }
 
        // make sure we tell all players to finish it's data
        if(m_CurrentAudio.inited)
          m_player_audio.SendMessage   (new CDVDMsg(CDVDMsg::GENERAL_EOF));
        if(m_CurrentVideo.inited)
          m_player_video.SendMessage   (new CDVDMsg(CDVDMsg::GENERAL_EOF));
        if(m_CurrentSubtitle.inited)
          m_player_subtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
        m_CurrentAudio.inited    = false;
        m_CurrentVideo.inited    = false;
        m_CurrentSubtitle.inited = false;
        m_CurrentAudio.started    = false;
        m_CurrentVideo.started    = false;
        m_CurrentSubtitle.started = false;

        // if we are caching, start playing it again
        SetCaching(CACHESTATE_DONE);

        // while players are still playing, keep going to allow seekbacks
        if(m_player_video.HasData()
        || m_player_audio.HasData())
        {
          Sleep(100);
          continue;
        }

        // wait for omx components to finish
        if(HasVideo() && !m_player_video.IsEOS())
        {
          Sleep(100);
          continue;
        }
        if(HasAudio() && !m_player_audio.IsEOS())
        {
          Sleep(100);
          continue;
        }

        if (!m_pInputStream->IsEOF())
          CLog::Log(LOGINFO, "%s - eof reading from demuxer", __FUNCTION__);

        break;
      }

      // check so that none of our streams has become invalid
      if (!IsValidStream(m_CurrentAudio)    && m_player_audio.IsStalled())    CloseAudioStream(true);
      if (!IsValidStream(m_CurrentVideo)    && m_player_video.IsStalled())    CloseVideoStream(true);
      if (!IsValidStream(m_CurrentSubtitle) && m_player_subtitle.IsStalled()) CloseSubtitleStream(true);

      // see if we can find something better to play
      if (IsBetterStream(m_CurrentAudio,    pStream)) OpenAudioStream   (pStream->iId, pStream->source);
      if (IsBetterStream(m_CurrentVideo,    pStream)) OpenVideoStream   (pStream->iId, pStream->source);
      if (IsBetterStream(m_CurrentSubtitle, pStream)) OpenSubtitleStream(pStream->iId, pStream->source);

      if(m_change_volume)
      {
        m_player_audio.SetCurrentVolume(m_current_volume);
        m_change_volume = false;
      }

      ProcessPacket(pStream, pPacket);

      // check if in a cut or commercial break that should be automatically skipped
      CheckAutoSceneSkip();
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "COMXPlayer::Process: Exception thrown");
  }

  /*
  if(bAEStopped)
  {
    // start AE
    CAEFactory::LoadEngine();
    CAEFactory::StartEngine();

    CAEFactory::SetMute     (g_settings.m_bMute);
    CAEFactory::SetSoundMode(g_guiSettings.GetInt("audiooutput.guisoundmode"));
  }
  */

  // let thumbgen jobs resume.
  CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);
}

bool COMXPlayer::CheckDelayedChannelEntry(void)
{
  bool bReturn(false);

  if (m_iChannelEntryTimeOut > 0 && XbmcThreads::SystemClockMillis() >= m_iChannelEntryTimeOut)
  {
    CFileItem currentFile(g_application.CurrentFileItem());
    CPVRChannel *currentChannel = currentFile.GetPVRChannelInfoTag();
    SwitchChannel(*currentChannel);

    bReturn = true;
    m_iChannelEntryTimeOut = 0;
  }

  return bReturn;
}

void COMXPlayer::ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket)
{
    /* process packet if it belongs to selected stream. for dvd's don't allow automatic opening of streams*/
    OMXStreamLock lock(this);

    try
    {
      if (pPacket->iStreamId == m_CurrentAudio.id && pStream->source == m_CurrentAudio.source && pStream->type == STREAM_AUDIO)
        ProcessAudioData(pStream, pPacket);
      else if (pPacket->iStreamId == m_CurrentVideo.id && pStream->source == m_CurrentVideo.source && pStream->type == STREAM_VIDEO)
        ProcessVideoData(pStream, pPacket);
      else if (pPacket->iStreamId == m_CurrentSubtitle.id && pStream->source == m_CurrentSubtitle.source && pStream->type == STREAM_SUBTITLE)
        ProcessSubData(pStream, pPacket);
      else
      {
        pStream->SetDiscard(AVDISCARD_ALL);
        CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
      }
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when processing demux packet", __FUNCTION__);
    }

}

void COMXPlayer::ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentAudio.stream != (void*)pStream
  ||  m_CurrentAudio.changes != pStream->changes)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if (m_CurrentAudio.hint != CDVDStreamInfo(*pStream, true))
      OpenAudioStream( pPacket->iStreamId, pStream->source );

    m_CurrentAudio.stream = (void*)pStream;
  }

  // check if we are too slow and need to recache
  CheckStartCaching(m_CurrentAudio);

  CheckContinuity(m_CurrentAudio, pPacket);
  UpdateTimestamps(m_CurrentAudio, pPacket);
  bool drop = false;
  if (CheckPlayerInit(m_CurrentAudio, DVDPLAYER_AUDIO))
    drop = true;

  /*
   * If CheckSceneSkip() returns true then demux point is inside an EDL cut and the packets are dropped.
   * If not inside a hard cut, but the demux point has reached an EDL mute section then trigger the
   * AUDIO_SILENCE state. The AUDIO_SILENCE state is reverted as soon as the demux point is outside
   * of any EDL section while EDL mute is still active.
   */
  CEdl::Cut cut;
  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;
  else if (m_Edl.InCut(DVD_TIME_TO_MSEC(m_CurrentAudio.dts + m_offset_pts), &cut) && cut.action == CEdl::MUTE // Inside EDL mute
  &&      !m_EdlAutoSkipMarkers.mute) // Mute not already triggered
  {
    m_player_audio.SendMessage(new CDVDMsgBool(CDVDMsg::AUDIO_SILENCE, true));
    m_EdlAutoSkipMarkers.mute = true;
  }
  else if (!m_Edl.InCut(DVD_TIME_TO_MSEC(m_CurrentAudio.dts + m_offset_pts), &cut) // Outside of any EDL
  &&        m_EdlAutoSkipMarkers.mute) // But the mute hasn't been removed yet
  {
    m_player_audio.SendMessage(new CDVDMsgBool(CDVDMsg::AUDIO_SILENCE, false));
    m_EdlAutoSkipMarkers.mute = false;
  }

  m_player_audio.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void COMXPlayer::ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentVideo.stream != (void*)pStream
  ||  m_CurrentVideo.changes != pStream->changes)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentVideo.hint != CDVDStreamInfo(*pStream, true))
      OpenVideoStream(pPacket->iStreamId, pStream->source);

    m_CurrentVideo.stream = (void*)pStream;
  }

  // check if we are too slow and need to recache
  CheckStartCaching(m_CurrentVideo);

  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    CheckContinuity(m_CurrentVideo, pPacket);
    UpdateTimestamps(m_CurrentVideo, pPacket);
  }

  bool drop = false;
  if (CheckPlayerInit(m_CurrentVideo, DVDPLAYER_VIDEO))
    drop = true;

  if (CheckSceneSkip(m_CurrentVideo))
    drop = true;

  m_player_video.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void COMXPlayer::ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentSubtitle.stream != (void*)pStream
  ||  m_CurrentSubtitle.changes != pStream->changes)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentSubtitle.hint != CDVDStreamInfo(*pStream, true))
      OpenSubtitleStream(pPacket->iStreamId, pStream->source);

    m_CurrentSubtitle.stream = (void*)pStream;
  }

  UpdateTimestamps(m_CurrentSubtitle, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentSubtitle, DVDPLAYER_SUBTITLE))
    drop = true;

  if (CheckSceneSkip(m_CurrentSubtitle))
    drop = true;

  m_player_subtitle.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));

  if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    m_player_subtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
}

bool COMXPlayer::GetCachingTimes(double& level, double& delay, double& offset)
{
  if(!m_pInputStream || !m_pDemuxer)
    return false;

  XFILE::SCacheStatus status;
  if (!m_pInputStream->GetCacheStatus(&status))
    return false;

  int64_t cached   = status.forward;
  unsigned currate = status.currate;
  unsigned maxrate = status.maxrate;
  bool full        = status.full;

  int64_t length  = m_pInputStream->GetLength();
  int64_t remain  = length - m_pInputStream->Seek(0, SEEK_CUR);

  if(cached < 0 || length <= 0 || remain < 0)
    return false;

  double play_sbp  = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) / length;
  double queued = 1000.0 * GetQueueTime() / play_sbp;

  delay  = 0.0;
  level  = 0.0;
  offset = (double)(cached + queued) / length;

  if (currate == 0)
    return true;

  double cache_sbp   = 1.1 * (double)DVD_TIME_BASE / currate;         /* underestimate by 10 % */
  double play_left   = play_sbp  * (remain + queued);                 /* time to play out all remaining bytes */
  double cache_left  = cache_sbp * (remain - cached);                 /* time to cache the remaining bytes */
  double cache_need  = std::max(0.0, remain - play_left / cache_sbp); /* bytes needed until play_left == cache_left */

  delay = cache_left - play_left;

  if (full && (currate < maxrate) )
    level = -1.0;                          /* buffer is full & our read rate is too low  */
  else
    level = (cached + queued) / (cache_need + queued);

  return true;
}

void COMXPlayer::HandlePlaySpeed()
{
  ECacheState caching = m_caching;

  if(IsInMenu() && caching != CACHESTATE_DONE)
    caching = CACHESTATE_DONE;

  if(caching == CACHESTATE_FULL)
  {
    double level, delay, offset;
    if(GetCachingTimes(level, delay, offset))
    {
      if(level  < 0.0)
        caching = CACHESTATE_INIT;
      if(level >= 1.0)
        caching = CACHESTATE_INIT;
    }
    else
    {
      if ((!m_player_audio.AcceptsData() && m_CurrentAudio.id >= 0)
      ||  (!m_player_video.AcceptsData() && m_CurrentVideo.id >= 0))
        caching = CACHESTATE_INIT;
    }
  }

  if(caching == CACHESTATE_INIT)
  {
    // if all enabled streams have been inited we are done
    if((m_CurrentVideo.id < 0 || m_CurrentVideo.started)
    && (m_CurrentAudio.id < 0 || m_CurrentAudio.started))
      caching = CACHESTATE_PLAY;

    // handle situation that we get no data on one stream
    if(m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0)
    {
      if ((!m_player_audio.AcceptsData() && !m_CurrentVideo.started)
      ||  (!m_player_video.AcceptsData() && !m_CurrentAudio.started))
      {
        caching = CACHESTATE_DONE;
      }
    }
  }

  if (caching == CACHESTATE_PVR)
  {
    bool bGotAudio(m_pDemuxer->GetNrOfAudioStreams() > 0);
    bool bGotVideo(m_pDemuxer->GetNrOfVideoStreams() > 0);
    bool bAudioLevelOk(m_player_audio.GetLevel() > g_advancedSettings.m_iPVRMinAudioCacheLevel);
    bool bVideoLevelOk(m_player_video.GetLevel() > g_advancedSettings.m_iPVRMinVideoCacheLevel);
    bool bAudioFull(!m_player_audio.AcceptsData());
    bool bVideoFull(!m_player_video.AcceptsData());

    if (/* if all streams got at least g_advancedSettings.m_iPVRMinCacheLevel in their buffers, we're done */
        ((bGotVideo || bGotAudio) && (!bGotAudio || bAudioLevelOk) && (!bGotVideo || bVideoLevelOk)) ||
        /* or if one of the buffers is full */
        (bAudioFull || bVideoFull))
    {
      CLog::Log(LOGDEBUG, "set caching from pvr to done. audio (%d) = %d. video (%d) = %d",
          bGotAudio, m_player_audio.GetLevel(),
          bGotVideo, m_player_video.GetLevel());

      CFileItem currentItem(g_application.CurrentFileItem());
      if (currentItem.HasPVRChannelInfoTag())
        g_PVRManager.LoadCurrentChannelSettings();

      caching = CACHESTATE_DONE;
    }
    else
    {
      /* ensure that automatically started players are stopped while caching */
      if (m_CurrentAudio.started)
        m_player_audio.SetSpeed(DVD_PLAYSPEED_PAUSE);
      if (m_CurrentVideo.started)
        m_player_video.SetSpeed(DVD_PLAYSPEED_PAUSE);
    }
  }

  if(caching == CACHESTATE_PLAY)
  {
    // if all enabled streams have started playing we are done
    if((m_CurrentVideo.id < 0 || !m_player_video.IsStalled())
    && (m_CurrentAudio.id < 0 || !m_player_audio.IsStalled()))
      caching = CACHESTATE_DONE;
  }

  if(m_caching != caching)
    SetCaching(caching);

  if(GetPlaySpeed() != DVD_PLAYSPEED_NORMAL && GetPlaySpeed() != DVD_PLAYSPEED_PAUSE)
  {
    if (IsInMenu())
    {
      // this can't be done in menu
      SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    }
    else if (m_CurrentVideo.id >= 0
          &&  m_CurrentVideo.inited == true
          &&  m_SpeedState.lastpts  != m_player_video.GetCurrentPTS()
          &&  m_SpeedState.lasttime != GetTime())
    {
      m_SpeedState.lastpts  = m_player_video.GetCurrentPTS();
      m_SpeedState.lasttime = GetTime();
      // check how much off clock video is when ff/rw:ing
      // a problem here is that seeking isn't very accurate
      // and since the clock will be resynced after seek
      // we might actually not really be playing at the wanted
      // speed. we'd need to have some way to not resync the clock
      // after a seek to remember timing. still need to handle
      // discontinuities somehow

      // when seeking, give the player a headstart to make sure
      // the time it takes to seek doesn't make a difference.
      double error;
      error  = m_av_clock.GetClock() - m_SpeedState.lastpts;
      error *= m_playSpeed / abs(m_playSpeed);

      if(error > DVD_MSEC_TO_TIME(1000))
      {
        CLog::Log(LOGDEBUG, "COMXPlayer::Process - Seeking to catch up");
        int64_t iTime = (int64_t)DVD_TIME_TO_MSEC(m_av_clock.GetClock() + m_State.time_offset + 500000.0 * m_playSpeed / DVD_PLAYSPEED_NORMAL);
        m_messenger.Put(new CDVDMsgPlayerSeek(iTime, (GetPlaySpeed() < 0), true, false, false, true));
      }
    }
  }
}

bool COMXPlayer::CheckStartCaching(COMXCurrentStream& current)
{
  if(m_caching   != CACHESTATE_DONE
  || m_playSpeed != DVD_PLAYSPEED_NORMAL)
    return false;

  if(IsInMenu())
    return false;

  if((current.type == STREAM_AUDIO && m_player_audio.IsStalled())
  || (current.type == STREAM_VIDEO && m_player_video.IsStalled()))
  {
    if (CachePVRStream())
    {
      if ((current.type == STREAM_AUDIO && current.started && m_player_audio.GetLevel() == 0) ||
         (current.type == STREAM_VIDEO && current.started && m_player_video.GetLevel() == 0))
      {
        CLog::Log(LOGDEBUG, "%s stream stalled. start buffering", current.type == STREAM_AUDIO ? "audio" : "video");
        SetCaching(CACHESTATE_PVR);
      }
      return true;
    }

    // don't start caching if it's only a single stream that has run dry
    if(m_player_audio.GetLevel() > 50
    || m_player_video.GetLevel() > 50)
      return false;

    if(current.inited)
      SetCaching(CACHESTATE_FULL);
    else
      SetCaching(CACHESTATE_INIT);
    return true;
  }
  return false;
}

bool COMXPlayer::CheckPlayerInit(COMXCurrentStream& current, unsigned int source)
{
  if(current.inited)
    return false;

  if(current.startpts != DVD_NOPTS_VALUE)
  {
    if(current.dts == DVD_NOPTS_VALUE)
    {
      CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, source,  current.dts, current.startpts);
      return true;
    }

    if((current.startpts - current.dts) > DVD_SEC_TO_TIME(20))
    {
      CLog::Log(LOGDEBUG, "%s - too far to decode before finishing seek", __FUNCTION__);
      if(m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
        m_CurrentAudio.startpts = current.dts;
      if(m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
        m_CurrentVideo.startpts = current.dts;
      if(m_CurrentSubtitle.startpts != DVD_NOPTS_VALUE)
        m_CurrentSubtitle.startpts = current.dts;
      if(m_CurrentTeletext.startpts != DVD_NOPTS_VALUE)
        m_CurrentTeletext.startpts = current.dts;
    }

    if(current.dts < current.startpts)
    {
      CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, source,  current.dts, current.startpts);
      return true;
    }
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if (current.dts != DVD_NOPTS_VALUE)
  {
    current.inited   = true;
    current.startpts = current.dts;

    bool setclock = false;
    if(m_playSpeed == DVD_PLAYSPEED_NORMAL)
    {
      if(     source == DVDPLAYER_AUDIO)
        setclock = !m_CurrentVideo.inited;
      else if(source == DVDPLAYER_VIDEO)
        setclock = !m_CurrentAudio.inited;
    }
    else
    {
      if(source == DVDPLAYER_VIDEO)
        setclock = true;
    }

    double starttime = current.startpts;
    if(m_CurrentAudio.inited
    && m_CurrentAudio.startpts != DVD_NOPTS_VALUE
    && m_CurrentAudio.startpts < starttime)
      starttime = m_CurrentAudio.startpts;
    if(m_CurrentVideo.inited
    && m_CurrentVideo.startpts != DVD_NOPTS_VALUE
    && m_CurrentVideo.startpts < starttime)
      starttime = m_CurrentVideo.startpts;

    starttime = current.startpts - starttime;
    if(starttime > 0 && setclock)
    {
      if(starttime > DVD_SEC_TO_TIME(2))
        CLog::Log(LOGWARNING, "COMXPlayer::CheckPlayerInit(%d) - Ignoring too large delay of %f", source, starttime);
      else
        SendPlayerMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_DELAY, starttime), source);
    }

    SendPlayerMessage(new CDVDMsgGeneralResync(current.dts, setclock), source);
  }
  return false;
}

void COMXPlayer::UpdateCorrection(DemuxPacket* pkt, double correction)
{
  if(pkt->dts != DVD_NOPTS_VALUE) pkt->dts -= correction;
  if(pkt->pts != DVD_NOPTS_VALUE) pkt->pts -= correction;
}

void COMXPlayer::UpdateTimestamps(COMXCurrentStream& current, DemuxPacket* pPacket)
{
  double dts = current.dts;
  /* update stored values */
  if(pPacket->dts != DVD_NOPTS_VALUE)
    dts = pPacket->dts;
  else if(pPacket->pts != DVD_NOPTS_VALUE)
    dts = pPacket->pts;

  /* calculate some average duration */
  if(pPacket->duration != DVD_NOPTS_VALUE)
    current.dur = pPacket->duration;
  else if(dts != DVD_NOPTS_VALUE && current.dts != DVD_NOPTS_VALUE)
    current.dur = 0.1 * (current.dur * 9 + (dts - current.dts));

  current.dts = dts;
}

void COMXPlayer::UpdateLimits(double& minimum, double& maximum, double dts)
{
  if(dts == DVD_NOPTS_VALUE)
    return;
  if(minimum == DVD_NOPTS_VALUE || minimum > dts) minimum = dts;
  if(maximum == DVD_NOPTS_VALUE || maximum < dts) maximum = dts;
}

void COMXPlayer::CheckContinuity(COMXCurrentStream& current, DemuxPacket* pPacket)
{
  if (m_playSpeed < DVD_PLAYSPEED_PAUSE)
    return;

  if( pPacket->dts == DVD_NOPTS_VALUE || current.dts == DVD_NOPTS_VALUE)
    return;

  double mindts = DVD_NOPTS_VALUE, maxdts = DVD_NOPTS_VALUE;
  UpdateLimits(mindts, maxdts, m_CurrentAudio.dts);
  UpdateLimits(mindts, maxdts, m_CurrentVideo.dts);
  UpdateLimits(mindts, maxdts, m_CurrentAudio.dts_end());
  UpdateLimits(mindts, maxdts, m_CurrentVideo.dts_end());

  /* if we don't have max and min, we can't do anything more */
  if( mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE )
    return;

  double correction = 0.0;
  if( pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000))
  {
    CLog::Log(LOGDEBUG, "COMXPlayer::CheckContinuity - resync forward :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - maxdts);
    correction = pPacket->dts - maxdts;
  }

  /* if it's large scale jump, correct for it */
  if(pPacket->dts + DVD_MSEC_TO_TIME(100) < current.dts_end())
  {
    CLog::Log(LOGDEBUG, "COMXPlayer::CheckContinuity - resync backward :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
    correction = pPacket->dts - current.dts_end();
  }
  else if(pPacket->dts < current.dts)
  {
    CLog::Log(LOGDEBUG, "COMXPlayer::CheckContinuity - wrapback :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
  }

  if(correction != 0.0)
  {
    /* disable detection on next packet on other stream to avoid ping pong-ing */
    if(m_CurrentAudio.player != current.player) m_CurrentAudio.dts = DVD_NOPTS_VALUE;
    if(m_CurrentVideo.player != current.player) m_CurrentVideo.dts = DVD_NOPTS_VALUE;

    m_offset_pts += correction;
    UpdateCorrection(pPacket, correction);
  }
}

bool COMXPlayer::CheckSceneSkip(COMXCurrentStream& current)
{
  if(!m_Edl.HasCut())
    return false;

  if(current.dts == DVD_NOPTS_VALUE)
    return false;

  if(current.inited == false)
    return false;

  CEdl::Cut cut;
  return m_Edl.InCut(DVD_TIME_TO_MSEC(current.dts + m_offset_pts), &cut) && cut.action == CEdl::CUT;
}

void COMXPlayer::CheckAutoSceneSkip()
{
  if(!m_Edl.HasCut())
    return;

  /*
   * Check that there is an audio and video stream.
   */
  if(m_CurrentAudio.id < 0
  || m_CurrentVideo.id < 0)
    return;

  /*
   * If there is a startpts defined for either the audio or video stream then dvdplayer is still
   * still decoding frames to get to the previously requested seek point.
   */
  if(m_CurrentAudio.inited == false
  || m_CurrentVideo.inited == false)
    return;

  if(m_CurrentAudio.dts == DVD_NOPTS_VALUE
  || m_CurrentVideo.dts == DVD_NOPTS_VALUE)
    return;

  const int64_t clock = DVD_TIME_TO_MSEC(min(m_CurrentAudio.dts, m_CurrentVideo.dts) + m_offset_pts);

  CEdl::Cut cut;
  if(!m_Edl.InCut(clock, &cut))
    return;

  if(cut.action == CEdl::CUT
  && !(cut.end == m_EdlAutoSkipMarkers.cut || cut.start == m_EdlAutoSkipMarkers.cut)) // To prevent looping if same cut again
  {
    CLog::Log(LOGDEBUG, "%s - Clock in EDL cut [%s - %s]: %s. Automatically skipping over.",
              __FUNCTION__, CEdl::MillisecondsToTimeString(cut.start).c_str(),
              CEdl::MillisecondsToTimeString(cut.end).c_str(), CEdl::MillisecondsToTimeString(clock).c_str());
    /*
     * Seeking either goes to the start or the end of the cut depending on the play direction.
     */
    int64_t seek = GetPlaySpeed() >= 0 ? cut.end : cut.start;
    /*
     * Seeking is NOT flushed so any content up to the demux point is retained when playing forwards.
     */
    m_messenger.Put(new CDVDMsgPlayerSeek((int)seek, true, false, true, false, true));
    /*
     * Seek doesn't always work reliably. Last physical seek time is recorded to prevent looping
     * if there was an error with seeking and it landed somewhere unexpected, perhaps back in the
     * cut. The cut automatic skip marker is reset every 500ms allowing another attempt at the seek.
     */
    m_EdlAutoSkipMarkers.cut = GetPlaySpeed() >= 0 ? cut.end : cut.start;
  }
  else if(cut.action == CEdl::COMM_BREAK
  &&      GetPlaySpeed() >= 0
  &&      cut.start > m_EdlAutoSkipMarkers.commbreak_end)
  {
    CLog::Log(LOGDEBUG, "%s - Clock in commercial break [%s - %s]: %s. Automatically skipping to end of commercial break (only done once per break)",
              __FUNCTION__, CEdl::MillisecondsToTimeString(cut.start).c_str(), CEdl::MillisecondsToTimeString(cut.end).c_str(),
              CEdl::MillisecondsToTimeString(clock).c_str());
    /*
     * Seeking is NOT flushed so any content up to the demux point is retained when playing forwards.
     */
    m_messenger.Put(new CDVDMsgPlayerSeek(cut.end + 1, true, false, true, false, true));
    /*
     * Each commercial break is only skipped once so poorly detected commercial breaks can be
     * manually re-entered. Start and end are recorded to prevent looping and to allow seeking back
     * to the start of the commercial break if incorrectly flagged.
     */
    m_EdlAutoSkipMarkers.commbreak_start = cut.start;
    m_EdlAutoSkipMarkers.commbreak_end   = cut.end;
    m_EdlAutoSkipMarkers.seek_to_start   = true; // Allow backwards Seek() to go directly to the start
  }
}

void COMXPlayer::SynchronizeDemuxer(unsigned int timeout)
{
  if(IsCurrentThread())
    return;
  if(!m_messenger.IsInited())
    return;

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, 0);
  m_messenger.Put(message->Acquire());
  message->Wait(&m_bStop, 0);
  message->Release();
}

void COMXPlayer::SynchronizePlayers(unsigned int sources)
{
  /* we need a big timeout as audio queue is about 8seconds for 2ch ac3 */
  const int timeout = 10*1000; // in milliseconds

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, sources);
  if (m_CurrentAudio.id >= 0)
    m_player_audio.SendMessage(message->Acquire());

  if (m_CurrentVideo.id >= 0)
    m_player_video.SendMessage(message->Acquire());
/* TODO - we have to rewrite the sync class, to not require
          all other players waiting for subtitle, should only
          be the oposite way
  if (m_CurrentSubtitle.id >= 0)
    m_player_subtitle.SendMessage(message->Acquire()); 
*/
  message->Release();
}

void COMXPlayer::SendPlayerMessage(CDVDMsg* pMsg, unsigned int target)
{
  if(target == DVDPLAYER_AUDIO)
    m_player_audio.SendMessage(pMsg);
  if(target == DVDPLAYER_VIDEO)
    m_player_video.SendMessage(pMsg);
  if(target == DVDPLAYER_SUBTITLE)
    m_player_subtitle.SendMessage(pMsg);
}

void COMXPlayer::OnExit()
{
  try
  {
    CLog::Log(LOGNOTICE, "COMXPlayer::OnExit()");

    m_av_clock.OMXStop();
    m_av_clock.OMXStateIdle();

    // set event to inform openfile something went wrong in case openfile is still waiting for this event
    SetCaching(CACHESTATE_DONE);

    // close each stream
    if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "OMXPlayer: eof, waiting for queues to empty");
    if (m_CurrentAudio.id >= 0)
    {
      CLog::Log(LOGNOTICE, "OMXPlayer: closing audio stream");
      CloseAudioStream(!m_bAbortRequest);
    }
    if (m_CurrentVideo.id >= 0)
    {
      CLog::Log(LOGNOTICE, "OMXPlayer: closing video stream");
      CloseVideoStream(!m_bAbortRequest);
    }
    if (m_CurrentSubtitle.id >= 0)
    {
      CLog::Log(LOGNOTICE, "OMXPlayer: closing subtitle stream");
      CloseSubtitleStream(!m_bAbortRequest);
    }
    /*
    if (m_CurrentTeletext.id >= 0)
    {
      CLog::Log(LOGNOTICE, "OMXPlayer: closing teletext stream");
      CloseTeletextStream(!m_bAbortRequest);
    }
    */
    // destroy the demuxer
    if (m_pDemuxer)
    {
      CLog::Log(LOGNOTICE, "COMXPlayer::OnExit() deleting demuxer");
      delete m_pDemuxer;
    }
    m_pDemuxer = NULL;

    if (m_pSubtitleDemuxer)
    {
      CLog::Log(LOGNOTICE, "COMXPlayer::OnExit() deleting subtitle demuxer");
      delete m_pSubtitleDemuxer;
    }
    m_pSubtitleDemuxer = NULL;

    // destroy the inputstream
    if (m_pInputStream)
    {
      CLog::Log(LOGNOTICE, "COMXPlayer::OnExit() deleting input stream");
      delete m_pInputStream;
    }
    m_pInputStream = NULL;

    // clean up all selection streams
    m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

    m_messenger.End();

    m_av_clock.OMXDeinitialize();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close down player, memory leak will follow", __FUNCTION__);
    m_pInputStream = NULL;
    m_pDemuxer = NULL;
  }

  m_bStop = true;
  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if(m_PlayerOptions.identify == false)
  {
    if (m_bAbortRequest)
      m_callback.OnPlayBackStopped();
    else
      m_callback.OnPlayBackEnded();
  }

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  m_ready.Set();
}

void COMXPlayer::HandleMessages()
{
  CDVDMsg* pMsg;
  OMXStreamLock lock(this);

  while (m_messenger.Get(&pMsg, 0) == MSGQ_OK)
  {

    try
    {
      if (pMsg->IsType(CDVDMsg::PLAYER_SEEK) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK)         == 0
                                             && m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
      {
        CDVDMsgPlayerSeek &msg(*((CDVDMsgPlayerSeek*)pMsg));

        if (!m_State.canseek)
        {
          pMsg->Release();
          continue;
        }

        if(!msg.GetTrickPlay())
        {
          g_infoManager.SetDisplayAfterSeek(100000);
          if(msg.GetFlush())
            SetCaching(CACHESTATE_FLUSH);
        }

        double start = DVD_NOPTS_VALUE;

        int time = msg.GetRestore() ? (int)m_Edl.RestoreCutTime(msg.GetTime()) : msg.GetTime();
        CLog::Log(LOGDEBUG, "demuxer seek to: %d", time);
        if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start))
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", time);
          if(m_pSubtitleDemuxer)
          {
            if(!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
              CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %d, success", time);
          }
          FlushBuffers(!msg.GetFlush(), start, msg.GetAccurate());
        }
        else
          CLog::Log(LOGWARNING, "error while seeking");

        // set flag to indicate we have finished a seeking request
        if(!msg.GetTrickPlay())
          g_infoManager.SetDisplayAfterSeek();

        // dvd's will issue a HOP_CHANNEL that we need to skip
        if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          m_dvd.state = DVDSTATE_SEEK;
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK_CHAPTER) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK)         == 0
                                                          && m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
      {
        g_infoManager.SetDisplayAfterSeek(100000);
        SetCaching(CACHESTATE_FLUSH);

        CDVDMsgPlayerSeekChapter &msg(*((CDVDMsgPlayerSeekChapter*)pMsg));
        double start = DVD_NOPTS_VALUE;

        // This should always be the case.
        if(m_pDemuxer && m_pDemuxer->SeekChapter(msg.GetChapter(), &start))
        {
          FlushBuffers(false, start, true);
          m_callback.OnPlayBackSeekChapter(msg.GetChapter());
        }

        g_infoManager.SetDisplayAfterSeek();
      }
      else if (pMsg->IsType(CDVDMsg::DEMUXER_RESET))
      {
          m_CurrentAudio.stream = NULL;
          m_CurrentVideo.stream = NULL;
          m_CurrentSubtitle.stream = NULL;

          // we need to reset the demuxer, probably because the streams have changed
          if(m_pDemuxer)
            m_pDemuxer->Reset();
          if(m_pSubtitleDemuxer)
            m_pSubtitleDemuxer->Reset();
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_AUDIOSTREAM))
      {
        CDVDMsgPlayerSetAudioStream* pMsg2 = (CDVDMsgPlayerSetAudioStream*)pMsg;

        OMXSelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveAudioStream(st.id))
            {
              m_dvd.iSelectedAudioStream = -1;
              CloseAudioStream(false);
              // TODO : check //CloseVideoStream(false);
              m_messenger.Put(new CDVDMsgPlayerSeek(GetTime(), true, true, true));
            }
          }
          else
          {
            CloseAudioStream(false);
            OpenAudioStream(st.id, st.source);
            m_messenger.Put(new CDVDMsgPlayerSeek(GetTime(), true, true, true));
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
      {
        CDVDMsgPlayerSetSubtitleStream* pMsg2 = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

        OMXSelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveSubtitleStream(st.id))
            {
              m_dvd.iSelectedSPUStream = -1;
              CloseSubtitleStream(false);
            }
          }
          else
          {
            CloseSubtitleStream(false);
            OpenSubtitleStream(st.id, st.source);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
      {
        CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;

        m_player_video.EnableSubtitle(pValue->m_value);

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->EnableSubtitleStream(pValue->m_value);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
      {
        g_infoManager.SetDisplayAfterSeek(100000);
        SetCaching(CACHESTATE_FLUSH);

        CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::string s = pMsgPlayerSetState->GetState();
          ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(s);
          m_dvd.state = DVDSTATE_NORMAL;
          m_dvd.iDVDStillStartTime = 0;
          m_dvd.iDVDStillTime = 0;
        }

        g_infoManager.SetDisplayAfterSeek();
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_RECORD))
      {
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input)
          input->Record(*(CDVDMsgBool*)pMsg);
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
      {
        FlushBuffers(false);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
      {
        int speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

        // correct our current clock, as it would start going wrong otherwise
        if(m_State.timestamp > 0)
        {
          double offset;
          offset  = m_av_clock.GetAbsoluteClock() - m_State.timestamp;
          offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
          if(offset >  1000) offset =  1000;
          if(offset < -1000) offset = -1000;
          m_State.time     += DVD_TIME_TO_MSEC(offset);
          m_State.timestamp =  m_av_clock.GetAbsoluteClock();
        }

        if (speed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_PAUSE && speed != m_playSpeed)
          m_callback.OnPlayBackSpeedChanged(speed / DVD_PLAYSPEED_NORMAL);

        if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) && speed != m_playSpeed)
        {
          CDVDInputStreamPVRManager* pvrinputstream = static_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
          pvrinputstream->Pause( speed == 0 );
        }

        // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
        // audioplayer, stops outputing audio to audiorendere, but still tries to
        // sleep an correct amount for each packet
        // videoplayer just plays faster after the clock speed has been increased
        // 1. disable audio
        // 2. skip frames and adjust their pts or the clock
        m_playSpeed = speed;
        m_caching = CACHESTATE_DONE;
        m_av_clock.SetSpeed(speed);
        m_player_audio.SetSpeed(speed);
        m_player_video.SetSpeed(speed);
        m_av_clock.OMXSetSpeed(m_playSpeed);

        // TODO - we really shouldn't pause demuxer
        //        until our buffers are somewhat filled
        if(m_pDemuxer)
          m_pDemuxer->SetSpeed(speed);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) == 0)
      {
        FlushBuffers(false);
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input && input->SelectChannelByNumber(static_cast<CDVDMsgInt*>(pMsg)->m_value))
        {
          SAFE_DELETE(m_pDemuxer);
        }else
        {
          CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
          CApplicationMessenger::Get().MediaStop(false);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT) == 0)
      {
        FlushBuffers(false);
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input && input->SelectChannel(static_cast<CDVDMsgType <CPVRChannel> *>(pMsg)->m_value))
        {
          SAFE_DELETE(m_pDemuxer);
        }else
        {
          CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
          CApplicationMessenger::Get().MediaStop(false);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREV))
      {
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input)
        {
          bool bSwitchSuccessful(false);
          bool bShowPreview(g_guiSettings.GetInt("pvrplayback.channelentrytimeout") > 0);

          if (!bShowPreview)
          {
            g_infoManager.SetDisplayAfterSeek(100000);
            FlushBuffers(false);
          }

          if(pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT))
            bSwitchSuccessful = input->NextChannel(bShowPreview);
          else
            bSwitchSuccessful = input->PrevChannel(bShowPreview);

          if(bSwitchSuccessful)
          {
            if (bShowPreview)
            {
              UpdateApplication(0);
              m_iChannelEntryTimeOut = XbmcThreads::SystemClockMillis() + g_guiSettings.GetInt("pvrplayback.channelentrytimeout");
            }
            else
            {
              m_iChannelEntryTimeOut = 0;
              SAFE_DELETE(m_pDemuxer);

              g_infoManager.SetDisplayAfterSeek();
            }
          }
          else
          {
            CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
            CApplicationMessenger::Get().MediaStop(false);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
        OnAction(((CDVDMsgType<CAction>*)pMsg)->m_value);
      else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
      {
        int player = ((CDVDMsgInt*)pMsg)->m_value;
        if(player == DVDPLAYER_AUDIO)
          m_CurrentAudio.started = true;
        if(player == DVDPLAYER_VIDEO)
          m_CurrentVideo.started = true;
        CLog::Log(LOGDEBUG, "COMXPlayer::HandleMessages - player started %d", player);
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when handling message", __FUNCTION__);
    }

    pMsg->Release();
  }
}

void COMXPlayer::SetCaching(ECacheState state)
{
  if(state == CACHESTATE_FLUSH)
  {
    double level, delay, offset;
    if(GetCachingTimes(level, delay, offset))
      state = CACHESTATE_FULL;
    else
      state = CACHESTATE_INIT;
  }

  if(m_caching == state)
    return;

  CLog::Log(LOGDEBUG, "COMXPlayer::SetCaching - caching state %d", state);
  if(state == CACHESTATE_FULL
  || state == CACHESTATE_INIT
  || state == CACHESTATE_PVR)
  {
    m_av_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_av_clock.OMXSetSpeed(DVD_PLAYSPEED_PAUSE);
    m_player_audio.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_player_audio.SendMessage(new CDVDMsg(CDVDMsg::PLAYER_STARTED), 1);
    m_player_video.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_player_video.SendMessage(new CDVDMsg(CDVDMsg::PLAYER_STARTED), 1);

    if (state == CACHESTATE_PVR)
      m_pInputStream->ResetScanTimeout((unsigned int) g_guiSettings.GetInt("pvrplayback.scantime") * 1000);
  }

  if(state == CACHESTATE_PLAY
  ||(state == CACHESTATE_DONE && m_caching != CACHESTATE_PLAY))
  {
    m_av_clock.SetSpeed(m_playSpeed);
    m_av_clock.OMXSetSpeed(m_playSpeed);
    m_player_audio.SetSpeed(m_playSpeed);
    m_player_video.SetSpeed(m_playSpeed);
  }
  m_caching = state;
}

void COMXPlayer::SetPlaySpeed(int speed)
{
  /* only pause and normal playspeeds are allowed */
  if(speed < 0 || speed > DVD_PLAYSPEED_NORMAL)
    return;

  m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
  m_player_audio.SetSpeed(speed);
  m_player_video.SetSpeed(speed);
  SynchronizeDemuxer(100);
}

bool COMXPlayer::CanPause()
{
  CSingleLock lock(m_StateSection);
  return m_State.canpause;
}

void COMXPlayer::Pause()
{
  CSingleLock lock(m_StateSection);
  if (!m_State.canpause)
    return;
  lock.Leave();

  if(m_playSpeed != DVD_PLAYSPEED_PAUSE && (m_caching == CACHESTATE_FULL || m_caching == CACHESTATE_PVR))
  {
    SetCaching(CACHESTATE_DONE);
    return;
  }

  // return to normal speed if it was paused before, pause otherwise
  if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
  {
    SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
    m_callback.OnPlayBackResumed();
  }
  else
  {
    SetPlaySpeed(DVD_PLAYSPEED_PAUSE);
    m_callback.OnPlayBackPaused();
  }
}

bool COMXPlayer::IsPaused() const
{
  return m_playSpeed == DVD_PLAYSPEED_PAUSE || m_caching == CACHESTATE_FULL || m_caching == CACHESTATE_PVR;
}

bool COMXPlayer::HasVideo() const
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)) return true;

  return m_SelectionStreams.Count(STREAM_VIDEO) > 0 ? true : false;
}

bool COMXPlayer::HasAudio() const
{
  return m_SelectionStreams.Count(STREAM_AUDIO) > 0 ? true : false;
}

bool COMXPlayer::IsPassthrough() const
{
  return m_player_audio.Passthrough();
}

bool COMXPlayer::CanSeek()
{
  CSingleLock lock(m_StateSection);
  return m_State.canseek;
}

void COMXPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (!m_State.canseek)
    return;

  if(((bPlus && GetChapter() < GetChapterCount())
  || (!bPlus && GetChapter() > 1)) && bLargeStep)
  {
    if(bPlus)
      SeekChapter(GetChapter() + 1);
    else
      SeekChapter(GetChapter() - 1);
    return;
  }

  int64_t seek;
  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2000*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;
    seek = (int64_t)(GetTotalTimeInMsec()*(GetPercentage()+percent)/100);
  }

  bool restore = true;

  if (m_Edl.HasCut())
  {
    /*
     * Alter the standard seek position based on whether any commercial breaks have been
     * automatically skipped.
     */
    const int clock = DVD_TIME_TO_MSEC(m_av_clock.GetClock());
    /*
     * If a large backwards seek occurs within 10 seconds of the end of the last automated
     * commercial skip, then seek back to the start of the commercial break under the assumption
     * it was flagged incorrectly. 10 seconds grace period is allowed in case the watcher has to
     * fumble around finding the remote. Only happens once per commercial break.
     *
     * Small skip does not trigger this in case the start of the commercial break was in fact fine
     * but it skipped too far into the program. In that case small skip backwards behaves as normal.
     */
    if (!bPlus && bLargeStep
    &&  m_EdlAutoSkipMarkers.seek_to_start
    &&  clock >= m_EdlAutoSkipMarkers.commbreak_end
    &&  clock <= m_EdlAutoSkipMarkers.commbreak_end + 10*1000) // Only if within 10 seconds of the end (in msec)
    {
      CLog::Log(LOGDEBUG, "%s - Seeking back to start of commercial break [%s - %s] as large backwards skip activated within 10 seconds of the automatic commercial skip (only done once per break).",
                __FUNCTION__, CEdl::MillisecondsToTimeString(m_EdlAutoSkipMarkers.commbreak_start).c_str(),
                CEdl::MillisecondsToTimeString(m_EdlAutoSkipMarkers.commbreak_end).c_str());
      seek = m_EdlAutoSkipMarkers.commbreak_start;
      restore = false;
      m_EdlAutoSkipMarkers.seek_to_start = false; // So this will only happen within the 10 second grace period once.
    }
    /*
     * If big skip forward within the last "reverted" commercial break, seek to the end of the
     * commercial break under the assumption that the break was incorrectly flagged and playback has
     * now reached the actual start of the commercial break. Assume that the end is flagged more
     * correctly than the landing point for a standard big skip (ends seem to be flagged more
     * accurately than the start).
     */
    else if (bPlus && bLargeStep
    &&       clock >= m_EdlAutoSkipMarkers.commbreak_start
    &&       clock <= m_EdlAutoSkipMarkers.commbreak_end)
    {
      CLog::Log(LOGDEBUG, "%s - Seeking to end of previously skipped commercial break [%s - %s] as big forwards skip activated within the break.",
                __FUNCTION__, CEdl::MillisecondsToTimeString(m_EdlAutoSkipMarkers.commbreak_start).c_str(),
                CEdl::MillisecondsToTimeString(m_EdlAutoSkipMarkers.commbreak_end).c_str());
      seek = m_EdlAutoSkipMarkers.commbreak_end;
      restore = false;
    }
  }

  int64_t time = GetTime();
  if(g_application.CurrentFileItem().IsStack()
  && (seek > GetTotalTimeInMsec() || seek < 0))
  {
    g_application.SeekTime((seek - time) * 0.001 + g_application.GetTime());
    // warning, don't access any dvdplayer variables here as
    // the dvdplayer object may have been destroyed
    return;
  }

  m_messenger.Put(new CDVDMsgPlayerSeek((int)seek, !bPlus, true, false, restore));
  SynchronizeDemuxer(100);
  if (seek < 0) seek = 0;
  m_callback.OnPlayBackSeek((int)seek, (int)(seek - time));
}

bool COMXPlayer::SeekScene(bool bPlus)
{
  if (!m_Edl.HasSceneMarker())
    return false;

  /*
   * There is a 5 second grace period applied when seeking for scenes backwards. If there is no
   * grace period applied it is impossible to go backwards past a scene marker.
   */
  int64_t clock = GetTime();
  if (!bPlus && clock > 5 * 1000) // 5 seconds
    clock -= 5 * 1000;

  int64_t iScenemarker;
  if (m_Edl.GetNextSceneMarker(bPlus, clock, &iScenemarker))
  {
    /*
     * Seeking is flushed and inaccurate, just like Seek()
     */
    m_messenger.Put(new CDVDMsgPlayerSeek((int)iScenemarker, !bPlus, true, false, false));
    SynchronizeDemuxer(100);
    return true;
  }
  return false;
}

void COMXPlayer::GetAudioInfo(CStdString &strAudioInfo)
{
  { CSingleLock lock(m_StateSection);
    strAudioInfo.Format("D(%s)", m_State.demux_audio.c_str());
  }
  strAudioInfo.AppendFormat(" P(%s)", m_player_audio.GetPlayerInfo().c_str());
}

void COMXPlayer::GetVideoInfo(CStdString &strVideoInfo)
{
  { CSingleLock lock(m_StateSection);
    strVideoInfo.Format("D(%s)", m_State.demux_video.c_str());
  }
  strVideoInfo.AppendFormat(" P(%s)", m_player_video.GetPlayerInfo().c_str());
}

void COMXPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  if (!m_bStop)
  {
    double dDelay = m_player_video.GetDelay() / DVD_TIME_BASE;

    double apts = m_player_audio.GetCurrentPTS();
    double vpts = m_player_video.GetCurrentPTS();
    double dDiff = 0;

    if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
      dDiff = (apts - vpts) / DVD_TIME_BASE;

    CStdString strEDL;
    strEDL.AppendFormat(", edl:%s", m_Edl.GetInfo().c_str());

    CStdString strBuf;
    CSingleLock lock(m_StateSection);
    if(m_State.cache_bytes >= 0)
    {
      strBuf.AppendFormat(" cache:%s %2.0f%%"
                         , StringUtils::SizeToString(m_State.cache_bytes).c_str()
                         , m_State.cache_level * 100);
      if(m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
        strBuf.AppendFormat(" %d sec", DVD_TIME_TO_SEC(m_State.cache_delay));
    }

    strGeneralInfo.Format("C( ad:% 6.3f, a/v:% 6.3f%s, dcpu:%2i%% acpu:%2i%% vcpu:%2i%%%s, omx vb:%8d ad:% 6.3f )"
                         , dDelay
                         , dDiff
                         , strEDL.c_str()
                         , (int)(CThread::GetRelativeUsage()*100)
                         , (int)(m_player_audio.GetRelativeUsage()*100)
                         , (int)(m_player_video.GetRelativeUsage()*100)
                         , strBuf.c_str()
                         , m_player_video.GetFreeSpace()
                         , m_player_audio.GetDelay());

  }
}

void COMXPlayer::SeekPercentage(float fPercent)
{
  int64_t iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return;

  SeekTime((int64_t)(iTotalTime * fPercent / 100));
}

float COMXPlayer::GetPercentage()
{
  int64_t iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

float COMXPlayer::GetCachePercentage()
{
  CSingleLock lock(m_StateSection);
  return m_State.cache_offset * 100; // NOTE: Percentage returned is relative
}

void COMXPlayer::SetAVDelay(float fValue)
{
  m_player_video.SetDelay(fValue * DVD_TIME_BASE);
}

float COMXPlayer::GetAVDelay()
{
  return m_player_video.GetDelay() / (float)DVD_TIME_BASE;
}

void COMXPlayer::SetSubTitleDelay(float fValue)
{
  m_player_video.SetSubtitleDelay(-fValue * DVD_TIME_BASE);
}

float COMXPlayer::GetSubTitleDelay()
{
  return -m_player_video.GetSubtitleDelay() / DVD_TIME_BASE;
}

int COMXPlayer::GetSubtitleCount()
{
  OMXStreamLock lock(this);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
  return m_SelectionStreams.Count(STREAM_SUBTITLE);
}

int COMXPlayer::GetSubtitle()
{
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, *this);
}

void COMXPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  strStreamName = "";
  OMXSelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, iStream);
  if(s.name.length() > 0)
    strStreamName = s.name;
  else
    strStreamName = g_localizeStrings.Get(13205); // Unknown

  if(s.type == STREAM_NONE)
    strStreamName += "(Invalid)";
}

void COMXPlayer::GetSubtitleLanguage(int iStream, CStdString &strStreamLang)
{
  OMXSelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, iStream);
  if (!g_LangCodeExpander.Lookup(strStreamLang, s.language))
    strStreamLang = g_localizeStrings.Get(13205); // Unknown
}

void COMXPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetSubtitleStream(iStream));
}

bool COMXPlayer::GetSubtitleVisible()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    if(pStream->IsInMenu())
      return g_settings.m_currentVideoSettings.m_SubtitleOn;
    else
      return pStream->IsSubtitleStreamEnabled();
  }

  return m_player_video.IsSubtitleEnabled();
}

void COMXPlayer::SetSubtitleVisible(bool bVisible)
{
  g_settings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
}

int COMXPlayer::GetAudioStreamCount()
{
  OMXStreamLock lock(this);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
  return m_SelectionStreams.Count(STREAM_AUDIO);
}

int COMXPlayer::GetAudioStream()
{
  return m_SelectionStreams.IndexOf(STREAM_AUDIO, *this);
}

void COMXPlayer::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  strStreamName = "";
  OMXSelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, iStream);
  if(s.name.length() > 0)
    strStreamName += s.name;
  else
    strStreamName += "Unknown";

  if(s.type == STREAM_NONE)
    strStreamName += " (Invalid)";
}
 
void COMXPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
  SynchronizeDemuxer(100);
}

void COMXPlayer::SeekTime(int64_t iTime)
{
  int seekOffset = (int)(iTime - GetTime());
  m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime, true, true, true));
  SynchronizeDemuxer(100);
  m_callback.OnPlayBackSeek((int)iTime, seekOffset);
}

// return the time in milliseconds
int64_t COMXPlayer::GetTime()
{
  CSingleLock lock(m_StateSection);
  double offset = 0;
  if(m_State.timestamp > 0)
  {
    offset  = m_av_clock.GetAbsoluteClock() - m_State.timestamp;
    offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
    if(offset >  1000) offset =  1000;
    if(offset < -1000) offset = -1000;
  }
  return llrint(m_State.time + DVD_TIME_TO_MSEC(offset));
}

// return length in msec
int64_t COMXPlayer::GetTotalTimeInMsec()
{
  CSingleLock lock(m_StateSection);
  return llrint(m_State.time_total);
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int64_t COMXPlayer::GetTotalTime()
{
  return GetTotalTimeInMsec();
}

void COMXPlayer::ToFFRW(int iSpeed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if (iSpeed < 0 && IsInMenu()) return;

  /* only pause and normal playspeeds are allowed */
  if(iSpeed > 1 || iSpeed < 0)
    return;

  SetPlaySpeed(iSpeed * DVD_PLAYSPEED_NORMAL);
}

bool COMXPlayer::OpenAudioStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening audio stream: %i source: %i", iStream, source);

  if (!m_pDemuxer)
    return false;

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if (!pStream || pStream->disabled)
    return false;

  if( m_CurrentAudio.id < 0 &&  m_CurrentVideo.id >= 0 )
  {
    // up until now we wheren't playing audio, but we did play video
    // this will change what is used to sync the dvdclock.
    // since the new audio data doesn't have to have any relation
    // to the current video data in the packet que, we have to
    // wait for it to empty

    // this happens if a new cell has audio data, but previous didn't
    // and both have video data

    SynchronizePlayers(SYNCSOURCE_AUDIO);
  }

  CDVDStreamInfo hint(*pStream, true);

  if(m_CurrentAudio.id    < 0
  || m_CurrentAudio.hint != hint)
  {
    if(!m_player_audio.OpenStream(hint))
    {
      /* mark stream as disabled, to disallaw further attempts*/
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      pStream->disabled = true;
      pStream->SetDiscard(AVDISCARD_ALL);
      return false;
    }
    m_av_clock.SetSpeed(DVD_PLAYSPEED_NORMAL);
    m_av_clock.OMXSetSpeed(DVD_PLAYSPEED_NORMAL);
  }
  else
    m_player_audio.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));

  /* store information about stream */
  m_CurrentAudio.id = iStream;
  m_CurrentAudio.source = source;
  m_CurrentAudio.hint = hint;
  m_CurrentAudio.stream = (void*)pStream;
  m_CurrentAudio.started = false;

  /* we are potentially going to be waiting on this */
  m_player_audio.SendMessage(new CDVDMsg(CDVDMsg::PLAYER_STARTED), 1);

  /* software decoding normaly consumes full cpu time so prio it */
  m_player_audio.SetPriority(GetPriority()+1);

  return true;
}

bool COMXPlayer::OpenVideoStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening video stream: %i source: %i", iStream, source);

  if (!m_pDemuxer)
    return false;

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if(!pStream || pStream->disabled)
    return false;
  pStream->SetDiscard(AVDISCARD_NONE);

  CDVDStreamInfo hint(*pStream, true);

  if( m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) )
  {
    /* set aspect ratio as requested by navigator for dvd's */
    float aspect = static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->GetVideoAspectRatio();
    if(aspect != 0.0)
    {
      hint.aspect = aspect;
      hint.forced_aspect = true;
    }
    hint.software = true;
  }

  CDVDInputStream::IMenus* pMenus = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if(pMenus && pMenus->IsInMenu())
    hint.stills = true;

  if(m_CurrentVideo.id    < 0
  || m_CurrentVideo.hint != hint)
  {
    if(!m_player_video.OpenStream(hint))
    {
      /* mark stream as disabled, to disallaw further attempts */
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      pStream->disabled = true;
      pStream->SetDiscard(AVDISCARD_ALL);
      return false;
    }
    m_av_clock.SetSpeed(DVD_PLAYSPEED_NORMAL);
    m_av_clock.OMXSetSpeed(DVD_PLAYSPEED_NORMAL);
  }
  else
    m_player_video.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));

  unsigned flags = 0;
  if(m_filename.find("3DSBS") != string::npos) 
    flags = CONF_FLAGS_FORMAT_SBS;
  m_player_video.SetFlags(flags);

  /* store information about stream */
  m_CurrentVideo.id = iStream;
  m_CurrentVideo.source = source;
  m_CurrentVideo.hint = hint;
  m_CurrentVideo.stream = (void*)pStream;
  m_CurrentVideo.started = false;

  /* we are potentially going to be waiting on this */
  m_player_video.SendMessage(new CDVDMsg(CDVDMsg::PLAYER_STARTED), 1);

  /* use same priority for video thread as demuxing thread, as */
  /* otherwise demuxer will starve if video consumes the full cpu */
  m_player_video.SetPriority(GetPriority());

  return true;
}

bool COMXPlayer::OpenSubtitleStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening Subtitle stream: %i source: %i", iStream, source);

  CDemuxStream* pStream = NULL;
  std::string filename;
  CDVDStreamInfo hint;

  if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
  {
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, source, iStream);
    if(index < 0)
      return false;
    OMXSelectionStream st = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

    if(!m_pSubtitleDemuxer || m_pSubtitleDemuxer->GetFileName() != st.filename)
    {
      CLog::Log(LOGNOTICE, "Opening Subtitle file: %s", st.filename.c_str());
      auto_ptr<CDVDDemuxVobsub> demux(new CDVDDemuxVobsub());
      if(!demux->Open(st.filename, st.filename2))
        return false;
      m_pSubtitleDemuxer = demux.release();
    }

    pStream = m_pSubtitleDemuxer->GetStream(iStream);
    if(!pStream || pStream->disabled)
      return false;
    pStream->SetDiscard(AVDISCARD_NONE);
    double pts = m_player_video.GetCurrentPTS();
    if(pts == DVD_NOPTS_VALUE)
      pts = m_CurrentVideo.dts;
    if(pts == DVD_NOPTS_VALUE)
      pts = 0;
    pts += m_offset_pts;
    m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE));

    hint.Assign(*pStream, true);
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
  {
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, source, iStream);
    if(index < 0)
      return false;
    filename = m_SelectionStreams.Get(STREAM_SUBTITLE, index).filename;

    hint.Clear();
    hint.fpsscale = m_CurrentVideo.hint.fpsscale;
    hint.fpsrate  = m_CurrentVideo.hint.fpsrate;
  }
  else
  {
    if(!m_pDemuxer)
      return false;
    pStream = m_pDemuxer->GetStream(iStream);
    if(!pStream || pStream->disabled)
      return false;
    pStream->SetDiscard(AVDISCARD_NONE);

    hint.Assign(*pStream, true);

    if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      filename = "dvd";
  }

  if(m_CurrentSubtitle.id    < 0
  || m_CurrentSubtitle.hint != hint)
  {
    if(m_CurrentSubtitle.id >= 0)
    {
      CLog::Log(LOGDEBUG, " - codecs hints have changed, must close previous stream");
      CloseSubtitleStream(false);
    }

    if(!m_player_subtitle.OpenStream(hint, filename))
    {
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      if(pStream)
      {
        pStream->disabled = true;
        pStream->SetDiscard(AVDISCARD_ALL);
      }
      return false;
    }
  }
  else
    m_player_subtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));

  m_CurrentSubtitle.id     = iStream;
  m_CurrentSubtitle.source = source;
  m_CurrentSubtitle.hint   = hint;
  m_CurrentSubtitle.stream = (void*)pStream;
  m_CurrentSubtitle.started = false;

  return true;
}

bool COMXPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  if (m_CurrentAudio.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing audio stream");

  if(bWaitForBuffers)
    SetCaching(CACHESTATE_DONE);

  m_player_audio.CloseStream(bWaitForBuffers);

  m_CurrentAudio.Clear();
  return true;
}

bool COMXPlayer::CloseVideoStream(bool bWaitForBuffers)
{
  if (m_CurrentVideo.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing video stream");

  if(bWaitForBuffers)
    SetCaching(CACHESTATE_DONE);

  m_player_video.CloseStream(bWaitForBuffers);

  m_CurrentVideo.Clear();
  return true;
}

bool COMXPlayer::CloseSubtitleStream(bool bKeepOverlays)
{
  if (m_CurrentSubtitle.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing subtitle stream");

  m_player_subtitle.CloseStream(!bKeepOverlays);

  m_CurrentSubtitle.Clear();
  return true;
}

void COMXPlayer::FlushBuffers(bool queued, double pts, bool accurate)
{
  double startpts;
  if(accurate)
    startpts = pts;
  else
    startpts = DVD_NOPTS_VALUE;

  /* call with demuxer pts */
  if(startpts != DVD_NOPTS_VALUE)
    startpts -= m_offset_pts;

  m_CurrentAudio.inited      = false;
  m_CurrentAudio.dts         = DVD_NOPTS_VALUE;
  m_CurrentAudio.startpts    = startpts;

  m_CurrentVideo.inited      = false;
  m_CurrentVideo.dts         = DVD_NOPTS_VALUE;
  m_CurrentVideo.startpts    = startpts;

  m_CurrentSubtitle.inited   = false;
  m_CurrentSubtitle.dts      = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.startpts = startpts;

  m_CurrentTeletext.inited   = false;
  m_CurrentTeletext.dts      = DVD_NOPTS_VALUE;
  m_CurrentTeletext.startpts = startpts;

  if(queued)
  {
    m_player_audio.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_player_video.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_player_video.SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
    m_player_subtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    SynchronizePlayers(SYNCSOURCE_ALL);
  }
  else
  {
    m_player_video.Flush();
    m_player_audio.Flush();
    m_player_subtitle.Flush();

    // clear subtitle and menu overlays
    m_overlayContainer.Clear();

    if(m_playSpeed == DVD_PLAYSPEED_NORMAL
    || m_playSpeed == DVD_PLAYSPEED_PAUSE)
    {
      // make sure players are properly flushed, should put them in stalled state
      CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(1000, 0);
      m_player_video.SendMessage(msg->Acquire(), 1);
      m_player_audio.SendMessage(msg->Acquire(), 1);
      msg->Wait(&m_bStop, 0);
      msg->Release();

      // purge any pending PLAYER_STARTED messages
      m_messenger.Flush(CDVDMsg::PLAYER_STARTED);

      // we should now wait for init cache
      SetCaching(CACHESTATE_FLUSH);
      m_CurrentAudio.started    = false;
      m_CurrentVideo.started    = false;
      m_CurrentSubtitle.started = false;
      m_CurrentTeletext.started = false;
    }

    if(pts != DVD_NOPTS_VALUE)
      m_av_clock.Discontinuity(pts);
    UpdatePlayState(0);
  }
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int COMXPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if(iMessage == 0)
      m_overlayContainer.Add((CDVDOverlay*)pData);
    else if(iMessage == 1)
      m_messenger.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
    else if(iMessage == 2)
      m_dvd.iSelectedAudioStream = *(int*)pData;
    else if(iMessage == 3)
      m_dvd.iSelectedSPUStream   = *(int*)pData;
    else if(iMessage == 4)
      m_player_video.EnableSubtitle(*(int*)pData ? true: false);

    return 0;
  }

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (iMessage)
    {
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

        dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)pData;
        // should wait the specified time here while we let the player running
        // after that call dvdnav_still_skip(m_dvdnav);

        if (m_dvd.state != DVDSTATE_STILL)
        {
          // else notify the player we have received a still frame

          if(still_event->length < 0xff)
            m_dvd.iDVDStillTime = still_event->length * 1000;
          else
            m_dvd.iDVDStillTime = 0;

          m_dvd.iDVDStillStartTime = XbmcThreads::SystemClockMillis();

          /* adjust for the output delay in the video queue */
          DWORD time = 0;
          if( m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0 )
          {
            time = (DWORD)(m_player_video.GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
            if( time < 10000 && time > 0 )
              m_dvd.iDVDStillTime += time;
          }
          m_dvd.state = DVDSTATE_STILL;
          CLog::Log(LOGDEBUG,
                    "DVDNAV_STILL_FRAME - waiting %i sec, with delay of %d sec",
                    still_event->length, time / 1000);
        }
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      {
        m_player_subtitle.SendMessage(new CDVDMsgSubtitleClutChange((BYTE*)pData));
      }
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      {
        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        bool visible = !(iStream & 0x80);

        m_player_video.EnableSubtitle(visible);

        if (iStream >= 0)
          m_dvd.iSelectedSPUStream = (iStream & ~0x80);
        else
          m_dvd.iSelectedSPUStream = -1;

        m_CurrentSubtitle.stream = NULL;
      }
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      {
        // This should be the correct way i think, however we don't have any streams right now
        // since the demuxer hasn't started so it doesn't change. not sure how to do this.
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;

        // Tell system what audiostream should be opened by default
        if (event->logical >= 0)
          m_dvd.iSelectedAudioStream = event->physical;
        else
          m_dvd.iSelectedAudioStream = -1;

        m_CurrentAudio.stream = NULL;
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        //dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);
        m_player_subtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        //dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();

        //Force an aspect ratio that is set in the dvdheaders if available
        m_CurrentVideo.hint.aspect = pStream->GetVideoAspectRatio();
        if( m_player_video.IsInited() )
          m_player_video.SendMessage(new CDVDMsgDouble(CDVDMsg::VIDEO_SET_ASPECT, m_CurrentVideo.hint.aspect));

        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_CELL_CHANGE:
      {
        //dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");

        m_dvd.state = DVDSTATE_NORMAL;

        if( m_player_video.IsInited() )
          m_player_video.SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
      }
      break;
    case DVDNAV_NAV_PACKET:
      {
          //pci_t* pci = (pci_t*)pData;

          // this should be possible to use to make sure we get
          // seamless transitions over these boundaries
          // if we remember the old vobunits boundaries
          // when a packet comes out of demuxer that has
          // pts values outside that boundary, it belongs
          // to the new vobunit, wich has new timestamps
          UpdatePlayState(0);
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      {
        // This event is issued whenever a non-seamless operation has been executed.
        // Applications with fifos should drop the fifos content to speed up responsiveness.
        CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");
        if(m_dvd.state == DVDSTATE_SEEK)
          m_dvd.state = DVDSTATE_NORMAL;
        else
          m_messenger.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));

        return NAVRESULT_ERROR;
      }
      break;
    case DVDNAV_STOP:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_STOP");
        m_dvd.state = DVDSTATE_NORMAL;
      }
      break;
    default:
    {}
      break;
    }
  }
  return NAVRESULT_NOP;
}

bool COMXPlayer::ShowPVRChannelInfo(void)
{
  bool bReturn(false);

  if (g_guiSettings.GetBool("pvrmenu.infoswitch"))
  {
    int iTimeout = g_guiSettings.GetBool("pvrmenu.infotimeout") ? g_guiSettings.GetInt("pvrmenu.infotime") : 0;
    g_PVRManager.ShowPlayerInfo(iTimeout);

    bReturn = true;
  }

  return bReturn;
}

bool COMXPlayer::OnAction(const CAction &action)
{
#define THREAD_ACTION(action) \
  do { \
    if (!IsCurrentThread()) { \
      m_messenger.Put(new CDVDMsgType<CAction>(CDVDMsg::GENERAL_GUI_ACTION, action)); \
      return true; \
    } \
  } while(false)

  CDVDInputStream::IMenus* pMenus = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if (pMenus)
  {
    if( m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime != 0 && pMenus->GetTotalButtons() == 0 )
    {
      switch(action.GetID())
      {
        case ACTION_NEXT_ITEM:
        case ACTION_MOVE_RIGHT:
        case ACTION_MOVE_UP:
        case ACTION_SELECT_ITEM:
          {
            THREAD_ACTION(action);
            /* this will force us out of the stillframe */
            CLog::Log(LOGDEBUG, "%s - User asked to exit stillframe", __FUNCTION__);
            m_dvd.iDVDStillStartTime = 0;
            m_dvd.iDVDStillTime = 1;
          }
          return true;
      }
    }


    switch (action.GetID())
    {
/* this code is disabled to allow switching playlist items (dvdimage "stacks") */
#if 0
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed prev");
        pMenus->OnPrevious();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed next");
        pMenus->OnNext();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
#endif
    case ACTION_SHOW_VIDEOMENU:   // start button
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - go to menu");
        pMenus->OnMenu();
        // send a message to everyone that we've gone to the menu
        CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
        g_windowManager.SendMessage(msg);
        return true;
      }
      break;
    }
    if (pMenus->IsInMenu())
    {
      switch (action.GetID())
      {
      case ACTION_NEXT_ITEM:
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed next in menu, stream will decide");
        pMenus->OnNext();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      case ACTION_PREV_ITEM:
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed prev in menu, stream will decide");
        pMenus->OnPrevious();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      case ACTION_PREVIOUS_MENU:
      case ACTION_NAV_BACK:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - menu back");
          pMenus->OnBack();
        }
        break;
      case ACTION_MOVE_LEFT:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move left");
          pMenus->OnLeft();
        }
        break;
      case ACTION_MOVE_RIGHT:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move right");
          pMenus->OnRight();
        }
        break;
      case ACTION_MOVE_UP:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move up");
          pMenus->OnUp();
        }
        break;
      case ACTION_MOVE_DOWN:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move down");
          pMenus->OnDown();
        }
        break;

      case ACTION_MOUSE_MOVE:
      case ACTION_MOUSE_LEFT_CLICK:
        {
          CRect rs, rd;
          GetVideoRect(rs, rd);
          CPoint pt(action.GetAmount(), action.GetAmount(1));
          if (!rd.PtInRect(pt))
            return false; // out of bounds
          THREAD_ACTION(action);
          // convert to video coords...
          pt -= CPoint(rd.x1, rd.y1);
          pt.x *= rs.Width() / rd.Width();
          pt.y *= rs.Height() / rd.Height();
          pt += CPoint(rs.x1, rs.y1);
          if (action.GetID() == ACTION_MOUSE_LEFT_CLICK)
            return pMenus->OnMouseClick(pt);
          return pMenus->OnMouseMove(pt);
        }
        break;
      case ACTION_SELECT_ITEM:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - button select");
          // show button pushed overlay
          if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
            m_player_subtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_CLICKED);

          pMenus->ActivateButton();
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
        {
          THREAD_ACTION(action);
          // Offset from key codes back to button number
          int button = action.GetID() - REMOTE_0;
          CLog::Log(LOGDEBUG, " - button pressed %d", button);
          pMenus->SelectButton(button);
        }
       break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }
  if (dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream))
  {
    switch (action.GetID())
    {
      case ACTION_MOVE_UP:
      case ACTION_NEXT_ITEM:
        m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_NEXT));
        g_infoManager.SetDisplayAfterSeek();
        ShowPVRChannelInfo();
        return true;
      break;

      case ACTION_MOVE_DOWN:
      case ACTION_PREV_ITEM:
        m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_PREV));
        g_infoManager.SetDisplayAfterSeek();
        ShowPVRChannelInfo();
        return true;
      break;

      case ACTION_CHANNEL_SWITCH:
      {
        // Offset from key codes back to button number
        int channel = action.GetAmount();
        m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER, channel));
        g_infoManager.SetDisplayAfterSeek();
        ShowPVRChannelInfo();
        return true;
      }
      break;
    }
  }

  switch (action.GetID())
  {
    case ACTION_NEXT_ITEM:
      if(GetChapterCount() > 0)
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter()+1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
      if(GetChapterCount() > 0)
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter()-1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool COMXPlayer::IsInMenu() const
{
  CDVDInputStream::IMenus* pStream = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if (pStream)
  {
    if( m_dvd.state == DVDSTATE_STILL )
      return true;
    else
      return pStream->IsInMenu();
  }
  return false;
}

bool COMXPlayer::HasMenu()
{
  CDVDInputStream::IMenus* pStream = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if (pStream)
    return true;
  else
    return false;
}

bool COMXPlayer::GetCurrentSubtitle(CStdString& strSubtitle)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return false;

  double pts = m_av_clock.OMXMediaTime();

  m_player_subtitle.GetCurrentSubtitle(strSubtitle, pts - m_player_video.GetSubtitleDelay());

  // In case we stalled, don't output any subs
  if ((m_player_video.IsStalled() && HasVideo()) || (m_player_audio.IsStalled() && HasAudio()))
    strSubtitle = m_lastSub;
  else
    m_lastSub = strSubtitle;

  return !strSubtitle.IsEmpty();
}
  
CStdString COMXPlayer::GetPlayerState()
{
  CSingleLock lock(m_StateSection);
  return m_State.player_state;
}

bool COMXPlayer::SetPlayerState(CStdString state)
{
  m_messenger.Put(new CDVDMsgPlayerSetState(state));
  return true;
}

int COMXPlayer::GetChapterCount()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapter_count;
}

int COMXPlayer::GetChapter()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapter;
}

void COMXPlayer::GetChapterName(CStdString& strChapterName)
{
  CSingleLock lock(m_StateSection);
  strChapterName = m_State.chapter_name;
}

int COMXPlayer::SeekChapter(int iChapter)
{
  if (GetChapterCount() > 0)
  {
    if (iChapter < 0)
      iChapter = 0;
    if (iChapter > GetChapterCount())
      return 0;

    // Seek to the chapter.
    m_messenger.Put(new CDVDMsgPlayerSeekChapter(iChapter));
    SynchronizeDemuxer(100);
  }
  else
  {
    // Do a regular big jump.
    if (GetChapter() > 0 && iChapter > GetChapter())
      Seek(true, true);
    else
      Seek(false, true);
  }
  return 0;
}

int COMXPlayer::AddSubtitle(const CStdString& strSubPath)
{
  return AddSubtitleFile(strSubPath);
}

int COMXPlayer::GetCacheLevel() const
{
  CSingleLock lock(m_StateSection);
  return (int)(m_State.cache_level * 100);
}

double COMXPlayer::GetQueueTime()
{
  int a = m_player_video.GetLevel();
  int v = m_player_audio.GetLevel();
  return max(a, v) * 8000.0 / 100;
}

int COMXPlayer::GetAudioBitrate()
{
  return m_player_audio.GetAudioBitrate();
}

int COMXPlayer::GetVideoBitrate()
{
  return m_player_video.GetVideoBitrate();
}

int COMXPlayer::GetSourceBitrate()
{
  if (m_pInputStream)
    return (int)m_pInputStream->GetBitstreamStats().GetBitrate();

  return 0;
}

int COMXPlayer::AddSubtitleFile(const std::string& filename, const std::string& subfilename, CDemuxStream::EFlags flags)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if(ext == ".idx")
  {
    if (vobsubfile.empty())
      vobsubfile = URIUtils::ReplaceExtension(filename, ".sub");

    CDVDDemuxVobsub v;
    if(!v.Open(filename, vobsubfile))
      return -1;
    m_SelectionStreams.Update(NULL, &v);
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename), 0);
    m_SelectionStreams.Get(STREAM_SUBTITLE, index).flags = flags;
    m_SelectionStreams.Get(STREAM_SUBTITLE, index).filename2 = vobsubfile;
    return index;
  }
  if(ext == ".sub")
  {
    CStdString strReplace(URIUtils::ReplaceExtension(filename,".idx"));
    if (XFILE::CFile::Exists(strReplace))
      return -1;
  }
  OMXSelectionStream s;
  s.source   = m_SelectionStreams.Source(STREAM_SOURCE_TEXT, filename);
  s.type     = STREAM_SUBTITLE;
  s.id       = 0;
  s.filename = filename;
  s.name     = URIUtils::GetFileName(filename);
  s.flags    = flags;
  m_SelectionStreams.Update(s);
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, s.source, s.id);
}

void COMXPlayer::UpdatePlayState(double timeout)
{
  if(m_State.timestamp != 0
  && m_State.timestamp + DVD_MSEC_TO_TIME(timeout) > m_av_clock.GetAbsoluteClock())
    return;

  SPlayerState state(m_State);

  if     (m_CurrentVideo.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentVideo.dts;
  else if(m_CurrentAudio.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentAudio.dts;
  else
    state.dts = m_av_clock.GetClock();

  if(m_pDemuxer)
  {
    state.chapter       = m_pDemuxer->GetChapter();
    state.chapter_count = m_pDemuxer->GetChapterCount();
    m_pDemuxer->GetChapterName(state.chapter_name);

    state.time       = DVD_TIME_TO_MSEC(m_av_clock.GetClock() + m_offset_pts);
    state.time_total = m_pDemuxer->GetStreamLength();
  }

  if(m_pInputStream)
  {
    // override from input stream if needed
    CDVDInputStream::IChannel* pChannel = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
    if (pChannel)
    {
      state.canrecord = pChannel->CanRecord();
      state.recording = pChannel->IsRecording();
    }

    CDVDInputStream::IDisplayTime* pDisplayTime = dynamic_cast<CDVDInputStream::IDisplayTime*>(m_pInputStream);
    if (pDisplayTime && pDisplayTime->GetTotalTime() > 0)
    {
      state.time       = pDisplayTime->GetTime();
      state.time_total = pDisplayTime->GetTotalTime();
    }

    if (dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
    {
      if(m_dvd.state == DVDSTATE_STILL)
      {
        state.time       = XbmcThreads::SystemClockMillis() - m_dvd.iDVDStillStartTime;
        state.time_total = m_dvd.iDVDStillTime;
      }
    }

    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
    {
      CDVDInputStreamPVRManager* pvrinputstream = static_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
      state.canpause = pvrinputstream->CanPause();
      state.canseek  = pvrinputstream->CanSeek();
    }
    else
    {
      state.canseek  = state.time_total > 0 ? true : false;
      state.canpause = true;
    }
  }

  if (m_Edl.HasCut())
  {
    state.time        = m_Edl.RemoveCutTime(llrint(state.time));
    state.time_total  = m_Edl.RemoveCutTime(llrint(state.time_total));
  }

  state.player_state = "";
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    state.time_offset = DVD_MSEC_TO_TIME(state.time) - state.dts;
    if(!((CDVDInputStreamNavigator*)m_pInputStream)->GetNavigatorState(state.player_state))
      state.player_state = "";
  }
  else
    state.time_offset = 0;

  if (m_CurrentAudio.id >= 0 && m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentAudio.id);
    if (pStream && pStream->type == STREAM_AUDIO)
      ((CDemuxStreamAudio*)pStream)->GetStreamInfo(state.demux_audio);
  }
  else
    state.demux_audio = "";

  if (m_CurrentVideo.id >= 0 && m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentVideo.id);
    if (pStream && pStream->type == STREAM_VIDEO)
      ((CDemuxStreamVideo*)pStream)->GetStreamInfo(state.demux_video);
  }
  else
    state.demux_video = "";

  double level, delay, offset;
  if(GetCachingTimes(level, delay, offset))
  {
    state.cache_delay  = max(0.0, delay);
    state.cache_level  = max(0.0, min(1.0, level));
    state.cache_offset = offset;
  }
  else
  {
    state.cache_delay  = 0.0;
    state.cache_level  = min(1.0, GetQueueTime() / 8000.0);
    state.cache_offset = GetQueueTime() / state.time_total;
  }

  XFILE::SCacheStatus status;
  if(m_pInputStream && m_pInputStream->GetCacheStatus(&status))
  {
    state.cache_bytes = status.forward;
    if(state.time_total)
      state.cache_bytes += m_pInputStream->GetLength() * GetQueueTime() / state.time_total;
  }
  else
    state.cache_bytes = 0;

  state.timestamp = m_av_clock.GetAbsoluteClock();

  CSingleLock lock(m_StateSection);
  m_State = state;
}

void COMXPlayer::UpdateApplication(double timeout)
{
  if(m_UpdateApplication != 0
  && m_UpdateApplication + DVD_MSEC_TO_TIME(timeout) > m_av_clock.GetAbsoluteClock())
    return;

  CDVDInputStream::IChannel* pStream = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
  if(pStream)
  {
    CFileItem item(g_application.CurrentFileItem());
    if(pStream->UpdateItem(item))
    {
      g_application.CurrentFileItem() = item;
      CApplicationMessenger::Get().SetCurrentItem(item);
    }
  }
  m_UpdateApplication = m_av_clock.GetAbsoluteClock();
}

bool COMXPlayer::CanRecord()
{
  CSingleLock lock(m_StateSection);
  return m_State.canrecord;
}

bool COMXPlayer::IsRecording()
{
  CSingleLock lock(m_StateSection);
  return m_State.recording;
}

bool COMXPlayer::Record(bool bOnOff)
{
  if (m_pInputStream && (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV) ||
                         m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER)) )
  {
    m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_RECORD, bOnOff));
    return true;
  }
  return false;
}

int COMXPlayer::GetChannels()
{
  if (m_pDemuxer && (m_CurrentAudio.id != -1))
  {
    CDemuxStreamAudio* stream = static_cast<CDemuxStreamAudio*>(m_pDemuxer->GetStream(m_CurrentAudio.id));
    if (stream)
      return stream->iChannels;
  }
  return -1;
}

CStdString COMXPlayer::GetAudioCodecName()
{
  CStdString retVal;
  if (m_pDemuxer && (m_CurrentAudio.id != -1))
    m_pDemuxer->GetStreamCodecName(m_CurrentAudio.id, retVal);
  return retVal;
}

CStdString COMXPlayer::GetVideoCodecName()
{
  CStdString retVal;
  if (m_pDemuxer && (m_CurrentVideo.id != -1))
    m_pDemuxer->GetStreamCodecName(m_CurrentVideo.id, retVal);
  return retVal;
}

int COMXPlayer::GetPictureWidth()
{
  if (m_pDemuxer && (m_CurrentVideo.id != -1))
  {
    CDemuxStreamVideo* stream = static_cast<CDemuxStreamVideo*>(m_pDemuxer->GetStream(m_CurrentVideo.id));
    if (stream)
      return stream->iWidth;
  }
  return 0;
}

int COMXPlayer::GetPictureHeight()
{
  if (m_pDemuxer && (m_CurrentVideo.id != -1))
  {
    CDemuxStreamVideo* stream = static_cast<CDemuxStreamVideo*>(m_pDemuxer->GetStream(m_CurrentVideo.id));
    if (stream)
      return stream->iHeight;
  }
  return 0;
}

bool COMXPlayer::GetStreamDetails(CStreamDetails &details)
{
  if (m_pDemuxer)
  {
    bool result=CDVDFileInfo::DemuxerToStreamDetails(m_pInputStream, m_pDemuxer, details);
    if (result && details.GetStreamCount(CStreamDetail::VIDEO) > 0) // this is more correct (dvds in particular)
    {
      GetVideoAspectRatio(((CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO,0))->m_fAspect);
      ((CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO,0))->m_iDuration = GetTotalTime() / 1000;
    }
    return result;
  }
  else
    return false;
}

CStdString COMXPlayer::GetPlayingTitle()
{
  return "";
}

bool COMXPlayer::SwitchChannel(const CPVRChannel &channel)
{
  if (!g_PVRManager.CheckParentalLock(channel))
    return false;

  /* set GUI info */
  if (!g_PVRManager.PerformChannelSwitch(channel, true))
    return false;

  UpdateApplication(0);
  UpdatePlayState(0);

  /* make sure the pvr window is updated */
  CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
  if (pWindow)
    pWindow->SetInvalid();

  /* select the new channel */
  m_messenger.Put(new CDVDMsgType<CPVRChannel>(CDVDMsg::PLAYER_CHANNEL_SELECT, channel));

  return true;
}

bool COMXPlayer::CachePVRStream(void) const
{
  return m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) &&
      !g_PVRManager.IsPlayingRecording() &&
      g_advancedSettings.m_bPVRCacheInDvdPlayer;
}

void COMXPlayer::GetVideoRect(CRect& SrcRect, CRect& DestRect)
{
  g_renderManager.GetVideoRect(SrcRect, DestRect);
}

void COMXPlayer::SetVolume(float fVolume)
{
  m_current_volume = fVolume;
  m_change_volume = true;
}

void COMXPlayer::Update(bool bPauseDrawing)
{
  g_renderManager.Update(bPauseDrawing);
}

void COMXPlayer::GetVideoAspectRatio(float &fAR)
{
  fAR = g_renderManager.GetAspectRatio();
}

void COMXPlayer::GetRenderFeatures(std::vector<int> &renderFeatures)
{
  renderFeatures.push_back(RENDERFEATURE_STRETCH);
  renderFeatures.push_back(RENDERFEATURE_CROP);
}

void COMXPlayer::GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods)
{
  deinterlaceMethods.push_back(VS_INTERLACEMETHOD_DEINTERLACE);
}

void COMXPlayer::GetDeinterlaceModes(std::vector<int> &deinterlaceModes)
{
  deinterlaceModes.push_back(VS_DEINTERLACEMODE_AUTO);
  deinterlaceModes.push_back(VS_DEINTERLACEMODE_OFF);
  deinterlaceModes.push_back(VS_DEINTERLACEMODE_FORCE);
}

void COMXPlayer::GetScalingMethods(std::vector<int> &scalingMethods)
{
}

void COMXPlayer::GetAudioCapabilities(std::vector<int> &audioCaps)
{
  audioCaps.push_back(IPC_AUD_OFFSET);
  audioCaps.push_back(IPC_AUD_SELECT_STREAM);
  audioCaps.push_back(IPC_AUD_SELECT_OUTPUT);
}

void COMXPlayer::GetSubtitleCapabilities(std::vector<int> &subCaps)
{
  subCaps.push_back(IPC_SUBS_ALL);
}

#endif
