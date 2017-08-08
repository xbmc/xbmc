/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoPlayer.h"
#include "VideoPlayerRadioRDS.h"
#include "system.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#if defined(HAVE_LIBBLURAY)
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
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
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"

#include "DVDDemuxers/DVDDemuxCC.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
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

#ifdef TARGET_RASPBERRY_PI
#include "cores/omxplayer/OMXPlayerAudio.h"
#include "cores/omxplayer/OMXPlayerVideo.h"
#include "cores/omxplayer/OMXHelper.h"
#endif
#include "VideoPlayerAudio.h"
#include "windowing/WindowingFactory.h"
#include "DVDCodecs/DVDCodecUtils.h"

#include <iterator>

using namespace PVR;
using namespace KODI::MESSAGING;

void CSelectionStreams::Clear(StreamType type, StreamSource source)
{
  CSingleLock lock(m_section);
  auto new_end = std::remove_if(m_Streams.begin(), m_Streams.end(),
    [type, source](const SelectionStream &stream)
    {
      return (type == STREAM_NONE || stream.type == type) &&
      (source == 0 || stream.source == source);
    });
  m_Streams.erase(new_end, m_Streams.end());
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
  return m_invalid;
}

std::vector<SelectionStream> CSelectionStreams::Get(StreamType type)
{
  std::vector<SelectionStream> streams;
  std::copy_if(m_Streams.begin(), m_Streams.end(), std::back_inserter(streams),
    [type](const SelectionStream &stream)
    {
      return stream.type == type;
    });
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
  *          - it's a forced sub and "original stream's language" was selected and audio stream language matches, or
  *          - it's a default and a forced sub (could lead to users seeing forced subs in a foreign language!), or
  *          - its language matches the preferred subtitle's language (unequal to "original stream's language")
  */
  PredicateSubtitleFilter(std::string& lang)
    : audiolang(lang),
      original(StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "original")),
      nosub(StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "none")),
      onlyforced(StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "forced_only"))
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

    if ((ss.flags & CDemuxStream::FLAG_FORCED) && g_LangCodeExpander.CompareISO639Codes(ss.language, audiolang))
      return false;

    if ((ss.flags & CDemuxStream::FLAG_FORCED) && (ss.flags & CDemuxStream::FLAG_DEFAULT))
      return false;

    if(!original)
    {
      std::string subtitle_language = g_langInfo.GetSubtitleLanguage();
      if (g_LangCodeExpander.CompareISO639Codes(subtitle_language, ss.language))
        return false;
    }
    else if (ss.flags & CDemuxStream::FLAG_DEFAULT)
      return false;

    return true;
  }
};

static bool PredicateAudioPriority(const SelectionStream& lh, const SelectionStream& rh)
{
  PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream
                 , rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream);

  if(!StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE), "original"))
  {
    std::string audio_language = g_langInfo.GetAudioLanguage();
    PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(audio_language, lh.language)
                   , g_LangCodeExpander.CompareISO639Codes(audio_language, rh.language));

    bool hearingimp = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING);
    PREDICATE_RETURN(!hearingimp ? !(lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED
                   , !hearingimp ? !(rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED);

    bool visualimp = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL);
    PREDICATE_RETURN(!visualimp ? !(lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED
                   , !visualimp ? !(rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED);
  }

  if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG))
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
*          - lh is a forced sub and ("original stream's language" was selected or subtitles are off) and audio stream language matches sub language and rh not, or
*          - lh is a default sub and ("original stream's language" was selected or subtitles are off) and audio stream language matches sub language and rh not, or
*          - lh is a sub where audio stream language matches sub language and (original stream's language" was selected or subtitles are off) and rh not, or
*          - lh is a forced sub and a default sub ("original stream's language" was selected or subtitles are off)
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
      original(StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "original")),
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

      PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT && g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , rh.flags & CDemuxStream::FLAG_DEFAULT && g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN(lh.flags & (CDemuxStream::FLAG_FORCED && CDemuxStream::FLAG_DEFAULT)
                     , rh.flags & (CDemuxStream::FLAG_FORCED && CDemuxStream::FLAG_DEFAULT));

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

      bool hearingimp = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_ACCESSIBILITY_SUBHEARING);
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
  PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VideoStream
                 , rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VideoStream);

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

int CSelectionStreams::IndexOf(StreamType type, int source, int64_t demuxerId, int id) const
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
    if(m_Streams[i].id == id && m_Streams[i].demuxerId == demuxerId)
      return count;
  }
  if(id < 0)
    return count;
  else
    return -1;
}

int CSelectionStreams::IndexOf(StreamType type, const CVideoPlayer& p) const
{
  if (p.m_pInputStream && p.m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    int id = -1;
    if(type == STREAM_AUDIO)
      id = static_cast<CDVDInputStreamNavigator*>(p.m_pInputStream)->GetActiveAudioStream();
    else if (type == STREAM_VIDEO)
      id = static_cast<CDVDInputStreamNavigator*>(p.m_pInputStream)->GetActiveAngle();
    else if(type == STREAM_SUBTITLE)
      id = static_cast<CDVDInputStreamNavigator*>(p.m_pInputStream)->GetActiveSubtitleStream();

    return IndexOf(type, STREAM_SOURCE_NAV, -1, id);
  }

  if(type == STREAM_AUDIO)
    return IndexOf(type, p.m_CurrentAudio.source, p.m_CurrentAudio.demuxerId, p.m_CurrentAudio.id);
  else if(type == STREAM_VIDEO)
    return IndexOf(type, p.m_CurrentVideo.source, p.m_CurrentVideo.demuxerId, p.m_CurrentVideo.id);
  else if(type == STREAM_SUBTITLE)
    return IndexOf(type, p.m_CurrentSubtitle.source, p.m_CurrentSubtitle.demuxerId, p.m_CurrentSubtitle.id);
  else if(type == STREAM_TELETEXT)
    return IndexOf(type, p.m_CurrentTeletext.source, p.m_CurrentTeletext.demuxerId, p.m_CurrentTeletext.id);
  else if(type == STREAM_RADIO_RDS)
    return IndexOf(type, p.m_CurrentRadioRDS.source, p.m_CurrentRadioRDS.demuxerId, p.m_CurrentRadioRDS.id);

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
  int index = IndexOf(s.type, s.source, s.demuxerId, s.id);
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
    CDVDInputStreamNavigator* nav = static_cast<CDVDInputStreamNavigator*>(input);
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

      DVDNavAudioStreamInfo info = nav->GetAudioStreamInfo(i);
      s.name     = info.name;
      s.language = g_LangCodeExpander.ConvertToISO6392B(info.language);
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
      s.filename = filename;
      s.channels = 0;

      DVDNavSubtitleStreamInfo info = nav->GetSubtitleStreamInfo(i);
      s.name     = info.name;
      s.flags = info.flags;
      s.language = g_LangCodeExpander.ConvertToISO6392B(info.language);
      Update(s);
    }

    DVDNavVideoStreamInfo info = nav->GetVideoStreamInfo();
    for (int i = 1; i <= info.angles; i++)
    {
      SelectionStream s;
      s.source = source;
      s.type = STREAM_VIDEO;
      s.id = i;
      s.flags = CDemuxStream::FLAG_NONE;
      s.filename = filename;
      s.channels = 0;
      s.aspect_ratio = info.aspectRatio;
      s.width = (int)info.width;
      s.height = (int)info.height;
      s.codec = info.codec;
      s.name = StringUtils::Format("%s %i", g_localizeStrings.Get(38032).c_str(), i);
      Update(s);
    }
  }
  else if(demuxer)
  {
    std::string filename = demuxer->GetFileName();
    int source;
    if(input) /* hack to know this is sub decoder */
      source = Source(STREAM_SOURCE_DEMUX, filename);
    else if (!filename2.empty())
      source = Source(STREAM_SOURCE_DEMUX_SUB, filename);
    else
      source = Source(STREAM_SOURCE_VIDEOMUX, filename);

    for (auto stream : demuxer->GetStreams())
    {
      /* skip streams with no type */
      if (stream->type == STREAM_NONE)
        continue;
      /* make sure stream is marked with right source */
      stream->source = source;

      SelectionStream s;
      s.source   = source;
      s.type     = stream->type;
      s.id       = stream->uniqueId;
      s.demuxerId = stream->demuxerId;
      s.language = g_LangCodeExpander.ConvertToISO6392B(stream->language);
      s.flags    = stream->flags;
      s.filename = demuxer->GetFileName();
      s.filename2 = filename2;
      s.name = stream->GetStreamName();
      s.codec    = demuxer->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      s.channels = 0; // Default to 0. Overwrite if STREAM_AUDIO below.
      if(stream->type == STREAM_VIDEO)
      {
        CDemuxStreamVideo* vstream = static_cast<CDemuxStreamVideo*>(stream);
        s.width = vstream->iWidth;
        s.height = vstream->iHeight;
        s.stereo_mode = vstream->stereo_mode;
        s.bitrate = vstream->iBitRate;
      }
      if(stream->type == STREAM_AUDIO)
      {
        std::string type;
        type = static_cast<CDemuxStreamAudio*>(stream)->GetStreamType();
        if(type.length() > 0)
        {
          if(s.name.length() > 0)
            s.name += " - ";
          s.name += type;
        }
        s.channels = static_cast<CDemuxStreamAudio*>(stream)->iChannels;
        s.bitrate = static_cast<CDemuxStreamAudio*>(stream)->iBitRate;
      }
      Update(s);
    }
  }
  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
  CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
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
  }
  return count;
}

void CVideoPlayer::CreatePlayers()
{
#ifdef TARGET_RASPBERRY_PI
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
#ifdef TARGET_RASPBERRY_PI
    m_VideoPlayerVideo = new OMXPlayerVideo(&m_OmxPlayerState.av_clock, &m_overlayContainer, m_messenger, m_renderManager, *m_processInfo);
    m_VideoPlayerAudio = new OMXPlayerAudio(&m_OmxPlayerState.av_clock, m_messenger, *m_processInfo);
#endif
  }
  else
  {
    m_VideoPlayerVideo = new CVideoPlayerVideo(&m_clock, &m_overlayContainer, m_messenger, m_renderManager, *m_processInfo);
    m_VideoPlayerAudio = new CVideoPlayerAudio(&m_clock, m_messenger, *m_processInfo);
  }
  m_VideoPlayerSubtitle = new CVideoPlayerSubtitle(&m_overlayContainer, *m_processInfo);
  m_VideoPlayerTeletext = new CDVDTeletextData(*m_processInfo);
  m_VideoPlayerRadioRDS = new CDVDRadioRDSData(*m_processInfo);
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
      m_renderManager(m_clock, this)
{
  m_players_created = false;
  m_pDemuxer = NULL;
  m_pSubtitleDemuxer = NULL;
  m_pCCDemuxer = NULL;
  m_pInputStream = NULL;

  m_dvd.Clear();
  m_State.Clear();

  m_bAbortRequest = false;
  m_errorCount = 0;
  m_offset_pts = 0.0;
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_streamPlayerSpeed = DVD_PLAYSPEED_NORMAL;
  m_canTempo = false;
  m_caching = CACHESTATE_DONE;
  m_HasVideo = false;
  m_HasAudio = false;

  memset(&m_SpeedState, 0, sizeof(m_SpeedState));

  // omxplayer variables
  m_OmxPlayerState.last_check_time     = 0;
  m_OmxPlayerState.stamp               = 0.0;
  m_OmxPlayerState.bOmxWaitVideo       = false;
  m_OmxPlayerState.bOmxWaitAudio       = false;
  m_OmxPlayerState.bOmxSentEOFs        = false;
  m_OmxPlayerState.threshold           = 0.2f;
  m_OmxPlayerState.interlace_method    = VS_INTERLACEMETHOD_MAX;
#ifdef TARGET_RASPBERRY_PI
  m_omxplayer_mode                     = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEOMXPLAYER);
#else
  m_omxplayer_mode                     = false;
