/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#include "VideoPlayerRadioRDS.h"
#include "VideoPlayer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"

#include "DVDFileInfo.h"

#include "utils/LangCodeExpander.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"

#include "utils/URIUtils.h"
#include "GUIInfoManager.h"
#include "cores/DataCacheCore.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"

#include "DVDDemuxers/DVDDemuxCC.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "xbmc/utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include "utils/StreamDetails.h"
#include "pvr/PVRManager.h"
#include "utils/StreamUtils.h"
#include "utils/Variant.h"
#include "storage/MediaManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "LangInfo.h"
#include "URL.h"
#include "video/VideoReferenceClock.h"

#ifdef HAS_OMXPLAYER
#include "cores/omxplayer/OMXPlayerAudio.h"
#include "cores/omxplayer/OMXPlayerVideo.h"
#include "cores/omxplayer/OMXHelper.h"
#endif
#include "VideoPlayerAudio.h"
#include "windowing/WindowingFactory.h"
#include "DVDCodecs/DVDCodecUtils.h"

using namespace PVR;
using namespace KODI::MESSAGING;

void CSelectionStreams::Clear(StreamType type, StreamSource source)
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

SelectionStream& CSelectionStreams::Get(StreamType type, int index)
{
  CSingleLock lock(m_section);
  int count = -1;
  for(size_t i=0;i<m_Streams.size();i++)
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

std::vector<SelectionStream> CSelectionStreams::Get(StreamType type)
{
  std::vector<SelectionStream> streams;
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

class PredicateSubtitleFilter
{
private:
  std::string audiolang;
  bool original;
  bool nosub;
  bool onlyforced;
public:
  /** \brief The class' operator() decides if the given (subtitle) SelectionStream is relevant wrt.
  *          preferred subtitle language and audio language. If the subtitle is relevant <B>false</B> false is returned.
  *
  *          A subtitle is relevant if
  *          - it was previously selected, or
  *          - it's an external sub, or
  *          - it's a forced sub and "original stream's language" was selected, or
  *          - it's a forced sub and its language matches the audio's language, or
  *          - it's a default sub, or
  *          - its language matches the preferred subtitle's language (unequal to "original stream's language")
  */
  PredicateSubtitleFilter(std::string& lang)
    : audiolang(lang),
      original(StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "original")),
      nosub(StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "none")),
      onlyforced(StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "foced_only"))
  {
  };

  bool operator()(const SelectionStream& ss) const
  {
    if (ss.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleStream)
      return false;

    if (nosub)
      return true;

    if (onlyforced)
    {
      if ((ss.flags & CDemuxStream::FLAG_FORCED) && g_LangCodeExpander.CompareISO639Codes(ss.language, audiolang))
        return false;
      else
        return true;
    }
      
    if(STREAM_SOURCE_MASK(ss.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(ss.source) == STREAM_SOURCE_TEXT)
      return false;

    if ((ss.flags & CDemuxStream::FLAG_FORCED) && (original || g_LangCodeExpander.CompareISO639Codes(ss.language, audiolang)))
      return false;

    if ((ss.flags & CDemuxStream::FLAG_DEFAULT))
      return false;

    if(!original)
    {
      std::string subtitle_language = g_langInfo.GetSubtitleLanguage();
      if (g_LangCodeExpander.CompareISO639Codes(subtitle_language, ss.language))
        return false;
    }

    return true;
  }
};

static bool PredicateAudioPriority(const SelectionStream& lh, const SelectionStream& rh)
{
  PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream
                 , rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream);

  if(!StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE), "original"))
  {
    std::string audio_language = g_langInfo.GetAudioLanguage();
    PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(audio_language, lh.language)
                   , g_LangCodeExpander.CompareISO639Codes(audio_language, rh.language));

    bool hearingimp = CSettings::GetInstance().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING);
    PREDICATE_RETURN(!hearingimp ? !(lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED
                   , !hearingimp ? !(rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED);

    bool visualimp = CSettings::GetInstance().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL);
    PREDICATE_RETURN(!visualimp ? !(lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED
                   , !visualimp ? !(rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED);
  }

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG))
  {
    PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                   , rh.flags & CDemuxStream::FLAG_DEFAULT);
  }

  PREDICATE_RETURN(lh.channels
                 , rh.channels);

  PREDICATE_RETURN(StreamUtils::GetCodecPriority(lh.codec)
                 , StreamUtils::GetCodecPriority(rh.codec));

  PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                 , rh.flags & CDemuxStream::FLAG_DEFAULT);

  return false;
}

/** \brief The class' operator() decides if the given (subtitle) SelectionStream lh is 'better than' the given (subtitle) SelectionStream rh.
*          If lh is 'better than' rh the return value is true, false otherwise.
*
*          A subtitle lh is 'better than' a subtitle rh (in evaluation order) if
*          - lh was previously selected, or
*          - lh is an external sub and rh not, or
*          - lh is a forced sub and ("original stream's language" was selected or subtitles are off) and rh not, or
*          - lh is an external sub and its language matches the preferred subtitle's language (unequal to "original stream's language") and rh not, or
*          - lh is language matches the preferred subtitle's language (unequal to "original stream's language") and rh not, or
*          - lh is a default sub and rh not
*/
class PredicateSubtitlePriority
{
private:
  std::string audiolang;
  bool original;
  bool subson;
  PredicateSubtitleFilter filter;
public:
  PredicateSubtitlePriority(std::string& lang)
    : audiolang(lang),
      original(StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "original")),
      subson(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleOn),
      filter(lang)
  {
  };

  bool relevant(const SelectionStream& ss) const
  {
    return !filter(ss);
  }

  bool operator()(const SelectionStream& lh, const SelectionStream& rh) const
  {
    PREDICATE_RETURN(relevant(lh)
                   , relevant(rh));

    PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleStream
                   , rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleStream);

    // prefer external subs
    PREDICATE_RETURN(STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_TEXT
                   , STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_TEXT);

    if(!subson || original)
    {
      PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , rh.flags & CDemuxStream::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_FORCED
                     , rh.flags & CDemuxStream::FLAG_FORCED);
    }

    std::string subtitle_language = g_langInfo.GetSubtitleLanguage();
    if(!original)
    {
      PREDICATE_RETURN((STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_TEXT) && g_LangCodeExpander.CompareISO639Codes(subtitle_language, lh.language)
                     , (STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_TEXT) && g_LangCodeExpander.CompareISO639Codes(subtitle_language, rh.language));
    }

    if(!original)
    {
      PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(subtitle_language, lh.language)
                     , g_LangCodeExpander.CompareISO639Codes(subtitle_language, rh.language));

      bool hearingimp = CSettings::GetInstance().GetBool(CSettings::SETTING_ACCESSIBILITY_SUBHEARING);
      PREDICATE_RETURN(!hearingimp ? !(lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED
                     , !hearingimp ? !(rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED);
    }

    PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                   , rh.flags & CDemuxStream::FLAG_DEFAULT);

    return false;
  }
};

static bool PredicateVideoPriority(const SelectionStream& lh, const SelectionStream& rh)
{
  PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
                 , rh.flags & CDemuxStream::FLAG_DEFAULT);
  return false;
}