#endif

  m_SkipCommercials = true;

  m_processInfo.reset(CProcessInfo::CreateInstance());
  // if we have a gui, register the cache
  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
  m_processInfo->SetSpeed(1.0);
  m_processInfo->SetTempo(1.0);

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
  if (IsRunning())
  {
    m_openEvent.Reset();
    CDVDMsgOpenFile::FileParams params;
    params.m_item = file;
    params.m_options = options;
    params.m_item.SetMimeTypeForInternetFile();
    m_messenger.Put(new CDVDMsgOpenFile(params), 1);

    if (m_openEvent.WaitMSec(2000))
    {
      if (!m_bStop && !m_bAbortRequest)
        return true;
    }
    CloseFile();
  }

  m_playerOptions = options;
  m_item = file;
  // Try to resolve the correct mime type
  m_item.SetMimeTypeForInternetFile();

  m_bAbortRequest = false;
  m_renderManager.PreInit();

  m_openEvent.Reset();
  Create();
  CGUIDialogBusy::WaitOnEvent(m_openEvent, g_advancedSettings.m_videoBusyDialogDelay_ms, false);

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

  m_HasVideo = false;
  m_HasAudio = false;
  m_canTempo = false;

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
  std::string filename(m_item.GetPath());
  if (URIUtils::IsProtocol(filename, "dvd") ||
      StringUtils::EqualsNoCase(filename, "iso9660://video_ts/video_ts.ifo"))
  {
    m_item.SetPath(g_mediaManager.TranslateDevicePath(""));
  }

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_item, true);
  if(m_pInputStream == NULL)
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - unable to create input stream for [%s]", CURL::GetRedacted(m_item.GetPath()).c_str());
    return false;
  }

  if (!m_pInputStream->Open())
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - error opening [%s]", CURL::GetRedacted(m_item.GetPath()).c_str());
    return false;
  }

  // find any available external subtitles for non dvd files
  if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
  {
    // find any available external subtitles
    std::vector<std::string> filenames;
    CUtil::ScanForExternalSubtitles(m_item.GetPath(), filenames);

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

  if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK) &&
      !m_pInputStream->IsRealtime())
  {
    m_canTempo = true;
  }
  else
  {
    m_canTempo = false;
  }

  return true;
}

bool CVideoPlayer::OpenDemuxStream()
{
  CloseDemuxer();

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

  m_offset_pts = 0;

  return true;
}

void CVideoPlayer::CloseDemuxer()
{
  delete m_pDemuxer;
  m_pDemuxer = nullptr;
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);

  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
  CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
}

void CVideoPlayer::OpenDefaultStreams(bool reset)
{
  // if input stream dictate, we will open later
  if(m_dvd.iSelectedAudioStream >= 0
  || m_dvd.iSelectedSPUStream   >= 0)
    return;

  bool valid;

  // open video stream
  valid   = false;
  
  for (const auto &stream : m_SelectionStreams.Get(STREAM_VIDEO, PredicateVideoPriority))
  {
    if(OpenStream(m_CurrentVideo, stream.demuxerId, stream.id, stream.source, reset))
    {
      valid = true;
      break;
    }
  }
  if(!valid)
  {
    CloseStream(m_CurrentVideo, true);
    m_processInfo->ResetVideoCodecInfo();
  }

  // open audio stream
  valid   = false;
  if (!m_playerOptions.video_only)
  {
    for (const auto &stream : m_SelectionStreams.Get(STREAM_AUDIO, PredicateAudioPriority))
    {
      if(OpenStream(m_CurrentAudio, stream.demuxerId, stream.id, stream.source, reset))
      {
        valid = true;
        break;
      }
    }
  }

  if(!valid)
  {
    CloseStream(m_CurrentAudio, true);
    m_processInfo->ResetAudioCodecInfo();
  }

  // enable  or disable subtitles
  bool visible = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleOn;

  // open subtitle stream
  SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
  PredicateSubtitlePriority psp(as.language);
  valid   = false;
  CloseStream(m_CurrentSubtitle, false);
  for (const auto &stream : m_SelectionStreams.Get(STREAM_SUBTITLE, psp))
  {
    if(OpenStream(m_CurrentSubtitle, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      if(!psp.relevant(stream))
        visible = false;
      else if(stream.flags & CDemuxStream::FLAG_FORCED)
        visible = true;
      break;
    }
  }
  if(!valid)
    CloseStream(m_CurrentSubtitle, false);

  if (!dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream) || m_playerOptions.state.empty())
    SetSubtitleVisibleInternal(visible); // only set subtitle visibility if state not stored by dvd navigator, because navigator will restore it (if visible)

  // open teletext stream
  valid   = false;
  for (const auto &stream : m_SelectionStreams.Get(STREAM_TELETEXT))
  {
    if(OpenStream(m_CurrentTeletext, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      break;
    }
  }
  if(!valid)
    CloseStream(m_CurrentTeletext, false);

  // open RDS stream
  valid   = false;
  for (const auto &stream : m_SelectionStreams.Get(STREAM_RADIO_RDS))
  {
    if(OpenStream(m_CurrentRadioRDS, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      break;
    }
  }
  if(!valid)
    CloseStream(m_CurrentRadioRDS, false);

  // disable demux streams
  if (m_item.IsRemote() && m_pDemuxer)
  {
    for (auto &stream : m_SelectionStreams.m_Streams)
    {
      if (STREAM_SOURCE_MASK(stream.source) == STREAM_SOURCE_DEMUX)
      {
        if (stream.id != m_CurrentVideo.id &&
            stream.id != m_CurrentAudio.id &&
            stream.id != m_CurrentSubtitle.id &&
            stream.id != m_CurrentTeletext.id &&
            stream.id != m_CurrentRadioRDS.id)
        {
          m_pDemuxer->EnableStream(stream.demuxerId, stream.id, false);
        }
      }
    }
  }
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

      stream = m_pSubtitleDemuxer->GetStream(packet->demuxerId, packet->iStreamId);
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
      stream = m_pDemuxer->GetStream(packet->demuxerId, packet->iStreamId);
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
    CDemuxStream* st = m_pSubtitleDemuxer->GetStream(stream.demuxerId, stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;
    return true;
  }
  if(source == STREAM_SOURCE_DEMUX)
  {
    CDemuxStream* st = m_pDemuxer->GetStream(stream.demuxerId, stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;

    if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if(stream.type == STREAM_AUDIO    && st->dvdNavId != m_dvd.iSelectedAudioStream)
        return false;
      if(stream.type == STREAM_SUBTITLE && st->dvdNavId != m_dvd.iSelectedSPUStream)
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
  if (m_playerOptions.video_only && current.type != STREAM_VIDEO)
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
    if(source_type  != STREAM_SOURCE_DEMUX ||
       stream->type != current.type ||
       stream->uniqueId  == current.id)
      return false;

    if(current.type == STREAM_AUDIO && stream->dvdNavId == m_dvd.iSelectedAudioStream)
      return true;
    if(current.type == STREAM_SUBTITLE && stream->dvdNavId == m_dvd.iSelectedSPUStream)
      return true;
    if(current.type == STREAM_VIDEO    && current.id < 0)
      return true;
  }
  else
  {
    if(stream->source == current.source &&
       stream->uniqueId == current.id &&
       stream->demuxerId == current.demuxerId)
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
    OpenStream(current, stream->demuxerId, stream->uniqueId, stream->source);
}

void CVideoPlayer::Prepare()
{
  CFFmpegLog::SetLogLevel(1);
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
  m_processInfo->SetSpeed(1.0);
  m_processInfo->SetTempo(1.0);
  m_State.Clear();
  memset(&m_SpeedState, 0, sizeof(m_SpeedState));
  m_offset_pts = 0;
  m_CurrentAudio.lastdts = DVD_NOPTS_VALUE;
  m_CurrentVideo.lastdts = DVD_NOPTS_VALUE;

  if (!OpenInputStream())
  {
    m_bAbortRequest = true;
    return;
  }

  if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
  {
    CLog::Log(LOGNOTICE, "VideoPlayer: playing a file with menu's");
    if(dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream))
      m_playerOptions.starttime = 0;

    if (!m_playerOptions.state.empty())
      ptr->SetState(m_playerOptions.state);
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
  m_VideoPlayerVideo->EnableFullscreen(m_playerOptions.fullscreen);

  if (m_omxplayer_mode)
  {
    if (!m_OmxPlayerState.av_clock.OMXInitialize(&m_clock))
      m_bAbortRequest = true;
    if (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
      m_OmxPlayerState.av_clock.HDMIClockSync();
    m_OmxPlayerState.av_clock.OMXStateIdle();
    m_OmxPlayerState.av_clock.OMXStateExecute();
    m_OmxPlayerState.av_clock.OMXStop();
    m_OmxPlayerState.av_clock.OMXPause();
  }

  OpenDefaultStreams();

  // look for any EDL files
  m_Edl.Clear();
  if (m_CurrentVideo.id >= 0 && m_CurrentVideo.hint.fpsrate > 0 && m_CurrentVideo.hint.fpsscale > 0)
  {
    float fFramesPerSecond = (float)m_CurrentVideo.hint.fpsrate / (float)m_CurrentVideo.hint.fpsscale;
    m_Edl.ReadEditDecisionLists(m_item.GetPath(), fFramesPerSecond, m_CurrentVideo.hint.height);
  }

  /*
   * Check to see if the demuxer should start at something other than time 0. This will be the case
   * if there was a start time specified as part of the "Start from where last stopped" (aka
   * auto-resume) feature or if there is an EDL cut or commercial break that starts at time 0.
   */
  CEdl::Cut cut;
  int starttime = 0;
  if (m_playerOptions.starttime > 0 || m_playerOptions.startpercent > 0)
  {
    if (m_playerOptions.startpercent > 0 && m_pDemuxer)
    {
      int playerStartTime = (int)( ( (float) m_pDemuxer->GetStreamLength() ) * ( m_playerOptions.startpercent/(float)100 ) );
      starttime = m_Edl.RestoreCutTime(playerStartTime);
    }
    else
    {
      starttime = m_Edl.RestoreCutTime(static_cast<int>(m_playerOptions.starttime * 1000)); // s to ms
    }
    CLog::Log(LOGDEBUG, "%s - Start position set to last stopped position: %d", __FUNCTION__, starttime);
  }
  else if (m_Edl.InCut(starttime, &cut))
  {
    if (cut.action == CEdl::CUT)
    {
      starttime = cut.end;
      CLog::Log(LOGDEBUG, "%s - Start position set to end of first cut: %d", __FUNCTION__, starttime);
    }
    else if (cut.action == CEdl::COMM_BREAK)
    {
      if (m_SkipCommercials)
      {
        starttime = cut.end;
        CLog::Log(LOGDEBUG, "%s - Start position set to end of first commercial break: %d", __FUNCTION__, starttime);
      }

      std::string strTimeString = StringUtils::SecondsToTimeString(cut.end / 1000, TIME_FORMAT_MM_SS);
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011), strTimeString);
    }
  }

  if (starttime > 0)
  {
    double startpts = DVD_NOPTS_VALUE;
    if (m_pDemuxer)
    {
      if (m_pDemuxer->SeekTime(starttime, true, &startpts))
        CLog::Log(LOGDEBUG, "%s - starting demuxer from: %d", __FUNCTION__, starttime);
      else
        CLog::Log(LOGDEBUG, "%s - failed to start demuxing from: %d", __FUNCTION__, starttime);
    }

    if (m_pSubtitleDemuxer)
    {
      if(m_pSubtitleDemuxer->SeekTime(starttime, true, &startpts))
        CLog::Log(LOGDEBUG, "%s - starting subtitle demuxer from: %d", __FUNCTION__, starttime);
      else
        CLog::Log(LOGDEBUG, "%s - failed to start subtitle demuxing from: %d", __FUNCTION__, starttime);
    }

    m_clock.Discontinuity(DVD_MSEC_TO_TIME(starttime));
  }

  UpdatePlayState(0);

  m_callback.OnPlayBackStarted();

  // we are done initializing now, set the readyevent
  m_openEvent.Set();

  SetCaching(CACHESTATE_FLUSH);
}

void CVideoPlayer::Process()
{
  Prepare();

  while (!m_bAbortRequest)
  {
#ifdef TARGET_RASPBERRY_PI
    if (m_omxplayer_mode && OMXDoProcessing(m_OmxPlayerState, m_playSpeed, m_VideoPlayerVideo, m_VideoPlayerAudio, m_CurrentAudio, m_CurrentVideo, m_HasVideo, m_HasAudio, m_renderManager))
    {
      CloseStream(m_CurrentVideo, false);
      OpenStream(m_CurrentVideo, m_CurrentVideo.demuxerId, m_CurrentVideo.id, m_CurrentVideo.source);
      if (m_State.canseek)
      {
        CDVDMsgPlayerSeek::CMode mode;
        mode.time = (int)GetTime();
        mode.backward = true;
        mode.accurate = true;
        mode.sync = true;
        m_messenger.Put(new CDVDMsgPlayerSeek(mode));
      }
    }
#endif

    // check display lost
    if (m_displayLost)
    {
      Sleep(50);
      continue;
    }

    // check if in a cut or commercial break that should be automatically skipped
    CheckAutoSceneSkip();

    // handle messages send to this thread, like seek or demuxer reset requests
    HandleMessages();

    if (m_bAbortRequest)
      break;

    // should we open a new input stream?
    if (!m_pInputStream)
    {
      if (OpenInputStream() == false)
      {
        m_bAbortRequest = true;
        break;
      }
    }

    // should we open a new demuxer?
    if (!m_pDemuxer)
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

      UpdatePlayState(0);
    }

    // handle eventual seeks due to playspeed
    HandlePlaySpeed();

    // update player state
    UpdatePlayState(200);

    // make sure we run subtitle process here
    m_VideoPlayerSubtitle->Process(m_clock.GetClock() + m_State.time_offset - m_VideoPlayerVideo->GetSubtitleDelay(), m_State.time_offset);

    // if the queues are full, no need to read more
    if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
        (!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
    {
      Sleep(10);
      continue;
    }

    // always yield to players if they have data levels > 50 percent
    if((m_VideoPlayerAudio->GetLevel() > 50 || m_CurrentAudio.id < 0) &&
       (m_processInfo->GetLevelVQ() > 50 || m_CurrentVideo.id < 0))
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
        CloseDemuxer();

        SetCaching(CACHESTATE_DONE);
        CLog::Log(LOGNOTICE, "VideoPlayer: next stream, wait for old streams to be finished");
        CloseStream(m_CurrentAudio, true);
        CloseStream(m_CurrentVideo, true);

        m_CurrentAudio.Clear();
        m_CurrentVideo.Clear();
        m_CurrentSubtitle.Clear();
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

      if (m_CurrentVideo.inited)
      {
        m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::VIDEO_DRAIN));
      }

      { // TODO: only used by OMXPlayer, find better solution
        if (m_CurrentAudio.inited)
          m_VideoPlayerAudio->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
        if (m_CurrentVideo.inited)
          m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      }

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
#ifdef TARGET_RASPBERRY_PI
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
    if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_SUBTITLES_PARSECAPTIONS) && CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
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

    if (IsInMenuInternal())
    {
      if (CDVDInputStream::IMenus* menu = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
      {
        double correction = menu->GetTimeStampCorrection();
        if (pPacket->dts != DVD_NOPTS_VALUE && pPacket->dts > correction)
          pPacket->dts -= correction;
        if (pPacket->pts != DVD_NOPTS_VALUE && pPacket->pts > correction)
          pPacket->pts -= correction;
      }
      if (m_dvd.syncClock)
      {
        m_clock.Discontinuity(pPacket->dts);
        m_dvd.syncClock = false;
      }
    }

    // process the packet
    ProcessPacket(pStream, pPacket);

    // update the player info for streams
    if (m_player_status_timer.IsTimePast())
    {
      m_player_status_timer.Set(500);
      UpdateStreamInfos();
    }
  }
}

bool CVideoPlayer::CheckIsCurrent(CCurrentStream& current, CDemuxStream* stream, DemuxPacket* pkg)
{
  if(current.id     == pkg->iStreamId
  && current.demuxerId == stream->demuxerId
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
    {
      m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
      m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
      OpenDefaultStreams(false);
    }

    current.stream = (void*)stream;
    current.changes = stream->changes;
  }
}

void CVideoPlayer::ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentAudio, pStream);

  bool checkcont = CheckContinuity(m_CurrentAudio, pPacket);
  UpdateTimestamps(m_CurrentAudio, pPacket);

  if (checkcont && (m_CurrentAudio.avsync == CCurrentStream::AV_SYNC_CHECK))
    m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;

  bool drop = false;
  if (CheckPlayerInit(m_CurrentAudio))
    drop = true;

  /*
   * If CheckSceneSkip() returns true then demux point is inside an EDL cut and the packets are dropped.
   */
  CEdl::Cut cut;
  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;
  else if (m_Edl.InCut(DVD_TIME_TO_MSEC(m_CurrentAudio.dts + m_offset_pts), &cut) && cut.action == CEdl::MUTE)
  {
    drop = true;
  }

  m_VideoPlayerAudio->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
  m_CurrentAudio.packets++;
}

void CVideoPlayer::ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentVideo, pStream);
  bool checkcont = false;

  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    checkcont = CheckContinuity(m_CurrentVideo, pPacket);
    UpdateTimestamps(m_CurrentVideo, pPacket);
  }
  if (checkcont && (m_CurrentVideo.avsync == CCurrentStream::AV_SYNC_CHECK))
    m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;

  bool drop = false;
  if (CheckPlayerInit(m_CurrentVideo))
    drop = true;

  if (CheckSceneSkip(m_CurrentVideo))
    drop = true;

  m_VideoPlayerVideo->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
  m_CurrentVideo.packets++;
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
    m_VideoPlayerSubtitle->UpdateOverlayInfo(static_cast<CDVDInputStreamNavigator*>(m_pInputStream), LIBDVDNAV_BUTTON_NORMAL);
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

  uint64_t &cached = status.forward;
  unsigned &currate = status.currate;
  unsigned &maxrate = status.maxrate;
  float &cache_level = status.level;

  int64_t length = m_pInputStream->GetLength();
  int64_t remain = length - m_pInputStream->Seek(0, SEEK_CUR);

  if (length <= 0 || remain < 0)
    return false;

  double play_sbp = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) / length;
  double queued = 1000.0 * GetQueueTime() / play_sbp;

  delay  = 0.0;
  level  = 0.0;
  offset = (double)(cached + queued) / length;

  if (currate == 0)
    return true;

  double cache_sbp = 1.1 * (double)DVD_TIME_BASE / currate;          /* underestimate by 10 % */
  double play_left = play_sbp  * (remain + queued);                  /* time to play out all remaining bytes */
  double cache_left = cache_sbp * (remain - cached);                 /* time to cache the remaining bytes */
  double cache_need = std::max(0.0, remain - play_left / cache_sbp); /* bytes needed until play_left == cache_left */

  delay = cache_left - play_left;

  /* NOTE: We can only reliably test for low readrate, when the cache is not
   * already *near* full. This is because as soon as it's full the average-
   * rate will become approximately the current-rate which can flag false
   * low read-rate conditions. To work around this we don't check the currate at 100%
   * but between 80% and 90%
   */
  if (cache_level > 0.8 && cache_level < 0.9 && currate < maxrate)
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
  bool isInMenu = IsInMenuInternal();

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

    // handle exceptions
    if (m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0)
    {
      if ((!m_VideoPlayerAudio->AcceptsData() || !m_VideoPlayerVideo->AcceptsData()) &&
          m_cachingTimer.IsTimePast())
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
    if (m_playSpeed == DVD_PLAYSPEED_NORMAL && !isInMenu)
    {
      // take action is audio or video stream is stalled
      if (((m_VideoPlayerAudio->IsStalled() && m_CurrentAudio.inited) ||
           (m_VideoPlayerVideo->IsStalled() && m_CurrentVideo.inited)) &&
          m_syncTimer.IsTimePast())
      {
        if (m_pInputStream->IsRealtime())
        {
          if ((m_CurrentAudio.id >= 0 && m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC && m_VideoPlayerAudio->IsStalled()) ||
              (m_CurrentVideo.id >= 0 && m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
               m_processInfo->GetLevelVQ() == 0))
          {
            CLog::Log(LOGDEBUG, "Stream stalled, start buffering. Audio: %d - Video: %d",
                                 m_VideoPlayerAudio->GetLevel(), m_processInfo->GetLevelVQ());
            FlushBuffers(DVD_NOPTS_VALUE, true, true);
          }
        }
        else
        {
          // start caching if audio and video have run dry
          if (m_VideoPlayerAudio->GetLevel() <= 50 &&
              m_processInfo->GetLevelVQ() <= 50)
          {
            SetCaching(CACHESTATE_FULL);
          }
          else if (m_CurrentAudio.id >= 0 && m_CurrentAudio.inited &&
                   m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
                   m_VideoPlayerAudio->GetLevel() == 0)
          {
            CLog::Log(LOGDEBUG,"CVideoPlayer::HandlePlaySpeed - audio stream stalled, triggering re-sync");
            FlushBuffers(DVD_NOPTS_VALUE, true, true);
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = (int)GetUpdatedTime();
            mode.backward = false;
            mode.accurate = true;
            mode.sync = true;
            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
          }
        }
      }
      // care for live streams
      else if (m_pInputStream->IsRealtime())
      {
        if (m_CurrentAudio.id >= 0)
        {
          double adjust = -1.0; // a unique value
          if (m_clock.GetSpeedAdjust() >= 0 && m_VideoPlayerAudio->GetLevel() < 5)
            adjust = -0.05;

          if (m_clock.GetSpeedAdjust() < 0 && m_VideoPlayerAudio->GetLevel() > 10)
            adjust = 0.0;

          if (adjust != -1.0)
          {
            m_clock.SetSpeedAdjust(adjust);
            if (m_omxplayer_mode)
              m_OmxPlayerState.av_clock.OMXSetSpeedAdjust(adjust);
          }
        }
      }
    }
  }

  // sync streams to clock
  if ((m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
      (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC))
  {
    unsigned int threshold = 20;
    if (m_pInputStream->IsRealtime())
      threshold = 40;

    bool video = m_CurrentVideo.id < 0 || (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
                 (m_CurrentVideo.packets == 0 && m_CurrentAudio.packets > threshold);
    bool audio = m_CurrentAudio.id < 0 || (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
                 (m_CurrentAudio.packets == 0 && m_CurrentVideo.packets > threshold);

    if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
        (m_CurrentAudio.avsync == CCurrentStream::AV_SYNC_CONT ||
         m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC))
    {
      m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerAudio->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
    }
    else if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
             m_CurrentVideo.avsync == CCurrentStream::AV_SYNC_CONT)
    {
      m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
    }
    else if (video && audio)
    {
      double clock = 0;
      if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Audio - pts: %f, cache: %f, totalcache: %f",
                             m_CurrentAudio.starttime, m_CurrentAudio.cachetime, m_CurrentAudio.cachetotal);
      if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Video - pts: %f, cache: %f, totalcache: %f",
                             m_CurrentVideo.starttime, m_CurrentVideo.cachetime, m_CurrentVideo.cachetotal);

      if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE && m_CurrentVideo.packets > 0 &&
          m_playSpeed == DVD_PLAYSPEED_PAUSE)
      {
        clock = m_CurrentVideo.starttime;
      }
      else if (m_CurrentAudio.starttime != DVD_NOPTS_VALUE && m_CurrentAudio.packets > 0)
      {
        if (m_pInputStream->IsRealtime())
          clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetotal - DVD_MSEC_TO_TIME(400);
        else
          clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetime;

        if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE && (m_CurrentVideo.packets > 0))
        {
          if (m_CurrentVideo.starttime - m_CurrentVideo.cachetotal < clock)
            clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
          else if (m_CurrentVideo.starttime > m_CurrentAudio.starttime)
          {
            int audioLevel = m_VideoPlayerAudio->GetLevel();
            //@todo hardcoded 8 seconds in message queue
            double maxAudioTime = clock + DVD_MSEC_TO_TIME(80 * audioLevel);
            if ((m_CurrentVideo.starttime - m_CurrentVideo.cachetotal) > maxAudioTime)
              clock = maxAudioTime;
            else
              clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
          }
        }
      }
      else if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE && m_CurrentVideo.packets > 0)
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
      m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
      m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerAudio->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);
      m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);
      SetCaching(CACHESTATE_DONE);
      UpdatePlayState(0);

      m_syncTimer.Set(3000);
    }
    else
    {
      // exceptions for which stream players won't start properly
      // 1. videoplayer has not detected a keyframe within length of demux buffers
      if (m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0 &&
          !m_VideoPlayerAudio->AcceptsData() &&
          m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_STARTING &&
          m_VideoPlayerVideo->IsStalled() &&
          m_CurrentVideo.packets > 10)
      {
        m_VideoPlayerAudio->AcceptsData();
        CLog::Log(LOGWARNING, "VideoPlayer::Sync - stream player video does not start, flushing buffers");
        FlushBuffers(DVD_NOPTS_VALUE, true, true);
      }
    }
  }

  // handle ff/rw
  if (m_playSpeed != DVD_PLAYSPEED_NORMAL && m_playSpeed != DVD_PLAYSPEED_PAUSE)
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
        m_SpeedState.lastabstime = m_clock.GetAbsoluteClock();
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

        if (error > DVD_MSEC_TO_TIME(1000))
        {
          error  = (m_clock.GetClock() - m_SpeedState.lastseekpts) / 1000;

          if (std::abs(error) > 1000 || (m_VideoPlayerVideo->IsRewindStalled() && std::abs(error) > 100))
          {
            CLog::Log(LOGDEBUG, "CVideoPlayer::Process - Seeking to catch up, error was: %f", error);
            m_SpeedState.lastseekpts = m_clock.GetClock();
            int direction = (m_playSpeed > 0) ? 1 : -1;
            double iTime = (m_clock.GetClock() + m_State.time_offset + 1000000.0 * direction) / 1000;
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = iTime;
            mode.backward = (m_playSpeed < 0);
            mode.accurate = false;
            mode.restore = false;
            mode.trickplay = true;
            mode.sync = false;
            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
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
  if(pkt->dts != DVD_NOPTS_VALUE)
    pkt->dts -= correction;
  if(pkt->pts != DVD_NOPTS_VALUE)
    pkt->pts -= correction;
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

  current.dispTime = pPacket->dispTime;
}

static void UpdateLimits(double& minimum, double& maximum, double dts)
{
  if(dts == DVD_NOPTS_VALUE)
    return;
  if(minimum == DVD_NOPTS_VALUE || minimum > dts) minimum = dts;
  if(maximum == DVD_NOPTS_VALUE || maximum < dts) maximum = dts;
}

bool CVideoPlayer::CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket)
{
  if (m_playSpeed < DVD_PLAYSPEED_PAUSE)
    return false;

  if( pPacket->dts == DVD_NOPTS_VALUE || current.dts == DVD_NOPTS_VALUE)
    return false;

  double mindts = DVD_NOPTS_VALUE, maxdts = DVD_NOPTS_VALUE;
  UpdateLimits(mindts, maxdts, m_CurrentAudio.dts);
  UpdateLimits(mindts, maxdts, m_CurrentVideo.dts);
  UpdateLimits(mindts, maxdts, m_CurrentAudio.dts_end());
  UpdateLimits(mindts, maxdts, m_CurrentVideo.dts_end());

  /* if we don't have max and min, we can't do anything more */
  if( mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE )
    return false;

  double correction = 0.0;
  if( pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000))
  {
    CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - resync forward :%d, prev:%f, curr:%f, diff:%f"
                            , current.type, current.dts, pPacket->dts, pPacket->dts - maxdts);
    correction = pPacket->dts - maxdts;
  }

  /* if it's large scale jump, correct for it after having confirmed the jump */
  if(pPacket->dts + DVD_MSEC_TO_TIME(500) < current.dts_end())
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
      CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - update correction: %f", correction);
    }
    else
    {
      // not sure yet - flags the packets as unknown until we get confirmation on another audio/video packet
      pPacket->dts = DVD_NOPTS_VALUE;
      pPacket->pts = DVD_NOPTS_VALUE;
    }
  }
  else
  {
    if (current.avsync == CCurrentStream::AV_SYNC_CHECK)
      current.avsync = CCurrentStream::AV_SYNC_CONT;
  }
  current.lastdts = lastdts;
  return true;
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
  if (!m_Edl.HasCut())
    return;

  // Check that there is an audio and video stream.
  if((m_CurrentAudio.id < 0 || m_CurrentAudio.syncState != IDVDStreamPlayer::SYNC_INSYNC) ||
     (m_CurrentVideo.id < 0 || m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_INSYNC))
    return;

  // If there is a startpts defined for either the audio or video stream then VideoPlayer is still
  // still decoding frames to get to the previously requested seek point.
  if (m_CurrentAudio.inited == false ||
      m_CurrentVideo.inited == false)
    return;

  const int64_t clock = GetTime();

  CEdl::Cut cut;
  if (!m_Edl.InCut(clock, &cut))
    return;

  if (cut.action == CEdl::CUT)
  {
    if ((m_playSpeed > 0 && clock < cut.end - 1000) ||
        (m_playSpeed < 0 && clock < cut.start + 1000))
    {
      CLog::Log(LOGDEBUG, "%s - Clock in EDL cut [%s - %s]: %s. Automatically skipping over.",
                __FUNCTION__, CEdl::MillisecondsToTimeString(cut.start).c_str(),
                CEdl::MillisecondsToTimeString(cut.end).c_str(), CEdl::MillisecondsToTimeString(clock).c_str());

      //Seeking either goes to the start or the end of the cut depending on the play direction.
      int seek = m_playSpeed >= 0 ? cut.end : cut.start;

      CDVDMsgPlayerSeek::CMode mode;
      mode.time = seek;
      mode.backward = true;
      mode.accurate = true;
      mode.restore = true;
      mode.trickplay = false;
      mode.sync = true;
      m_messenger.Put(new CDVDMsgPlayerSeek(mode));
    }
  }
  else if (cut.action == CEdl::COMM_BREAK)
  {
    // marker for commbrak may be inaccurate. allow user to skip into break from the back
    if (m_playSpeed >= 0 && m_Edl.GetLastCutTime() != cut.start && clock < cut.end - 1000)
    {
      std::string strTimeString = StringUtils::SecondsToTimeString((cut.end - cut.start) / 1000, TIME_FORMAT_MM_SS);
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011), strTimeString);

      m_Edl.SetLastCutTime(cut.start);

      if (m_SkipCommercials)
      {
        CLog::Log(LOGDEBUG, "%s - Clock in commercial break [%s - %s]: %s. Automatically skipping to end of commercial break",
                  __FUNCTION__, CEdl::MillisecondsToTimeString(cut.start).c_str(),
                  CEdl::MillisecondsToTimeString(cut.end).c_str(),
                  CEdl::MillisecondsToTimeString(clock).c_str());

        CDVDMsgPlayerSeek::CMode mode;
        mode.time = cut.end;
        mode.backward = true;
        mode.accurate = true;
        mode.restore = false;
        mode.trickplay = false;
        mode.sync = true;
        m_messenger.Put(new CDVDMsgPlayerSeek(mode));
      }
    }
  }
}