bool CSelectionStreams::Get(StreamType type, CDemuxStream::EFlags flag, SelectionStream& out)
{
  CSingleLock lock(m_section);
  for(size_t i=0;i<m_Streams.size();i++)
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

int CSelectionStreams::IndexOf(StreamType type, int source, int id) const
{
  CSingleLock lock(m_section);
  int count = -1;
  for(size_t i=0;i<m_Streams.size();i++)
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

int CSelectionStreams::IndexOf(StreamType type, CVideoPlayer& p) const
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
  else if(type == STREAM_RADIO_RDS)
    return IndexOf(type, p.m_CurrentRadioRDS.source, p.m_CurrentRadioRDS.id);

  return -1;
}

int CSelectionStreams::Source(StreamSource source, std::string filename)
{
  CSingleLock lock(m_section);
  int index = source - 1;
  for(size_t i=0;i<m_Streams.size();i++)
  {
    SelectionStream &s = m_Streams[i];
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

void CSelectionStreams::Update(SelectionStream& s)
{
  CSingleLock lock(m_section);
  int index = IndexOf(s.type, s.source, s.id);
  if(index >= 0)
  {
    SelectionStream& o = Get(s.type, index);
    s.type_index = o.type_index;
    o = s;
  }
  else
  {
    s.type_index = Count(s.type);
    m_Streams.push_back(s);
  }
}

void CSelectionStreams::Update(CDVDInputStream* input, CDVDDemux* demuxer, std::string filename2)
{
  if(input && input->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* nav = (CDVDInputStreamNavigator*)input;
    std::string filename = nav->GetFileName();
    int source = Source(STREAM_SOURCE_NAV, filename);

    int count;
    count = nav->GetAudioStreamCount();
    for(int i=0;i<count;i++)
    {
      SelectionStream s;
      s.source   = source;
      s.type     = STREAM_AUDIO;
      s.id       = i;
      s.flags    = CDemuxStream::FLAG_NONE;
      s.filename = filename;

      DVDNavStreamInfo info;
      nav->GetAudioStreamInfo(i, info);
      s.name     = info.name;
      s.language = g_LangCodeExpander.ConvertToISO6392T(info.language);
      s.channels = info.channels;
      Update(s);
    }

    count = nav->GetSubTitleStreamCount();
    for(int i=0;i<count;i++)
    {
      SelectionStream s;
      s.source   = source;
      s.type     = STREAM_SUBTITLE;
      s.id       = i;
      s.flags    = CDemuxStream::FLAG_NONE;
      s.filename = filename;
      s.channels = 0;

      DVDNavStreamInfo info;
      nav->GetSubtitleStreamInfo(i, info);
      s.name     = info.name;
      s.language = g_LangCodeExpander.ConvertToISO6392T(info.language);
      Update(s);
    }
  }
  else if(demuxer)
  {
    std::string filename = demuxer->GetFileName();
    int count = demuxer->GetNrOfStreams();
    int source;
    if(input) /* hack to know this is sub decoder */
      source = Source(STREAM_SOURCE_DEMUX, filename);
    else if (!filename2.empty())
      source = Source(STREAM_SOURCE_DEMUX_SUB, filename);
    else
      source = Source(STREAM_SOURCE_VIDEOMUX, filename);

    for(int i=0;i<count;i++)
    {
      CDemuxStream* stream = demuxer->GetStream(i);
      /* skip streams with no type */
      if (stream->type == STREAM_NONE)
        continue;
      /* make sure stream is marked with right source */
      stream->source = source;

      SelectionStream s;
      s.source   = source;
      s.type     = stream->type;
      s.id       = stream->iId;
      s.language = g_LangCodeExpander.ConvertToISO6392T(stream->language);
      s.flags    = stream->flags;
      s.filename = demuxer->GetFileName();
      s.filename2 = filename2;
      stream->GetStreamName(s.name);
      std::string codec;
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
  g_dataCacheCore.SignalAudioInfoChange();
  g_dataCacheCore.SignalVideoInfoChange();
}

int CSelectionStreams::CountSource(StreamType type, StreamSource source) const
{
  CSingleLock lock(m_section);
  int count = 0;
  for(size_t i=0;i<m_Streams.size();i++)
  {
    if(type && m_Streams[i].type != type)
      continue;
    if (source && m_Streams[i].source != source)
      continue;
    count++;
    continue;
  }
  return count;
}

void CVideoPlayer::CreatePlayers()
{
#ifdef HAS_OMXPLAYER
  bool omx_suitable = !OMXPlayerUnsuitable(m_HasVideo, m_HasAudio, m_pDemuxer, m_pInputStream, m_SelectionStreams);
  if (m_omxplayer_mode != omx_suitable)
  {
    DestroyPlayers();
    m_omxplayer_mode = omx_suitable;
  }
#endif
  if (m_players_created)
    return;

  if (m_omxplayer_mode)
  {
#ifdef HAS_OMXPLAYER
    m_VideoPlayerVideo = new OMXPlayerVideo(&m_OmxPlayerState.av_clock, &m_overlayContainer, m_messenger);
    m_VideoPlayerAudio = new OMXPlayerAudio(&m_OmxPlayerState.av_clock, m_messenger);
#endif
  }
  else
  {
    m_VideoPlayerVideo = new CVideoPlayerVideo(&m_clock, &m_overlayContainer, m_messenger, m_renderManager);
    m_VideoPlayerAudio = new CVideoPlayerAudio(&m_clock, m_messenger);
  }
  m_VideoPlayerSubtitle = new CVideoPlayerSubtitle(&m_overlayContainer);
  m_VideoPlayerTeletext = new CDVDTeletextData();
  m_VideoPlayerRadioRDS = new CDVDRadioRDSData();
  m_players_created = true;
}

void CVideoPlayer::DestroyPlayers()
{
  if (!m_players_created)
    return;

  delete m_VideoPlayerVideo;
  delete m_VideoPlayerAudio;
  delete m_VideoPlayerSubtitle;
  delete m_VideoPlayerTeletext;
  delete m_VideoPlayerRadioRDS;

  m_players_created = false;
}

CVideoPlayer::CVideoPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread("VideoPlayer"),
      m_CurrentAudio(STREAM_AUDIO, VideoPlayer_AUDIO),
      m_CurrentVideo(STREAM_VIDEO, VideoPlayer_VIDEO),
      m_CurrentSubtitle(STREAM_SUBTITLE, VideoPlayer_SUBTITLE),
      m_CurrentTeletext(STREAM_TELETEXT, VideoPlayer_TELETEXT),
      m_CurrentRadioRDS(STREAM_RADIO_RDS, VideoPlayer_RDS),
      m_messenger("player"),
      m_ready(true),
      m_DemuxerPausePending(false),
      m_renderManager(m_clock)
{
  m_players_created = false;
  m_pDemuxer = NULL;
  m_pSubtitleDemuxer = NULL;
  m_pCCDemuxer = NULL;
  m_pInputStream = NULL;

  m_dvd.Clear();
  m_State.Clear();
  m_EdlAutoSkipMarkers.Clear();
  m_UpdateApplication = 0;

  m_bAbortRequest = false;
  m_errorCount = 0;
  m_offset_pts = 0.0;
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_caching = CACHESTATE_DONE;
  m_HasVideo = false;
  m_HasAudio = false;

  memset(&m_SpeedState, 0, sizeof(m_SpeedState));

  // omxplayer variables
  m_OmxPlayerState.video_fifo          = 0;
  m_OmxPlayerState.audio_fifo          = 0;
  m_OmxPlayerState.last_check_time     = 0.0;
  m_OmxPlayerState.stamp               = 0.0;
  m_OmxPlayerState.bOmxWaitVideo       = false;
  m_OmxPlayerState.bOmxWaitAudio       = false;
  m_OmxPlayerState.bOmxSentEOFs        = false;
  m_OmxPlayerState.threshold           = 0.2f;
  m_OmxPlayerState.current_deinterlace = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_DeinterlaceMode;
  m_OmxPlayerState.interlace_method    = VS_INTERLACEMETHOD_MAX;
#ifdef HAS_OMXPLAYER
  m_omxplayer_mode                     = CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEOMXPLAYER);
#else
  m_omxplayer_mode                     = false;
#endif

  CreatePlayers();

  m_displayLost = false;
  g_Windowing.Register(this);
}

CVideoPlayer::~CVideoPlayer()
{
  g_Windowing.Unregister(this);

  CloseFile();
  DestroyPlayers();
}

bool CVideoPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  CLog::Log(LOGNOTICE, "VideoPlayer: Opening: %s", CURL::GetRedacted(file.GetPath()).c_str());

  // if playing a file close it first
  // this has to be changed so we won't have to close it.
  if(IsRunning())
    CloseFile();

  m_bAbortRequest = false;
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  m_State.Clear();
  memset(&m_SpeedState, 0, sizeof(m_SpeedState));
  m_UpdateApplication = 0;
  m_offset_pts = 0;
  m_CurrentAudio.lastdts = DVD_NOPTS_VALUE;
  m_CurrentVideo.lastdts = DVD_NOPTS_VALUE;

  m_PlayerOptions = options;
  m_item     = file;
  m_mimetype  = file.GetMimeType();
  m_filename = file.GetPath();

  m_ready.Reset();

  m_renderManager.PreInit();

  Create();

  // wait for the ready event
  CGUIDialogBusy::WaitOnEvent(m_ready, g_advancedSettings.m_videoBusyDialogDelay_ms, false);

  // Playback might have been stopped due to some error
  if (m_bStop || m_bAbortRequest)
    return false;

  return true;
}

bool CVideoPlayer::CloseFile(bool reopen)
{
  CLog::Log(LOGNOTICE, "CVideoPlayer::CloseFile()");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  if(m_pSubtitleDemuxer)
    m_pSubtitleDemuxer->Abort();

  if(m_pInputStream)
    m_pInputStream->Abort();

  CLog::Log(LOGNOTICE, "VideoPlayer: waiting for threads to exit");

  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();

  m_Edl.Clear();
  m_EdlAutoSkipMarkers.Clear();

  m_HasVideo = false;
  m_HasAudio = false;

  CLog::Log(LOGNOTICE, "VideoPlayer: finished waiting");
  m_renderManager.UnInit();
  return true;
}

bool CVideoPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CVideoPlayer::OnStartup()
{
  m_CurrentVideo.Clear();
  m_CurrentAudio.Clear();
  m_CurrentSubtitle.Clear();
  m_CurrentTeletext.Clear();
  m_CurrentRadioRDS.Clear();

  m_messenger.Init();

  CUtil::ClearTempFonts();
}

bool CVideoPlayer::OpenInputStream()
{
  if(m_pInputStream)
    SAFE_DELETE(m_pInputStream);

  CLog::Log(LOGNOTICE, "Creating InputStream");

  // correct the filename if needed
  std::string filename(m_filename);
  if (URIUtils::IsProtocol(filename, "dvd")
  ||  StringUtils::EqualsNoCase(filename, "iso9660://video_ts/video_ts.ifo"))
  {
    m_filename = g_mediaManager.TranslateDevicePath("");
  }

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename, m_mimetype, m_item.ContentLookup());
  if(m_pInputStream == NULL)
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - unable to create input stream for [%s]", m_filename.c_str());
    return false;
  }
  else
    m_pInputStream->SetFileItem(m_item);

  if (!m_pInputStream->Open(m_filename.c_str(), m_mimetype, m_item.ContentLookup()))
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - error opening [%s]", m_filename.c_str());
    return false;
  }

  // find any available external subtitles for non dvd files
  if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
  {
    // find any available external subtitles
    std::vector<std::string> filenames;
    CUtil::ScanForExternalSubtitles( m_filename, filenames );

    // load any subtitles from file item
    std::string key("subtitle:1");
    for(unsigned s = 1; m_item.HasProperty(key); key = StringUtils::Format("subtitle:%u", ++s))
      filenames.push_back(m_item.GetProperty(key).asString());

    for(unsigned int i=0;i<filenames.size();i++)
    {
      // if vobsub subtitle:
      if (URIUtils::HasExtension(filenames[i], ".idx"))
      {
        std::string strSubFile;
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

    CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleCached = true;
  }

  SetAVDelay(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay);
  SetSubTitleDelay(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay);
  m_clock.Reset();
  m_dvd.Clear();
  m_errorCount = 0;
  m_ChannelEntryTimeOut.SetInfinite();

  return true;
}

bool CVideoPlayer::OpenDemuxStream()
{
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);

  CLog::Log(LOGNOTICE, "Creating Demuxer");

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

  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

  int64_t len = m_pInputStream->GetLength();
  int64_t tim = m_pDemuxer->GetStreamLength();
  if(len > 0 && tim > 0)
    m_pInputStream->SetReadRate((unsigned int) (len * 1000 / tim));

  return true;
}

void CVideoPlayer::OpenDefaultStreams(bool reset)
{
  // if input stream dictate, we will open later
  if(m_dvd.iSelectedAudioStream >= 0
  || m_dvd.iSelectedSPUStream   >= 0)
    return;

  SelectionStreams streams;
  bool valid;

  // open video stream
  streams = m_SelectionStreams.Get(STREAM_VIDEO, PredicateVideoPriority);
  valid   = false;
  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenStream(m_CurrentVideo, it->id, it->source, reset))
      valid = true;
  }
  if(!valid)
    CloseStream(m_CurrentVideo, true);

  // open audio stream
  if(m_PlayerOptions.video_only)
    streams.clear();
  else
    streams = m_SelectionStreams.Get(STREAM_AUDIO, PredicateAudioPriority);
  valid   = false;

  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenStream(m_CurrentAudio, it->id, it->source, reset))
      valid = true;
  }
  if(!valid)
    CloseStream(m_CurrentAudio, true);

  // enable  or disable subtitles
  bool visible = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleOn;

  // open subtitle stream
  SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
  PredicateSubtitlePriority psp(as.language);
  streams = m_SelectionStreams.Get(STREAM_SUBTITLE, psp);
  valid   = false;
  CloseStream(m_CurrentSubtitle, false);
  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenStream(m_CurrentSubtitle, it->id, it->source))
    {
      valid = true;
      if(!psp.relevant(*it))
        visible = false;
      else if(it->flags & CDemuxStream::FLAG_FORCED)
        visible = true;
    }
  }
  if(!valid)
    CloseStream(m_CurrentSubtitle, false);

  if (!dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream) || m_PlayerOptions.state.size() == 0)
    SetSubtitleVisibleInternal(visible); // only set subtitle visibility if state not stored by dvd navigator, because navigator will restore it (if visible)

  // open teletext stream
  streams = m_SelectionStreams.Get(STREAM_TELETEXT);
  valid   = false;
  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenStream(m_CurrentTeletext, it->id, it->source))
      valid = true;
  }
  if(!valid)
    CloseStream(m_CurrentTeletext, false);

  // open RDS stream
  streams = m_SelectionStreams.Get(STREAM_RADIO_RDS);
  valid   = false;
  for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
  {
    if(OpenStream(m_CurrentRadioRDS, it->id, it->source))
      valid = true;
  }
  if(!valid)
    CloseStream(m_CurrentRadioRDS, false);
}

bool CVideoPlayer::ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream)
{

  // check if we should read from subtitle demuxer
  if( m_pSubtitleDemuxer && m_VideoPlayerSubtitle->AcceptsData() )
  {
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

  if (m_omxplayer_mode)
  {
    // reset eos state when we get a packet (e.g. for case of seek after eos)
    if (packet && stream)
    {
      m_OmxPlayerState.bOmxWaitVideo = false;
      m_OmxPlayerState.bOmxWaitAudio = false;
      m_OmxPlayerState.bOmxSentEOFs = false;
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
        OpenDefaultStreams(false);

        // reevaluate HasVideo/Audio, we may have switched from/to a radio channel
        if(m_CurrentVideo.id < 0)
          m_HasVideo = false;
        if(m_CurrentAudio.id < 0)
          m_HasAudio = false;

        return true;
    }

    UpdateCorrection(packet, m_offset_pts);

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

bool CVideoPlayer::IsValidStream(CCurrentStream& stream)
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
  if (source == STREAM_SOURCE_VIDEOMUX)
  {
    CDemuxStream* st = m_pCCDemuxer->GetStream(stream.id);
    if (st == NULL || st->disabled)
      return false;
    if (st->type != stream.type)
      return false;
    return true;
  }

  return false;
}

bool CVideoPlayer::IsBetterStream(CCurrentStream& current, CDemuxStream* stream)
{
  // Do not reopen non-video streams if we're in video-only mode
  if(m_PlayerOptions.video_only && current.type != STREAM_VIDEO)
    return false;

  if(stream->disabled)
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

    if(stream->type != current.type)
      return false;

    if(current.type == STREAM_SUBTITLE)
      return false;

    if(current.id < 0)
      return true;
  }
  return false;
}

void CVideoPlayer::CheckBetterStream(CCurrentStream& current, CDemuxStream* stream)
{
  IDVDStreamPlayer* player = GetStreamPlayer(current.player);
  if (!IsValidStream(current) && (player == NULL || player->IsStalled()))
    CloseStream(current, true);

  if (IsBetterStream(current, stream))
    OpenStream(current, stream->iId, stream->source);
}

void CVideoPlayer::Process()
{
  if (!OpenInputStream())
  {
    m_bAbortRequest = true;
    return;
  }

  if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
  {
    CLog::Log(LOGNOTICE, "VideoPlayer: playing a file with menu's");
    if(dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream))
      m_PlayerOptions.starttime = 0;

    if(m_PlayerOptions.state.size() > 0)
      ptr->SetState(m_PlayerOptions.state);
    else if(CDVDInputStreamNavigator* nav = dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream))
      nav->EnableSubtitleStream(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleOn);

    CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleCached = true;
  }

  if(!OpenDemuxStream())
  {
    m_bAbortRequest = true;
    return;
  }
  // give players a chance to reconsider now codecs are known
  CreatePlayers();

  // allow renderer to switch to fullscreen if requested
  m_VideoPlayerVideo->EnableFullscreen(m_PlayerOptions.fullscreen);

  if (m_omxplayer_mode)
  {
    if (!m_OmxPlayerState.av_clock.OMXInitialize(&m_clock))
      m_bAbortRequest = true;
    if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
      m_OmxPlayerState.av_clock.HDMIClockSync();
    m_OmxPlayerState.av_clock.OMXStateIdle();
    m_OmxPlayerState.av_clock.OMXStateExecute();
    m_OmxPlayerState.av_clock.OMXStop();
    m_OmxPlayerState.av_clock.OMXPause();
  }

  OpenDefaultStreams();

  // look for any EDL files
  m_Edl.Clear();
  m_EdlAutoSkipMarkers.Clear();
  if (m_CurrentVideo.id >= 0 && m_CurrentVideo.hint.fpsrate > 0 && m_CurrentVideo.hint.fpsscale > 0)
  {
    float fFramesPerSecond = (float)m_CurrentVideo.hint.fpsrate / (float)m_CurrentVideo.hint.fpsscale;
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
      int playerStartTime = (int)( ( (float) m_pDemuxer->GetStreamLength() ) * ( m_PlayerOptions.startpercent/(float)100 ) );
      starttime = m_Edl.RestoreCutTime(playerStartTime);
    }
    else
    {
      starttime = m_Edl.RestoreCutTime(m_PlayerOptions.starttime * 1000); // s to ms
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

  // make sure application know our info
  UpdateApplication(0);
  UpdatePlayState(0);

  if(m_PlayerOptions.identify == false)
    m_callback.OnPlayBackStarted();

  // we are done initializing now, set the readyevent
  m_ready.Set();

  SetCaching(CACHESTATE_FLUSH);

  while (!m_bAbortRequest)
  {
#ifdef HAS_OMXPLAYER
    if (m_omxplayer_mode && OMXDoProcessing(m_OmxPlayerState, m_playSpeed, m_VideoPlayerVideo, m_VideoPlayerAudio, m_CurrentAudio, m_CurrentVideo, m_HasVideo, m_HasAudio))
    {
      CloseStream(m_CurrentVideo, false);
      OpenStream(m_CurrentVideo, m_CurrentVideo.id, m_CurrentVideo.source);
      if (m_State.canseek)
        m_messenger.Put(new CDVDMsgPlayerSeek(GetTime(), true, true, true, true, true));
    }
#endif

    // check display lost
    {
      CSingleLock lock(m_StateSection);
      if (m_displayLost)
      {
        Sleep(50);
        continue;
      }
    }

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

      // on channel switch we don't want to close stream players at this
      // time. we'll get the stream change event later
      if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) ||
          !m_SelectionStreams.m_Streams.empty())
        OpenDefaultStreams();

      // never allow first frames after open to be skipped
      if( m_VideoPlayerVideo->IsInited() )
        m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));

      UpdateApplication(0);
      UpdatePlayState(0);
    }

    // handle eventual seeks due to playspeed
    HandlePlaySpeed();

    // update player state
    UpdatePlayState(200);

    // update application with our state
    UpdateApplication(1000);

    // make sure we run subtitle process here
    m_VideoPlayerSubtitle->Process(m_clock.GetClock() + m_State.time_offset - m_VideoPlayerVideo->GetSubtitleDelay(), m_State.time_offset);

    if (CheckDelayedChannelEntry())
      continue;

    // if the queues are full, no need to read more
    if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
        (!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
    {
      if(m_pDemuxer && m_DemuxerPausePending)
      {
        m_DemuxerPausePending = false;
        m_pDemuxer->SetSpeed(DVD_PLAYSPEED_PAUSE);
      }

      Sleep(10);
      continue;
    }

    // always yield to players if they have data levels > 50 percent
    if((m_VideoPlayerAudio->GetLevel() > 50 || m_CurrentAudio.id < 0)
    && (m_VideoPlayerVideo->GetLevel() > 50 || m_CurrentVideo.id < 0))
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
      if (CDVDInputStream::IMenus* pStream = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
      {
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
      if (m_omxplayer_mode && !m_OmxPlayerState.bOmxSentEOFs)
      {
        if(m_CurrentAudio.inited)
          m_OmxPlayerState.bOmxWaitAudio = true;

        if(m_CurrentVideo.inited)
          m_OmxPlayerState.bOmxWaitVideo = true;

        m_OmxPlayerState.bOmxSentEOFs = true;
      }

      if(m_CurrentAudio.inited)
        m_VideoPlayerAudio->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentVideo.inited)
        m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentSubtitle.inited)
        m_VideoPlayerSubtitle->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentTeletext.inited)
        m_VideoPlayerTeletext->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentRadioRDS.inited)
        m_VideoPlayerRadioRDS->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));

      m_CurrentAudio.inited = false;
      m_CurrentVideo.inited = false;
      m_CurrentSubtitle.inited = false;
      m_CurrentTeletext.inited = false;
      m_CurrentRadioRDS.inited = false;

      // if we are caching, start playing it again
      SetCaching(CACHESTATE_DONE);

      // while players are still playing, keep going to allow seekbacks
      if(m_VideoPlayerAudio->HasData()
      || m_VideoPlayerVideo->HasData())
      {
        Sleep(100);
        continue;
      }