void CVideoPlayer::SynchronizeDemuxer()
{
  if(IsCurrentThread())
    return;
  if(!m_messenger.IsInited())
    return;

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(500, SYNCSOURCE_PLAYER);
  m_messenger.Put(message->Acquire());
  message->Wait(m_bStop, 0);
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
  if (!m_bAbortRequest)
    CLog::Log(LOGNOTICE, "VideoPlayer: eof, waiting for queues to empty");

  CloseStream(m_CurrentAudio, !m_bAbortRequest);
  CloseStream(m_CurrentVideo, !m_bAbortRequest);
  CloseStream(m_CurrentTeletext,!m_bAbortRequest);
  CloseStream(m_CurrentRadioRDS, !m_bAbortRequest);
  // the generalization principle was abused for subtitle player. actually it is not a stream player like
  // video and audio. subtitle player does not run on its own thread, hence waitForBuffers makes
  // no sense here. waitForBuffers is abused to clear overlay container (false clears container)
  // subtitles are added from video player. after video player has finished, overlays have to be cleared.
  CloseStream(m_CurrentSubtitle, false);  // clear overlay container

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
  if (m_bAbortRequest)
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  m_openEvent.Set();

  CFFmpegLog::ClearLogLevel();
}

void CVideoPlayer::HandleMessages()
{
  CDVDMsg* pMsg;

  while (m_messenger.Get(&pMsg, 0) == MSGQ_OK)
  {
    if (pMsg->IsType(CDVDMsg::PLAYER_OPENFILE) &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_OPENFILE) == 0)
    {
      CDVDMsgOpenFile &msg(*static_cast<CDVDMsgOpenFile*>(pMsg));

      FlushBuffers(DVD_NOPTS_VALUE, true, true);
      m_renderManager.Flush(false);
      CloseDemuxer();
      SAFE_DELETE(m_pSubtitleDemuxer);
      SAFE_DELETE(m_pCCDemuxer);
      SAFE_DELETE(m_pInputStream);

      m_item = msg.GetItem();
      m_playerOptions = msg.GetOptions();
      Prepare();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK) &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
    {
      CDVDMsgPlayerSeek &msg(*static_cast<CDVDMsgPlayerSeek*>(pMsg));

      if (!m_State.canseek)
      {
        m_processInfo->SetStateSeeking(false);
        pMsg->Release();
        continue;
      }

      // skip seeks if player has not finished the last seek
      if (m_CurrentVideo.id >= 0 &&
          m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_INSYNC)
      {
        double now = m_clock.GetAbsoluteClock();
        if (m_playSpeed == DVD_PLAYSPEED_NORMAL &&
            (now - m_State.lastSeek)/1000 < 2000 &&
            !msg.GetAccurate())
        {
          m_processInfo->SetStateSeeking(false);
          pMsg->Release();
          continue;
        }
      }

      if (!msg.GetTrickPlay())
      {
        g_infoManager.SetDisplayAfterSeek(100000);
        SetCaching(CACHESTATE_FLUSH);
      }

      double start = DVD_NOPTS_VALUE;

      double time = msg.GetTime();
      if (msg.GetRelative())
        time = (m_clock.GetClock() + m_State.time_offset) / 1000l + time;

      time = msg.GetRestore() ? m_Edl.RestoreCutTime(time) : time;

      // if input stream doesn't support ISeekTime, convert back to pts
      //! @todo
      //! After demuxer we add an offset to input pts so that displayed time and clock are
      //! increasing steadily. For seeking we need to determine the boundaries and offset
      //! of the desired segment. With the current approach calculated time may point
      //! to nirvana
      if (m_pInputStream->GetIPosTime() == nullptr)
        time -= m_State.time_offset/1000l;

      CLog::Log(LOGDEBUG, "demuxer seek to: %f", time);
      if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start))
      {
        CLog::Log(LOGDEBUG, "demuxer seek to: %f, success", time);
        if(m_pSubtitleDemuxer)
        {
          if(!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
            CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %f, success", time);
        }
        // dts after successful seek
        if (start == DVD_NOPTS_VALUE)
          start = DVD_MSEC_TO_TIME(time) - m_State.time_offset;

        m_State.dts = start;
        m_State.lastSeek = m_clock.GetAbsoluteClock();

        FlushBuffers(start, msg.GetAccurate(), msg.GetSync());
      }
      else if (m_pDemuxer)
      {
        CLog::Log(LOGDEBUG, "VideoPlayer: seek failed or hit end of stream");
        // dts after successful seek
        if (start == DVD_NOPTS_VALUE)
          start = DVD_MSEC_TO_TIME(time) - m_State.time_offset;

        m_State.dts = start;

        FlushBuffers(start, false, true);
        if (m_playSpeed != DVD_PLAYSPEED_PAUSE)
        {
          SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
        }
      }

      // set flag to indicate we have finished a seeking request
      if(!msg.GetTrickPlay())
        g_infoManager.SetDisplayAfterSeek();

      // dvd's will issue a HOP_CHANNEL that we need to skip
      if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        m_dvd.state = DVDSTATE_SEEK;

      m_processInfo->SetStateSeeking(false);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK_CHAPTER) &&
             m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
             m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
    {
      g_infoManager.SetDisplayAfterSeek(100000);
      SetCaching(CACHESTATE_FLUSH);

      CDVDMsgPlayerSeekChapter &msg(*static_cast<CDVDMsgPlayerSeekChapter*>(pMsg));
      double start = DVD_NOPTS_VALUE;
      int offset = 0;

      // This should always be the case.
      if(m_pDemuxer && m_pDemuxer->SeekChapter(msg.GetChapter(), &start))
      {
        FlushBuffers(start, true, true);
        int64_t beforeSeek = GetTime();
        offset = DVD_TIME_TO_MSEC(start) - static_cast<int>(beforeSeek);
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
      CDVDMsgPlayerSetAudioStream* pMsg2 = static_cast<CDVDMsgPlayerSetAudioStream*>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
      if(st.source != STREAM_SOURCE_NONE)
      {
        if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);
          if(pStream->SetActiveAudioStream(st.id))
          {
            m_dvd.iSelectedAudioStream = -1;
            CloseStream(m_CurrentAudio, false);
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = (int)GetUpdatedTime();
            mode.backward = true;
            mode.accurate = true;
            mode.trickplay = true;
            mode.sync = true;
            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
          }
        }
        else
        {
          CloseStream(m_CurrentAudio, false);
          OpenStream(m_CurrentAudio, st.demuxerId, st.id, st.source);
          AdaptForcedSubtitles();

          CDVDMsgPlayerSeek::CMode mode;
          mode.time = (int)GetUpdatedTime();
          mode.backward = true;
          mode.accurate = true;
          mode.trickplay = true;
          mode.sync = true;
          m_messenger.Put(new CDVDMsgPlayerSeek(mode));
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_VIDEOSTREAM))
    {
      CDVDMsgPlayerSetVideoStream* pMsg2 = static_cast<CDVDMsgPlayerSetVideoStream*>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_VIDEO, pMsg2->GetStreamId());
      if (st.source != STREAM_SOURCE_NONE)
      {
        if (st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);
          if (pStream->SetAngle(st.id))
          {
            m_dvd.iSelectedVideoStream = st.id;

            CDVDMsgPlayerSeek::CMode mode;
            mode.time = (int)GetUpdatedTime();
            mode.backward = true;
            mode.accurate = true;
            mode.trickplay = true;
            mode.sync = true;
            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
          }
        }
        else
        {
          CloseStream(m_CurrentVideo, false);
          OpenStream(m_CurrentVideo, st.demuxerId, st.id, st.source);
          CDVDMsgPlayerSeek::CMode mode;
          mode.time = (int)GetUpdatedTime();
          mode.backward = true;
          mode.accurate = true;
          mode.trickplay = true;
          mode.sync = true;
          m_messenger.Put(new CDVDMsgPlayerSeek(mode));
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
    {
      CDVDMsgPlayerSetSubtitleStream* pMsg2 = static_cast<CDVDMsgPlayerSetSubtitleStream*>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
      if(st.source != STREAM_SOURCE_NONE)
      {
        if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);
          if(pStream->SetActiveSubtitleStream(st.id))
          {
            m_dvd.iSelectedSPUStream = -1;
            CloseStream(m_CurrentSubtitle, false);
          }
        }
        else
        {
          CloseStream(m_CurrentSubtitle, false);
          OpenStream(m_CurrentSubtitle, st.demuxerId, st.id, st.source);
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
    {
      CDVDMsgBool* pValue = static_cast<CDVDMsgBool*>(pMsg);
      SetSubtitleVisibleInternal(pValue->m_value);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
    {
      g_infoManager.SetDisplayAfterSeek(100000);
      SetCaching(CACHESTATE_FLUSH);

      CDVDMsgPlayerSetState* pMsgPlayerSetState = static_cast<CDVDMsgPlayerSetState*>(pMsg);

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
      CDVDInputStreamPVRManager* input = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
      if(input)
        input->Record(*static_cast<CDVDMsgBool*>(pMsg));
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      FlushBuffers(DVD_NOPTS_VALUE, true, true);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      int speed = static_cast<CDVDMsgPlayerSetSpeed*>(pMsg)->GetSpeed();

      // correct our current clock, as it would start going wrong otherwise
      if (m_State.timestamp > 0)
      {
        double offset;
        offset = m_clock.GetAbsoluteClock() - m_State.timestamp;
        offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
        offset = DVD_TIME_TO_MSEC(offset);
        if (offset > 1000)
          offset = 1000;
        if (offset < -1000)
          offset = -1000;
        m_State.time += offset;
        m_State.timestamp = m_clock.GetAbsoluteClock();
      }

      if (speed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_PAUSE && speed != m_playSpeed)
        m_callback.OnPlayBackSpeedChanged(speed / DVD_PLAYSPEED_NORMAL);

      // notify GUI, skins may want to show the seekbar
      g_infoManager.SetDisplayAfterSeek();

      if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) && speed != m_playSpeed)
      {
        CDVDInputStreamPVRManager* pvrinputstream = static_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
        pvrinputstream->Pause( speed == 0 );
      }

      // do a seek after rewind, clock is not in sync with current pts
      if ((speed == DVD_PLAYSPEED_NORMAL) &&
          (m_playSpeed != DVD_PLAYSPEED_NORMAL) &&
          (m_playSpeed != DVD_PLAYSPEED_PAUSE) &&
          !m_omxplayer_mode)
      {
        double iTime = m_VideoPlayerVideo->GetCurrentPts();
        if (iTime == DVD_NOPTS_VALUE)
          iTime = m_clock.GetClock();
        iTime = (iTime + m_State.time_offset) / 1000;

        CDVDMsgPlayerSeek::CMode mode;
        mode.time = iTime;
        mode.backward = m_playSpeed < 0;
        mode.accurate = true;
        mode.trickplay = true;
        mode.sync = true;
        m_messenger.Put(new CDVDMsgPlayerSeek(mode));
      }

      // !!! omx alterative code path !!!
      // should be done differently
      if (m_omxplayer_mode)
      {
        // when switching from trickplay to normal, we may not have a full set of reference frames
        // in decoder and we may get corrupt frames out. Seeking to current time will avoid this.
        if ( (speed != DVD_PLAYSPEED_PAUSE && speed != DVD_PLAYSPEED_NORMAL) ||
            (m_playSpeed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_NORMAL) )
        {
          CDVDMsgPlayerSeek::CMode mode;
          mode.time = (int)GetTime();
          mode.backward = (speed < 0);
          mode.accurate = true;
          mode.restore = false;
          mode.trickplay = true;
          mode.sync = true;
          m_messenger.Put(new CDVDMsgPlayerSeek(mode));
        }
        else
        {
          m_OmxPlayerState.av_clock.OMXPause();
        }

        m_OmxPlayerState.av_clock.OMXSetSpeed(speed);
        CLog::Log(LOGDEBUG, "%s::%s CDVDMsg::PLAYER_SETSPEED speed : %d (%d)", "CVideoPlayer", __FUNCTION__, speed, static_cast<int>(m_playSpeed));
      }

      if (static_cast<CDVDMsgPlayerSetSpeed*>(pMsg)->IsTempo())
        m_processInfo->SetTempo(static_cast<float>(speed) / DVD_PLAYSPEED_NORMAL);
      else
        m_processInfo->SetSpeed(static_cast<float>(speed) / DVD_PLAYSPEED_NORMAL);

      m_playSpeed = speed;

      m_caching = CACHESTATE_DONE;
      m_clock.SetSpeed(speed);
      m_VideoPlayerAudio->SetSpeed(speed);
      m_VideoPlayerVideo->SetSpeed(speed);
      m_streamPlayerSpeed = speed;
      if (m_pDemuxer)
        m_pDemuxer->SetSpeed(speed);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
      OnAction(static_cast<CDVDMsgType<CAction>*>(pMsg)->m_value);
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      SStartMsg& msg = static_cast<CDVDMsgType<SStartMsg>*>(pMsg)->m_value;
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
    else if (pMsg->IsType(CDVDMsg::PLAYER_REPORT_STATE))
    {
      SStateMsg& msg = static_cast<CDVDMsgType<SStateMsg>*>(pMsg)->m_value;
      if (msg.player == VideoPlayer_AUDIO)
      {
        m_CurrentAudio.syncState = msg.syncState;
      }
      if (msg.player == VideoPlayer_VIDEO)
      {
        m_CurrentVideo.syncState = msg.syncState;
      }
      CLog::Log(LOGDEBUG, "CVideoPlayer::HandleMessages - player %d reported state: %d", msg.player, msg.syncState);
    }
    else if (pMsg->IsType(CDVDMsg::SUBTITLE_ADDFILE))
    {
      int id = AddSubtitleFile(static_cast<CDVDMsgType<std::string>*>(pMsg)->m_value);
      if (id >= 0)
      {
        SetSubtitle(id);
        SetSubtitleVisibleInternal(true);
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if (static_cast<CDVDMsgGeneralSynchronize*>(pMsg)->Wait(100, SYNCSOURCE_PLAYER))
        CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::GENERAL_SYNCHRONIZE");
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_AVCHANGE))
    {
      UpdateStreamInfos();
      CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
      CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_ABORT))
    {
      CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::PLAYER_ABORT");
      m_bAbortRequest = true;
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
    m_streamPlayerSpeed = DVD_PLAYSPEED_PAUSE;

    m_pInputStream->ResetScanTimeout((unsigned int) CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPLAYBACK_SCANTIME) * 1000);

    m_cachingTimer.Set(5000);
  }

  if (state == CACHESTATE_PLAY ||
     (state == CACHESTATE_DONE && m_caching != CACHESTATE_PLAY))
  {
    m_clock.SetSpeed(m_playSpeed);
    m_VideoPlayerAudio->SetSpeed(m_playSpeed);
    m_VideoPlayerVideo->SetSpeed(m_playSpeed);
    m_streamPlayerSpeed = m_playSpeed;
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
  {
    CDVDMsgPlayerSetSpeed::SpeedParams params = { speed, false };
    m_messenger.Put(new CDVDMsgPlayerSetSpeed(params));
  }
  else
  {
    m_playSpeed = speed;
    m_streamPlayerSpeed = speed;
  }
}