#ifdef HAS_OMXPLAYER
      if (m_omxplayer_mode && OMXStillPlaying(m_OmxPlayerState.bOmxWaitVideo, m_OmxPlayerState.bOmxWaitAudio, m_VideoPlayerVideo->IsEOS(), m_VideoPlayerAudio->IsEOS()))
      {
        Sleep(100);
        continue;
      }
#endif

      if (!m_pInputStream->IsEOF())
        CLog::Log(LOGINFO, "%s - eof reading from demuxer", __FUNCTION__);

      break;
    }

    // it's a valid data packet, reset error counter
    m_errorCount = 0;

    // see if we can find something better to play
    CheckBetterStream(m_CurrentAudio,    pStream);
    CheckBetterStream(m_CurrentVideo,    pStream);
    CheckBetterStream(m_CurrentSubtitle, pStream);
    CheckBetterStream(m_CurrentTeletext, pStream);
    CheckBetterStream(m_CurrentRadioRDS, pStream);

    // demux video stream
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SUBTITLES_PARSECAPTIONS) && CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
    {
      if (m_pCCDemuxer)
      {
        bool first = true;
        while(!m_bAbortRequest)
        {
          DemuxPacket *pkt = m_pCCDemuxer->Read(first ? pPacket : NULL);
          if (!pkt)
            break;

          first = false;
          if (m_pCCDemuxer->GetNrOfStreams() != m_SelectionStreams.CountSource(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX))
          {
            m_SelectionStreams.Clear(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX);
            m_SelectionStreams.Update(NULL, m_pCCDemuxer, "");
            OpenDefaultStreams(false);
          }
          CDemuxStream *pSubStream = m_pCCDemuxer->GetStream(pkt->iStreamId);
          if (pSubStream && m_CurrentSubtitle.id == pkt->iStreamId && m_CurrentSubtitle.source == STREAM_SOURCE_VIDEOMUX)
            ProcessSubData(pSubStream, pkt);
          else
            CDVDDemuxUtils::FreeDemuxPacket(pkt);
        }
      }
    }

    // process the packet
    ProcessPacket(pStream, pPacket);

    // check if in a cut or commercial break that should be automatically skipped
    CheckAutoSceneSkip();
  }
}

bool CVideoPlayer::CheckDelayedChannelEntry(void)
{
  bool bReturn(false);

  if (m_ChannelEntryTimeOut.IsTimePast())
  {
    CFileItem currentFile(g_application.CurrentFileItem());
    CPVRChannelPtr currentChannel(currentFile.GetPVRChannelInfoTag());
    if (currentChannel)
    {
      SwitchChannel(currentChannel);

      bReturn = true;
    }
    m_ChannelEntryTimeOut.SetInfinite();
  }

  return bReturn;
}

bool CVideoPlayer::CheckIsCurrent(CCurrentStream& current, CDemuxStream* stream, DemuxPacket* pkg)
{
  if(current.id     == pkg->iStreamId
  && current.source == stream->source
  && current.type   == stream->type)
    return true;
  else
    return false;
}

void CVideoPlayer::ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  // process packet if it belongs to selected stream.
  // for dvd's don't allow automatic opening of streams*/

  if (CheckIsCurrent(m_CurrentAudio, pStream, pPacket))
    ProcessAudioData(pStream, pPacket);
  else if (CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
    ProcessVideoData(pStream, pPacket);
  else if (CheckIsCurrent(m_CurrentSubtitle, pStream, pPacket))
    ProcessSubData(pStream, pPacket);
  else if (CheckIsCurrent(m_CurrentTeletext, pStream, pPacket))
    ProcessTeletextData(pStream, pPacket);
  else if (CheckIsCurrent(m_CurrentRadioRDS, pStream, pPacket))
    ProcessRadioRDSData(pStream, pPacket);
  else
  {
    pStream->SetDiscard(AVDISCARD_ALL);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
  }
}

void CVideoPlayer::CheckStreamChanges(CCurrentStream& current, CDemuxStream* stream)
{
  if (current.stream  != (void*)stream
  ||  current.changes != stream->changes)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if (current.hint != CDVDStreamInfo(*stream, true))
      OpenStream(current, stream->iId, stream->source );

    current.stream = (void*)stream;
    current.changes = stream->changes;
  }
}

void CVideoPlayer::ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentAudio, pStream);

  CheckContinuity(m_CurrentAudio, pPacket);
  UpdateTimestamps(m_CurrentAudio, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentAudio))
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
    m_VideoPlayerAudio->SendMessage(new CDVDMsgBool(CDVDMsg::AUDIO_SILENCE, true));
    m_EdlAutoSkipMarkers.mute = true;
  }
  else if (!m_Edl.InCut(DVD_TIME_TO_MSEC(m_CurrentAudio.dts + m_offset_pts), &cut) // Outside of any EDL
  &&        m_EdlAutoSkipMarkers.mute) // But the mute hasn't been removed yet
  {
    m_VideoPlayerAudio->SendMessage(new CDVDMsgBool(CDVDMsg::AUDIO_SILENCE, false));
    m_EdlAutoSkipMarkers.mute = false;
  }

  m_VideoPlayerAudio->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void CVideoPlayer::ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentVideo, pStream);

  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    CheckContinuity(m_CurrentVideo, pPacket);
    UpdateTimestamps(m_CurrentVideo, pPacket);
  }

  bool drop = false;
  if (CheckPlayerInit(m_CurrentVideo))
    drop = true;

  if (CheckSceneSkip(m_CurrentVideo))
    drop = true;

  m_VideoPlayerVideo->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void CVideoPlayer::ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentSubtitle, pStream);

  UpdateTimestamps(m_CurrentSubtitle, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentSubtitle))
    drop = true;

  if (CheckSceneSkip(m_CurrentSubtitle))
    drop = true;

  m_VideoPlayerSubtitle->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));

  if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    m_VideoPlayerSubtitle->UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
}

void CVideoPlayer::ProcessTeletextData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentTeletext, pStream);

  UpdateTimestamps(m_CurrentTeletext, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentTeletext))
    drop = true;

  if (CheckSceneSkip(m_CurrentTeletext))
    drop = true;

  m_VideoPlayerTeletext->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void CVideoPlayer::ProcessRadioRDSData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentRadioRDS, pStream);

  UpdateTimestamps(m_CurrentRadioRDS, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentRadioRDS))
    drop = true;

  if (CheckSceneSkip(m_CurrentRadioRDS))
    drop = true;

  m_VideoPlayerRadioRDS->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

bool CVideoPlayer::GetCachingTimes(double& level, double& delay, double& offset)
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
  {
    CLog::Log(LOGDEBUG, "Readrate %u is too low with %u required", currate, maxrate);
    level = -1.0;                          /* buffer is full & our read rate is too low  */
  }
  else
    level = (cached + queued) / (cache_need + queued);

  return true;
}

void CVideoPlayer::HandlePlaySpeed()
{
  bool isInMenu = IsInMenu();

  if (isInMenu && m_caching != CACHESTATE_DONE)
    SetCaching(CACHESTATE_DONE);

  if (m_caching == CACHESTATE_FULL)
  {
    double level, delay, offset;
    if (GetCachingTimes(level, delay, offset))
    {
      if (level < 0.0)
      {
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(21454), g_localizeStrings.Get(21455));
        SetCaching(CACHESTATE_INIT);
      }
      if (level >= 1.0)
        SetCaching(CACHESTATE_INIT);
    }
    else
    {
      if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
          (!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
        SetCaching(CACHESTATE_INIT);
    }
  }

  if (m_caching == CACHESTATE_INIT)
  {
    // if all enabled streams have been inited we are done
    if ((m_CurrentVideo.id >= 0 || m_CurrentAudio.id >= 0) &&
        (m_CurrentVideo.id < 0 || m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_STARTING) &&
        (m_CurrentAudio.id < 0 || m_CurrentAudio.syncState != IDVDStreamPlayer::SYNC_STARTING))
      SetCaching(CACHESTATE_PLAY);

    // handle situation that we get no data on one stream
    if (m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0)
    {
      if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_STARTING) ||
          (!m_VideoPlayerVideo->AcceptsData() && m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_STARTING))
      {
        SetCaching(CACHESTATE_DONE);
      }
    }
  }

  if (m_caching == CACHESTATE_PLAY)
  {
    // if all enabled streams have started playing we are done
    if ((m_CurrentVideo.id < 0 || !m_VideoPlayerVideo->IsStalled()) &&
        (m_CurrentAudio.id < 0 || !m_VideoPlayerAudio->IsStalled()))
      SetCaching(CACHESTATE_DONE);
  }

  if (m_caching == CACHESTATE_DONE)
  {
    if (m_playSpeed == DVD_PLAYSPEED_NORMAL && !isInMenu && m_syncTimer.IsTimePast())
    {
      // take action is audio or video stream is stalled
      if (m_VideoPlayerAudio->IsStalled() || m_VideoPlayerVideo->IsStalled())
      {
        if (CachePVRStream())
        {
          if ((m_CurrentAudio.id >= 0 && m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC && m_VideoPlayerAudio->GetLevel() == 0) ||
              (m_CurrentVideo.id >= 0 && m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC && m_VideoPlayerVideo->GetLevel() == 0))
          {
            CLog::Log(LOGDEBUG, "Stream stalled, start buffering. Audio: %d - Video: %d",
                                 m_VideoPlayerAudio->GetLevel(),m_VideoPlayerVideo->GetLevel());
            FlushBuffers(false);
          }
        }
        else
        {
          // start caching if audio and video have run dry
          if (m_VideoPlayerAudio->GetLevel() <= 50 &&
              m_VideoPlayerVideo->GetLevel() <= 50)
          {
            SetCaching(CACHESTATE_FULL);
          }
          else if (m_CurrentAudio.id >= 0 && m_CurrentAudio.inited &&
                   m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
                   m_VideoPlayerAudio->GetLevel() == 0)
          {
            CLog::Log(LOGDEBUG,"CVideoPlayer::HandlePlaySpeed - audio stream stalled, triggering re-sync");
            FlushBuffers(false);
          }
        }
      }
      // care for live streams
      else if (CachePVRStream())
      {
        if (m_CurrentAudio.id >= 0)
        {
          double adjust = -1.0; // a unique value
          if (m_clock.GetSpeedAdjust() >= 0 && m_VideoPlayerAudio->GetLevel() < 5)
            adjust = -0.01;

          if (m_clock.GetSpeedAdjust() < 0 && m_VideoPlayerAudio->GetLevel() > 10)
            adjust = 0.0;

          if (adjust != -1.0)
          {
            m_clock.SetSpeedAdjust(adjust);
            if (m_omxplayer_mode)
              m_OmxPlayerState.av_clock.OMXSetSpeedAdjust(adjust);
            CLog::Log(LOGDEBUG, "CVideoPlayer::HandlePlaySpeed set clock adjust: %f", adjust);
          }
        }
      }
    }
  }

  // sync streams to clock
  if ((m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
      (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC))
  {
    bool video = m_CurrentVideo.id < 0 || (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC);
    bool audio = m_CurrentAudio.id < 0 || (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC);
    if (video && audio)
    {
      double clock = 0;
      if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Audio - pts: %f, cache: %f, totalcache: %f",
                             m_CurrentAudio.starttime, m_CurrentAudio.cachetime, m_CurrentAudio.cachetotal);
      if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Video - pts: %f, cache: %f, totalcache: %f",
                             m_CurrentVideo.starttime, m_CurrentVideo.cachetime, m_CurrentVideo.cachetotal);

      if (m_CurrentAudio.starttime != DVD_NOPTS_VALUE)
      {
        if (m_pInputStream->IsRealtime())
          clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetotal - DVD_MSEC_TO_TIME(400);
        else
          clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetime;
        if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE &&
            m_CurrentVideo.starttime - m_CurrentVideo.cachetotal < clock)
        {
          clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
        }
      }
      else if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE)
      {
        clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
      }
      if (m_omxplayer_mode)
      {
        CLog::Log(LOGDEBUG, "%s::%s player started RESET", "CVideoPlayer", __FUNCTION__);
        m_OmxPlayerState.av_clock.OMXReset(m_CurrentVideo.id >= 0, m_playSpeed != DVD_PLAYSPEED_NORMAL && m_playSpeed != DVD_PLAYSPEED_PAUSE ? false: (m_CurrentAudio.id >= 0));
      }
      m_clock.Discontinuity(clock);
      m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_VideoPlayerAudio->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);
      m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);

      m_syncTimer.Set(3000);
    }
  }

  // handle ff/rw
  if(m_playSpeed != DVD_PLAYSPEED_NORMAL && m_playSpeed != DVD_PLAYSPEED_PAUSE)
  {
    if (isInMenu)
    {
      // this can't be done in menu
      SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    }
    else
    {
      bool check = true;

      // only check if we have video
      if (m_CurrentVideo.id < 0 || m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_INSYNC)
        check = false;
      // video message queue either initiated or already seen eof
      else if (m_CurrentVideo.inited == false && m_playSpeed >= 0)
        check = false;
      // don't check if time has not advanced since last check
      else if (m_SpeedState.lasttime == GetTime())
        check = false;
      // skip if frame at screen has no valid timestamp
      else if (m_VideoPlayerVideo->GetCurrentPts() == DVD_NOPTS_VALUE)
        check = false;
      // skip if frame on screen has not changed
      else if (m_SpeedState.lastpts == m_VideoPlayerVideo->GetCurrentPts() &&
               (m_SpeedState.lastpts > m_State.dts || m_playSpeed > 0))
        check = false;

      if (check)
      {
        m_SpeedState.lastpts  = m_VideoPlayerVideo->GetCurrentPts();
        m_SpeedState.lasttime = GetTime();
        m_SpeedState.lastabstime = CDVDClock::GetAbsoluteClock();
        // check how much off clock video is when ff/rw:ing
        // a problem here is that seeking isn't very accurate
        // and since the clock will be resynced after seek
        // we might actually not really be playing at the wanted
        // speed. we'd need to have some way to not resync the clock
        // after a seek to remember timing. still need to handle
        // discontinuities somehow

        double error;
        error  = m_clock.GetClock() - m_SpeedState.lastpts;
        error *= m_playSpeed / abs(m_playSpeed);

        // allow a bigger error when going ff, the faster we go
        // the the bigger is the error we allow
        if (m_playSpeed > DVD_PLAYSPEED_NORMAL)
        {
          int errorwin = m_playSpeed / DVD_PLAYSPEED_NORMAL;
          if (errorwin > 8)
            errorwin = 8;
          error /= errorwin;
        }

        if(error > DVD_MSEC_TO_TIME(1000))
        {
          error  = (int)DVD_TIME_TO_MSEC(m_clock.GetClock()) - m_SpeedState.lastseekpts;

          if(std::abs(error) > 1000)
          {
            CLog::Log(LOGDEBUG, "CVideoPlayer::Process - Seeking to catch up");
            m_SpeedState.lastseekpts = (int)DVD_TIME_TO_MSEC(m_clock.GetClock());
            int iTime = DVD_TIME_TO_MSEC(m_clock.GetClock() + m_State.time_offset + 1000000.0 * m_playSpeed / m_playSpeed);
            m_messenger.Put(new CDVDMsgPlayerSeek(iTime, (GetPlaySpeed() < 0), true, false, false, true, false));
          }
        }
      }
    }
  }
}

bool CVideoPlayer::CheckPlayerInit(CCurrentStream& current)
{
  if (current.inited)
    return false;

  if (current.startpts != DVD_NOPTS_VALUE)
  {
    if(current.dts == DVD_NOPTS_VALUE)
    {
      CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, current.player,  current.dts, current.startpts);
      return true;
    }

    if ((current.startpts - current.dts) > DVD_SEC_TO_TIME(20))
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
      if(m_CurrentRadioRDS.startpts != DVD_NOPTS_VALUE)
        m_CurrentRadioRDS.startpts = current.dts;
    }

    if(current.dts < current.startpts)
    {
      CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, current.player,  current.dts, current.startpts);
      return true;
    }
  }

  if (current.dts != DVD_NOPTS_VALUE)
  {
    current.inited = true;
    current.startpts = current.dts;
  }
  return false;
}

void CVideoPlayer::UpdateCorrection(DemuxPacket* pkt, double correction)
{
  if(pkt->dts != DVD_NOPTS_VALUE) pkt->dts -= correction;
  if(pkt->pts != DVD_NOPTS_VALUE) pkt->pts -= correction;
}

void CVideoPlayer::UpdateTimestamps(CCurrentStream& current, DemuxPacket* pPacket)
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

  /* send a playback state structure periodically */
  if(current.dts_state == DVD_NOPTS_VALUE
  || std::abs(current.dts - current.dts_state) > DVD_MSEC_TO_TIME(200))
  {
    current.dts_state = current.dts;
    if (current.inited)
    {
      // make sure we send no outdated state to a/v players
      UpdatePlayState(0);
      SendPlayerMessage(new CDVDMsgType<SPlayerState>(CDVDMsg::PLAYER_DISPLAYTIME, m_StateInput), current.player);
    }
    else
    {
      CSingleLock lock(m_StateSection);
      m_State = m_StateInput;
    }
  }
}

static void UpdateLimits(double& minimum, double& maximum, double dts)
{
  if(dts == DVD_NOPTS_VALUE)
    return;
  if(minimum == DVD_NOPTS_VALUE || minimum > dts) minimum = dts;
  if(maximum == DVD_NOPTS_VALUE || maximum < dts) maximum = dts;
}

void CVideoPlayer::CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket)
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
    CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - resync forward :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - maxdts);
    correction = pPacket->dts - maxdts;
  }

  /* if it's large scale jump, correct for it after having confirmed the jump */
  if(pPacket->dts + DVD_MSEC_TO_TIME(100) < current.dts_end())
  {
    CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - resync backward :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
    correction = pPacket->dts - current.dts_end();
  }
  else if(pPacket->dts < current.dts)
  {
    CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - wrapback :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
  }

  double lastdts = pPacket->dts;
  if(correction != 0.0)
  {
    // we want the dts values of two streams to close, or for one to be invalid (e.g. from a missing audio stream)
    double this_dts = pPacket->dts;
    double that_dts = current.type == STREAM_AUDIO ? m_CurrentVideo.lastdts : m_CurrentAudio.lastdts;

    if (m_CurrentAudio.id == -1 || m_CurrentVideo.id == -1 ||
       current.lastdts == DVD_NOPTS_VALUE ||
       fabs(this_dts - that_dts) < DVD_MSEC_TO_TIME(1000))
    {
      m_offset_pts += correction;
      UpdateCorrection(pPacket, correction);
      lastdts = pPacket->dts;
    }
    else
    {
      // not sure yet - flags the packets as unknown until we get confirmation on another audio/video packet
      pPacket->dts = DVD_NOPTS_VALUE;
      pPacket->pts = DVD_NOPTS_VALUE;
    }
  }
  current.lastdts = lastdts;
}

bool CVideoPlayer::CheckSceneSkip(CCurrentStream& current)
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

void CVideoPlayer::CheckAutoSceneSkip()
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
   * If there is a startpts defined for either the audio or video stream then VideoPlayer is still
   * still decoding frames to get to the previously requested seek point.
   */
  if(m_CurrentAudio.inited == false
  || m_CurrentVideo.inited == false)
    return;

  if(m_CurrentAudio.dts == DVD_NOPTS_VALUE
  || m_CurrentVideo.dts == DVD_NOPTS_VALUE)
    return;

  const int64_t clock = m_omxplayer_mode ? GetTime() : DVD_TIME_TO_MSEC(std::min(m_CurrentAudio.dts, m_CurrentVideo.dts) + m_offset_pts);

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
    int seek = GetPlaySpeed() >= 0 ? cut.end : cut.start;
    /*
     * Seeking is NOT flushed so any content up to the demux point is retained when playing forwards.
     */
    m_messenger.Put(new CDVDMsgPlayerSeek(seek, true, m_omxplayer_mode, true, false, true));
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
    m_messenger.Put(new CDVDMsgPlayerSeek(cut.end + 1, true, m_omxplayer_mode, true, false, true));
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


void CVideoPlayer::SynchronizeDemuxer(unsigned int timeout)
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

void CVideoPlayer::SynchronizePlayers(unsigned int sources)
{
  /* we need a big timeout as audio queue is about 8seconds for 2ch ac3 */
  const int timeout = 10*1000; // in milliseconds

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, sources);
  if (m_CurrentAudio.id >= 0)
    m_VideoPlayerAudio->SendMessage(message->Acquire());

  if (m_CurrentVideo.id >= 0)
    m_VideoPlayerVideo->SendMessage(message->Acquire());
/* TODO - we have to rewrite the sync class, to not require
          all other players waiting for subtitle, should only
          be the oposite way
  if (m_CurrentSubtitle.id >= 0)
    m_VideoPlayerSubtitle->SendMessage(message->Acquire());
*/
  message->Release();
}

IDVDStreamPlayer* CVideoPlayer::GetStreamPlayer(unsigned int target)
{
  if(target == VideoPlayer_AUDIO)
    return m_VideoPlayerAudio;
  if(target == VideoPlayer_VIDEO)
    return m_VideoPlayerVideo;
  if(target == VideoPlayer_SUBTITLE)
    return m_VideoPlayerSubtitle;
  if(target == VideoPlayer_TELETEXT)
    return m_VideoPlayerTeletext;
  if(target == VideoPlayer_RDS)
    return m_VideoPlayerRadioRDS;
  return NULL;
}

void CVideoPlayer::SendPlayerMessage(CDVDMsg* pMsg, unsigned int target)
{
  IDVDStreamPlayer* player = GetStreamPlayer(target);
  if(player)
    player->SendMessage(pMsg, 0);
}