bool CVideoPlayer::CanPause()
{
  CSingleLock lock(m_StateSection);
  return m_State.canpause;
}

void CVideoPlayer::Pause()
{
  // toggle between pause and normal speed
  if (m_processInfo->GetNewSpeed() == 0)
  {
    SetSpeed(1);
  }
  else
  {
    SetSpeed(0);
  }
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
  if (!m_State.canseek)
    return;

  if (bLargeStep && bChapterOverride && GetChapter() > 0 && GetChapterCount() > 1)
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

  int64_t seekTarget;
  if (g_advancedSettings.m_videoUseTimeSeeking && m_processInfo->GetMaxTime() > 2000*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seekTarget = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig :
                           g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seekTarget = bPlus ? g_advancedSettings.m_videoTimeSeekForward :
                           g_advancedSettings.m_videoTimeSeekBackward;
    seekTarget *= 1000;
    seekTarget += GetTime();
  }
  else
  {
    int percent;
    if (bLargeStep)
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;
    seekTarget = (int64_t)(m_processInfo->GetMaxTime()*(GetPercentage()+percent)/100);
  }

  bool restore = true;

  int64_t time = GetTime();
  if(g_application.CurrentFileItem().IsStack() &&
     (seekTarget > m_processInfo->GetMaxTime() || seekTarget < 0))
  {
    g_application.SeekTime((seekTarget - time) * 0.001 + g_application.GetTime());
    // warning, don't access any VideoPlayer variables here as
    // the VideoPlayer object may have been destroyed
    return;
  }

  CDVDMsgPlayerSeek::CMode mode;
  mode.time = (int)seekTarget;
  mode.backward = !bPlus;
  mode.accurate = false;
  mode.restore = restore;
  mode.trickplay = false;
  mode.sync = true;

  m_messenger.Put(new CDVDMsgPlayerSeek(mode));
  SynchronizeDemuxer();
  if (seekTarget < 0)
    seekTarget = 0;
  m_callback.OnPlayBackSeek(seekTarget, seekTarget - time);
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
    CDVDMsgPlayerSeek::CMode mode;
    mode.time = iScenemarker;
    mode.backward = !bPlus;
    mode.accurate = false;
    mode.restore = false;
    mode.trickplay = false;
    mode.sync = true;

    m_messenger.Put(new CDVDMsgPlayerSeek(mode));
    SynchronizeDemuxer();
    return true;
  }
  return false;
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
      if(m_State.cache_bytes >= 0)
      {
        strBuf += StringUtils::Format(" forward:%s %2.0f%%"
                                      , StringUtils::SizeToString(m_State.cache_bytes).c_str()
                                      , m_State.cache_level * 100);
        if(m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
          strBuf += StringUtils::Format(" %d msec", DVD_TIME_TO_MSEC(m_State.cache_delay));
      }

      strGeneralInfo = StringUtils::Format("C( a/v:% 6.3f%s, %s amp:% 5.2f )"
          , dDiff
          , strEDL.c_str()
          , strBuf.c_str()
          , m_VideoPlayerAudio->GetDynamicRangeAmplification());
    }
    else
    {
      double apts = m_VideoPlayerAudio->GetCurrentPts();
      double vpts = m_VideoPlayerVideo->GetCurrentPts();
      double dDiff = 0;

      if (apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE)
        dDiff = (apts - vpts) / DVD_TIME_BASE;

      std::string strBuf;
      CSingleLock lock(m_StateSection);
      if(m_State.cache_bytes >= 0)
      {
        strBuf += StringUtils::Format(" forward:%s %2.0f%%"
                                      , StringUtils::SizeToString(m_State.cache_bytes).c_str()
                                      , m_State.cache_level * 100);
        if(m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
          strBuf += StringUtils::Format(" %d msec", DVD_TIME_TO_MSEC(m_State.cache_delay));
      }

      strGeneralInfo = StringUtils::Format("Player: a/v:% 6.3f, %s"
                                           , dDiff
                                           , strBuf.c_str());
    }
  }
}

void CVideoPlayer::SeekPercentage(float iPercent)
{
  int64_t iTotalTime = m_processInfo->GetMaxTime();

  if (!iTotalTime)
    return;

  SeekTime((int64_t)(iTotalTime * iPercent / 100));
}

float CVideoPlayer::GetPercentage()
{
  int64_t iTotalTime = m_processInfo->GetMaxTime();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

float CVideoPlayer::GetCachePercentage()
{
  CSingleLock lock(m_StateSection);
  return (float) (m_State.cache_offset * 100); // NOTE: Percentage returned is relative
}

void CVideoPlayer::SetAVDelay(float fValue)
{
  m_renderManager.SetDelay(static_cast<int>(fValue * 1000.0f));
}

float CVideoPlayer::GetAVDelay()
{
  return static_cast<float>(m_renderManager.GetDelay()) / 1000.0f;
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

void CVideoPlayer::UpdateStreamInfos()
{
  if (!m_pDemuxer)
    return;

  CSingleLock lock(m_SelectionStreams.m_section);
  int streamId;
  std::string retVal;

  // video
  streamId = GetVideoStream();

  if (streamId >= 0 && streamId < GetVideoStreamCount())
  {
    SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, streamId);
    s.aspect_ratio = m_renderManager.GetAspectRatio();
    CRect viewRect;
    m_renderManager.GetVideoRect(s.SrcRect, s.DestRect, viewRect);
    CDemuxStream* stream = m_pDemuxer->GetStream(m_CurrentVideo.demuxerId, m_CurrentVideo.id);
    if (stream && stream->type == STREAM_VIDEO)
    {
      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        int cout = m_SelectionStreams.Count(STREAM_VIDEO);
        for (int i = 0; i < cout; ++i)
        {
          SelectionStream& select = m_SelectionStreams.Get(STREAM_VIDEO, i);
          select.width = static_cast<CDemuxStreamVideo*>(stream)->iWidth;
          select.height = static_cast<CDemuxStreamVideo*>(stream)->iHeight;
          select.bitrate = static_cast<CDemuxStreamVideo*>(stream)->iBitRate;
        }
      }
      else
      {
        s.width = static_cast<CDemuxStreamVideo*>(stream)->iWidth;
        s.height = static_cast<CDemuxStreamVideo*>(stream)->iHeight;
        s.bitrate = static_cast<CDemuxStreamVideo*>(stream)->iBitRate;
      }
      s.stereo_mode = m_VideoPlayerVideo->GetStereoMode();
      if (s.stereo_mode == "mono")
        s.stereo_mode = "";
    }
  }

  // audio
  streamId = GetAudioStream();

  if (streamId >= 0 && streamId < GetAudioStreamCount())
  {
    SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, streamId);
    s.channels = m_VideoPlayerAudio->GetAudioChannels();

    CDemuxStream* stream = m_pDemuxer->GetStream(m_CurrentAudio.demuxerId, m_CurrentAudio.id);
    if (stream && stream->type == STREAM_AUDIO)
    {
      s.codec = m_pDemuxer->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      s.bitrate = static_cast<CDemuxStreamAudio*>(stream)->iBitRate;
    }
  }
}

void CVideoPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info)
{
  CSingleLock lock(m_SelectionStreams.m_section);
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
    CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);
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
  SynchronizeDemuxer();
}

int CVideoPlayer::GetVideoStreamCount() const
{
  return m_SelectionStreams.Count(STREAM_VIDEO);
}

int CVideoPlayer::GetVideoStream() const
{
  return m_SelectionStreams.IndexOf(STREAM_VIDEO, *this);
}

void CVideoPlayer::SetVideoStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetVideoStream(iStream));
  SynchronizeDemuxer();
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
  int64_t seekOffset = iTime - GetTime();

  CDVDMsgPlayerSeek::CMode mode;
  mode.time = static_cast<double>(iTime);
  mode.backward = true;
  mode.accurate = true;
  mode.trickplay = false;
  mode.sync = true;

  m_messenger.Put(new CDVDMsgPlayerSeek(mode));
  SynchronizeDemuxer();
  m_callback.OnPlayBackSeek(iTime, seekOffset);
}

bool CVideoPlayer::SeekTimeRelative(int64_t iTime)
{
  int64_t abstime = GetTime() + iTime;

  CDVDMsgPlayerSeek::CMode mode;
  mode.time = (int)iTime;
  mode.relative = true;
  mode.backward = (iTime < 0) ? true : false;
  mode.accurate = false;
  mode.trickplay = false;
  mode.sync = true;

  m_messenger.Put(new CDVDMsgPlayerSeek(mode));
  m_processInfo->SetStateSeeking(true);

  m_callback.OnPlayBackSeek(abstime, iTime);
  return true;
}

// return the time in milliseconds
int64_t CVideoPlayer::GetTime()
{
  CSingleLock lock(m_StateSection);
  return llrint(m_State.time);
}

void CVideoPlayer::SetSpeed(float speed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if (speed < 0 && IsInMenu())
    return;

  if (!CanSeek() && !CanPause())
    return;

  int iSpeed = static_cast<int>(speed * DVD_PLAYSPEED_NORMAL);

  if (!CanSeek())
  {
    if ((iSpeed != DVD_PLAYSPEED_NORMAL) && (iSpeed != DVD_PLAYSPEED_PAUSE))
      return;
  }

  float currentSpeed = m_processInfo->GetNewSpeed();
  m_processInfo->SetNewSpeed(speed);
  if (iSpeed != currentSpeed)
  {
    if (iSpeed == DVD_PLAYSPEED_NORMAL)
      m_callback.OnPlayBackResumed();
    else if (iSpeed == DVD_PLAYSPEED_PAUSE)
      m_callback.OnPlayBackPaused();

    if (iSpeed == DVD_PLAYSPEED_NORMAL)
    {
      float currentTempo = m_processInfo->GetNewTempo();
      if (currentTempo != 1.0)
      {
        SetTempo(currentTempo);
        return;
      }
    }
    SetPlaySpeed(iSpeed);
  }
}

void CVideoPlayer::SetTempo(float tempo)
{
  tempo = floor(tempo * 100 + 0.5) / 100;
  if (m_processInfo->IsTempoAllowed(tempo))
  {
    int speed = tempo * DVD_PLAYSPEED_NORMAL;
    CDVDMsgPlayerSetSpeed::SpeedParams params = { speed, true };
    m_messenger.Put(new CDVDMsgPlayerSetSpeed(params));

    m_processInfo->SetNewTempo(tempo);
  }
}

bool CVideoPlayer::SupportsTempo()
{
  return m_canTempo;
}

bool CVideoPlayer::OpenStream(CCurrentStream& current, int64_t demuxerId, int iStream, int source, bool reset /*= true*/)
{
  CDemuxStream* stream = NULL;
  CDVDStreamInfo hint;

  CLog::Log(LOGNOTICE, "Opening stream: %i source: %i", iStream, source);

  if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
  {
    int index = m_SelectionStreams.IndexOf(current.type, source, demuxerId, iStream);
    if(index < 0)
      return false;
    SelectionStream st = m_SelectionStreams.Get(current.type, index);

    if(!m_pSubtitleDemuxer || m_pSubtitleDemuxer->GetFileName() != st.filename)
    {
      CLog::Log(LOGNOTICE, "Opening Subtitle file: %s", st.filename.c_str());
      SAFE_DELETE(m_pSubtitleDemuxer);
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
    stream = m_pSubtitleDemuxer->GetStream(demuxerId, iStream);
    if(!stream || stream->disabled)
      return false;
    
    m_pSubtitleDemuxer->EnableStream(demuxerId, iStream, true);

    hint.Assign(*stream, true);
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
  {
    int index = m_SelectionStreams.IndexOf(current.type, source, demuxerId, iStream);
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

    m_pDemuxer->OpenStream(demuxerId, iStream);

    stream = m_pDemuxer->GetStream(demuxerId, iStream);
    if (!stream || stream->disabled)
      return false;

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
    int oldId = current.id;
    current.id = iStream;
    current.demuxerId = demuxerId;
    current.source = source;
    current.hint = hint;
    current.stream = (void*)stream;
    current.lastdts = DVD_NOPTS_VALUE;
    if (oldId >= 0 && current.avsync != CCurrentStream::AV_SYNC_FORCE)
      current.avsync = CCurrentStream::AV_SYNC_CHECK;
    if(stream)
      current.changes = stream->changes;
  }
  else
  {
    if(stream)
    {
      /* mark stream as disabled, to disallow further attempts*/
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, stream->uniqueId);
      stream->disabled = true;
    }
  }

  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
  CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();

  return res;
}

bool CVideoPlayer::OpenAudioStream(CDVDStreamInfo& hint, bool reset)
{
  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentAudio.player);
  if(player == nullptr)
    return false;

  if(m_CurrentAudio.id < 0 ||
     m_CurrentAudio.hint != hint)
  {
    if (!player->OpenStream(hint))
      return false;

    static_cast<IDVDStreamPlayerAudio*>(player)->SetSpeed(m_streamPlayerSpeed);
    m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_STARTING;
    m_CurrentAudio.packets = 0;
  }
  else if (reset)
    player->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET), 0);

  m_HasAudio = true;

  static_cast<IDVDStreamPlayerAudio*>(player)->SendMessage(new CDVDMsg(CDVDMsg::PLAYER_REQUEST_STATE), 1);

  return true;
}

bool CVideoPlayer::OpenVideoStream(CDVDStreamInfo& hint, bool reset)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    /* set aspect ratio as requested by navigator for dvd's */
    float aspect = static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->GetVideoAspectRatio();
    if (aspect != 0.0)
    {
      hint.aspect = aspect;
      hint.forced_aspect = true;
    }
    hint.dvd = true;
  }
  else if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    // set framerate if not set by demuxer
    if (hint.fpsrate == 0 || hint.fpsscale == 0)
    {
      int fpsidx = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPLAYBACK_FPS);
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
    hint.stereo_mode = CStereoscopicsManager::GetInstance().DetectStereoModeByString(m_item.GetPath());

  SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, 0);

  if (hint.flags & AV_DISPOSITION_ATTACHED_PIC)
    return false;

  // set desired refresh rate
  if (m_playerOptions.fullscreen && g_graphicsContext.IsFullScreenRoot() &&
      hint.fpsrate != 0 && hint.fpsscale != 0)
  {
    if (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
    {
      double framerate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE * hint.fpsscale / hint.fpsrate);
      m_renderManager.TriggerUpdateResolution(static_cast<float>(framerate), hint.width, RenderManager::GetStereoModeFlags(hint.stereo_mode));
    }
  }

  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentVideo.player);
  if(player == nullptr)
    return false;

  if(m_CurrentVideo.id < 0 ||
     m_CurrentVideo.hint != hint)
  {
    if (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264)
      SAFE_DELETE(m_pCCDemuxer);

    if (!player->OpenStream(hint))
      return false;

    s.stereo_mode = static_cast<IDVDStreamPlayerVideo*>(player)->GetStereoMode();
    if (s.stereo_mode == "mono")
      s.stereo_mode = "";

    static_cast<IDVDStreamPlayerVideo*>(player)->SetSpeed(m_streamPlayerSpeed);
    m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_STARTING;
    m_CurrentVideo.packets = 0;
  }
  else if (reset)
    player->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET), 0);

  m_HasVideo = true;

  static_cast<IDVDStreamPlayerVideo*>(player)->SendMessage(new CDVDMsg(CDVDMsg::PLAYER_REQUEST_STATE), 1);

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
  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentSubtitle.player);
  if(player == nullptr)
    return false;

  if(m_CurrentSubtitle.id < 0 ||
     m_CurrentSubtitle.hint != hint)
  {
    if (!player->OpenStream(hint))
      return false;
  }

  return true;
}

void CVideoPlayer::AdaptForcedSubtitles()
{
  SelectionStream ss = m_SelectionStreams.Get(STREAM_SUBTITLE, GetSubtitle());
  if (ss.flags & CDemuxStream::FLAG_FORCED)
  {
    SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
    bool found = false;
    for (const auto &stream : m_SelectionStreams.Get(STREAM_SUBTITLE))
    {
      if (stream.flags & CDemuxStream::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(stream.language, as.language))
      {
        if (OpenStream(m_CurrentSubtitle, stream.demuxerId, stream.id, stream.source))
        {
          found = true;
          SetSubtitleVisibleInternal(true);
          break;
        }
      }
    }
    if (!found)
    {
      SetSubtitleVisibleInternal(false);
    }
  }
}