void CVideoPlayer::OnExit()
{
    CLog::Log(LOGNOTICE, "CVideoPlayer::OnExit()");

    // set event to inform openfile something went wrong in case openfile is still waiting for this event
    SetCaching(CACHESTATE_DONE);

    // close each stream
    if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "VideoPlayer: eof, waiting for queues to empty");
    CloseStream(m_CurrentAudio,    !m_bAbortRequest);
    CloseStream(m_CurrentVideo,    !m_bAbortRequest);

    // the generalization principle was abused for subtitle player. actually it is not a stream player like
    // video and audio. subtitle player does not run on its own thread, hence waitForBuffers makes
    // no sense here. waitForBuffers is abused to clear overlay container (false clears container)
    // subtitles are added from video player. after video player has finished, overlays have to be cleared.
    CloseStream(m_CurrentSubtitle, false);  // clear overlay container

    CloseStream(m_CurrentTeletext, !m_bAbortRequest);
    CloseStream(m_CurrentRadioRDS, !m_bAbortRequest);

    // destroy objects
    SAFE_DELETE(m_pDemuxer);
    SAFE_DELETE(m_pSubtitleDemuxer);
    SAFE_DELETE(m_pCCDemuxer);
    SAFE_DELETE(m_pInputStream);

    // clean up all selection streams
    m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

    m_messenger.End();

    if (m_omxplayer_mode)
    {
      m_OmxPlayerState.av_clock.OMXStop();
      m_OmxPlayerState.av_clock.OMXStateIdle();
      m_OmxPlayerState.av_clock.OMXDeinitialize();
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

void CVideoPlayer::HandleMessages()
{
  CDVDMsg* pMsg;

  while (m_messenger.Get(&pMsg, 0) == MSGQ_OK)
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

        int time = msg.GetRestore() ? m_Edl.RestoreCutTime(msg.GetTime()) : msg.GetTime();

        // if input streams doesn't support seektime we must convert back to clock
        if(dynamic_cast<CDVDInputStream::ISeekTime*>(m_pInputStream) == NULL)
          time -= DVD_TIME_TO_MSEC(m_State.time_offset - m_offset_pts);

        CLog::Log(LOGDEBUG, "demuxer seek to: %d", time);
        if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start))
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", time);
          if(m_pSubtitleDemuxer)
          {
            if(!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
              CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %d, success", time);
          }
          // dts after successful seek
          if (m_StateInput.time_src  == ETIMESOURCE_CLOCK && start == DVD_NOPTS_VALUE)
            m_StateInput.dts = DVD_MSEC_TO_TIME(time);
          else
            m_StateInput.dts = start;

          FlushBuffers(!msg.GetFlush(), start, msg.GetAccurate(), msg.GetSync());
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
        double offset = 0;
        int64_t beforeSeek = GetTime();

        // This should always be the case.
        if(m_pDemuxer && m_pDemuxer->SeekChapter(msg.GetChapter(), &start))
        {
          FlushBuffers(false, start, true);
          offset = DVD_TIME_TO_MSEC(start) - beforeSeek;
          m_callback.OnPlayBackSeekChapter(msg.GetChapter());
        }

        g_infoManager.SetDisplayAfterSeek(2500, offset);
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

        SelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveAudioStream(st.id))
            {
              m_dvd.iSelectedAudioStream = -1;
              CloseStream(m_CurrentAudio, false);
              m_messenger.Put(new CDVDMsgPlayerSeek((int) GetTime(), true, true, true, true, true));
            }
          }
          else
          {
            CloseStream(m_CurrentAudio, false);
            OpenStream(m_CurrentAudio, st.id, st.source);
            AdaptForcedSubtitles();
            m_messenger.Put(new CDVDMsgPlayerSeek((int) GetTime(), true, true, true, true, true));
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
      {
        CDVDMsgPlayerSetSubtitleStream* pMsg2 = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

        SelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveSubtitleStream(st.id))
            {
              m_dvd.iSelectedSPUStream = -1;
              CloseStream(m_CurrentSubtitle, false);
            }
          }
          else
          {
            CloseStream(m_CurrentSubtitle, false);
            OpenStream(m_CurrentSubtitle, st.id, st.source);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
      {
        CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;
        SetSubtitleVisibleInternal(pValue->m_value);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
      {
        g_infoManager.SetDisplayAfterSeek(100000);
        SetCaching(CACHESTATE_FLUSH);

        CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

        if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
        {
          if(ptr->SetState(pMsgPlayerSetState->GetState()))
          {
            m_dvd.state = DVDSTATE_NORMAL;
            m_dvd.iDVDStillStartTime = 0;
            m_dvd.iDVDStillTime = 0;
          }
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
          offset  = CDVDClock::GetAbsoluteClock() - m_State.timestamp;
          offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
          offset  = DVD_TIME_TO_MSEC(offset);
          if(offset >  1000) offset =  1000;
          if(offset < -1000) offset = -1000;
          m_State.time     += offset;
          m_State.timestamp =  CDVDClock::GetAbsoluteClock();
        }

        if (speed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_PAUSE && speed != m_playSpeed)
          m_callback.OnPlayBackSpeedChanged(speed / DVD_PLAYSPEED_NORMAL);

        if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) && speed != m_playSpeed)
        {
          CDVDInputStreamPVRManager* pvrinputstream = static_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
          pvrinputstream->Pause( speed == 0 );
        }

        // do a seek after rewind, clock is not in sync with current pts
        if (m_omxplayer_mode)
        {
          // when switching from trickplay to normal, we may not have a full set of reference frames
          // in decoder and we may get corrupt frames out. Seeking to current time will avoid this.
          if ( (speed != DVD_PLAYSPEED_PAUSE && speed != DVD_PLAYSPEED_NORMAL) ||
               (m_playSpeed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_NORMAL) )
          {
            m_messenger.Put(new CDVDMsgPlayerSeek(GetTime(), (speed < 0), true, true, false, true));
          }
          else
          {
            m_OmxPlayerState.av_clock.OMXPause();
          }

          m_OmxPlayerState.av_clock.OMXSetSpeed(speed);
          CLog::Log(LOGDEBUG, "%s::%s CDVDMsg::PLAYER_SETSPEED speed : %d (%d)", "CVideoPlayer", __FUNCTION__, speed, m_playSpeed);
        }
        else if ((speed == DVD_PLAYSPEED_NORMAL) &&
                 (m_playSpeed != DVD_PLAYSPEED_NORMAL) &&
                 (m_playSpeed != DVD_PLAYSPEED_PAUSE))
        {
          int64_t iTime = (int64_t)DVD_TIME_TO_MSEC(m_clock.GetClock() + m_State.time_offset);
          if (m_State.disptime != DVD_NOPTS_VALUE)
            iTime = m_State.disptime;
          m_messenger.Put(new CDVDMsgPlayerSeek(iTime, m_playSpeed < 0, true, false, false, true));
        }

        // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
        // audioplayer, stops outputing audio to audiorendere, but still tries to
        // sleep an correct amount for each packet
        // videoplayer just plays faster after the clock speed has been increased
        // 1. disable audio
        // 2. skip frames and adjust their pts or the clock
        m_playSpeed = speed;
        m_caching = CACHESTATE_DONE;
        m_clock.SetSpeed(speed);
        m_VideoPlayerAudio->SetSpeed(speed);
        m_VideoPlayerVideo->SetSpeed(speed);

        // We can't pause demuxer until our buffers are full. Doing so will result in continued
        // calls to Read() which may then block indefinitely (CDVDInputStreamRTMP for example).
        if(m_pDemuxer)
        {
          m_DemuxerPausePending = (speed == DVD_PLAYSPEED_PAUSE);
          if (!m_DemuxerPausePending)
            m_pDemuxer->SetSpeed(speed);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) == 0)
      {
        FlushBuffers(false);
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input && input->SelectChannelByNumber(static_cast<CDVDMsgInt*>(pMsg)->m_value))
        {
          SAFE_DELETE(m_pDemuxer);
          m_playSpeed = DVD_PLAYSPEED_NORMAL;

          // when using fast channel switching some shortcuts are taken which 
          // means we'll have to update the view mode manually
          m_renderManager.SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
        }else
        {
          CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
          CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT) && m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT) == 0)
      {
        FlushBuffers(false);
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input && input->SelectChannel(static_cast<CDVDMsgType <CPVRChannelPtr> *>(pMsg)->m_value))
        {
          SAFE_DELETE(m_pDemuxer);
          m_playSpeed = DVD_PLAYSPEED_NORMAL;
        }else
        {
          CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
          CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREV) ||
               pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV))
      {
        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if (input)
        {
          bool bSwitchSuccessful(false);
          bool bShowPreview(!g_infoManager.IsPlayerOSDActive() &&
                            (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT) ||
                             pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV)));

          if (!bShowPreview)
          {
            g_infoManager.SetDisplayAfterSeek(100000);
            FlushBuffers(false);
          }

          if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT))
            bSwitchSuccessful = input->NextChannel(bShowPreview);
          else
            bSwitchSuccessful = input->PrevChannel(bShowPreview);

          if (bSwitchSuccessful)
          {
            if (bShowPreview)
            {
              UpdateApplication(0);

              if (!g_infoManager.IsPlayerOSDActive() &&
                  CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH))
                m_ChannelEntryTimeOut.SetInfinite();
              else
                m_ChannelEntryTimeOut.Set(CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT));
            }
            else
            {
              m_ChannelEntryTimeOut.SetInfinite();
              SAFE_DELETE(m_pDemuxer);
              m_playSpeed = DVD_PLAYSPEED_NORMAL;

              g_infoManager.SetDisplayAfterSeek();

              // when using fast channel switching some shortcuts are taken which 
              // means we'll have to update the view mode manually
              m_renderManager.SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
            }
          }
          else
          {
            CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
            CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
        OnAction(((CDVDMsgType<CAction>*)pMsg)->m_value);
      else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
      {
        SStartMsg& msg = ((CDVDMsgType<SStartMsg>*)pMsg)->m_value;
        if (msg.player == VideoPlayer_AUDIO)
        {
          m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
          m_CurrentAudio.cachetime = msg.cachetime;
          m_CurrentAudio.cachetotal = msg.cachetotal;
          m_CurrentAudio.starttime = msg.timestamp;
        }
        if (msg.player == VideoPlayer_VIDEO)
        {
          m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
          m_CurrentVideo.cachetime = msg.cachetime;
          m_CurrentVideo.cachetotal = msg.cachetotal;
          m_CurrentVideo.starttime = msg.timestamp;
        }
        CLog::Log(LOGDEBUG, "CVideoPlayer::HandleMessages - player started %d", msg.player);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_DISPLAYTIME))
      {
        SPlayerState& state = ((CDVDMsgType<SPlayerState>*)pMsg)->m_value;

        CSingleLock lock(m_StateSection);
        /* prioritize data from video player, but only accept data        *
         * after it has been started to avoid race conditions after seeks */
        if(m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC && !m_VideoPlayerVideo->SubmittedEOS())
        {
          if(state.player == VideoPlayer_VIDEO)
            m_State = state;
        }
        else if(m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC)
        {
          if(state.player == VideoPlayer_AUDIO)
            m_State = state;
        }
      }
      else if (pMsg->IsType(CDVDMsg::SUBTITLE_ADDFILE))
      {
        int id = AddSubtitleFile(((CDVDMsgType<std::string>*) pMsg)->m_value);
        if (id >= 0)
        {
          SetSubtitle(id);
          SetSubtitleVisibleInternal(true);
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
      {
        if (((CDVDMsgGeneralSynchronize*)pMsg)->Wait(100, SYNCSOURCE_OWNER))
          CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::GENERAL_SYNCHRONIZE");
      }

    pMsg->Release();
  }

}

void CVideoPlayer::SetCaching(ECacheState state)
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

  CLog::Log(LOGDEBUG, "CVideoPlayer::SetCaching - caching state %d", state);
  if (state == CACHESTATE_FULL ||
      state == CACHESTATE_INIT)
  {
    m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);

    if (m_omxplayer_mode)
      m_OmxPlayerState.av_clock.OMXPause();

    m_VideoPlayerAudio->SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_VideoPlayerVideo->SetSpeed(DVD_PLAYSPEED_PAUSE);

    if (CachePVRStream())
      m_pInputStream->ResetScanTimeout((unsigned int) CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_SCANTIME) * 1000);
  }

  if (state == CACHESTATE_PLAY ||
     (state == CACHESTATE_DONE && m_caching != CACHESTATE_PLAY))
  {
    m_clock.SetSpeed(m_playSpeed);
    m_VideoPlayerAudio->SetSpeed(m_playSpeed);
    m_VideoPlayerVideo->SetSpeed(m_playSpeed);
    m_pInputStream->ResetScanTimeout(0);
  }
  m_caching = state;

  m_clock.SetSpeedAdjust(0);
  if (m_omxplayer_mode)
    m_OmxPlayerState.av_clock.OMXSetSpeedAdjust(0);
}

void CVideoPlayer::SetPlaySpeed(int speed)
{
  if (IsPlaying())
    m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
  else
    m_playSpeed = speed;

  m_VideoPlayerAudio->SetSpeed(speed);
  m_VideoPlayerVideo->SetSpeed(speed);
  SynchronizeDemuxer(100);
}

bool CVideoPlayer::CanPause()
{
  CSingleLock lock(m_StateSection);
  return m_State.canpause;
}

void CVideoPlayer::Pause()
{
  CSingleLock lock(m_StateSection);
  if (!m_State.canpause)
    return;
  lock.Leave();

  if(m_playSpeed != DVD_PLAYSPEED_PAUSE && IsCaching())
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

bool CVideoPlayer::IsPaused() const
{
  return m_playSpeed == DVD_PLAYSPEED_PAUSE || IsCaching();
}

bool CVideoPlayer::HasVideo() const
{
  return m_HasVideo;
}

bool CVideoPlayer::HasAudio() const
{
  return m_HasAudio;
}

bool CVideoPlayer::HasRDS() const
{
  return m_CurrentRadioRDS.id >= 0;
}

bool CVideoPlayer::IsPassthrough() const
{
  return m_VideoPlayerAudio->IsPassthrough();
}

bool CVideoPlayer::CanSeek()
{
  CSingleLock lock(m_StateSection);
  return m_State.canseek;
}

void CVideoPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  if( m_playSpeed == DVD_PLAYSPEED_PAUSE && bPlus && !bLargeStep)
  {
    if (m_VideoPlayerVideo->StepFrame())
      return;
  }
  if (!m_State.canseek)
    return;

  if (bLargeStep && bChapterOverride && GetChapter() > 0)
  {
    if (!bPlus)
    {
      SeekChapter(GetChapter() - 1);
      return;
    }
    else if (GetChapter() < GetChapterCount())
    {
      SeekChapter(GetChapter() + 1);
      return;
    }
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
    int percent;
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
    const int clock = DVD_TIME_TO_MSEC(m_clock.GetClock());
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
    // warning, don't access any VideoPlayer variables here as
    // the VideoPlayer object may have been destroyed
    return;
  }

  m_messenger.Put(new CDVDMsgPlayerSeek((int)seek, !bPlus, true, false, restore));
  SynchronizeDemuxer(100);
  if (seek < 0) seek = 0;
  m_callback.OnPlayBackSeek((int)seek, (int)(seek - time));
}

bool CVideoPlayer::SeekScene(bool bPlus)
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

  int iScenemarker;
  if (m_Edl.GetNextSceneMarker(bPlus, clock, &iScenemarker))
  {
    /*
     * Seeking is flushed and inaccurate, just like Seek()
     */
    m_messenger.Put(new CDVDMsgPlayerSeek(iScenemarker, !bPlus, true, false, false));
    SynchronizeDemuxer(100);
    return true;
  }
  return false;
}

void CVideoPlayer::GetAudioInfo(std::string& strAudioInfo)
{
  { CSingleLock lock(m_StateSection);
    strAudioInfo = StringUtils::Format("D(%s)", m_StateInput.demux_audio.c_str());
  }
  strAudioInfo += StringUtils::Format("\nP(%s)", m_VideoPlayerAudio->GetPlayerInfo().c_str());
}

void CVideoPlayer::GetVideoInfo(std::string& strVideoInfo)
{
  { CSingleLock lock(m_StateSection);
    strVideoInfo = StringUtils::Format("D(%s)", m_StateInput.demux_video.c_str());
  }
  strVideoInfo += StringUtils::Format("\nP(%s)", m_VideoPlayerVideo->GetPlayerInfo().c_str());
}

void CVideoPlayer::GetGeneralInfo(std::string& strGeneralInfo)
{
  if (!m_bStop)
  {
    if (m_omxplayer_mode)
    {
      double apts = m_VideoPlayerAudio->GetCurrentPts();
      double vpts = m_VideoPlayerVideo->GetCurrentPts();
      double dDiff = 0;

      if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
        dDiff = (apts - vpts) / DVD_TIME_BASE;

      std::string strEDL;
      strEDL += StringUtils::Format(", edl:%s", m_Edl.GetInfo().c_str());

      std::string strBuf;
      CSingleLock lock(m_StateSection);
      if(m_StateInput.cache_bytes >= 0)
      {
        strBuf += StringUtils::Format(" forward:%s %2.0f%%"
                                      , StringUtils::SizeToString(m_StateInput.cache_bytes).c_str()
                                      , m_StateInput.cache_level * 100);
        if(m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
          strBuf += StringUtils::Format(" %d sec", DVD_TIME_TO_SEC(m_StateInput.cache_delay));
      }

      strGeneralInfo = StringUtils::Format("C( a/v:% 6.3f%s, dcpu:%2i%% acpu:%2i%% vcpu:%2i%%%s af:%d%% vf:%d%% amp:% 5.2f )"
          , dDiff
          , strEDL.c_str()
          , (int)(CThread::GetRelativeUsage()*100)
          , (int)(m_VideoPlayerAudio->GetRelativeUsage()*100)
          , (int)(m_VideoPlayerVideo->GetRelativeUsage()*100)
          , strBuf.c_str()
          , m_OmxPlayerState.audio_fifo
          , m_OmxPlayerState.video_fifo
          , m_VideoPlayerAudio->GetDynamicRangeAmplification());
    }
    else
    {
      double dDelay = m_VideoPlayerVideo->GetDelay() / DVD_TIME_BASE - m_renderManager.GetDisplayLatency();

      double apts = m_VideoPlayerAudio->GetCurrentPts();
      double vpts = m_VideoPlayerVideo->GetCurrentPts();
      double dDiff = 0;

      if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
        dDiff = (apts - vpts) / DVD_TIME_BASE;

      std::string strEDL = StringUtils::Format(", edl:%s", m_Edl.GetInfo().c_str());

      std::string strBuf;
      CSingleLock lock(m_StateSection);
      if(m_StateInput.cache_bytes >= 0)
      {
        strBuf += StringUtils::Format(" forward:%s %2.0f%%"
                                      , StringUtils::SizeToString(m_StateInput.cache_bytes).c_str()
                                      , m_StateInput.cache_level * 100);
        if(m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
          strBuf += StringUtils::Format(" %d sec", DVD_TIME_TO_SEC(m_StateInput.cache_delay));
      }

      strGeneralInfo = StringUtils::Format("C( ad:% 6.3f, a/v:% 6.3f%s, dcpu:%2i%% acpu:%2i%% vcpu:%2i%%%s )"
                                           , dDelay
                                           , dDiff
                                           , strEDL.c_str()
                                           , (int)(CThread::GetRelativeUsage()*100)
                                           , (int)(m_VideoPlayerAudio->GetRelativeUsage()*100)
                                           , (int)(m_VideoPlayerVideo->GetRelativeUsage()*100)
                                           , strBuf.c_str());
    }
  }
}

void CVideoPlayer::SeekPercentage(float iPercent)
{
  int64_t iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return;

  SeekTime((int64_t)(iTotalTime * iPercent / 100));
}

float CVideoPlayer::GetPercentage()
{
  int64_t iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

float CVideoPlayer::GetCachePercentage()
{
  CSingleLock lock(m_StateSection);
  return (float) (m_StateInput.cache_offset * 100); // NOTE: Percentage returned is relative
}

void CVideoPlayer::SetAVDelay(float fValue)
{
  m_VideoPlayerVideo->SetDelay( (fValue * DVD_TIME_BASE) ) ;
}

float CVideoPlayer::GetAVDelay()
{
  return (float) m_VideoPlayerVideo->GetDelay() / (float)DVD_TIME_BASE;
}

void CVideoPlayer::SetSubTitleDelay(float fValue)
{
  m_VideoPlayerVideo->SetSubtitleDelay(-fValue * DVD_TIME_BASE);
}

float CVideoPlayer::GetSubTitleDelay()
{
  return (float) -m_VideoPlayerVideo->GetSubtitleDelay() / DVD_TIME_BASE;
}

// priority: 1: libdvdnav, 2: external subtitles, 3: muxed subtitles
int CVideoPlayer::GetSubtitleCount()
{
  return m_SelectionStreams.Count(STREAM_SUBTITLE);
}

int CVideoPlayer::GetSubtitle()
{
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, *this);
}

void CVideoPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info)
{
  if (index < 0 || index > (int) GetSubtitleCount() - 1)
    return;

  SelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, index);
  if(s.name.length() > 0)
    info.name = s.name;

  if(s.type == STREAM_NONE)
    info.name += "(Invalid)";

  info.language = s.language;
}

void CVideoPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetSubtitleStream(iStream));
}

bool CVideoPlayer::GetSubtitleVisible()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->IsSubtitleStreamEnabled();
  }

  return m_VideoPlayerVideo->IsSubtitleEnabled();
}

void CVideoPlayer::SetSubtitleVisible(bool bVisible)
{
  m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
}

void CVideoPlayer::SetSubtitleVisibleInternal(bool bVisible)
{
  m_VideoPlayerVideo->EnableSubtitle(bVisible);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->EnableSubtitleStream(bVisible);
}

int CVideoPlayer::GetAudioStreamCount()
{
  return m_SelectionStreams.Count(STREAM_AUDIO);
}

int CVideoPlayer::GetAudioStream()
{
  return m_SelectionStreams.IndexOf(STREAM_AUDIO, *this);
}

void CVideoPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
  SynchronizeDemuxer(100);
}

TextCacheStruct_t* CVideoPlayer::GetTeletextCache()
{
  if (m_CurrentTeletext.id < 0)
    return 0;

  return m_VideoPlayerTeletext->GetTeletextCache();
}

void CVideoPlayer::LoadPage(int p, int sp, unsigned char* buffer)
{
  if (m_CurrentTeletext.id < 0)
      return;

  return m_VideoPlayerTeletext->LoadPage(p, sp, buffer);
}

std::string CVideoPlayer::GetRadioText(unsigned int line)
{
  if (m_CurrentRadioRDS.id < 0)
      return "";

  return m_VideoPlayerRadioRDS->GetRadioText(line);
}

void CVideoPlayer::SeekTime(int64_t iTime)
{
  int seekOffset = (int)(iTime - GetTime());
  m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime, true, true, true));
  SynchronizeDemuxer(100);
  m_callback.OnPlayBackSeek((int)iTime, seekOffset);
}

bool CVideoPlayer::SeekTimeRelative(int64_t iTime)
{
  int64_t abstime = GetTime() + iTime;
  m_messenger.Put(new CDVDMsgPlayerSeek((int)abstime, (iTime < 0) ? true : false, true, false));
  SynchronizeDemuxer(100);
  m_callback.OnPlayBackSeek((int)abstime, iTime);
  return true;
}

// return the time in milliseconds
int64_t CVideoPlayer::GetTime()
{
  CSingleLock lock(m_StateSection);
  double offset = 0;
  const double limit  = DVD_MSEC_TO_TIME(200);
  if(m_State.timestamp > 0)
  {
    offset  = CDVDClock::GetAbsoluteClock() - m_State.timestamp;
    offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
    if(offset >  limit) offset =  limit;
    if(offset < -limit) offset = -limit;
  }
  return llrint(m_State.time + DVD_TIME_TO_MSEC(offset));
}

// return the time in milliseconds
int64_t CVideoPlayer::GetDisplayTime()
{
  CSingleLock lock(m_StateSection);
  double offset = 0;
  const double limit = DVD_MSEC_TO_TIME(200);
  if (m_State.timestamp > 0)
  {
    offset = CDVDClock::GetAbsoluteClock() - m_State.timestamp;
    offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
    if (offset > limit)
      offset = limit;
    if (offset < 0)
      offset = 0;
  }
  int64_t ret = llrint(m_State.disptime + DVD_TIME_TO_MSEC(offset));
  if (ret < 0)
    ret = 0;
  return ret;
}

// return length in msec
int64_t CVideoPlayer::GetTotalTimeInMsec()
{
  CSingleLock lock(m_StateSection);
  return llrint(m_State.time_total);
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int64_t CVideoPlayer::GetTotalTime()
{
  return GetTotalTimeInMsec();
}

void CVideoPlayer::ToFFRW(int iSpeed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if (iSpeed < 0 && IsInMenu()) return;
  SetPlaySpeed(iSpeed * DVD_PLAYSPEED_NORMAL);
}

bool CVideoPlayer::OpenStream(CCurrentStream& current, int iStream, int source, bool reset)
{
  CDemuxStream* stream = NULL;
  CDVDStreamInfo hint;

  CLog::Log(LOGNOTICE, "Opening stream: %i source: %i", iStream, source);

  if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
  {
    int index = m_SelectionStreams.IndexOf(current.type, source, iStream);
    if(index < 0)
      return false;
    SelectionStream st = m_SelectionStreams.Get(current.type, index);

    if(!m_pSubtitleDemuxer || m_pSubtitleDemuxer->GetFileName() != st.filename)
    {
      CLog::Log(LOGNOTICE, "Opening Subtitle file: %s", st.filename.c_str());
      std::unique_ptr<CDVDDemuxVobsub> demux(new CDVDDemuxVobsub());
      if(!demux->Open(st.filename, source, st.filename2))
        return false;
      m_pSubtitleDemuxer = demux.release();
    }

    double pts = m_VideoPlayerVideo->GetCurrentPts();
    if(pts == DVD_NOPTS_VALUE)
      pts = m_CurrentVideo.dts;
    if(pts == DVD_NOPTS_VALUE)
      pts = 0;
    pts += m_offset_pts;
    if (!m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE)))
        CLog::Log(LOGDEBUG, "%s - failed to start subtitle demuxing from: %f", __FUNCTION__, pts);
    stream = m_pSubtitleDemuxer->GetStream(iStream);
    if(!stream || stream->disabled)
      return false;
    stream->SetDiscard(AVDISCARD_NONE);

    hint.Assign(*stream, true);
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
  {
    int index = m_SelectionStreams.IndexOf(current.type, source, iStream);
    if(index < 0)
      return false;

    hint.Clear();
    hint.filename = m_SelectionStreams.Get(current.type, index).filename;
    hint.fpsscale = m_CurrentVideo.hint.fpsscale;
    hint.fpsrate  = m_CurrentVideo.hint.fpsrate;
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX)
  {
    if(!m_pDemuxer)
      return false;

    stream = m_pDemuxer->GetStream(iStream);
    if(!stream || stream->disabled)
      return false;
    stream->SetDiscard(AVDISCARD_NONE);

    hint.Assign(*stream, true);

    if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      hint.filename = "dvd";
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_VIDEOMUX)
  {
    if(!m_pCCDemuxer)
      return false;

    stream = m_pCCDemuxer->GetStream(iStream);
    if(!stream || stream->disabled)
      return false;

    hint.Assign(*stream, false);
  }

  bool res;
  switch(current.type)
  {
    case STREAM_AUDIO:
      res = OpenAudioStream(hint, reset);
      break;
    case STREAM_VIDEO:
      res = OpenVideoStream(hint, reset);
      break;
    case STREAM_SUBTITLE:
      res = OpenSubtitleStream(hint);
      break;
    case STREAM_TELETEXT:
      res = OpenTeletextStream(hint);
      break;
    case STREAM_RADIO_RDS:
      res = OpenRadioRDSStream(hint);
      break;
    default:
      res = false;
      break;
  }

  if (res)
  {
    current.id      = iStream;
    current.source  = source;
    current.hint    = hint;
    current.stream  = (void*)stream;
    current.syncState = IDVDStreamPlayer::SYNC_STARTING;
    current.lastdts = DVD_NOPTS_VALUE;
    if(stream)
      current.changes = stream->changes;
  }
  else
  {
    if(stream)
    {
      /* mark stream as disabled, to disallaw further attempts*/
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, stream->iId);
      stream->disabled = true;
      stream->SetDiscard(AVDISCARD_ALL);
    }
  }

  g_dataCacheCore.SignalAudioInfoChange();
  g_dataCacheCore.SignalVideoInfoChange();

  return res;
}

bool CVideoPlayer::OpenStreamPlayer(CCurrentStream& current, CDVDStreamInfo& hint, bool reset)
{
  IDVDStreamPlayer* player = GetStreamPlayer(current.player);
  if(player == NULL)
    return false;

  if(current.id < 0 ||
     current.hint != hint)
  {
    if (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264)
      SAFE_DELETE(m_pCCDemuxer);

    if (!player->OpenStream(hint))
      return false;
  }
  else if (reset)
    player->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET), 0);
  return true;
}

bool CVideoPlayer::OpenAudioStream(CDVDStreamInfo& hint, bool reset)
{
  if(!OpenStreamPlayer(m_CurrentAudio, hint, reset))
    return false;

  m_HasAudio = true;

  return true;
}

bool CVideoPlayer::OpenVideoStream(CDVDStreamInfo& hint, bool reset)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
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
  else if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    // set framerate if not set by demuxer
    if (hint.fpsrate == 0 || hint.fpsscale == 0)
    {
      int fpsidx = CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_FPS);
      if (fpsidx == 1)
      {
        hint.fpsscale = 1000;
        hint.fpsrate = 50000;
      }
      else if (fpsidx == 2)
      {
        hint.fpsscale = 1001;
        hint.fpsrate = 60000;
      }
    }
  }

  CDVDInputStream::IMenus* pMenus = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if(pMenus && pMenus->IsInMenu())
    hint.stills = true;

  if (hint.stereo_mode.empty())
    hint.stereo_mode = CStereoscopicsManager::GetInstance().DetectStereoModeByString(m_filename);

  if(hint.flags & AV_DISPOSITION_ATTACHED_PIC)
    return false;

  // set desired refresh rate
  if (m_PlayerOptions.fullscreen && g_graphicsContext.IsFullScreenRoot() &&
      hint.fpsrate != 0 && hint.fpsscale != 0)
  {
    if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
    {
      float framerate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE * hint.fpsscale / hint.fpsrate);
      m_renderManager.TriggerUpdateResolution(framerate, hint.width, RenderManager::GetStereoModeFlags(hint.stereo_mode));
    }
  }

  if(!OpenStreamPlayer(m_CurrentVideo, hint, reset))
    return false;

  m_HasVideo = true;

  // open CC demuxer if video is mpeg2
  if ((hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264) && !m_pCCDemuxer)
  {
    m_pCCDemuxer = new CDVDDemuxCC(hint.codec);
    m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_VIDEOMUX);
  }

  return true;

}