bool CVideoPlayer::OpenTeletextStream(CDVDStreamInfo& hint)
{
  if (!m_VideoPlayerTeletext->CheckStream(hint))
    return false;

  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentTeletext.player);
  if(player == nullptr)
    return false;

  if(m_CurrentTeletext.id < 0 ||
     m_CurrentTeletext.hint != hint)
  {
    if (!player->OpenStream(hint))
      return false;
  }

  return true;
}

bool CVideoPlayer::OpenRadioRDSStream(CDVDStreamInfo& hint)
{
  if (!m_VideoPlayerRadioRDS->CheckStream(hint))
    return false;

  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentRadioRDS.player);
  if(player == nullptr)
    return false;

  if(m_CurrentRadioRDS.id < 0 ||
     m_CurrentRadioRDS.hint != hint)
  {
    if (!player->OpenStream(hint))
      return false;
  }

  return true;
}

bool CVideoPlayer::CloseStream(CCurrentStream& current, bool bWaitForBuffers)
{
  if (current.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing stream player %d", current.player);

  if(bWaitForBuffers)
    SetCaching(CACHESTATE_DONE);

  if (m_pDemuxer && STREAM_SOURCE_MASK(current.source) == STREAM_SOURCE_DEMUX)
    m_pDemuxer->EnableStream(current.demuxerId, current.id, false);

  IDVDStreamPlayer* player = GetStreamPlayer(current.player);
  if(player)
  {
    if ((current.type == STREAM_AUDIO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC) ||
        (current.type == STREAM_VIDEO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC))
      bWaitForBuffers = false;
    player->CloseStream(bWaitForBuffers);
  }

  current.Clear();
  return true;
}

void CVideoPlayer::FlushBuffers(double pts, bool accurate, bool sync)
{
  CLog::Log(LOGDEBUG, "CVideoPlayer::FlushBuffers - flushing buffers");

  double startpts;
  if (accurate && !m_omxplayer_mode)
    startpts = pts;
  else
    startpts = DVD_NOPTS_VALUE;

  if (sync)
  {
    m_CurrentAudio.inited = false;
    m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_FORCE;
    m_CurrentVideo.inited = false;
    m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_FORCE;
    m_CurrentSubtitle.inited = false;
    m_CurrentTeletext.inited = false;
    m_CurrentRadioRDS.inited  = false;
  }

  m_CurrentAudio.dts         = DVD_NOPTS_VALUE;
  m_CurrentAudio.startpts    = startpts;
  m_CurrentAudio.packets = 0;

  m_CurrentVideo.dts         = DVD_NOPTS_VALUE;
  m_CurrentVideo.startpts    = startpts;
  m_CurrentVideo.packets = 0;

  m_CurrentSubtitle.dts      = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.startpts = startpts;
  m_CurrentSubtitle.packets = 0;

  m_CurrentTeletext.dts      = DVD_NOPTS_VALUE;
  m_CurrentTeletext.startpts = startpts;
  m_CurrentTeletext.packets = 0;

  m_CurrentRadioRDS.dts      = DVD_NOPTS_VALUE;
  m_CurrentRadioRDS.startpts = startpts;
  m_CurrentRadioRDS.packets = 0;

  m_VideoPlayerAudio->Flush(sync);
  m_VideoPlayerVideo->Flush(sync);
  m_VideoPlayerSubtitle->Flush();
  m_VideoPlayerTeletext->Flush();
  m_VideoPlayerRadioRDS->Flush();

  // clear subtitle and menu overlays
  m_overlayContainer.Clear();

  if (m_playSpeed == DVD_PLAYSPEED_NORMAL ||
      m_playSpeed == DVD_PLAYSPEED_PAUSE)
  {
    // make sure players are properly flushed, should put them in stalled state
    CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(1000, SYNCSOURCE_AUDIO | SYNCSOURCE_VIDEO);
    m_VideoPlayerAudio->SendMessage(msg->Acquire(), 1);
    m_VideoPlayerVideo->SendMessage(msg->Acquire(), 1);
    msg->Wait(m_bStop, 0);
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

  if (m_omxplayer_mode)
  {
    m_OmxPlayerState.av_clock.OMXFlush();
    m_OmxPlayerState.av_clock.OMXStop();
    m_OmxPlayerState.av_clock.OMXPause();
    m_OmxPlayerState.av_clock.OMXMediaTime(0.0);
  }
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CVideoPlayer::OnDiscNavResult(void* pData, int iMessage)
{
#if defined(HAVE_LIBBLURAY)
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    switch (iMessage)
    {
    case BD_EVENT_MENU_OVERLAY:
      m_overlayContainer.Add(static_cast<CDVDOverlay*>(pData));
      break;
    case BD_EVENT_PLAYLIST_STOP:
      m_messenger.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
      break;
    case BD_EVENT_AUDIO_STREAM:
      m_dvd.iSelectedAudioStream = *static_cast<int*>(pData);
      break;

    case BD_EVENT_PG_TEXTST_STREAM:
      m_dvd.iSelectedSPUStream = *static_cast<int*>(pData);
      break;
    case BD_EVENT_PG_TEXTST:
    {
      bool enable = (*static_cast<int*>(pData) != 0);
      m_VideoPlayerVideo->EnableSubtitle(enable);
    }
    break;
    case BD_EVENT_STILL_TIME:
    {
      if (m_dvd.state != DVDSTATE_STILL)
      {
        // else notify the player we have received a still frame

        m_dvd.iDVDStillTime = *(int*)pData;
        m_dvd.iDVDStillStartTime = XbmcThreads::SystemClockMillis();

        /* adjust for the output delay in the video queue */
        unsigned int time = 0;
        if (m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0)
        {
          time = (unsigned int)(m_VideoPlayerVideo->GetOutputDelay() / (DVD_TIME_BASE / 1000));
          if (time < 10000 && time > 0)
            m_dvd.iDVDStillTime += time;
        }
        m_dvd.state = DVDSTATE_STILL;
        CLog::Log(LOGDEBUG,
          "BD_EVENT_STILL_TIME - waiting %i sec, with delay of %d sec",
          m_dvd.iDVDStillTime, time / 1000);
      }
    }
    break;
    case BD_EVENT_MENU_ERROR:
    {
      m_dvd.state = DVDSTATE_NORMAL;
      CLog::Log(LOGDEBUG, "CVideoPlayer::OnDiscNavResult - libbluray menu not supported (DVDSTATE_NORMAL)");
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25008), g_localizeStrings.Get(25009));
    }
    break;
    case BD_EVENT_ENC_ERROR:
    {
      m_dvd.state = DVDSTATE_NORMAL;
      CLog::Log(LOGDEBUG, "CVideoPlayer::OnDiscNavResult - libbluray the disc/file is encrypted and can't be played (DVDSTATE_NORMAL)");
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(16026), g_localizeStrings.Get(29805));
    }
    break;
    default:
      break;
    }

    return 0;
  }
#endif

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

    switch (iMessage)
    {
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

        dvdnav_still_event_t *still_event = static_cast<dvdnav_still_event_t*>(pData);
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
        dvdnav_spu_stream_change_event_t* event = static_cast<dvdnav_spu_stream_change_event_t*>(pData);

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
        dvdnav_audio_stream_change_event_t* event = static_cast<dvdnav_audio_stream_change_event_t*>(pData);

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
        m_VideoPlayerSubtitle->UpdateOverlayInfo(static_cast<CDVDInputStreamNavigator*>(m_pInputStream), LIBDVDNAV_BUTTON_NORMAL);
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

        if (m_dvd.state != DVDSTATE_STILL)
          m_dvd.state = DVDSTATE_NORMAL;
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
        {
          bool sync = !IsInMenuInternal();
          FlushBuffers(DVD_NOPTS_VALUE, false, sync);
          m_dvd.syncClock = true;
          m_dvd.state = DVDSTATE_NORMAL;
          if (m_pDemuxer)
            m_pDemuxer->Flush();
        }

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

void CVideoPlayer::GetVideoResolution(unsigned int &width, unsigned int &height)
{
  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  width = res.iWidth;
  height = res.iHeight;
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
    if (m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime != 0 && pMenus->GetTotalButtons() == 0)
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
            m_VideoPlayerSubtitle->UpdateOverlayInfo(static_cast<CDVDInputStreamNavigator*>(m_pInputStream), LIBDVDNAV_BUTTON_CLICKED);

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

  if (dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream))
  {
    switch (action.GetID())
    {
      case ACTION_MOVE_UP:
      case ACTION_NEXT_ITEM:
      case ACTION_CHANNEL_UP:
      {
        if (m_Edl.HasCut()) 
        {
          // If the clip has an EDL, we'll search through that instead of sending a CHANNEL message
          const int64_t clock = m_omxplayer_mode ? GetTime() : DVD_TIME_TO_MSEC(std::min(m_CurrentAudio.dts, m_CurrentVideo.dts) + m_offset_pts);
          CEdl::Cut cut;
          if (m_Edl.GetNearestCut(true, clock, &cut)) 
          {
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = cut.end + 1;
            mode.backward = false;
            mode.accurate = false;
            mode.restore = true;
            mode.trickplay = false;
            mode.sync = true;

            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
            return true;
          }
        }
        return true;
      }

      case ACTION_MOVE_DOWN:
      case ACTION_PREV_ITEM:
      case ACTION_CHANNEL_DOWN:
      {
        if (m_Edl.HasCut())
        {
          // If the clip has an EDL, we'll search through that instead of sending a CHANNEL message
          const int64_t clock = m_omxplayer_mode ? GetTime() : DVD_TIME_TO_MSEC(std::min(m_CurrentAudio.dts, m_CurrentVideo.dts) + m_offset_pts);
          CEdl::Cut cut;
          if (m_Edl.GetNearestCut(false, clock, &cut)) 
          {
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = cut.start - 1;
            mode.backward = true;
            mode.accurate = false;
            mode.restore = true;
            mode.trickplay = false;
            mode.sync = true;

            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
            return true;
          }
          else
          {
            // Go back to beginning
            CDVDMsgPlayerSeek::CMode mode;
            mode.time = 0;
            mode.backward = true;
            mode.accurate = false;
            mode.restore = true;
            mode.trickplay = false;
            mode.sync = true;

            m_messenger.Put(new CDVDMsgPlayerSeek(mode));
            return true;
          }
        }
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
    case ACTION_TOGGLE_COMMSKIP:
      m_SkipCommercials = !m_SkipCommercials;
      CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011),
                                            g_localizeStrings.Get(m_SkipCommercials ? 25013 : 25012));
      break;
    case ACTION_PLAYER_DEBUG:
      m_renderManager.ToggleDebug();
      break;

    case ACTION_PLAYER_PROCESS_INFO:
      if (g_windowManager.GetActiveWindow() != WINDOW_DIALOG_PLAYER_PROCESS_INFO)
      {
        g_windowManager.ActivateWindow(WINDOW_DIALOG_PLAYER_PROCESS_INFO);
        return true;
      }
      break;
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CVideoPlayer::IsInMenuInternal() const
{
  CDVDInputStream::IMenus* pStream = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream);
  if (pStream)
  {
    if (m_dvd.state == DVDSTATE_STILL)
      return true;
    else
      return pStream->IsInMenu();
  }
  return false;
}


bool CVideoPlayer::IsInMenu() const
{
  CSingleLock lock(m_StateSection);
  return m_State.isInMenu;
}

bool CVideoPlayer::HasMenu() const
{
  CSingleLock lock(m_StateSection);
  return m_State.hasMenu;
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
    SynchronizeDemuxer();
  }

  return 0;
}

int64_t CVideoPlayer::GetChapterPos(int chapterIdx)
{
  CSingleLock lock(m_StateSection);
  if (chapterIdx > 0 && chapterIdx <= (int) m_State.chapters.size())
    return m_State.chapters[chapterIdx - 1].second;

  return -1;
}

void CVideoPlayer::AddSubtitle(const std::string& strSubPath)
{
  m_messenger.Put(new CDVDMsgType<std::string>(CDVDMsg::SUBTITLE_ADDFILE, strSubPath));
}

bool CVideoPlayer::IsCaching() const
{
  CSingleLock lock(m_StateSection);
  return m_State.caching;
}

int CVideoPlayer::GetCacheLevel() const
{
  CSingleLock lock(m_StateSection);
  return (int)(m_State.cache_level * 100);
}

double CVideoPlayer::GetQueueTime()
{
  int a = m_VideoPlayerAudio->GetLevel();
  int v = m_processInfo->GetLevelVQ();
  return std::max(a, v) * 8000.0 / 100;
}