bool CVideoPlayer::OpenSubtitleStream(CDVDStreamInfo& hint)
{
  if(!OpenStreamPlayer(m_CurrentSubtitle, hint, true))
    return false;

  return true;
}

bool CVideoPlayer::AdaptForcedSubtitles()
{
  bool valid = false;
  SelectionStream ss = m_SelectionStreams.Get(STREAM_SUBTITLE, GetSubtitle());
  if (ss.flags & CDemuxStream::FLAG_FORCED || !GetSubtitleVisible())
  {
    SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
    SelectionStreams streams = m_SelectionStreams.Get(STREAM_SUBTITLE);

    for(SelectionStreams::iterator it = streams.begin(); it != streams.end() && !valid; ++it)
    {
      if (it->flags & CDemuxStream::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(it->language, as.language))
      {
        if(OpenStream(m_CurrentSubtitle, it->id, it->source))
        {
          valid = true;
          SetSubtitleVisibleInternal(true);
        }
      }
    }
    if(!valid)
    {
      CloseStream(m_CurrentSubtitle, true);
      SetSubtitleVisibleInternal(false);
    }
  }
  return valid;
}

bool CVideoPlayer::OpenTeletextStream(CDVDStreamInfo& hint)
{
  if (!m_VideoPlayerTeletext->CheckStream(hint))
    return false;

  if(!OpenStreamPlayer(m_CurrentTeletext, hint, true))
    return false;

  return true;
}

bool CVideoPlayer::OpenRadioRDSStream(CDVDStreamInfo& hint)
{
  if (!m_VideoPlayerRadioRDS->CheckStream(hint))
    return false;

  if(!OpenStreamPlayer(m_CurrentRadioRDS, hint, true))
    return false;

  return true;
}

bool CVideoPlayer::CloseStream(CCurrentStream& current, bool bWaitForBuffers)
{
  if (current.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing stream player %d", current.player);

  if(bWaitForBuffers)
    SetCaching(CACHESTATE_DONE);

  IDVDStreamPlayer* player = GetStreamPlayer(current.player);
  if(player)
    player->CloseStream(bWaitForBuffers);

  current.Clear();
  return true;
}

void CVideoPlayer::FlushBuffers(bool queued, double pts, bool accurate, bool sync)
{
  CLog::Log(LOGDEBUG, "CVideoPlayer::FlushBuffers - flushing buffers");

  double startpts;
  if (accurate && !m_omxplayer_mode)
    startpts = pts;
  else
    startpts = DVD_NOPTS_VALUE;

  /* call with demuxer pts */
  if(startpts != DVD_NOPTS_VALUE)
    startpts -= m_offset_pts;

  if (sync)
  {
    m_CurrentAudio.inited = false;
    m_CurrentVideo.inited = false;
    m_CurrentSubtitle.inited = false;
    m_CurrentTeletext.inited = false;
  }

  m_CurrentAudio.dts         = DVD_NOPTS_VALUE;
  m_CurrentAudio.startpts    = startpts;

  m_CurrentVideo.dts         = DVD_NOPTS_VALUE;
  m_CurrentVideo.startpts    = startpts;

  m_CurrentSubtitle.dts      = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.startpts = startpts;

  m_CurrentTeletext.dts      = DVD_NOPTS_VALUE;
  m_CurrentTeletext.startpts = startpts;

  m_CurrentRadioRDS.inited   = false;
  m_CurrentRadioRDS.dts      = DVD_NOPTS_VALUE;
  m_CurrentRadioRDS.startpts = startpts;

  if (queued)
  {
    m_VideoPlayerAudio->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
    m_VideoPlayerSubtitle->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_VideoPlayerTeletext->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
    m_VideoPlayerRadioRDS->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));

    SynchronizePlayers(SYNCSOURCE_ALL);
  }
  else
  {
    m_VideoPlayerAudio->Flush(sync);
    m_VideoPlayerVideo->Flush(sync);
    m_VideoPlayerSubtitle->Flush();
    m_VideoPlayerTeletext->Flush();
    m_VideoPlayerRadioRDS->Flush();

    // clear subtitle and menu overlays
    m_overlayContainer.Clear();

    if(m_playSpeed == DVD_PLAYSPEED_NORMAL
    || m_playSpeed == DVD_PLAYSPEED_PAUSE)
    {
      // make sure players are properly flushed, should put them in stalled state
      CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(1000, 0);
      m_VideoPlayerAudio->SendMessage(msg->Acquire(), 1);
      m_VideoPlayerVideo->SendMessage(msg->Acquire(), 1);
      msg->Wait(&m_bStop, 0);
      msg->Release();

      // purge any pending PLAYER_STARTED messages
      m_messenger.Flush(CDVDMsg::PLAYER_STARTED);

      // we should now wait for init cache
      SetCaching(CACHESTATE_FLUSH);
      if (sync)
      {
        m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_STARTING;
        m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_STARTING;
      }
    }

    if(pts != DVD_NOPTS_VALUE && sync)
      m_clock.Discontinuity(pts);
    UpdatePlayState(0);

    // update state, buffers are flushed and it may take some time until
    // we get an update from players
    CSingleLock lock(m_StateSection);
    m_State = m_StateInput;
  }

  if (m_omxplayer_mode)
  {
    m_OmxPlayerState.av_clock.OMXFlush();
    if (!queued)
      m_OmxPlayerState.av_clock.OMXStop();
    m_OmxPlayerState.av_clock.OMXPause();
    m_OmxPlayerState.av_clock.OMXMediaTime(0.0);
  }
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CVideoPlayer::OnDVDNavResult(void* pData, int iMessage)
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
      m_VideoPlayerVideo->EnableSubtitle(*(int*)pData ? true: false);
    else if(iMessage == 5)
    {
      if (m_dvd.state != DVDSTATE_STILL)
      {
        // else notify the player we have received a still frame

        m_dvd.iDVDStillTime      = *(int*)pData;
        m_dvd.iDVDStillStartTime = XbmcThreads::SystemClockMillis();

        /* adjust for the output delay in the video queue */
        unsigned int time = 0;
        if( m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0 )
        {
          time = (unsigned int)(m_VideoPlayerVideo->GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
          if( time < 10000 && time > 0 )
            m_dvd.iDVDStillTime += time;
        }
        m_dvd.state = DVDSTATE_STILL;
        CLog::Log(LOGDEBUG,
                  "DVDNAV_STILL_FRAME - waiting %i sec, with delay of %d sec",
                  m_dvd.iDVDStillTime, time / 1000);
      }
    }
    else if (iMessage == 6)
    {
      m_dvd.state = DVDSTATE_NORMAL;
      CLog::Log(LOGDEBUG, "CVideoPlayer::OnDVDNavResult - libbluray read error (DVDSTATE_NORMAL)");
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25008), g_localizeStrings.Get(25009));
    }

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
          unsigned int time = 0;
          if( m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0 )
          {
            time = (unsigned int)(m_VideoPlayerVideo->GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
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
        m_VideoPlayerSubtitle->SendMessage(new CDVDMsgSubtitleClutChange((uint8_t*)pData));
      }
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      {
        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        bool visible = !(iStream & 0x80);

        SetSubtitleVisibleInternal(visible);

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
        m_VideoPlayerSubtitle->UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
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
        if( m_VideoPlayerVideo->IsInited() )
          m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::VIDEO_SET_ASPECT, m_CurrentVideo.hint.aspect));

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

        if( m_VideoPlayerVideo->IsInited() )
          m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
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
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(16026), g_localizeStrings.Get(16029));
      }
      break;
    default:
    {}
      break;
    }
  }
  return NAVRESULT_NOP;
}

bool CVideoPlayer::ShowPVRChannelInfo(void)
{
  bool bReturn(false);

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO) > 0)
  {
    g_PVRManager.ShowPlayerInfo(CSettings::GetInstance().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO));

    bReturn = true;
  }

  return bReturn;
}

bool CVideoPlayer::OnAction(const CAction &action)
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
        if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
        {
          SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
          m_callback.OnPlayBackResumed();
        }
        // send a message to everyone that we've gone to the menu
        CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
        g_windowManager.SendThreadMessage(msg);
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
          CRect rs, rd, rv;
          m_renderManager.GetVideoRect(rs, rd, rv);
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
          {
            if (pMenus->OnMouseClick(pt))
              return true;
            else
            {
              CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_TRIGGER_OSD)));
              return false;
            }
          }
          return pMenus->OnMouseMove(pt);
        }
        break;
      case ACTION_SELECT_ITEM:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - button select");
          // show button pushed overlay
          if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
            m_VideoPlayerSubtitle->UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_CLICKED);

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
      case ACTION_CHANNEL_UP:
      {
        bool bPreview(action.GetID() == ACTION_MOVE_UP && // only up/down shows a preview, all others do switch
                      (CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH) ||
                       CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT) > 0));

        if (bPreview)
          m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT));
        else
          m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_NEXT));

        if (!bPreview || g_infoManager.IsPlayerOSDActive())
          g_infoManager.SetDisplayAfterSeek();

        ShowPVRChannelInfo();
        return true;
      }

      case ACTION_MOVE_DOWN:
      case ACTION_PREV_ITEM:
      case ACTION_CHANNEL_DOWN:
      {
        bool bPreview(action.GetID() == ACTION_MOVE_DOWN && // only up/down shows a preview, all others do switch
                      (CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH) ||
                       CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT) > 0));

        if (bPreview)
          m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV));
        else
          m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_PREV));

        if (!bPreview || g_infoManager.IsPlayerOSDActive())
          g_infoManager.SetDisplayAfterSeek();

        ShowPVRChannelInfo();
        return true;
      }

      case ACTION_CHANNEL_SWITCH:
      {
        // Offset from key codes back to button number
        int channel = (int) action.GetAmount();
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
      if (GetChapter() > 0 && GetChapter() < GetChapterCount())
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter() + 1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
      if (GetChapter() > 0)
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter() - 1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CVideoPlayer::IsInMenu() const
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

bool CVideoPlayer::HasMenu()
{
  CDVDInputStream::IMenus* pStream = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if (pStream)
    return pStream->HasMenu();
  else
    return false;
}

std::string CVideoPlayer::GetPlayerState()
{
  CSingleLock lock(m_StateSection);
  return m_State.player_state;
}

bool CVideoPlayer::SetPlayerState(const std::string& state)
{
  m_messenger.Put(new CDVDMsgPlayerSetState(state));
  return true;
}

int CVideoPlayer::GetChapterCount()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapters.size();
}

int CVideoPlayer::GetChapter()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapter;
}

void CVideoPlayer::GetChapterName(std::string& strChapterName, int chapterIdx)
{
  CSingleLock lock(m_StateSection);
  if (chapterIdx == -1 && m_State.chapter > 0 && m_State.chapter <= (int) m_State.chapters.size())
    strChapterName = m_State.chapters[m_State.chapter - 1].first;
  else if (chapterIdx > 0 && chapterIdx <= (int) m_State.chapters.size())
    strChapterName = m_State.chapters[chapterIdx - 1].first;
}

int CVideoPlayer::SeekChapter(int iChapter)
{
  if (GetChapter() > 0)
  {
    if (iChapter < 0)
      iChapter = 0;
    if (iChapter > GetChapterCount())
      return 0;

    // Seek to the chapter.
    m_messenger.Put(new CDVDMsgPlayerSeekChapter(iChapter));
    SynchronizeDemuxer(100);
  }

  return 0;
}

int64_t CVideoPlayer::GetChapterPos(int chapterIdx)
{
  CSingleLock lock(m_StateSection);
  if (chapterIdx > 0 && chapterIdx <= (int) m_StateInput.chapters.size())
    return m_State.chapters[chapterIdx - 1].second;

  return -1;
}

void CVideoPlayer::AddSubtitle(const std::string& strSubPath)
{
  m_messenger.Put(new CDVDMsgType<std::string>(CDVDMsg::SUBTITLE_ADDFILE, strSubPath));
}

int CVideoPlayer::GetCacheLevel() const
{
  CSingleLock lock(m_StateSection);
  return (int)(m_StateInput.cache_level * 100);
}

double CVideoPlayer::GetQueueTime()
{
  int a = m_VideoPlayerAudio->GetLevel();
  int v = m_VideoPlayerVideo->GetLevel();
  return std::max(a, v) * 8000.0 / 100;
}

void CVideoPlayer::GetVideoStreamInfo(SPlayerVideoStreamInfo &info)
{
  info.bitrate = m_VideoPlayerVideo->GetVideoBitrate();

  std::string retVal;
  if (m_pDemuxer && (m_CurrentVideo.id != -1))
  {
    m_pDemuxer->GetStreamCodecName(m_CurrentVideo.id, retVal);
    CDemuxStreamVideo* stream = static_cast<CDemuxStreamVideo*>(m_pDemuxer->GetStream(m_CurrentVideo.id));
    if (stream)
    {
      info.width  = stream->iWidth;
      info.height = stream->iHeight;
    }
  }
  info.videoCodecName = retVal;
  info.videoAspectRatio = m_renderManager.GetAspectRatio();
  CRect viewRect;
  m_renderManager.GetVideoRect(info.SrcRect, info.DestRect, viewRect);
  info.stereoMode = m_VideoPlayerVideo->GetStereoMode();
  if (info.stereoMode == "mono")
    info.stereoMode = "";
}

int CVideoPlayer::GetSourceBitrate()
{
  if (m_pInputStream)
    return (int)m_pInputStream->GetBitstreamStats().GetBitrate();

  return 0;
}

void CVideoPlayer::GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info)
{
  if (index == CURRENT_STREAM)
    index = GetAudioStream();

  if (index < 0 || index > GetAudioStreamCount() - 1 )
    return;

  if (index == GetAudioStream())
  {
    info.bitrate = m_VideoPlayerAudio->GetAudioBitrate();
    info.channels = m_VideoPlayerAudio->GetAudioChannels();
  }
  else if (m_pDemuxer)
  {
    CDemuxStreamAudio* stream = m_pDemuxer->GetStreamFromAudioId(index);
    if (stream)
    {
      info.bitrate = stream->iBitRate;
      info.channels = stream->iChannels;
    }
  }

  SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, index);
  if(s.language.length() > 0)
    info.language = s.language;

  if(s.name.length() > 0)
    info.name = s.name;

  if(s.type == STREAM_NONE)
    info.name += " (Invalid)";

  if (m_pDemuxer)
  {
    CDemuxStreamAudio* stream = static_cast<CDemuxStreamAudio*>(m_pDemuxer->GetStreamFromAudioId(index));
    if (stream)
    {
      std::string codecName;
      m_pDemuxer->GetStreamCodecName(stream->iId, codecName);
      info.audioCodecName = codecName;
    }
  }
}

int CVideoPlayer::AddSubtitleFile(const std::string& filename, const std::string& subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if (ext == ".idx")
  {
    if (vobsubfile.empty()) {
      // find corresponding .sub (e.g. in case of manually selected .idx sub)
      vobsubfile = CUtil::GetVobSubSubFromIdx(filename);
      if (vobsubfile.empty())
        return -1;
    }

    CDVDDemuxVobsub v;
    if (!v.Open(filename, STREAM_SOURCE_NONE, vobsubfile))
      return -1;
    m_SelectionStreams.Update(NULL, &v, vobsubfile);

    ExternalStreamInfo info;
    CUtil::GetExternalStreamDetailsFromFilename(m_filename, vobsubfile, info);

    for (int i = 0; i < v.GetNrOfSubtitleStreams(); ++i)
    {
      int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename), i);
      SelectionStream& stream = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

      if (stream.name.empty())
        stream.name = info.name;

      if (stream.language.empty())
        stream.language = info.language;

      if (static_cast<CDemuxStream::EFlags>(info.flag) != CDemuxStream::FLAG_NONE)
        stream.flags = static_cast<CDemuxStream::EFlags>(info.flag);
    }

    return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename), 0);
  }
  if(ext == ".sub")
  {
    // if this looks like vobsub file (i.e. .idx found), add it as such
    std::string vobsubidx = CUtil::GetVobSubIdxFromSub(filename);
    if (!vobsubidx.empty())
      return AddSubtitleFile(vobsubidx, filename);
  }
  SelectionStream s;
  s.source   = m_SelectionStreams.Source(STREAM_SOURCE_TEXT, filename);
  s.type     = STREAM_SUBTITLE;
  s.id       = 0;
  s.filename = filename;
  ExternalStreamInfo info;
  CUtil::GetExternalStreamDetailsFromFilename(m_filename, filename, info);
  s.name = info.name;
  s.language = info.language;
  if (static_cast<CDemuxStream::EFlags>(info.flag) != CDemuxStream::FLAG_NONE)
    s.flags = static_cast<CDemuxStream::EFlags>(info.flag);

  m_SelectionStreams.Update(s);
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, s.source, s.id);
}

void CVideoPlayer::UpdatePlayState(double timeout)
{
  if(m_StateInput.timestamp != 0
  && m_StateInput.timestamp + DVD_MSEC_TO_TIME(timeout) > CDVDClock::GetAbsoluteClock())
    return;

  SPlayerState state(m_StateInput);

  if     (m_CurrentVideo.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentVideo.dts;
  else if(m_CurrentAudio.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentAudio.dts;
  else if(m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentVideo.startpts;
  else if(m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentAudio.startpts;


  if(m_pDemuxer)
  {
    if (IsInMenu())
      state.chapter = 0;
    else
      state.chapter       = m_pDemuxer->GetChapter();

    state.chapters.clear();
    if (m_pDemuxer->GetChapterCount() > 0)
    {
      for (int i = 0; i < m_pDemuxer->GetChapterCount(); ++i)
      {
        std::string name;
        m_pDemuxer->GetChapterName(name, i + 1);
        state.chapters.push_back(make_pair(name, m_pDemuxer->GetChapterPos(i + 1)));
      }
    }

    if(state.dts == DVD_NOPTS_VALUE)
      state.time     = 0;
    else
      state.time     = DVD_TIME_TO_MSEC(state.dts + m_offset_pts);
    state.time_total = m_pDemuxer->GetStreamLength();
    state.time_src   = ETIMESOURCE_CLOCK;
  }

  state.canpause     = true;
  state.canseek      = true;

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
      state.time_src   = ETIMESOURCE_INPUT;
    }

    if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
    {
      if(!ptr->GetState(state.player_state))
        state.player_state = "";

      if(m_dvd.state == DVDSTATE_STILL)
      {
        state.time       = XbmcThreads::SystemClockMillis() - m_dvd.iDVDStillStartTime;
        state.time_total = m_dvd.iDVDStillTime;
        state.time_src   = ETIMESOURCE_MENU;
      }
    }

    if (CDVDInputStream::ISeekable* ptr = dynamic_cast<CDVDInputStream::ISeekable*>(m_pInputStream))
    {
      state.canpause = ptr->CanPause();
      state.canseek  = ptr->CanSeek();
    }
  }

  if (m_Edl.HasCut())
  {
    state.time        = (double) m_Edl.RemoveCutTime(llrint(state.time));
    state.time_total  = (double) m_Edl.RemoveCutTime(llrint(state.time_total));
  }

  state.disptime = state.time;
  if (m_CurrentVideo.id >= 0 && state.time_src == ETIMESOURCE_CLOCK)
  {
    double pts = m_VideoPlayerVideo->GetCurrentPts();
    if (pts != DVD_NOPTS_VALUE)
    {
      state.disptime = DVD_TIME_TO_MSEC(pts + m_offset_pts);
    }
  }

  if(state.time_total <= 0)
    state.canseek  = false;

  if (state.time_src == ETIMESOURCE_CLOCK)
    state.time_offset = m_offset_pts;
  else if (state.dts != DVD_NOPTS_VALUE)
    state.time_offset = DVD_MSEC_TO_TIME(state.time) - state.dts;

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
    state.cache_delay  = std::max(0.0, delay);
    state.cache_level  = std::max(0.0, std::min(1.0, level));
    state.cache_offset = offset;
  }
  else
  {
    state.cache_delay  = 0.0;
    state.cache_level  = std::min(1.0, GetQueueTime() / 8000.0);
    state.cache_offset = GetQueueTime() / state.time_total;
  }

  XFILE::SCacheStatus status;
  if(m_pInputStream && m_pInputStream->GetCacheStatus(&status))
  {
    state.cache_bytes = status.forward;
    if(state.time_total)
      state.cache_bytes += m_pInputStream->GetLength() * (int64_t) (GetQueueTime() / state.time_total);
  }
  else
    state.cache_bytes = 0;

  state.timestamp = CDVDClock::GetAbsoluteClock();

  CSingleLock lock(m_StateSection);
  m_StateInput = state;
}

void CVideoPlayer::UpdateApplication(double timeout)
{
  if(m_UpdateApplication != 0
  && m_UpdateApplication + DVD_MSEC_TO_TIME(timeout) > CDVDClock::GetAbsoluteClock())
    return;

  CDVDInputStream::IChannel* pStream = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
  if(pStream)
  {
    CFileItem item(g_application.CurrentFileItem());
    if(pStream->UpdateItem(item))
    {
      g_application.CurrentFileItem() = item;
      CApplicationMessenger::GetInstance().PostMsg(TMSG_UPDATE_CURRENT_ITEM, 0, -1, static_cast<void*>(new CFileItem(item)));
    }
  }
  m_UpdateApplication = CDVDClock::GetAbsoluteClock();
}

bool CVideoPlayer::CanRecord()
{
  CSingleLock lock(m_StateSection);
  return m_State.canrecord;
}

bool CVideoPlayer::IsRecording()
{
  CSingleLock lock(m_StateSection);
  return m_State.recording;
}

bool CVideoPlayer::Record(bool bOnOff)
{
  if (m_pInputStream && (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV) ||
                         m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER)) )
  {
    m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_RECORD, bOnOff));
    return true;
  }
  return false;
}

bool CVideoPlayer::GetStreamDetails(CStreamDetails &details)
{
  if (m_pDemuxer)
  {
    std::vector<SelectionStream> subs = m_SelectionStreams.Get(STREAM_SUBTITLE);
    std::vector<CStreamDetailSubtitle> extSubDetails;
    for (unsigned int i = 0; i < subs.size(); i++)
    {
      if (subs[i].filename == m_filename)
        continue;

      CStreamDetailSubtitle p;
      p.m_strLanguage = subs[i].language;
      extSubDetails.push_back(p);
    }
    
    bool result = CDVDFileInfo::DemuxerToStreamDetails(m_pInputStream, m_pDemuxer, extSubDetails, details);
    if (result && details.GetStreamCount(CStreamDetail::VIDEO) > 0) // this is more correct (dvds in particular)
    {
      /* 
       * We can only obtain the aspect & duration from VideoPlayer when the Process() thread is running
       * and UpdatePlayState() has been called at least once. In this case VideoPlayer duration/AR will
       * return 0 and we'll have to fallback to the (less accurate) info from the demuxer.
       */
      float aspect = m_renderManager.GetAspectRatio();
      if (aspect > 0.0f)
        ((CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO,0))->m_fAspect = aspect;

      int64_t duration = GetTotalTime() / 1000;
      if (duration > 0)
        ((CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO,0))->m_iDuration = (int) duration;
    }
    return result;
  }
  else
    return false;
}

std::string CVideoPlayer::GetPlayingTitle()
{
  /* Currently we support only Title Name from Teletext line 30 */
  TextCacheStruct_t* ttcache = m_VideoPlayerTeletext->GetTeletextCache();
  if (ttcache && !ttcache->line30.empty())
    return ttcache->line30;

  return "";
}

bool CVideoPlayer::SwitchChannel(const CPVRChannelPtr &channel)
{
  if (g_PVRManager.IsPlayingChannel(channel))
    return false; // desired channel already active, nothing to do.

  if (!g_PVRManager.CheckParentalLock(channel))
    return false;

  /* set GUI info */
  if (!g_PVRManager.PerformChannelSwitch(channel, true))
    return false;

  UpdateApplication(0);
  UpdatePlayState(0);

  /* select the new channel */
  m_messenger.Put(new CDVDMsgType<CPVRChannelPtr>(CDVDMsg::PLAYER_CHANNEL_SELECT, channel));

  return true;
}

bool CVideoPlayer::CachePVRStream(void) const
{
  return m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) &&
         !g_PVRManager.IsPlayingRecording();
}

void CVideoPlayer::FrameMove()
{
  m_renderManager.FrameMove();
  m_renderManager.UpdateResolution();
  m_renderManager.ManageCaptures();
}

void CVideoPlayer::FrameWait(int ms)
{
  m_renderManager.FrameWait(ms);
}

bool CVideoPlayer::HasFrame()
{
  return m_renderManager.HasFrame();
}

void CVideoPlayer::Render(bool clear, uint32_t alpha, bool gui)
{
  m_renderManager.Render(clear, 0, alpha, gui);
}

void CVideoPlayer::AfterRender()
{
  m_renderManager.FrameFinish();
}

void CVideoPlayer::FlushRenderer()
{
  m_renderManager.Flush();
}

void CVideoPlayer::SetRenderViewMode(int mode)
{
  m_renderManager.SetViewMode(mode);
}

float CVideoPlayer::GetRenderAspectRatio()
{
  return m_renderManager.GetAspectRatio();
}

RESOLUTION CVideoPlayer::GetRenderResolution()
{
  return g_graphicsContext.GetVideoResolution();
}

bool CVideoPlayer::IsRenderingVideo()
{
  return m_renderManager.IsConfigured();
}

bool CVideoPlayer::IsRenderingGuiLayer()
{
  return m_renderManager.IsGuiLayer();
}

bool CVideoPlayer::IsRenderingVideoLayer()
{
  return m_renderManager.IsVideoLayer();
}

bool CVideoPlayer::Supports(EDEINTERLACEMODE mode)
{
  return m_renderManager.Supports(mode);
}

bool CVideoPlayer::Supports(EINTERLACEMETHOD method)
{
  return m_renderManager.Supports(method);
}

bool CVideoPlayer::Supports(ESCALINGMETHOD method)
{
  return m_renderManager.Supports(method);
}

bool CVideoPlayer::Supports(ERENDERFEATURE feature)
{
  return m_renderManager.Supports(feature);
}

CRenderCapture *CVideoPlayer::RenderCaptureAlloc()
{
  return m_renderManager.AllocRenderCapture();
}

void CVideoPlayer::RenderCapture(CRenderCapture* capture, unsigned int width, unsigned int height, int flags)
{
  m_renderManager.Capture(capture, width, height, flags);
}

void CVideoPlayer::RenderCaptureRelease(CRenderCapture* capture)
{
  m_renderManager.ReleaseRenderCapture(capture);
}

std::string CVideoPlayer::GetRenderVSyncState()
{
  return m_renderManager.GetVSyncState();
}

// IDispResource interface
void CVideoPlayer::OnLostDisplay()
{
  CLog::Log(LOGNOTICE, "VideoPlayer: OnLostDisplay received");
  CSingleLock lock(m_StateSection);
  m_displayLost = true;
}

void CVideoPlayer::OnResetDisplay()
{
  CLog::Log(LOGNOTICE, "VideoPlayer: OnResetDisplay received");
  CSingleLock lock(m_StateSection);
  m_displayLost = false;
}