void CVideoPlayer::GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo &info)
{
  CSingleLock lock(m_SelectionStreams.m_section);
  if (streamId == CURRENT_STREAM)
    streamId = GetVideoStream();

  if (streamId < 0 || streamId > GetVideoStreamCount() - 1)
  {
    info.valid = false;
    return;
  }

  SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, streamId);
  if (s.language.length() > 0)
    info.language = s.language;

  if (s.name.length() > 0)
    info.name = s.name;

  info.valid = true;
  info.bitrate = s.bitrate;
  info.width = s.width;
  info.height = s.height;
  info.SrcRect = s.SrcRect;
  info.DestRect = s.DestRect;
  info.videoCodecName = s.codec;
  info.videoAspectRatio = s.aspect_ratio;
  info.stereoMode = s.stereo_mode;
}

int CVideoPlayer::GetSourceBitrate()
{
  if (m_pInputStream)
    return (int)m_pInputStream->GetBitstreamStats().GetBitrate();

  return 0;
}

void CVideoPlayer::GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info)
{
  CSingleLock lock(m_SelectionStreams.m_section);
  if (index == CURRENT_STREAM)
    index = GetAudioStream();

  if (index < 0 || index > GetAudioStreamCount() - 1)
  {
    info.valid = false;
    return;
  }

  SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, index);
  if (s.language.length() > 0)
    info.language = s.language;

  if (s.name.length() > 0)
    info.name = s.name;

  if (s.type == STREAM_NONE)
    info.name += " (Invalid)";

  info.valid = true;
  info.bitrate = s.bitrate;
  info.channels = s.channels;
  info.audioCodecName = s.codec;
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

    ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(m_item.GetPath(), vobsubfile);

    for (auto sub : v.GetStreams())
    {
      if (sub->type != STREAM_SUBTITLE)
        continue;

      int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE,
        m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename),
        sub->demuxerId, sub->uniqueId);
      SelectionStream& stream = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

      if (stream.name.empty())
        stream.name = info.name;

      if (stream.language.empty())
        stream.language = info.language;

      if (static_cast<CDemuxStream::EFlags>(info.flag) != CDemuxStream::FLAG_NONE)
        stream.flags = static_cast<CDemuxStream::EFlags>(info.flag);
    }

    return m_SelectionStreams.IndexOf(STREAM_SUBTITLE,
      m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename), -1, 0);
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
  ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(m_item.GetPath(), filename);
  s.name = info.name;
  s.language = info.language;
  if (static_cast<CDemuxStream::EFlags>(info.flag) != CDemuxStream::FLAG_NONE)
    s.flags = static_cast<CDemuxStream::EFlags>(info.flag);

  m_SelectionStreams.Update(s);
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, s.source, s.demuxerId, s.id);
}

void CVideoPlayer::UpdatePlayState(double timeout)
{
  if(m_State.timestamp != 0 &&
     m_State.timestamp + DVD_MSEC_TO_TIME(timeout) > m_clock.GetAbsoluteClock())
    return;

  SPlayerState state(m_State);

  state.dts = DVD_NOPTS_VALUE;
  if (m_CurrentVideo.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentVideo.dts;
  else if (m_CurrentAudio.dts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentAudio.dts;
  else if (m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentVideo.startpts;
  else if (m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
    state.dts = m_CurrentAudio.startpts;

  state.startTime = 0;
  state.timeMin = 0;

  if (m_pDemuxer)
  {
    if (IsInMenuInternal())
      state.chapter = 0;
    else
      state.chapter = m_pDemuxer->GetChapter();

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

    state.time = DVD_TIME_TO_MSEC(m_clock.GetClock(false));
    state.timeMax = m_pDemuxer->GetStreamLength();
  }

  state.canpause = true;
  state.canseek = true;
  state.isInMenu = false;
  state.hasMenu = false;

  if (m_pInputStream)
  {
    // override from input stream if needed
    CDVDInputStreamPVRManager* pvrStream = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
    if (pvrStream)
    {
      state.canrecord = pvrStream->CanRecord();
      state.recording = pvrStream->IsRecording();
    }

    CDVDInputStream::ITimes* pTimes = m_pInputStream->GetITimes();
    CDVDInputStream::IDisplayTime* pDisplayTime = m_pInputStream->GetIDisplayTime();

    CDVDInputStream::ITimes::Times times;
    if (pTimes && pTimes->GetTimes(times))
    {
      state.startTime = times.startTime;
      state.time = (m_clock.GetClock(false) - times.ptsStart) * 1000 / DVD_TIME_BASE;
      state.timeMax = (times.ptsEnd - times.ptsStart) * 1000 / DVD_TIME_BASE;
      state.timeMin = (times.ptsBegin - times.ptsStart) * 1000 / DVD_TIME_BASE;
      state.time_offset = 0;
    }
    else if (pDisplayTime && pDisplayTime->GetTotalTime() > 0)
    {
      if (state.dts != DVD_NOPTS_VALUE)
      {
        int dispTime = 0;
        if (m_CurrentVideo.id >= 0 && m_CurrentVideo.dispTime)
          dispTime = m_CurrentVideo.dispTime;
        else if (m_CurrentAudio.dispTime)
          dispTime = m_CurrentAudio.dispTime;

        state.time_offset = DVD_MSEC_TO_TIME(dispTime) - state.dts;
      }
      state.time += DVD_TIME_TO_MSEC(state.time_offset);
      state.timeMax = pDisplayTime->GetTotalTime();
    }
    else
    {
      state.time_offset = 0;
    }

    if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
    {
      if (!ptr->GetState(state.player_state))
        state.player_state = "";

      if (m_dvd.state == DVDSTATE_STILL)
      {
        state.time = XbmcThreads::SystemClockMillis() - m_dvd.iDVDStillStartTime;
        state.timeMax = m_dvd.iDVDStillTime;
        state.isInMenu = true;
      }
      else if (IsInMenuInternal())
      {
        state.time = pDisplayTime->GetTime();
        state.time_offset = 0;
        state.isInMenu = true;
      }
      state.hasMenu = true;
    }

    state.canpause = m_pInputStream->CanPause();
    state.canseek = m_pInputStream->CanSeek();
  }

  if (m_Edl.HasCut())
  {
    state.time        = (double) m_Edl.RemoveCutTime(llrint(state.time));
    state.timeMax  = (double) m_Edl.RemoveCutTime(llrint(state.timeMax));
  }

  if (state.timeMax <= 0)
    state.canseek  = false;

  if (m_caching > CACHESTATE_DONE && m_caching < CACHESTATE_PLAY)
    state.caching = true;
  else
    state.caching = false;

  double level, delay, offset;
  if (GetCachingTimes(level, delay, offset))
  {
    state.cache_delay  = std::max(0.0, delay);
    state.cache_level  = std::max(0.0, std::min(1.0, level));
    state.cache_offset = offset;
  }
  else
  {
    state.cache_delay  = 0.0;
    state.cache_level  = std::min(1.0, GetQueueTime() / 8000.0);
    state.cache_offset = GetQueueTime() / state.timeMax;
  }

  XFILE::SCacheStatus status;
  if (m_pInputStream && m_pInputStream->GetCacheStatus(&status))
  {
    state.cache_bytes = status.forward;
    if(state.timeMax)
      state.cache_bytes += m_pInputStream->GetLength() * (int64_t) (GetQueueTime() / state.timeMax);
  }
  else
    state.cache_bytes = 0;

  state.timestamp = m_clock.GetAbsoluteClock();

  m_processInfo->SetPlayTimes(state.startTime, state.time, state.timeMin, state.timeMax);
  
  CSingleLock lock(m_StateSection);
  m_State = state;
}

int64_t CVideoPlayer::GetUpdatedTime()
{
  UpdatePlayState(0);
  return llrint(m_State.time);
}

void CVideoPlayer::SetVolume(float nVolume)
{
  if (m_omxplayer_mode)
    m_VideoPlayerAudio->SetVolume(nVolume);
}

void CVideoPlayer::SetMute(bool bOnOff)
{
  if (m_omxplayer_mode)
    m_VideoPlayerAudio->SetMute(bOnOff);
}

void CVideoPlayer::SetDynamicRangeCompression(long drc)
{
  m_VideoPlayerAudio->SetDynamicRangeCompression(drc);
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
      if (subs[i].filename == m_item.GetPath())
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
      const CStreamDetailVideo* s1 = static_cast<const CStreamDetailVideo*>(details.GetNthStream(CStreamDetail::VIDEO,0));
      CStreamDetailVideo* stream = const_cast<CStreamDetailVideo*>(s1);
      float aspect = m_renderManager.GetAspectRatio();
      if (aspect > 0.0f)
        stream->m_fAspect = aspect;

      int64_t duration = m_processInfo->GetMaxTime() / 1000;
      if (duration > 0)
        stream->m_iDuration = (int) duration;
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

void CVideoPlayer::FrameMove()
{
  m_renderManager.FrameMove();
}

void CVideoPlayer::Render(bool clear, uint32_t alpha, bool gui)
{
  m_renderManager.Render(clear, 0, alpha, gui);
}

void CVideoPlayer::FlushRenderer()
{
  m_renderManager.Flush(true);
}

void CVideoPlayer::SetRenderViewMode(int mode)
{
  m_renderManager.SetViewMode(mode);
}

float CVideoPlayer::GetRenderAspectRatio()
{
  return m_renderManager.GetAspectRatio();
}

void CVideoPlayer::TriggerUpdateResolution()
{
  m_renderManager.TriggerUpdateResolution(0, 0, 0);
}

bool CVideoPlayer::IsRenderingVideo()
{
  return m_renderManager.IsConfigured();
}

bool CVideoPlayer::Supports(EINTERLACEMETHOD method)
{
  if (!m_processInfo)
    return false;
  return m_processInfo->Supports(method);
}

EINTERLACEMETHOD CVideoPlayer::GetDeinterlacingMethodDefault()
{
  if (!m_processInfo)
    return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE;
  return m_processInfo->GetDeinterlacingMethodDefault();
}

bool CVideoPlayer::Supports(ESCALINGMETHOD method)
{
  return m_renderManager.Supports(method);
}

bool CVideoPlayer::Supports(ERENDERFEATURE feature)
{
  return m_renderManager.Supports(feature);
}

unsigned int CVideoPlayer::RenderCaptureAlloc()
{
  return m_renderManager.AllocRenderCapture();
}

void CVideoPlayer::RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags)
{
  m_renderManager.StartRenderCapture(captureId, width, height, flags);
}

void CVideoPlayer::RenderCaptureRelease(unsigned int captureId)
{
  m_renderManager.ReleaseRenderCapture(captureId);
}

bool CVideoPlayer::RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size)
{
  return m_renderManager.RenderCaptureGetPixels(captureId, millis, buffer, size);
}

void CVideoPlayer::VideoParamsChange()
{
  m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_AVCHANGE));
}

void CVideoPlayer::GetDebugInfo(std::string &audio, std::string &video, std::string &general)
{
  audio = m_VideoPlayerAudio->GetPlayerInfo();
  video = m_VideoPlayerVideo->GetPlayerInfo();
  GetGeneralInfo(general);
}

void CVideoPlayer::UpdateClockSync(bool enabled)
{
  m_processInfo->SetRenderClockSync(enabled);
}

void CVideoPlayer::UpdateRenderInfo(CRenderInfo &info)
{
  m_processInfo->UpdateRenderInfo(info);
}

void CVideoPlayer::UpdateRenderBuffers(int queued, int discard, int free)
{
  m_processInfo->UpdateRenderBuffers(queued, discard, free);
}

void CVideoPlayer::UpdateGuiRender(bool gui)
{
  m_processInfo->SetGuiRender(gui);
}

void CVideoPlayer::UpdateVideoRender(bool video)
{
  m_processInfo->SetVideoRender(video);
}

// IDispResource interface
void CVideoPlayer::OnLostDisplay()
{
  CLog::Log(LOGNOTICE, "VideoPlayer: OnLostDisplay received");
  m_VideoPlayerAudio->SendMessage(new CDVDMsgBool(CDVDMsg::GENERAL_PAUSE, true), 1);
  m_VideoPlayerVideo->SendMessage(new CDVDMsgBool(CDVDMsg::GENERAL_PAUSE, true), 1);
  m_clock.Pause(true);
  m_displayLost = true;
}

void CVideoPlayer::OnResetDisplay()
{
  CLog::Log(LOGNOTICE, "VideoPlayer: OnResetDisplay received");
  m_VideoPlayerAudio->SendMessage(new CDVDMsgBool(CDVDMsg::GENERAL_PAUSE, false), 1);
  m_VideoPlayerVideo->SendMessage(new CDVDMsgBool(CDVDMsg::GENERAL_PAUSE, false), 1);
  m_clock.Pause(false);
  m_displayLost = false;
}
