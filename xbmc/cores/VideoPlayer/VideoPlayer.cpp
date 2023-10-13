/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayer.h"

#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxCC.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#if defined(HAVE_LIBBLURAY)
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/InputStreamPVRBase.h"
#include "DVDMessage.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "VideoPlayerAudio.h"
#include "VideoPlayerRadioRDS.h"
#include "VideoPlayerVideo.h"
#include "application/Application.h"
#include "cores/DataCacheCore.h"
#include "cores/EdlEdit.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "utils/FontUtils.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StreamDetails.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoInfoTag.h"
#include "windowing/WinSystem.h"

#include <iterator>
#include <memory>
#include <mutex>
#include <utility>

using namespace std::chrono_literals;

//------------------------------------------------------------------------------
// selection streams
//------------------------------------------------------------------------------

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
  int currentSubStream;
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
  explicit PredicateSubtitleFilter(const std::string& lang, int subStream)
  : audiolang(lang),
    currentSubStream(subStream)
  {
    const std::string subtitleLang = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);
    original = StringUtils::EqualsNoCase(subtitleLang, "original");
    nosub = StringUtils::EqualsNoCase(subtitleLang, "none");
    onlyforced = StringUtils::EqualsNoCase(subtitleLang, "forced_only");
  };

  bool operator()(const SelectionStream& ss) const
  {
    if (ss.type_index == currentSubStream)
      return false;

    if (nosub)
      return true;

    if (onlyforced)
    {
      if ((ss.flags & StreamFlags::FLAG_FORCED) && g_LangCodeExpander.CompareISO639Codes(ss.language, audiolang))
        return false;
      else
        return true;
    }

    if(STREAM_SOURCE_MASK(ss.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(ss.source) == STREAM_SOURCE_TEXT)
      return false;

    if ((ss.flags & StreamFlags::FLAG_FORCED) && g_LangCodeExpander.CompareISO639Codes(ss.language, audiolang))
      return false;

    if ((ss.flags & StreamFlags::FLAG_FORCED) && (ss.flags & StreamFlags::FLAG_DEFAULT))
      return false;

    if (ss.language == "cc" && ss.flags & StreamFlags::FLAG_HEARING_IMPAIRED)
      return false;

    if(!original)
    {
      std::string subtitle_language = g_langInfo.GetSubtitleLanguage();
      if (g_LangCodeExpander.CompareISO639Codes(subtitle_language, ss.language))
        return false;
    }
    else if (ss.flags & StreamFlags::FLAG_DEFAULT)
      return false;

    return true;
  }
};

class PredicateAudioFilter
{
private:
  int currentAudioStream;
  bool preferStereo;
public:
  explicit PredicateAudioFilter(int audioStream, bool preferStereo)
    : currentAudioStream(audioStream)
    , preferStereo(preferStereo)
  {
  };
  bool operator()(const SelectionStream& lh, const SelectionStream& rh)
  {
    PREDICATE_RETURN(lh.type_index == currentAudioStream
                     , rh.type_index == currentAudioStream);

    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    if (!StringUtils::EqualsNoCase(settings->GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE), "mediadefault"))
    {
      if (!StringUtils::EqualsNoCase(settings->GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE), "original"))
      {
        std::string audio_language = g_langInfo.GetAudioLanguage();
        PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(audio_language, lh.language)
          , g_LangCodeExpander.CompareISO639Codes(audio_language, rh.language));
      }
      else
      {
        PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_ORIGINAL,
          rh.flags & StreamFlags::FLAG_ORIGINAL);
      }

      bool hearingimp = settings->GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING);
      PREDICATE_RETURN(!hearingimp ? !(lh.flags & StreamFlags::FLAG_HEARING_IMPAIRED) : lh.flags & StreamFlags::FLAG_HEARING_IMPAIRED
                       , !hearingimp ? !(rh.flags & StreamFlags::FLAG_HEARING_IMPAIRED) : rh.flags & StreamFlags::FLAG_HEARING_IMPAIRED);

      bool visualimp = settings->GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL);
      PREDICATE_RETURN(!visualimp ? !(lh.flags & StreamFlags::FLAG_VISUAL_IMPAIRED) : lh.flags & StreamFlags::FLAG_VISUAL_IMPAIRED
                       , !visualimp ? !(rh.flags & StreamFlags::FLAG_VISUAL_IMPAIRED) : rh.flags & StreamFlags::FLAG_VISUAL_IMPAIRED);
    }

    if (settings->GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG))
    {
      PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_DEFAULT,
                       rh.flags & StreamFlags::FLAG_DEFAULT);
    }

    if (preferStereo)
      PREDICATE_RETURN(lh.channels == 2,
                       rh.channels == 2);
    else
      PREDICATE_RETURN(lh.channels,
                       rh.channels);

    PREDICATE_RETURN(StreamUtils::GetCodecPriority(lh.codec),
                     StreamUtils::GetCodecPriority(rh.codec));

    PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_DEFAULT,
                     rh.flags & StreamFlags::FLAG_DEFAULT);
    return false;
  };
};

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
  int subStream;
public:
  explicit PredicateSubtitlePriority(const std::string& lang, int stream, bool ison)
  : audiolang(lang),
    original(StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE), "original")),
    subson(ison),
    filter(lang, stream),
    subStream(stream)
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

    PREDICATE_RETURN(lh.type_index == subStream
                   , rh.type_index == subStream);

    // prefer external subs
    PREDICATE_RETURN(STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_TEXT
                   , STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_TEXT);

    if (!subson || original)
    {
      PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , rh.flags & StreamFlags::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_DEFAULT && g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , rh.flags & StreamFlags::FLAG_DEFAULT && g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(lh.language, audiolang)
                     , g_LangCodeExpander.CompareISO639Codes(rh.language, audiolang));

      PREDICATE_RETURN((lh.flags & (StreamFlags::FLAG_FORCED | StreamFlags::FLAG_DEFAULT)) == (StreamFlags::FLAG_FORCED | StreamFlags::FLAG_DEFAULT)
                     , (rh.flags & (StreamFlags::FLAG_FORCED | StreamFlags::FLAG_DEFAULT)) == (StreamFlags::FLAG_FORCED | StreamFlags::FLAG_DEFAULT));

    }

    std::string subtitle_language = g_langInfo.GetSubtitleLanguage();
    if (!original)
    {
      PREDICATE_RETURN((STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(lh.source) == STREAM_SOURCE_TEXT) && g_LangCodeExpander.CompareISO639Codes(subtitle_language, lh.language)
                     , (STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_DEMUX_SUB || STREAM_SOURCE_MASK(rh.source) == STREAM_SOURCE_TEXT) && g_LangCodeExpander.CompareISO639Codes(subtitle_language, rh.language));
    }

    if (!original)
    {
      PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(subtitle_language, lh.language)
                     , g_LangCodeExpander.CompareISO639Codes(subtitle_language, rh.language));

      bool hearingimp = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_ACCESSIBILITY_SUBHEARING);
      PREDICATE_RETURN(!hearingimp ? !(lh.flags & StreamFlags::FLAG_HEARING_IMPAIRED) : lh.flags & StreamFlags::FLAG_HEARING_IMPAIRED
                     , !hearingimp ? !(rh.flags & StreamFlags::FLAG_HEARING_IMPAIRED) : rh.flags & StreamFlags::FLAG_HEARING_IMPAIRED);
    }

    PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_DEFAULT
                   , rh.flags & StreamFlags::FLAG_DEFAULT);

    return false;
  }
};

class PredicateVideoFilter
{
private:
  int currentVideoStream;
public:
  explicit PredicateVideoFilter(int videoStream) : currentVideoStream(videoStream)
  {
  };
  bool operator()(const SelectionStream& lh, const SelectionStream& rh)
  {
    PREDICATE_RETURN(lh.type_index == currentVideoStream,
                     rh.type_index == currentVideoStream);

    PREDICATE_RETURN(lh.flags & StreamFlags::FLAG_DEFAULT,
                     rh.flags & StreamFlags::FLAG_DEFAULT);
    return false;
  }
};

void CSelectionStreams::Clear(StreamType type, StreamSource source)
{
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
  return const_cast<SelectionStream&>(std::as_const(*this).Get(type, index));
}

const SelectionStream& CSelectionStreams::Get(StreamType type, int index) const
{
  int count = -1;
  for (size_t i = 0; i < m_Streams.size(); ++i)
  {
    if (m_Streams[i].type != type)
      continue;
    count++;
    if (count == index)
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

bool CSelectionStreams::Get(StreamType type, StreamFlags flag, SelectionStream& out)
{
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

int CSelectionStreams::TypeIndexOf(StreamType type, int source, int64_t demuxerId, int id) const
{
  if (id < 0)
    return -1;

  auto it = std::find_if(m_Streams.begin(), m_Streams.end(),
    [&](const SelectionStream& stream) {return stream.type == type
    && stream.source == source && stream.id == id
    && stream.demuxerId == demuxerId;});

  if (it != m_Streams.end())
    return it->type_index;
  else
    return -1;
}

int CSelectionStreams::Source(StreamSource source, const std::string& filename)
{
  int index = source - 1;
  for (size_t i=0; i<m_Streams.size(); i++)
  {
    SelectionStream &s = m_Streams[i];
    if (STREAM_SOURCE_MASK(s.source) != source)
      continue;
    // if it already exists, return same
    if (s.filename == filename)
      return s.source;
    if (index < s.source)
      index = s.source;
  }
  // return next index
  return index + 1;
}

void CSelectionStreams::Update(SelectionStream& s)
{
  int index = TypeIndexOf(s.type, s.source, s.demuxerId, s.id);
  if(index >= 0)
  {
    SelectionStream& o = Get(s.type, index);
    s.type_index = o.type_index;
    o = s;
  }
  else
  {
    s.type_index = CountType(s.type);
    m_Streams.push_back(s);
  }
}

void CSelectionStreams::Update(const std::shared_ptr<CDVDInputStream>& input,
                               CDVDDemux* demuxer,
                               const std::string& filename2)
{
  if(input && input->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    std::shared_ptr<CDVDInputStreamNavigator> nav = std::static_pointer_cast<CDVDInputStreamNavigator>(input);
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
      s.flags    = StreamFlags::FLAG_NONE;
      s.filename = filename;

      AudioStreamInfo info = nav->GetAudioStreamInfo(i);
      s.name     = info.name;
      s.codec    = info.codecName;
      s.language = g_LangCodeExpander.ConvertToISO6392B(info.language);
      s.channels = info.channels;
      s.flags = info.flags;
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

      SubtitleStreamInfo info = nav->GetSubtitleStreamInfo(i);
      s.name     = info.name;
      s.flags = info.flags;
      s.language = g_LangCodeExpander.ConvertToISO6392B(info.language);
      Update(s);
    }

    VideoStreamInfo info = nav->GetVideoStreamInfo();
    for (int i = 1; i <= info.angles; i++)
    {
      SelectionStream s;
      s.source = source;
      s.type = STREAM_VIDEO;
      s.id = i;
      s.flags = StreamFlags::FLAG_NONE;
      s.filename = filename;
      s.channels = 0;
      s.aspect_ratio = info.videoAspectRatio;
      s.width = info.width;
      s.height = info.height;
      s.codec = info.codecName;
      s.name = StringUtils::Format("{} {}", g_localizeStrings.Get(38032), i);
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
        s.aspect_ratio = vstream->fAspect;
        s.stereo_mode = vstream->stereo_mode;
        s.bitrate = vstream->iBitRate;
        s.hdrType = vstream->hdr_type;
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
  CServiceBroker::GetDataCacheCore().SignalSubtitleInfoChange();
}

void CSelectionStreams::Update(const std::shared_ptr<CDVDInputStream>& input, CDVDDemux* demuxer)
{
  Update(input, demuxer, "");
}

int CSelectionStreams::CountTypeOfSource(StreamType type, StreamSource source) const
{
  return std::count_if(m_Streams.begin(), m_Streams.end(),
    [&](const SelectionStream& stream) {return (stream.type == type) && (stream.source == source);});
}

int CSelectionStreams::CountType(StreamType type) const
{
  return std::count_if(m_Streams.begin(), m_Streams.end(),
                       [&](const SelectionStream& stream) { return stream.type == type; });
}

//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

void CVideoPlayer::CreatePlayers()
{
  if (m_players_created)
    return;

  m_VideoPlayerVideo = new CVideoPlayerVideo(&m_clock, &m_overlayContainer, m_messenger, m_renderManager, *m_processInfo);
  m_VideoPlayerAudio = new CVideoPlayerAudio(&m_clock, m_messenger, *m_processInfo);
  m_VideoPlayerSubtitle = new CVideoPlayerSubtitle(&m_overlayContainer, *m_processInfo);
  m_VideoPlayerTeletext = new CDVDTeletextData(*m_processInfo);
  m_VideoPlayerRadioRDS = new CDVDRadioRDSData(*m_processInfo);
  m_VideoPlayerAudioID3 = std::make_unique<CVideoPlayerAudioID3>(*m_processInfo);
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
  m_VideoPlayerAudioID3.reset();

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
    m_CurrentAudioID3(STREAM_AUDIO_ID3, VideoPlayer_ID3),
    m_messenger("player"),
    m_renderManager(m_clock, this)
{
  m_outboundEvents = std::make_unique<CJobQueue>(false, 1, CJob::PRIORITY_NORMAL);
  m_players_created = false;
  m_pDemuxer = nullptr;
  m_pSubtitleDemuxer = nullptr;
  m_pCCDemuxer = nullptr;
  m_pInputStream = nullptr;

  m_dvd.Clear();
  m_State.Clear();

  m_bAbortRequest = false;
  m_offset_pts = 0.0;
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_streamPlayerSpeed = DVD_PLAYSPEED_NORMAL;
  m_caching = CACHESTATE_DONE;
  m_HasVideo = false;
  m_HasAudio = false;
  m_UpdateStreamDetails = false;

  memset(&m_SpeedState, 0, sizeof(m_SpeedState));

  m_SkipCommercials = true;

  m_processInfo.reset(CProcessInfo::CreateInstance());
  // if we have a gui, register the cache
  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
  m_processInfo->SetSpeed(1.0);
  m_processInfo->SetTempo(1.0);
  m_processInfo->SetFrameAdvance(false);

  CreatePlayers();

  m_displayLost = false;
  m_error = false;
  m_bCloseRequest = false;
  CServiceBroker::GetWinSystem()->Register(this);
}

CVideoPlayer::~CVideoPlayer()
{
  CServiceBroker::GetWinSystem()->Unregister(this);

  CloseFile();
  DestroyPlayers();

  while (m_outboundEvents->IsProcessing())
  {
    CThread::Sleep(10ms);
  }
}

bool CVideoPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  CLog::Log(LOGINFO, "VideoPlayer::OpenFile: {}", CURL::GetRedacted(file.GetPath()));

  if (IsRunning())
  {
    CDVDMsgOpenFile::FileParams params;
    params.m_item = file;
    params.m_options = options;
    params.m_item.SetMimeTypeForInternetFile();
    m_messenger.Put(std::make_shared<CDVDMsgOpenFile>(params), 1);

    return true;
  }

  m_item = file;
  m_playerOptions = options;

  m_processInfo->SetPlayTimes(0,0,0,0);
  m_bAbortRequest = false;
  m_error = false;
  m_bCloseRequest = false;
  m_renderManager.PreInit();

  Create();
  m_messenger.Init();

  m_callback.OnPlayBackStarted(m_item);

  return true;
}

bool CVideoPlayer::CloseFile(bool reopen)
{
  CLog::Log(LOGINFO, "CVideoPlayer::CloseFile()");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;
  m_bCloseRequest = true;

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  if(m_pSubtitleDemuxer)
    m_pSubtitleDemuxer->Abort();

  if(m_pInputStream)
    m_pInputStream->Abort();

  m_renderManager.UnInit();

  CLog::Log(LOGINFO, "VideoPlayer: waiting for threads to exit");

  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  {
    CSingleExit exitlock(CServiceBroker::GetWinSystem()->GetGfxContext());
    StopThread();
  }

  m_Edl.Clear();
  CServiceBroker::GetDataCacheCore().SetEditList(m_Edl.GetEditList());
  CServiceBroker::GetDataCacheCore().SetCuts(m_Edl.GetCutMarkers());
  CServiceBroker::GetDataCacheCore().SetSceneMarkers(m_Edl.GetSceneMarkers());

  m_HasVideo = false;
  m_HasAudio = false;

  CLog::Log(LOGINFO, "VideoPlayer: finished waiting");
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
  m_CurrentAudioID3.Clear();

  UTILS::FONT::ClearTemporaryFonts();
}

bool CVideoPlayer::OpenInputStream()
{
  if (m_pInputStream.use_count() > 1)
    throw std::runtime_error("m_pInputStream reference count is greater than 1");
  m_pInputStream.reset();

  CLog::Log(LOGINFO, "Creating InputStream");

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_item, true);
  if (m_pInputStream == nullptr)
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - unable to create input stream for [{}]",
              CURL::GetRedacted(m_item.GetPath()));
    return false;
  }

  if (!m_pInputStream->Open())
  {
    CLog::Log(LOGERROR, "CVideoPlayer::OpenInputStream - error opening [{}]",
              CURL::GetRedacted(m_item.GetPath()));
    return false;
  }

  // find any available external subtitles for non dvd files
  if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) &&
      !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    // find any available external subtitles
    std::vector<std::string> filenames;
    CUtil::ScanForExternalSubtitles(m_item.GetDynPath(), filenames);

    // load any subtitles from file item
    std::string key("subtitle:1");
    for (unsigned s = 1; m_item.HasProperty(key); key = StringUtils::Format("subtitle:{}", ++s))
      filenames.push_back(m_item.GetProperty(key).asString());

    for (unsigned int i=0;i<filenames.size();i++)
    {
      // if vobsub subtitle:
      if (URIUtils::HasExtension(filenames[i], ".idx"))
      {
        std::string strSubFile;
        if (CUtil::FindVobSubPair( filenames, filenames[i], strSubFile))
          AddSubtitleFile(filenames[i], strSubFile);
      }
      else
      {
        if (!CUtil::IsVobSub(filenames, filenames[i] ))
        {
          AddSubtitleFile(filenames[i]);
        }
      }
    } // end loop over all subtitle files
  }

  m_clock.Reset();
  m_dvd.Clear();

  return true;
}

bool CVideoPlayer::OpenDemuxStream()
{
  CloseDemuxer();

  CLog::Log(LOGINFO, "Creating Demuxer");

  int attempts = 10;
  while (!m_bStop && attempts-- > 0)
  {
    m_pDemuxer.reset(CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream));
    if(!m_pDemuxer && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
    {
      continue;
    }
    else if(!m_pDemuxer && m_pInputStream->NextStream() != CDVDInputStream::NEXTSTREAM_NONE)
    {
      CLog::Log(LOGDEBUG, "{} - New stream available from input, retry open", __FUNCTION__);
      continue;
    }
    break;
  }

  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, "{} - Error creating demuxer", __FUNCTION__);
    return false;
  }

  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer.get());
  m_pDemuxer->GetPrograms(m_programs);
  UpdateContent();
  m_demuxerSpeed = DVD_PLAYSPEED_NORMAL;
  m_processInfo->SetStateRealtime(false);

  int64_t len = m_pInputStream->GetLength();
  int64_t tim = m_pDemuxer->GetStreamLength();
  if (len > 0 && tim > 0)
    m_pInputStream->SetReadRate(static_cast<uint32_t>(len * 1000 / tim));

  m_offset_pts = 0;

  return true;
}

void CVideoPlayer::CloseDemuxer()
{
  m_pDemuxer.reset();
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);

  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
  CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
  CServiceBroker::GetDataCacheCore().SignalSubtitleInfoChange();
}

void CVideoPlayer::OpenDefaultStreams(bool reset)
{
  // if input stream dictate, we will open later
  if (m_dvd.iSelectedAudioStream >= 0 ||
      m_dvd.iSelectedSPUStream >= 0)
    return;

  bool valid;

  // open video stream
  valid   = false;

  PredicateVideoFilter vf(m_processInfo->GetVideoSettings().m_VideoStream);
  for (const auto &stream : m_SelectionStreams.Get(STREAM_VIDEO, vf))
  {
    if (OpenStream(m_CurrentVideo, stream.demuxerId, stream.id, stream.source, reset))
    {
      valid = true;
      break;
    }
  }
  if (!valid)
  {
    CloseStream(m_CurrentVideo, true);
    m_processInfo->ResetVideoCodecInfo();
  }

  // open audio stream
  valid = false;
  if (!m_playerOptions.videoOnly)
  {
    PredicateAudioFilter af(m_processInfo->GetVideoSettings().m_AudioStream, m_playerOptions.preferStereo);
    for (const auto &stream : m_SelectionStreams.Get(STREAM_AUDIO, af))
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
  bool visible = m_processInfo->GetVideoSettings().m_SubtitleOn;

  // open subtitle stream
  SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
  PredicateSubtitlePriority psp(as.language,
                                m_processInfo->GetVideoSettings().m_SubtitleStream,
                                m_processInfo->GetVideoSettings().m_SubtitleOn);
  valid = false;
  // We need to close CC subtitles to avoid conflicts with external sub stream
  if (m_CurrentSubtitle.source == STREAM_SOURCE_VIDEOMUX)
    CloseStream(m_CurrentSubtitle, false);

  for (const auto &stream : m_SelectionStreams.Get(STREAM_SUBTITLE, psp))
  {
    if (OpenStream(m_CurrentSubtitle, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      if(!psp.relevant(stream))
        visible = false;
      else if(stream.flags & StreamFlags::FLAG_FORCED)
        visible = true;
      break;
    }
  }
  if(!valid)
    CloseStream(m_CurrentSubtitle, false);

  if (!std::dynamic_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream) || m_playerOptions.state.empty())
    SetSubtitleVisibleInternal(visible); // only set subtitle visibility if state not stored by dvd navigator, because navigator will restore it (if visible)

  // open teletext stream
  valid   = false;
  for (const auto &stream : m_SelectionStreams.Get(STREAM_TELETEXT))
  {
    if (OpenStream(m_CurrentTeletext, stream.demuxerId, stream.id, stream.source))
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
    if (OpenStream(m_CurrentRadioRDS, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      break;
    }
  }
  if(!valid)
    CloseStream(m_CurrentRadioRDS, false);

  // open ID3 stream
  valid = false;
  for (const auto& stream : m_SelectionStreams.Get(STREAM_AUDIO_ID3))
  {
    if (OpenStream(m_CurrentAudioID3, stream.demuxerId, stream.id, stream.source))
    {
      valid = true;
      break;
    }
  }
  if (!valid)
    CloseStream(m_CurrentAudioID3, false);

  // disable demux streams
  if (m_item.IsRemote() && m_pDemuxer)
  {
    for (auto &stream : m_SelectionStreams.m_Streams)
    {
      if (STREAM_SOURCE_MASK(stream.source) == STREAM_SOURCE_DEMUX)
      {
        if (stream.id != m_CurrentVideo.id && stream.id != m_CurrentAudio.id &&
            stream.id != m_CurrentSubtitle.id && stream.id != m_CurrentTeletext.id &&
            stream.id != m_CurrentRadioRDS.id && stream.id != m_CurrentAudioID3.id)
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
  if (m_pSubtitleDemuxer && m_VideoPlayerSubtitle->AcceptsData())
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
        CLog::Log(LOGERROR, "{} - Error demux packet doesn't belong to a valid stream",
                  __FUNCTION__);
        return false;
      }
      if (stream->source == STREAM_SOURCE_NONE)
      {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX_SUB);
        m_SelectionStreams.Update(NULL, m_pSubtitleDemuxer.get());
        UpdateContent();
      }
      return true;
    }
  }

  // read a data frame from stream.
  if (m_pDemuxer)
    packet = m_pDemuxer->Read();

  if (packet)
  {
    // stream changed, update and open defaults
    if (packet->iStreamId == DMX_SPECIALID_STREAMCHANGE)
    {
      m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
      m_SelectionStreams.Update(m_pInputStream, m_pDemuxer.get());
      m_pDemuxer->GetPrograms(m_programs);
      UpdateContent();
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
        CLog::Log(LOGERROR, "{} - Error demux packet doesn't belong to a valid stream",
                  __FUNCTION__);
        return false;
      }
      if(stream->source == STREAM_SOURCE_NONE)
      {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer.get());
        UpdateContent();
      }
    }
    return true;
  }
  return false;
}

bool CVideoPlayer::IsValidStream(const CCurrentStream& stream)
{
  if(stream.id<0)
    return true; // we consider non selected as valid

  int source = STREAM_SOURCE_MASK(stream.source);
  if(source == STREAM_SOURCE_TEXT)
    return true;
  if (source == STREAM_SOURCE_DEMUX_SUB)
  {
    CDemuxStream* st = m_pSubtitleDemuxer->GetStream(stream.demuxerId, stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;
    return true;
  }
  if (source == STREAM_SOURCE_DEMUX)
  {
    CDemuxStream* st = m_pDemuxer->GetStream(stream.demuxerId, stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;

    if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if (stream.type == STREAM_AUDIO && st->dvdNavId != m_dvd.iSelectedAudioStream)
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

bool CVideoPlayer::IsBetterStream(const CCurrentStream& current, CDemuxStream* stream)
{
  // Do not reopen non-video streams if we're in video-only mode
  if (m_playerOptions.videoOnly && current.type != STREAM_VIDEO)
    return false;

  if(stream->disabled)
    return false;

  if (m_pInputStream && (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) ||
                         m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY)))
  {
    int source_type;

    source_type = STREAM_SOURCE_MASK(current.source);
    if (source_type != STREAM_SOURCE_DEMUX &&
        source_type != STREAM_SOURCE_NONE)
      return false;

    source_type = STREAM_SOURCE_MASK(stream->source);
    if(source_type != STREAM_SOURCE_DEMUX ||
       stream->type != current.type ||
       stream->uniqueId == current.id)
      return false;

    if(current.type == STREAM_AUDIO && stream->dvdNavId == m_dvd.iSelectedAudioStream)
      return true;
    if(current.type == STREAM_SUBTITLE && stream->dvdNavId == m_dvd.iSelectedSPUStream)
      return true;
    if(current.type == STREAM_VIDEO && current.id < 0)
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
  m_processInfo->SetFrameAdvance(false);
  m_State.Clear();
  m_CurrentVideo.hint.Clear();
  m_CurrentAudio.hint.Clear();
  m_CurrentSubtitle.hint.Clear();
  m_CurrentTeletext.hint.Clear();
  m_CurrentRadioRDS.hint.Clear();
  m_CurrentAudioID3.hint.Clear();
  memset(&m_SpeedState, 0, sizeof(m_SpeedState));
  m_offset_pts = 0;
  m_CurrentAudio.lastdts = DVD_NOPTS_VALUE;
  m_CurrentVideo.lastdts = DVD_NOPTS_VALUE;

  IPlayerCallback *cb = &m_callback;
  CFileItem fileItem = m_item;
  m_outboundEvents->Submit([=]() {
    cb->RequestVideoSettings(fileItem);
  });

  if (!OpenInputStream())
  {
    m_bAbortRequest = true;
    m_error = true;
    return;
  }

  bool discStateRestored = false;
  if (std::shared_ptr<CDVDInputStream::IMenus> ptr = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream))
  {
    CLog::Log(LOGINFO, "VideoPlayer: playing a file with menu's");

    if (!m_playerOptions.state.empty())
    {
      discStateRestored = ptr->SetState(m_playerOptions.state);
    }
    else if(std::shared_ptr<CDVDInputStreamNavigator> nav = std::dynamic_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream))
    {
      nav->EnableSubtitleStream(m_processInfo->GetVideoSettings().m_SubtitleOn);
    }
  }

  if (!OpenDemuxStream())
  {
    m_bAbortRequest = true;
    m_error = true;
    return;
  }
  // give players a chance to reconsider now codecs are known
  CreatePlayers();

  if (!discStateRestored)
    OpenDefaultStreams();

  /*
   * Check to see if the demuxer should start at something other than time 0. This will be the case
   * if there was a start time specified as part of the "Start from where last stopped" (aka
   * auto-resume) feature or if there is an EDL cut or commercial break that starts at time 0.
   */
  EDL::Edit edit;
  int starttime = 0;
  if (m_playerOptions.starttime > 0 || m_playerOptions.startpercent > 0)
  {
    if (m_playerOptions.startpercent > 0 && m_pDemuxer)
    {
      int playerStartTime = static_cast<int>((static_cast<double>(
          m_pDemuxer->GetStreamLength() * (m_playerOptions.startpercent / 100.0))));
      starttime = m_Edl.GetTimeAfterRestoringCuts(playerStartTime);
    }
    else
    {
      starttime = m_Edl.GetTimeAfterRestoringCuts(
          static_cast<int>(m_playerOptions.starttime * 1000)); // s to ms
    }
    CLog::Log(LOGDEBUG, "{} - Start position set to last stopped position: {}", __FUNCTION__,
              starttime);
  }
  else if (m_Edl.InEdit(starttime, &edit))
  {
    // save last edit times
    m_Edl.SetLastEditTime(edit.start);
    m_Edl.SetLastEditActionType(edit.action);

    if (edit.action == EDL::Action::CUT)
    {
      starttime = edit.end;
      CLog::Log(LOGDEBUG, "{} - Start position set to end of first cut: {}", __FUNCTION__,
                starttime);
    }
    else if (edit.action == EDL::Action::COMM_BREAK)
    {
      if (m_SkipCommercials)
      {
        starttime = edit.end;
        CLog::Log(LOGDEBUG, "{} - Start position set to end of first commercial break: {}",
                  __FUNCTION__, starttime);
      }

      const std::shared_ptr<CAdvancedSettings> advancedSettings =
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
      if (advancedSettings && advancedSettings->m_EdlDisplayCommbreakNotifications)
      {
        const std::string timeString =
            StringUtils::SecondsToTimeString(edit.end / 1000, TIME_FORMAT_MM_SS);
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011), timeString);
      }
    }
  }

  if (starttime > 0)
  {
    double startpts = DVD_NOPTS_VALUE;
    if (m_pDemuxer)
    {
      if (m_pDemuxer->SeekTime(starttime, true, &startpts))
      {
        FlushBuffers(starttime / 1000 * AV_TIME_BASE, true, true);
        CLog::Log(LOGDEBUG, "{} - starting demuxer from: {}", __FUNCTION__, starttime);
      }
      else
        CLog::Log(LOGDEBUG, "{} - failed to start demuxing from: {}", __FUNCTION__, starttime);
    }

    if (m_pSubtitleDemuxer)
    {
      if(m_pSubtitleDemuxer->SeekTime(starttime, true, &startpts))
        CLog::Log(LOGDEBUG, "{} - starting subtitle demuxer from: {}", __FUNCTION__, starttime);
      else
        CLog::Log(LOGDEBUG, "{} - failed to start subtitle demuxing from: {}", __FUNCTION__,
                  starttime);
    }

    m_clock.Discontinuity(DVD_MSEC_TO_TIME(starttime));
  }

  UpdatePlayState(0);

  SetCaching(CACHESTATE_FLUSH);
}

void CVideoPlayer::Process()
{
  // Try to resolve the correct mime type. This can take some time, for example if a requested
  // item is located at a slow/not reachable remote source. So, do mime type detection in vp worker
  // thread, not directly when initalizing the player to keep GUI responsible.
  m_item.SetMimeTypeForInternetFile();

  CServiceBroker::GetWinSystem()->RegisterRenderLoop(this);

  Prepare();

  while (!m_bAbortRequest)
  {
    // check display lost
    if (m_displayLost)
    {
      CThread::Sleep(50ms);
      continue;
    }

    // check if in an edit (cut or commercial break) that should be automatically skipped
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

    // tell demuxer if we want to fill buffers
    if (m_demuxerSpeed != DVD_PLAYSPEED_PAUSE)
    {
      int audioLevel = 90;
      int videoLevel = 90;
      bool fillBuffer = false;
      if (m_CurrentAudio.id >= 0)
        audioLevel = m_VideoPlayerAudio->GetLevel();
      if (m_CurrentVideo.id >= 0)
        videoLevel = m_processInfo->GetLevelVQ();
      if (videoLevel < 85 && audioLevel < 85)
      {
        fillBuffer = true;
      }
      if (m_pDemuxer)
        m_pDemuxer->FillBuffer(fillBuffer);
    }

    // if the queues are full, no need to read more
    if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
        (!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
    {
      if (m_playSpeed == DVD_PLAYSPEED_PAUSE &&
          m_demuxerSpeed != DVD_PLAYSPEED_PAUSE)
      {
        if (m_pDemuxer)
          m_pDemuxer->SetSpeed(DVD_PLAYSPEED_PAUSE);
        m_demuxerSpeed = DVD_PLAYSPEED_PAUSE;
      }
      CThread::Sleep(10ms);
      continue;
    }

    // adjust demuxer speed; some rtsp servers wants to know for i.e. ff
    // delay pause until queue is full
    if (m_playSpeed != DVD_PLAYSPEED_PAUSE &&
        m_demuxerSpeed != m_playSpeed)
    {
      if (m_pDemuxer)
        m_pDemuxer->SetSpeed(m_playSpeed);
      m_demuxerSpeed = m_playSpeed;
    }

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
      if (std::shared_ptr<CDVDInputStream::IMenus> pStream = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream))
      {
        // stills will be skipped
        if(m_dvd.state == DVDSTATE_STILL)
        {
          if (m_dvd.iDVDStillTime > 0ms)
          {
            const auto now = std::chrono::steady_clock::now();
            const auto duration = now - m_dvd.iDVDStillStartTime;

            if (duration >= m_dvd.iDVDStillTime)
            {
              m_dvd.iDVDStillTime = 0ms;
              m_dvd.iDVDStillStartTime = {};
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
        CLog::Log(LOGINFO, "VideoPlayer: next stream, wait for old streams to be finished");
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
        CThread::Sleep(100ms);
        continue;
      }

      if (m_CurrentVideo.inited)
      {
        m_VideoPlayerVideo->SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::VIDEO_DRAIN));
      }

      m_CurrentAudio.inited = false;
      m_CurrentVideo.inited = false;
      m_CurrentSubtitle.inited = false;
      m_CurrentTeletext.inited = false;
      m_CurrentRadioRDS.inited = false;
      m_CurrentAudioID3.inited = false;

      // if we are caching, start playing it again
      SetCaching(CACHESTATE_DONE);

      // while players are still playing, keep going to allow seekbacks
      if (m_VideoPlayerAudio->HasData() ||
          m_VideoPlayerVideo->HasData())
      {
        CThread::Sleep(100ms);
        continue;
      }

      if (!m_pInputStream->IsEOF())
        CLog::Log(LOGINFO, "{} - eof reading from demuxer", __FUNCTION__);

      break;
    }

    // see if we can find something better to play
    CheckBetterStream(m_CurrentAudio,    pStream);
    CheckBetterStream(m_CurrentVideo,    pStream);
    CheckBetterStream(m_CurrentSubtitle, pStream);
    CheckBetterStream(m_CurrentTeletext, pStream);
    CheckBetterStream(m_CurrentRadioRDS, pStream);
    CheckBetterStream(m_CurrentAudioID3, pStream);

    // demux video stream
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SUBTITLES_PARSECAPTIONS) && CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
    {
      if (m_pCCDemuxer)
      {
        bool first = true;
        while (!m_bAbortRequest)
        {
          DemuxPacket *pkt = m_pCCDemuxer->Read(first ? pPacket : NULL);
          if (!pkt)
            break;

          first = false;
          if (m_pCCDemuxer->GetNrOfStreams() != m_SelectionStreams.CountTypeOfSource(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX))
          {
            m_SelectionStreams.Clear(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX);
            m_SelectionStreams.Update(NULL, m_pCCDemuxer.get(), "");
            UpdateContent();
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
      if (std::shared_ptr<CDVDInputStream::IMenus> menu = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream))
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
  }
}

bool CVideoPlayer::CheckIsCurrent(const CCurrentStream& current,
                                  CDemuxStream* stream,
                                  DemuxPacket* pkg)
{
  if(current.id == pkg->iStreamId &&
     current.demuxerId == stream->demuxerId &&
     current.source == stream->source &&
     current.type == stream->type)
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
  else if (CheckIsCurrent(m_CurrentAudioID3, pStream, pPacket))
    ProcessAudioID3Data(pStream, pPacket);
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
      m_SelectionStreams.Update(m_pInputStream, m_pDemuxer.get());
      UpdateContent();
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
  EDL::Edit edit;
  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;
  else if (m_Edl.InEdit(DVD_TIME_TO_MSEC(m_CurrentAudio.dts + m_offset_pts), &edit) &&
           edit.action == EDL::Action::MUTE)
  {
    drop = true;
  }

  m_VideoPlayerAudio->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));

  if (!drop)
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

  m_VideoPlayerVideo->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));

  if (!drop)
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

  m_VideoPlayerSubtitle->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));

  if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    m_VideoPlayerSubtitle->UpdateOverlayInfo(std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream), LIBDVDNAV_BUTTON_NORMAL);
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

  m_VideoPlayerTeletext->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));
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

  m_VideoPlayerRadioRDS->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));
}

void CVideoPlayer::ProcessAudioID3Data(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  CheckStreamChanges(m_CurrentAudioID3, pStream);

  UpdateTimestamps(m_CurrentAudioID3, pPacket);

  bool drop = false;
  if (CheckPlayerInit(m_CurrentAudioID3))
    drop = true;

  if (CheckSceneSkip(m_CurrentAudioID3))
    drop = true;

  m_VideoPlayerAudioID3->SendMessage(std::make_shared<CDVDMsgDemuxerPacket>(pPacket, drop));
}

CacheInfo CVideoPlayer::GetCachingTimes()
{
  CacheInfo info{};

  if (!m_pInputStream || !m_pDemuxer)
    return info;

  XFILE::SCacheStatus status;
  if (!m_pInputStream->GetCacheStatus(&status))
    return info;

  const uint64_t& cached = status.forward;
  const uint32_t& currate = status.currate;
  const uint32_t& maxrate = status.maxrate;
  const uint32_t& lowrate = status.lowrate;

  int64_t length = m_pInputStream->GetLength();
  int64_t remain = length - m_pInputStream->Seek(0, SEEK_CUR);

  if (length <= 0 || remain < 0)
    return info;

  double queueTime = GetQueueTime();
  double play_sbp = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) / length;
  double queued = 1000.0 * queueTime / play_sbp;

  info.delay = 0.0;
  info.level = 0.0;
  info.offset = (cached + queued) / length;
  info.time = 0.0;
  info.valid = true;

  if (currate == 0)
    return info;

  double cache_sbp = 1.1 * (double)DVD_TIME_BASE / currate;          /* underestimate by 10 % */
  double play_left = play_sbp  * (remain + queued);                  /* time to play out all remaining bytes */
  double cache_left = cache_sbp * (remain - cached);                 /* time to cache the remaining bytes */
  double cache_need = std::max(0.0, remain - play_left / cache_sbp); /* bytes needed until play_left == cache_left */

  // estimated playback time of current cached bytes
  double cache_time = (static_cast<double>(cached) / currate) + (queueTime / 1000.0);

  info.delay = cache_left - play_left;
  info.time = cache_time;

  if (lowrate > 0)
  {
    // buffer is full & our read rate is too low
    CLog::Log(LOGDEBUG, "Readrate {} was too low with {} required", lowrate, maxrate);
    info.level = -1.0;
  }
  else
    info.level = (cached + queued) / (cache_need + queued);

  return info;
}

void CVideoPlayer::HandlePlaySpeed()
{
  const bool isInMenu = IsInMenuInternal();
  const bool tolerateStall =
      isInMenu || (m_CurrentVideo.hint.flags & StreamFlags::FLAG_STILL_IMAGES);

  if (tolerateStall && m_caching != CACHESTATE_DONE)
    SetCaching(CACHESTATE_DONE);

  if (m_caching == CACHESTATE_FULL)
  {
    CacheInfo cache = GetCachingTimes();
    if (cache.valid)
    {
      if (cache.level < 0.0)
      {
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(21454), g_localizeStrings.Get(21455));
        SetCaching(CACHESTATE_INIT);
      }
      // Note: Previously used cache.level >= 1 would keep video stalled
      // event after cache was full
      // Talk link: https://github.com/xbmc/xbmc/pull/23760
      if (cache.time > 8.0)
        SetCaching(CACHESTATE_INIT);
    }
    else
    {
      if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
          (!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
        SetCaching(CACHESTATE_INIT);
    }

    // if audio stream stalled, wait until demux queue filled 10%
    if (m_pInputStream->IsRealtime() &&
        (m_CurrentAudio.id < 0 || m_VideoPlayerAudio->GetLevel() > 10))
    {
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
    if (m_playSpeed == DVD_PLAYSPEED_NORMAL && !tolerateStall)
    {
      // take action if audio or video stream is stalled
      if (((m_VideoPlayerAudio->IsStalled() && m_CurrentAudio.inited) ||
           (m_VideoPlayerVideo->IsStalled() && m_CurrentVideo.inited)) &&
          m_syncTimer.IsTimePast())
      {
        if (m_pInputStream->IsRealtime())
        {
          if ((m_CurrentAudio.id >= 0 && m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
               m_VideoPlayerAudio->IsStalled()) ||
              (m_CurrentVideo.id >= 0 && m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
               m_processInfo->GetLevelVQ() == 0))
          {
            CLog::Log(LOGDEBUG, "Stream stalled, start buffering. Audio: {} - Video: {}",
                      m_VideoPlayerAudio->GetLevel(), m_processInfo->GetLevelVQ());

            if (m_VideoPlayerAudio->AcceptsData() && m_VideoPlayerVideo->AcceptsData())
              SetCaching(CACHESTATE_FULL);
            else
              FlushBuffers(DVD_NOPTS_VALUE, false, true);
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
            m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
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
                 (m_CurrentVideo.packets == 0 && m_CurrentAudio.packets > threshold) ||
                 (!m_VideoPlayerAudio->AcceptsData() && m_processInfo->GetLevelVQ() < 10);
    bool audio = m_CurrentAudio.id < 0 || (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
                 (m_CurrentAudio.packets == 0 && m_CurrentVideo.packets > threshold) ||
                 (!m_VideoPlayerVideo->AcceptsData() && m_VideoPlayerAudio->GetLevel() < 10);

    if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
        (m_CurrentAudio.avsync == CCurrentStream::AV_SYNC_CONT ||
         m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC))
    {
      m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerAudio->SendMessage(
          std::make_shared<CDVDMsgDouble>(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
    }
    else if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
             (m_CurrentVideo.avsync == CCurrentStream::AV_SYNC_CONT ||
             m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC))
    {
      m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerVideo->SendMessage(
          std::make_shared<CDVDMsgDouble>(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
    }
    else if (video && audio)
    {
      double clock = 0;
      if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Audio - pts: {:f}, cache: {:f}, totalcache: {:f}",
                  m_CurrentAudio.starttime, m_CurrentAudio.cachetime, m_CurrentAudio.cachetotal);
      if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
        CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Video - pts: {:f}, cache: {:f}, totalcache: {:f}",
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
          {
            clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
          }
          else if (m_CurrentVideo.starttime > m_CurrentAudio.starttime &&
                   !m_pInputStream->IsRealtime())
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

      m_clock.Discontinuity(clock);
      m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
      m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
      m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
      m_VideoPlayerAudio->SendMessage(
          std::make_shared<CDVDMsgDouble>(CDVDMsg::GENERAL_RESYNC, clock), 1);
      m_VideoPlayerVideo->SendMessage(
          std::make_shared<CDVDMsgDouble>(CDVDMsg::GENERAL_RESYNC, clock), 1);
      SetCaching(CACHESTATE_DONE);
      UpdatePlayState(0);

      m_syncTimer.Set(3000ms);

      if (!m_State.streamsReady)
      {
        if (m_playerOptions.fullscreen)
        {
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SWITCHTOFULLSCREEN);
        }

        IPlayerCallback *cb = &m_callback;
        CFileItem fileItem = m_item;
        m_outboundEvents->Submit([=]() {
          cb->OnAVStarted(fileItem);
        });
        m_State.streamsReady = true;
      }
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
            CLog::Log(LOGDEBUG, "CVideoPlayer::Process - Seeking to catch up, error was: {:f}",
                      error);
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
            m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
          }
        }
      }
    }
  }

  // reset tempo
  if (!m_State.cantempo)
  {
    float currentTempo = m_processInfo->GetNewTempo();
    if (currentTempo != 1.0f)
    {
      SetTempo(1.0f);
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
      CLog::Log(LOGDEBUG, "{} - dropping packet type:{} dts:{:f} to get to start point at {:f}",
                __FUNCTION__, current.player, current.dts, current.startpts);
      return true;
    }

    if ((current.startpts - current.dts) > DVD_SEC_TO_TIME(20))
    {
      CLog::Log(LOGDEBUG, "{} - too far to decode before finishing seek", __FUNCTION__);
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
      if (m_CurrentAudioID3.startpts != DVD_NOPTS_VALUE)
        m_CurrentAudioID3.startpts = current.dts;
    }

    if(current.dts < current.startpts)
    {
      CLog::Log(LOGDEBUG, "{} - dropping packet type:{} dts:{:f} to get to start point at {:f}",
                __FUNCTION__, current.player, current.dts, current.startpts);
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
  pkt->m_ptsOffsetCorrection = correction;

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
    CLog::Log(LOGDEBUG,
              "CVideoPlayer::CheckContinuity - resync forward :{}, prev:{:f}, curr:{:f}, diff:{:f}",
              current.type, current.dts, pPacket->dts, pPacket->dts - maxdts);
    correction = pPacket->dts - maxdts;
  }

  /* if it's large scale jump, correct for it after having confirmed the jump */
  if(pPacket->dts + DVD_MSEC_TO_TIME(500) < current.dts_end())
  {
    CLog::Log(
        LOGDEBUG,
        "CVideoPlayer::CheckContinuity - resync backward :{}, prev:{:f}, curr:{:f}, diff:{:f}",
        current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
    correction = pPacket->dts - current.dts_end();
  }
  else if(pPacket->dts < current.dts)
  {
    CLog::Log(LOGDEBUG,
              "CVideoPlayer::CheckContinuity - wrapback :{}, prev:{:f}, curr:{:f}, diff:{:f}",
              current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
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
      CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - update correction: {:f}", correction);
      if (current.avsync == CCurrentStream::AV_SYNC_CHECK)
        current.avsync = CCurrentStream::AV_SYNC_CONT;
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

bool CVideoPlayer::CheckSceneSkip(const CCurrentStream& current)
{
  if (!m_Edl.HasEdits())
    return false;

  if(current.dts == DVD_NOPTS_VALUE)
    return false;

  if(current.inited == false)
    return false;

  EDL::Edit edit;
  return m_Edl.InEdit(DVD_TIME_TO_MSEC(current.dts + m_offset_pts), &edit) &&
         edit.action == EDL::Action::CUT;
}

void CVideoPlayer::CheckAutoSceneSkip()
{
  if (!m_Edl.HasEdits())
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

  const double correctClock = m_Edl.GetTimeAfterRestoringCuts(clock);
  EDL::Edit edit;
  if (!m_Edl.InEdit(correctClock, &edit))
  {
    // @note: Users are allowed to jump back into EDL commercial breaks
    // do not reset the last edit time if the last surpassed edit is a commercial break
    if (m_Edl.GetLastEditActionType() != EDL::Action::COMM_BREAK)
    {
      m_Edl.ResetLastEditTime();
    }
    return;
  }

  if (edit.action == EDL::Action::CUT)
  {
    if ((m_playSpeed > 0 && correctClock < (edit.start + 1000)) ||
        (m_playSpeed < 0 && correctClock < (edit.end - 1000)))
    {
      CLog::Log(LOGDEBUG, "{} - Clock in EDL cut [{} - {}]: {}. Automatically skipping over.",
                __FUNCTION__, CEdl::MillisecondsToTimeString(edit.start),
                CEdl::MillisecondsToTimeString(edit.end), CEdl::MillisecondsToTimeString(clock));

      // Seeking either goes to the start or the end of the cut depending on the play direction.
      int seek = m_playSpeed >= 0 ? edit.end : edit.start;
      if (m_Edl.GetLastEditTime() != seek)
      {
        CDVDMsgPlayerSeek::CMode mode;
        mode.time = seek;
        mode.backward = true;
        mode.accurate = true;
        mode.restore = false;
        mode.trickplay = false;
        mode.sync = true;
        m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));

        m_Edl.SetLastEditTime(seek);
        m_Edl.SetLastEditActionType(edit.action);
      }
    }
  }
  else if (edit.action == EDL::Action::COMM_BREAK)
  {
    // marker for commbreak may be inaccurate. allow user to skip into break from the back
    if (m_playSpeed >= 0 && m_Edl.GetLastEditTime() != edit.start && clock < edit.end - 1000)
    {
      const std::shared_ptr<CAdvancedSettings> advancedSettings =
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
      if (advancedSettings && advancedSettings->m_EdlDisplayCommbreakNotifications)
      {
        const std::string timeString =
            StringUtils::SecondsToTimeString((edit.end - edit.start) / 1000, TIME_FORMAT_MM_SS);
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(25011), timeString);
      }

      m_Edl.SetLastEditTime(edit.start);
      m_Edl.SetLastEditActionType(edit.action);

      if (m_SkipCommercials)
      {
        CLog::Log(LOGDEBUG,
                  "{} - Clock in commercial break [{} - {}]: {}. Automatically skipping to end of "
                  "commercial break",
                  __FUNCTION__, CEdl::MillisecondsToTimeString(edit.start),
                  CEdl::MillisecondsToTimeString(edit.end), CEdl::MillisecondsToTimeString(clock));

        CDVDMsgPlayerSeek::CMode mode;
        mode.time = edit.end;
        mode.backward = true;
        mode.accurate = true;
        mode.restore = false;
        mode.trickplay = false;
        mode.sync = true;
        m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
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

  auto message = std::make_shared<CDVDMsgGeneralSynchronize>(500ms, SYNCSOURCE_PLAYER);
  m_messenger.Put(message);
  message->Wait(m_bStop, 0);
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
  if (target == VideoPlayer_ID3)
    return m_VideoPlayerAudioID3.get();
  return NULL;
}

void CVideoPlayer::SendPlayerMessage(std::shared_ptr<CDVDMsg> pMsg, unsigned int target)
{
  IDVDStreamPlayer* player = GetStreamPlayer(target);
  if(player)
    player->SendMessage(std::move(pMsg), 0);
}

void CVideoPlayer::OnExit()
{
  CLog::Log(LOGINFO, "CVideoPlayer::OnExit()");

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetCaching(CACHESTATE_DONE);

  // close each stream
  if (!m_bAbortRequest)
    CLog::Log(LOGINFO, "VideoPlayer: eof, waiting for queues to empty");

  CFileItem fileItem(m_item);
  UpdateFileItemStreamDetails(fileItem);

  CloseStream(m_CurrentAudio, !m_bAbortRequest);
  CloseStream(m_CurrentVideo, !m_bAbortRequest);
  CloseStream(m_CurrentTeletext,!m_bAbortRequest);
  CloseStream(m_CurrentRadioRDS, !m_bAbortRequest);
  CloseStream(m_CurrentAudioID3, !m_bAbortRequest);
  // the generalization principle was abused for subtitle player. actually it is not a stream player like
  // video and audio. subtitle player does not run on its own thread, hence waitForBuffers makes
  // no sense here. waitForBuffers is abused to clear overlay container (false clears container)
  // subtitles are added from video player. after video player has finished, overlays have to be cleared.
  CloseStream(m_CurrentSubtitle, false);  // clear overlay container

  CServiceBroker::GetWinSystem()->UnregisterRenderLoop(this);

  IPlayerCallback *cb = &m_callback;
  CVideoSettings vs = m_processInfo->GetVideoSettings();
  m_outboundEvents->Submit([=]() {
    cb->StoreVideoSettings(fileItem, vs);
  });

  CBookmark bookmark;
  bookmark.totalTimeInSeconds = 0;
  bookmark.timeInSeconds = 0;
  if (m_State.startTime == 0)
  {
    bookmark.totalTimeInSeconds = m_State.timeMax / 1000;
    bookmark.timeInSeconds = m_State.time / 1000;
  }
  bookmark.player = m_name;
  bookmark.playerState = GetPlayerState();
  m_outboundEvents->Submit([=]() {
    cb->OnPlayerCloseFile(fileItem, bookmark);
  });

  // destroy objects
  m_renderManager.Flush(false, false);
  m_pDemuxer.reset();
  m_pSubtitleDemuxer.reset();
  m_subtitleDemuxerMap.clear();
  m_pCCDemuxer.reset();
  if (m_pInputStream.use_count() > 1)
    throw std::runtime_error("m_pInputStream reference count is greater than 1");
  m_pInputStream.reset();

  // clean up all selection streams
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

  m_messenger.End();

  CFFmpegLog::ClearLogLevel();
  m_bStop = true;

  bool error = m_error;
  bool close = m_bCloseRequest;
  m_outboundEvents->Submit([=]() {
    if (close)
      cb->OnPlayBackStopped();
    else if (error)
      cb->OnPlayBackError();
    else
      cb->OnPlayBackEnded();
  });
}

void CVideoPlayer::HandleMessages()
{
  std::shared_ptr<CDVDMsg> pMsg = nullptr;

  while (m_messenger.Get(pMsg, 0ms) == MSGQ_OK)
  {
    if (pMsg->IsType(CDVDMsg::PLAYER_OPENFILE) &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_OPENFILE) == 0)
    {
      CDVDMsgOpenFile& msg(*std::static_pointer_cast<CDVDMsgOpenFile>(pMsg));

      IPlayerCallback *cb = &m_callback;
      CFileItem fileItem(m_item);
      UpdateFileItemStreamDetails(fileItem);
      CVideoSettings vs = m_processInfo->GetVideoSettings();
      m_outboundEvents->Submit([=]() {
        cb->StoreVideoSettings(fileItem, vs);
      });

      CBookmark bookmark;
      bookmark.totalTimeInSeconds = 0;
      bookmark.timeInSeconds = 0;
      if (m_State.startTime == 0)
      {
        bookmark.totalTimeInSeconds = m_State.timeMax / 1000;
        bookmark.timeInSeconds = m_State.time / 1000;
      }
      bookmark.player = m_name;
      bookmark.playerState = GetPlayerState();
      m_outboundEvents->Submit([=]() {
        cb->OnPlayerCloseFile(fileItem, bookmark);
      });

      m_item = msg.GetItem();
      m_playerOptions = msg.GetOptions();

      m_processInfo->SetPlayTimes(0,0,0,0);

      m_outboundEvents->Submit([this]() {
        m_callback.OnPlayBackStarted(m_item);
      });

      FlushBuffers(DVD_NOPTS_VALUE, true, true);
      m_renderManager.Flush(false, false);
      m_pDemuxer.reset();
      m_pSubtitleDemuxer.reset();
      m_subtitleDemuxerMap.clear();
      m_pCCDemuxer.reset();
      if (m_pInputStream.use_count() > 1)
        throw std::runtime_error("m_pInputStream reference count is greater than 1");
      m_pInputStream.reset();

      m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

      Prepare();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK) &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
        m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
    {
      CDVDMsgPlayerSeek& msg(*std::static_pointer_cast<CDVDMsgPlayerSeek>(pMsg));

      if (!m_State.canseek)
      {
        m_processInfo->SetStateSeeking(false);
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
          continue;
        }
      }

      if (!msg.GetTrickPlay())
      {
        m_processInfo->SeekFinished(0);
        SetCaching(CACHESTATE_FLUSH);
      }

      double start = DVD_NOPTS_VALUE;

      double time = msg.GetTime();
      if (msg.GetRelative())
        time = (m_clock.GetClock() + m_State.time_offset) / 1000l + time;

      time = msg.GetRestore() ? m_Edl.GetTimeAfterRestoringCuts(time) : time;

      // if input stream doesn't support ISeekTime, convert back to pts
      //! @todo
      //! After demuxer we add an offset to input pts so that displayed time and clock are
      //! increasing steadily. For seeking we need to determine the boundaries and offset
      //! of the desired segment. With the current approach calculated time may point
      //! to nirvana
      if (m_pInputStream->GetIPosTime() == nullptr)
        time -= m_State.time_offset/1000l;

      CLog::Log(LOGDEBUG, "demuxer seek to: {:f}", time);
      if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start))
      {
        CLog::Log(LOGDEBUG, "demuxer seek to: {:f}, success", time);
        if(m_pSubtitleDemuxer)
        {
          if(!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
            CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: {:f}, success", time);
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
      {
        m_processInfo->SeekFinished(0);
      }

      // dvd's will issue a HOP_CHANNEL that we need to skip
      if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        m_dvd.state = DVDSTATE_SEEK;

      m_processInfo->SetStateSeeking(false);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK_CHAPTER) &&
             m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
             m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
    {
      m_processInfo->SeekFinished(0);
      SetCaching(CACHESTATE_FLUSH);

      CDVDMsgPlayerSeekChapter& msg(*std::static_pointer_cast<CDVDMsgPlayerSeekChapter>(pMsg));
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
      else if (m_pInputStream)
      {
        CDVDInputStream::IChapter* pChapter = m_pInputStream->GetIChapter();
        if (pChapter && pChapter->SeekChapter(msg.GetChapter()))
        {
          FlushBuffers(start, true, true);
          int64_t beforeSeek = GetTime();
          offset = DVD_TIME_TO_MSEC(start) - static_cast<int>(beforeSeek);
          m_callback.OnPlayBackSeekChapter(msg.GetChapter());
        }
      }
      m_processInfo->SeekFinished(offset);
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
      auto pMsg2 = std::static_pointer_cast<CDVDMsgPlayerSetAudioStream>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
      if(st.source != STREAM_SOURCE_NONE)
      {
        if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::shared_ptr<CDVDInputStreamNavigator> pStream = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);
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
            m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
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
          m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_VIDEOSTREAM))
    {
      auto pMsg2 = std::static_pointer_cast<CDVDMsgPlayerSetVideoStream>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_VIDEO, pMsg2->GetStreamId());
      if (st.source != STREAM_SOURCE_NONE)
      {
        if (st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::shared_ptr<CDVDInputStreamNavigator> pStream = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);
          if (pStream->SetAngle(st.id))
          {
            m_dvd.iSelectedVideoStream = st.id;

            CDVDMsgPlayerSeek::CMode mode;
            mode.time = (int)GetUpdatedTime();
            mode.backward = true;
            mode.accurate = true;
            mode.trickplay = true;
            mode.sync = true;
            m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
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
          m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
    {
      auto pMsg2 = std::static_pointer_cast<CDVDMsgPlayerSetSubtitleStream>(pMsg);

      SelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
      if(st.source != STREAM_SOURCE_NONE)
      {
        if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::shared_ptr<CDVDInputStreamNavigator> pStream = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);
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
      auto pValue = std::static_pointer_cast<CDVDMsgBool>(pMsg);
      SetSubtitleVisibleInternal(pValue->m_value);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_PROGRAM))
    {
      auto msg = std::static_pointer_cast<CDVDMsgInt>(pMsg);
      if (m_pDemuxer)
      {
        m_pDemuxer->SetProgram(msg->m_value);
        FlushBuffers(DVD_NOPTS_VALUE, false, true);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
    {
      SetCaching(CACHESTATE_FLUSH);

      auto pMsgPlayerSetState = std::static_pointer_cast<CDVDMsgPlayerSetState>(pMsg);

      if (std::shared_ptr<CDVDInputStream::IMenus> ptr = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream))
      {
        if(ptr->SetState(pMsgPlayerSetState->GetState()))
        {
          m_dvd.state = DVDSTATE_NORMAL;
          m_dvd.iDVDStillStartTime = {};
          m_dvd.iDVDStillTime = 0ms;
        }
      }

      m_processInfo->SeekFinished(0);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      FlushBuffers(DVD_NOPTS_VALUE, true, true);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      int speed = std::static_pointer_cast<CDVDMsgPlayerSetSpeed>(pMsg)->GetSpeed();

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
      {
        m_callback.OnPlayBackSpeedChanged(speed / DVD_PLAYSPEED_NORMAL);
        m_processInfo->SeekFinished(0);
      }

      if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER) && speed != m_playSpeed)
      {
        std::shared_ptr<CInputStreamPVRBase> pvrinputstream = std::static_pointer_cast<CInputStreamPVRBase>(m_pInputStream);
        pvrinputstream->Pause(speed == 0);
      }

      // do a seek after rewind, clock is not in sync with current pts
      if ((speed == DVD_PLAYSPEED_NORMAL) &&
          (m_playSpeed != DVD_PLAYSPEED_NORMAL) &&
          (m_playSpeed != DVD_PLAYSPEED_PAUSE))
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
        mode.restore = false;
        m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
      }

      if (std::static_pointer_cast<CDVDMsgPlayerSetSpeed>(pMsg)->IsTempo())
        m_processInfo->SetTempo(static_cast<float>(speed) / DVD_PLAYSPEED_NORMAL);
      else
        m_processInfo->SetSpeed(static_cast<float>(speed) / DVD_PLAYSPEED_NORMAL);

      m_processInfo->SetFrameAdvance(false);

      m_playSpeed = speed;

      m_caching = CACHESTATE_DONE;
      m_clock.SetSpeed(speed);
      m_VideoPlayerAudio->SetSpeed(speed);
      m_VideoPlayerVideo->SetSpeed(speed);
      m_streamPlayerSpeed = speed;
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_FRAME_ADVANCE))
    {
      if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
      {
        int frames = std::static_pointer_cast<CDVDMsgInt>(pMsg)->m_value;
        double time = DVD_TIME_BASE / static_cast<double>(m_processInfo->GetVideoFps()) * frames;
        m_processInfo->SetFrameAdvance(true);
        m_clock.Advance(time);
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
      OnAction(std::static_pointer_cast<CDVDMsgType<CAction>>(pMsg)->m_value);
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      SStartMsg& msg = std::static_pointer_cast<CDVDMsgType<SStartMsg>>(pMsg)->m_value;
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
      CLog::Log(LOGDEBUG, "CVideoPlayer::HandleMessages - player started {}", msg.player);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_REPORT_STATE))
    {
      SStateMsg& msg = std::static_pointer_cast<CDVDMsgType<SStateMsg>>(pMsg)->m_value;
      if (msg.player == VideoPlayer_AUDIO)
      {
        m_CurrentAudio.syncState = msg.syncState;
      }
      if (msg.player == VideoPlayer_VIDEO)
      {
        m_CurrentVideo.syncState = msg.syncState;
      }
      CLog::Log(LOGDEBUG, "CVideoPlayer::HandleMessages - player {} reported state: {}", msg.player,
                msg.syncState);
    }
    else if (pMsg->IsType(CDVDMsg::SUBTITLE_ADDFILE))
    {
      int id = AddSubtitleFile(std::static_pointer_cast<CDVDMsgType<std::string>>(pMsg)->m_value);
      if (id >= 0)
      {
        SetSubtitle(id);
        SetSubtitleVisibleInternal(true);
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if (std::static_pointer_cast<CDVDMsgGeneralSynchronize>(pMsg)->Wait(100ms, SYNCSOURCE_PLAYER))
        CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::GENERAL_SYNCHRONIZE");
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_AVCHANGE))
    {
      CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
      CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
      CServiceBroker::GetDataCacheCore().SignalSubtitleInfoChange();
      IPlayerCallback *cb = &m_callback;
      m_outboundEvents->Submit([=]() {
        cb->OnAVChange();
      });
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_ABORT))
    {
      CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::PLAYER_ABORT");
      m_bAbortRequest = true;
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_UPDATE_STREAM_DETAILS))
      m_UpdateStreamDetails = true;
  }
}

void CVideoPlayer::SetCaching(ECacheState state)
{
  if(state == CACHESTATE_FLUSH)
  {
    CacheInfo cache = GetCachingTimes();
    if (cache.valid)
      state = CACHESTATE_FULL;
    else
      state = CACHESTATE_INIT;
  }

  if(m_caching == state)
    return;

  CLog::Log(LOGDEBUG, "CVideoPlayer::SetCaching - caching state {}", state);
  if (state == CACHESTATE_FULL ||
      state == CACHESTATE_INIT)
  {
    m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);

    m_VideoPlayerAudio->SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_VideoPlayerVideo->SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_streamPlayerSpeed = DVD_PLAYSPEED_PAUSE;

    m_cachingTimer.Set(5000ms);
  }

  if (state == CACHESTATE_PLAY ||
     (state == CACHESTATE_DONE && m_caching != CACHESTATE_PLAY))
  {
    m_clock.SetSpeed(m_playSpeed);
    m_VideoPlayerAudio->SetSpeed(m_playSpeed);
    m_VideoPlayerVideo->SetSpeed(m_playSpeed);
    m_streamPlayerSpeed = m_playSpeed;
  }
  m_caching = state;

  m_clock.SetSpeedAdjust(0);
}

void CVideoPlayer::SetPlaySpeed(int speed)
{
  if (IsPlaying())
  {
    CDVDMsgPlayerSetSpeed::SpeedParams params = { speed, false };
    m_messenger.Put(std::make_shared<CDVDMsgPlayerSetSpeed>(params));
  }
  else
  {
    m_playSpeed = speed;
    m_streamPlayerSpeed = speed;
  }
}

bool CVideoPlayer::CanPause() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
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

bool CVideoPlayer::HasID3() const
{
  return m_CurrentAudioID3.id >= 0;
}

bool CVideoPlayer::IsPassthrough() const
{
  return m_VideoPlayerAudio->IsPassthrough();
}

bool CVideoPlayer::CanSeek() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
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
  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_videoUseTimeSeeking && m_processInfo->GetMaxTime() > 2000 * advancedSettings->m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seekTarget = bPlus ? advancedSettings->m_videoTimeSeekForwardBig :
                           advancedSettings->m_videoTimeSeekBackwardBig;
    else
      seekTarget = bPlus ? advancedSettings->m_videoTimeSeekForward :
                           advancedSettings->m_videoTimeSeekBackward;
    seekTarget *= 1000;
    seekTarget += GetTime();
  }
  else
  {
    int percent;
    if (bLargeStep)
      percent = bPlus ? advancedSettings->m_videoPercentSeekForwardBig : advancedSettings->m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? advancedSettings->m_videoPercentSeekForward : advancedSettings->m_videoPercentSeekBackward;
    seekTarget = static_cast<int64_t>(m_processInfo->GetMaxTime() * (GetPercentage() + percent) / 100);
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

  m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
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

    m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
    SynchronizeDemuxer();
    return true;
  }
  return false;
}

void CVideoPlayer::GetGeneralInfo(std::string& strGeneralInfo)
{
  if (!m_bStop)
  {
    double apts = m_VideoPlayerAudio->GetCurrentPts();
    double vpts = m_VideoPlayerVideo->GetCurrentPts();
    double dDiff = 0;

    if (apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE)
      dDiff = (apts - vpts) / DVD_TIME_BASE;

    std::string strBuf;
    std::unique_lock<CCriticalSection> lock(m_StateSection);
    if (m_State.cache_bytes >= 0)
    {
      strBuf += StringUtils::Format("forward: {}", StringUtils::SizeToString(m_State.cache_bytes));

      if (m_State.cache_time > 0)
        strBuf += StringUtils::Format(" {:6.3f}s", m_State.cache_time);
      else
        strBuf += StringUtils::Format(" {:2.0f}%", m_State.cache_level * 100);

      if (m_playSpeed == 0 || m_caching == CACHESTATE_FULL)
        strBuf += StringUtils::Format(" {} msec", DVD_TIME_TO_MSEC(m_State.cache_delay));
    }

    strGeneralInfo = StringUtils::Format("Player: a/v:{: 6.3f}, {}", dDiff, strBuf);
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

float CVideoPlayer::GetCachePercentage() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return (float) (m_State.cache_offset * 100); // NOTE: Percentage returned is relative
}

void CVideoPlayer::SetAVDelay(float fValue)
{
  m_processInfo->GetVideoSettingsLocked().SetAudioDelay(fValue);
  m_renderManager.SetDelay(static_cast<int>(fValue * 1000.0f));
}

float CVideoPlayer::GetAVDelay()
{
  return static_cast<float>(m_renderManager.GetDelay()) / 1000.0f;
}

void CVideoPlayer::SetSubTitleDelay(float fValue)
{
  m_processInfo->GetVideoSettingsLocked().SetSubtitleDelay(fValue);
  m_VideoPlayerVideo->SetSubtitleDelay(static_cast<double>(-fValue) * DVD_TIME_BASE);
}

float CVideoPlayer::GetSubTitleDelay()
{
  return (float) -m_VideoPlayerVideo->GetSubtitleDelay() / DVD_TIME_BASE;
}

bool CVideoPlayer::GetSubtitleVisible() const
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    std::shared_ptr<CDVDInputStreamNavigator> pStream = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);
    return pStream->IsSubtitleStreamEnabled();
  }

  return m_VideoPlayerVideo->IsSubtitleEnabled();
}

void CVideoPlayer::SetSubtitleVisible(bool bVisible)
{
  m_messenger.Put(
      std::make_shared<CDVDMsgBool>(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
  m_processInfo->GetVideoSettingsLocked().SetSubtitleVisible(bVisible);
}

void CVideoPlayer::SetSubtitleVisibleInternal(bool bVisible)
{
  m_VideoPlayerVideo->EnableSubtitle(bVisible);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream)->EnableSubtitleStream(bVisible);

  CServiceBroker::GetDataCacheCore().SignalSubtitleInfoChange();
}

void CVideoPlayer::SetSubtitleVerticalPosition(int value, bool save)
{
  m_processInfo->GetVideoSettingsLocked().SetSubtitleVerticalPosition(value, save);
  m_renderManager.SetSubtitleVerticalPosition(value, save);
}

std::shared_ptr<TextCacheStruct_t> CVideoPlayer::GetTeletextCache()
{
  if (m_CurrentTeletext.id < 0)
    return nullptr;

  return m_VideoPlayerTeletext->GetTeletextCache();
}

bool CVideoPlayer::HasTeletextCache() const
{
  return m_CurrentTeletext.id >= 0;
}

void CVideoPlayer::LoadPage(int p, int sp, unsigned char* buffer)
{
  if (m_CurrentTeletext.id < 0)
      return;

  return m_VideoPlayerTeletext->LoadPage(p, sp, buffer);
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

  m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
  SynchronizeDemuxer();
  m_callback.OnPlayBackSeek(iTime, seekOffset);
  m_processInfo->SeekFinished(seekOffset);
}

bool CVideoPlayer::SeekTimeRelative(int64_t iTime)
{
  int64_t abstime = GetTime() + iTime;

  // if the file has EDL cuts we can't rely on m_clock for relative seeks
  // EDL cuts remove time from the original file, hence we might seek to
  // positions too far from the current m_clock position. Seek to absolute
  // time instead
  if (m_Edl.HasCuts())
  {
    SeekTime(abstime);
    return true;
  }

  CDVDMsgPlayerSeek::CMode mode;
  mode.time = (int)iTime;
  mode.relative = true;
  mode.backward = (iTime < 0) ? true : false;
  mode.accurate = false;
  mode.trickplay = false;
  mode.sync = true;

  m_messenger.Put(std::make_shared<CDVDMsgPlayerSeek>(mode));
  m_processInfo->SetStateSeeking(true);

  m_callback.OnPlayBackSeek(abstime, iTime);
  m_processInfo->SeekFinished(iTime);
  return true;
}

// return the time in milliseconds
int64_t CVideoPlayer::GetTime()
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
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
      if (currentTempo != 1.0f)
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
  tempo = floor(tempo * 100.0f + 0.5f) / 100.0f;
  if (m_processInfo->IsTempoAllowed(tempo))
  {
    int speed = tempo * DVD_PLAYSPEED_NORMAL;
    CDVDMsgPlayerSetSpeed::SpeedParams params = { speed, true };
    m_messenger.Put(std::make_shared<CDVDMsgPlayerSetSpeed>(params));

    m_processInfo->SetNewTempo(tempo);
  }
}

void CVideoPlayer::FrameAdvance(int frames)
{
  float currentSpeed = m_processInfo->GetNewSpeed();
  if (currentSpeed != DVD_PLAYSPEED_PAUSE)
    return;

  m_messenger.Put(std::make_shared<CDVDMsgInt>(CDVDMsg::PLAYER_FRAME_ADVANCE, frames));
}

bool CVideoPlayer::SupportsTempo() const
{
  return m_State.cantempo;
}

bool CVideoPlayer::OpenStream(CCurrentStream& current, int64_t demuxerId, int iStream, int source, bool reset /*= true*/)
{
  CDemuxStream* stream = NULL;
  CDVDStreamInfo hint;

  CLog::Log(LOGINFO, "Opening stream: {} source: {}", iStream, source);

  if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
  {
    int index = m_SelectionStreams.TypeIndexOf(current.type, source, demuxerId, iStream);
    if (index < 0)
      return false;
    const SelectionStream& st = m_SelectionStreams.Get(current.type, index);

    CLog::Log(LOGINFO, "Opening Subtitle file: {}", CURL::GetRedacted(st.filename));
    m_pSubtitleDemuxer.reset();
    const auto demux = m_subtitleDemuxerMap.find(demuxerId);
    if (demux == m_subtitleDemuxerMap.end())
    {
      CLog::Log(LOGINFO, "No demuxer found for file {}", CURL::GetRedacted(st.filename));
      return false;
    }

    m_pSubtitleDemuxer = demux->second;

    double pts = m_VideoPlayerVideo->GetCurrentPts();
    if(pts == DVD_NOPTS_VALUE)
      pts = m_CurrentVideo.dts;
    if(pts == DVD_NOPTS_VALUE)
      pts = 0;
    pts += m_offset_pts;
    if (!m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE)))
      CLog::Log(LOGDEBUG, "{} - failed to start subtitle demuxing from: {:f}", __FUNCTION__, pts);
    stream = m_pSubtitleDemuxer->GetStream(demuxerId, iStream);
    if(!stream || stream->disabled)
      return false;

    m_pSubtitleDemuxer->EnableStream(demuxerId, iStream, true);

    hint.Assign(*stream, true);
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
  {
    int index = m_SelectionStreams.TypeIndexOf(current.type, source, demuxerId, iStream);
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
    case STREAM_AUDIO_ID3:
      res = OpenAudioID3Stream(hint);
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
      CLog::Log(LOGWARNING, "{} - Unsupported stream {}. Stream disabled.", __FUNCTION__,
                stream->uniqueId);
      stream->disabled = true;
    }
  }

  UpdateContentState();
  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
  CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
  CServiceBroker::GetDataCacheCore().SignalSubtitleInfoChange();

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

    player->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, m_displayLost), 1);

    static_cast<IDVDStreamPlayerAudio*>(player)->SetSpeed(m_streamPlayerSpeed);
    m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_STARTING;
    m_CurrentAudio.packets = 0;
  }
  else if (reset)
    player->SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_RESET), 0);

  m_HasAudio = true;

  static_cast<IDVDStreamPlayerAudio*>(player)->SendMessage(
      std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_REQUEST_STATE), 1);

  return true;
}

bool CVideoPlayer::OpenVideoStream(CDVDStreamInfo& hint, bool reset)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    /* set aspect ratio as requested by navigator for dvd's */
    float aspect = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream)->GetVideoAspectRatio();
    if (aspect != 0.0f)
    {
      hint.aspect = static_cast<double>(aspect);
      hint.forced_aspect = true;
    }
    hint.dvd = true;
  }
  else if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    // set framerate if not set by demuxer
    if (hint.fpsrate == 0 || hint.fpsscale == 0)
    {
      int fpsidx = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRPLAYBACK_FPS);
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

  std::shared_ptr<CDVDInputStream::IMenus> pMenus = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream);
  if(pMenus && pMenus->IsInMenu())
    hint.stills = true;

  if (hint.stereo_mode.empty())
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui != nullptr)
    {
      const CStereoscopicsManager &stereoscopicsManager = gui->GetStereoscopicsManager();
      hint.stereo_mode = stereoscopicsManager.DetectStereoModeByString(m_item.GetPath());
    }
  }

  if (hint.flags & AV_DISPOSITION_ATTACHED_PIC)
    return false;

  // set desired refresh rate
  if (m_CurrentVideo.id < 0 && m_playerOptions.fullscreen &&
      CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot() && hint.fpsrate != 0 &&
      hint.fpsscale != 0)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
    {
      double framerate = DVD_TIME_BASE / CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE * hint.fpsscale / hint.fpsrate);
      RESOLUTION res = CResolutionUtils::ChooseBestResolution(static_cast<float>(framerate), hint.width, hint.height, !hint.stereo_mode.empty());
      CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(res, false);
      m_renderManager.TriggerUpdateResolution(framerate, hint.width, hint.height, hint.stereo_mode);
    }
  }

  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentVideo.player);
  if(player == nullptr)
    return false;

  if(m_CurrentVideo.id < 0 ||
     m_CurrentVideo.hint != hint)
  {
    if (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264)
      m_pCCDemuxer.reset();

    if (!player->OpenStream(hint))
      return false;

    player->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, m_displayLost), 1);

    // look for any EDL files
    m_Edl.Clear();
    float fFramesPerSecond = 0.0f;
    if (m_CurrentVideo.hint.fpsscale > 0.0f)
      fFramesPerSecond = static_cast<float>(m_CurrentVideo.hint.fpsrate) / static_cast<float>(m_CurrentVideo.hint.fpsscale);
    m_Edl.ReadEditDecisionLists(m_item, fFramesPerSecond);
    CServiceBroker::GetDataCacheCore().SetEditList(m_Edl.GetEditList());
    CServiceBroker::GetDataCacheCore().SetCuts(m_Edl.GetCutMarkers());
    CServiceBroker::GetDataCacheCore().SetSceneMarkers(m_Edl.GetSceneMarkers());

    static_cast<IDVDStreamPlayerVideo*>(player)->SetSpeed(m_streamPlayerSpeed);
    m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_STARTING;
    m_CurrentVideo.packets = 0;
  }
  else if (reset)
    player->SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_RESET), 0);

  m_HasVideo = true;

  static_cast<IDVDStreamPlayerVideo*>(player)->SendMessage(
      std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_REQUEST_STATE), 1);

  // open CC demuxer if video is mpeg2
  if ((hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264) && !m_pCCDemuxer)
  {
    m_pCCDemuxer = std::make_unique<CDVDDemuxCC>(hint.codec);
    m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_VIDEOMUX);
  }

  return true;
}

bool CVideoPlayer::OpenSubtitleStream(const CDVDStreamInfo& hint)
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
  if (ss.flags & StreamFlags::FLAG_FORCED)
  {
    SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
    bool found = false;
    for (const auto &stream : m_SelectionStreams.Get(STREAM_SUBTITLE))
    {
      if (stream.flags & StreamFlags::FLAG_FORCED && g_LangCodeExpander.CompareISO639Codes(stream.language, as.language))
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

bool CVideoPlayer::OpenAudioID3Stream(CDVDStreamInfo& hint)
{
  if (!m_VideoPlayerAudioID3->CheckStream(hint))
    return false;

  IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentAudioID3.player);
  if (player == nullptr)
    return false;

  if (m_CurrentAudioID3.id < 0 || m_CurrentAudioID3.hint != hint)
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

  CLog::Log(LOGINFO, "Closing stream player {}", current.player);

  if(bWaitForBuffers)
    SetCaching(CACHESTATE_DONE);

  if (m_pDemuxer && STREAM_SOURCE_MASK(current.source) == STREAM_SOURCE_DEMUX)
    m_pDemuxer->EnableStream(current.demuxerId, current.id, false);

  IDVDStreamPlayer* player = GetStreamPlayer(current.player);
  if (player)
  {
    if ((current.type == STREAM_AUDIO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC) ||
        (current.type == STREAM_VIDEO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC) ||
        m_bAbortRequest)
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
  if (accurate)
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

  m_CurrentAudioID3.dts = DVD_NOPTS_VALUE;
  m_CurrentAudioID3.startpts = startpts;
  m_CurrentAudioID3.packets = 0;

  m_VideoPlayerAudio->Flush(sync);
  m_VideoPlayerVideo->Flush(sync);
  m_VideoPlayerSubtitle->Flush();
  m_VideoPlayerTeletext->Flush();
  m_VideoPlayerRadioRDS->Flush();
  m_VideoPlayerAudioID3->Flush();

  if (m_playSpeed == DVD_PLAYSPEED_NORMAL ||
      m_playSpeed == DVD_PLAYSPEED_PAUSE)
  {
    // make sure players are properly flushed, should put them in stalled state
    auto msg = std::make_shared<CDVDMsgGeneralSynchronize>(1s, SYNCSOURCE_AUDIO | SYNCSOURCE_VIDEO);
    m_VideoPlayerAudio->SendMessage(msg, 1);
    m_VideoPlayerVideo->SendMessage(msg, 1);
    msg->Wait(m_bStop, 0);

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

  m_demuxerSpeed = DVD_PLAYSPEED_NORMAL;
  if (m_pDemuxer)
    m_pDemuxer->SetSpeed(DVD_PLAYSPEED_NORMAL);
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CVideoPlayer::OnDiscNavResult(void* pData, int iMessage)
{
  if (!m_pInputStream)
    return 0;

#if defined(HAVE_LIBBLURAY)
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    switch (iMessage)
    {
    case BD_EVENT_MENU_OVERLAY:
      m_overlayContainer.ProcessAndAddOverlayIfValid(
          *static_cast<std::shared_ptr<CDVDOverlay>*>(pData));
      break;
    case BD_EVENT_PLAYLIST_STOP:
      m_dvd.state = DVDSTATE_NORMAL;
      m_dvd.iDVDStillTime = 0ms;
      m_messenger.Put(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_FLUSH));
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

        m_dvd.iDVDStillTime = std::chrono::milliseconds(*static_cast<int*>(pData));
        m_dvd.iDVDStillStartTime = std::chrono::steady_clock::now();

        if (m_dvd.iDVDStillTime > 0ms)
          m_dvd.iDVDStillTime *= 1000;

        /* adjust for the output delay in the video queue */
        std::chrono::milliseconds time = 0ms;
        if (m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0ms)
        {
          time = std::chrono::milliseconds(
              static_cast<int>(m_VideoPlayerVideo->GetOutputDelay() / (DVD_TIME_BASE / 1000)));
          if (time < 10000ms && time > 0ms)
            m_dvd.iDVDStillTime += time;
        }
        m_dvd.state = DVDSTATE_STILL;
        CLog::Log(LOGDEBUG, "BD_EVENT_STILL_TIME - waiting {} msec, with delay of {} msec",
                  m_dvd.iDVDStillTime.count(), time.count());
      }
    }
    break;
    case BD_EVENT_STILL:
    {
      bool on = static_cast<bool>(*static_cast<int*>(pData));
      if (on && m_dvd.state != DVDSTATE_STILL)
      {
        m_dvd.state = DVDSTATE_STILL;
        m_dvd.iDVDStillStartTime = std::chrono::steady_clock::now();
        m_dvd.iDVDStillTime = 0ms;
        CLog::Log(LOGDEBUG, "CDVDPlayer::OnDVDNavResult - libbluray DVDSTATE_STILL start");
      }
      else if (!on && m_dvd.state == DVDSTATE_STILL)
      {
        m_dvd.state = DVDSTATE_NORMAL;
        m_dvd.iDVDStillStartTime = {};
        m_dvd.iDVDStillTime = 0ms;
        CLog::Log(LOGDEBUG, "CDVDPlayer::OnDVDNavResult - libbluray DVDSTATE_STILL end");
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
    std::shared_ptr<CDVDInputStreamNavigator> pStream = std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);

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
            m_dvd.iDVDStillTime = std::chrono::seconds(still_event->length);
          else
            m_dvd.iDVDStillTime = 0ms;

          m_dvd.iDVDStillStartTime = std::chrono::steady_clock::now();

          /* adjust for the output delay in the video queue */
          std::chrono::milliseconds time = 0ms;
          if (m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0ms)
          {
            time = std::chrono::milliseconds(
                static_cast<int>(m_VideoPlayerVideo->GetOutputDelay() / (DVD_TIME_BASE / 1000)));
            if (time < 10000ms && time > 0ms)
              m_dvd.iDVDStillTime += time;
          }
          m_dvd.state = DVDSTATE_STILL;
          CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME - waiting {} sec, with delay of {} msec",
                    still_event->length, time.count());
        }
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      {
        m_VideoPlayerSubtitle->SendMessage(
            std::make_shared<CDVDMsgSubtitleClutChange>((uint8_t*)pData));
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
        dvdnav_audio_stream_change_event_t* event = static_cast<dvdnav_audio_stream_change_event_t*>(pData);
        // Tell system what audiostream should be opened by default
        m_dvd.iSelectedAudioStream = event->physical;
        m_CurrentAudio.stream = NULL;
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        //dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button {}", iButton);
        m_VideoPlayerSubtitle->UpdateOverlayInfo(std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream), LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        //dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();

        //Force an aspect ratio that is set in the dvdheaders if available
        m_CurrentVideo.hint.aspect = static_cast<double>(pStream->GetVideoAspectRatio());
        if( m_VideoPlayerVideo->IsInited() )
          m_VideoPlayerVideo->SendMessage(std::make_shared<CDVDMsgDouble>(
              CDVDMsg::VIDEO_SET_ASPECT, m_CurrentVideo.hint.aspect));

        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer.get());
        UpdateContent();

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
          // to the new vobunit, which has new timestamps
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
      }
      break;
    case DVDNAV_ERROR:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_ERROR");
        m_dvd.state = DVDSTATE_NORMAL;
        CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(16026),
                                              g_localizeStrings.Get(16029));
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
  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  width = res.iWidth;
  height = res.iHeight;
}

bool CVideoPlayer::OnAction(const CAction &action)
{
#define THREAD_ACTION(action) \
  do \
  { \
    if (!IsCurrentThread()) \
    { \
      m_messenger.Put( \
          std::make_shared<CDVDMsgType<CAction>>(CDVDMsg::GENERAL_GUI_ACTION, action)); \
      return true; \
    } \
  } while (false)

  std::shared_ptr<CDVDInputStream::IMenus> pMenus = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream);
  if (pMenus)
  {
    if (m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime != 0ms &&
        pMenus->GetTotalButtons() == 0)
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
            CLog::Log(LOGDEBUG, "{} - User asked to exit stillframe", __FUNCTION__);
            m_dvd.iDVDStillStartTime = {};
            m_dvd.iDVDStillTime = 1ms;
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
        m_processInfo->SeekFinished(0);
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed next");
        pMenus->OnNext();
        m_processInfo->SeekFinished(0);
        return true;
      }
      break;
#endif
    case ACTION_SHOW_VIDEOMENU:   // start button
      {
        THREAD_ACTION(action);
        CLog::LogF(LOGDEBUG, "Trying to go to the menu");
        if (pMenus->OnMenu())
        {
          if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
          {
            SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
            m_callback.OnPlayBackResumed();
          }

          // send a message to everyone that we've gone to the menu
          CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        }
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
        if (pMenus->CanSeek() && GetChapterCount() > 0 && GetChapter() < GetChapterCount())
          m_messenger.Put(std::make_shared<CDVDMsgPlayerSeekChapter>(GetChapter() + 1));
        else
          pMenus->OnNext();

        m_processInfo->SeekFinished(0);
        return true;
      case ACTION_PREV_ITEM:
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed prev in menu, stream will decide");
        if (pMenus->CanSeek() && GetChapterCount() > 0 && GetChapter() > 0)
          m_messenger.Put(std::make_shared<CDVDMsgPlayerSeekChapter>(GetChapter() - 1));
        else
          pMenus->OnPrevious();

        m_processInfo->SeekFinished(0);
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
              CServiceBroker::GetAppMessenger()->PostMsg(
                  TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                  static_cast<void*>(new CAction(ACTION_TRIGGER_OSD)));
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
            m_VideoPlayerSubtitle->UpdateOverlayInfo(std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream), LIBDVDNAV_BUTTON_CLICKED);

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
          CLog::Log(LOGDEBUG, " - button pressed {}", button);
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

  pMenus.reset();

  switch (action.GetID())
  {
    case ACTION_NEXT_ITEM:
      if (GetChapter() > 0 && GetChapter() < GetChapterCount())
      {
        m_messenger.Put(std::make_shared<CDVDMsgPlayerSeekChapter>(GetChapter() + 1));
        m_processInfo->SeekFinished(0);
        return true;
      }
      else if (SeekScene(true))
        return true;
      else
        break;
    case ACTION_PREV_ITEM:
      if (GetChapter() > 0)
      {
        m_messenger.Put(std::make_shared<CDVDMsgPlayerSeekChapter>(GetChapter() - 1));
        m_processInfo->SeekFinished(0);
        return true;
      }
      else if (SeekScene(false))
        return true;
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
    case ACTION_PLAYER_DEBUG_VIDEO:
      m_renderManager.ToggleDebugVideo();
      break;

    case ACTION_PLAYER_PROCESS_INFO:
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_DIALOG_PLAYER_PROCESS_INFO)
      {
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_PLAYER_PROCESS_INFO);
        return true;
      }
      break;
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CVideoPlayer::IsInMenuInternal() const
{
  std::shared_ptr<CDVDInputStream::IMenus> pStream = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream);
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
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return m_State.isInMenu;
}

MenuType CVideoPlayer::GetSupportedMenuType() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return m_State.menuType;
}

std::string CVideoPlayer::GetPlayerState()
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return m_State.player_state;
}

bool CVideoPlayer::SetPlayerState(const std::string& state)
{
  m_messenger.Put(std::make_shared<CDVDMsgPlayerSetState>(state));
  return true;
}

int CVideoPlayer::GetChapterCount() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return m_State.chapters.size();
}

int CVideoPlayer::GetChapter() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return m_State.chapter;
}

void CVideoPlayer::GetChapterName(std::string& strChapterName, int chapterIdx) const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
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
    m_messenger.Put(std::make_shared<CDVDMsgPlayerSeekChapter>(iChapter));
    SynchronizeDemuxer();
  }

  return 0;
}

int64_t CVideoPlayer::GetChapterPos(int chapterIdx) const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  if (chapterIdx > 0 && chapterIdx <= (int) m_State.chapters.size())
    return m_State.chapters[chapterIdx - 1].second;

  return -1;
}

void CVideoPlayer::AddSubtitle(const std::string& strSubPath)
{
  m_messenger.Put(
      std::make_shared<CDVDMsgType<std::string>>(CDVDMsg::SUBTITLE_ADDFILE, strSubPath));
}

bool CVideoPlayer::IsCaching() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return !m_State.isInMenu && m_State.caching;
}

int CVideoPlayer::GetCacheLevel() const
{
  std::unique_lock<CCriticalSection> lock(m_StateSection);
  return (int)(m_State.cache_level * 100);
}

double CVideoPlayer::GetQueueTime()
{
  int a = m_VideoPlayerAudio->GetLevel();
  int v = m_processInfo->GetLevelVQ();
  return std::max(a, v) * 8000.0 / 100;
}

int CVideoPlayer::AddSubtitleFile(const std::string& filename, const std::string& subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if (ext == ".idx" || ext == ".sup")
  {
    std::shared_ptr<CDVDDemux> pDemux;
    if (ext == ".idx")
    {
      if (vobsubfile.empty())
      {
        // find corresponding .sub (e.g. in case of manually selected .idx sub)
        vobsubfile = CUtil::GetVobSubSubFromIdx(filename);
        if (vobsubfile.empty())
          return -1;
      }

      auto pDemuxVobsub = std::make_shared<CDVDDemuxVobsub>();
      if (!pDemuxVobsub->Open(filename, STREAM_SOURCE_NONE, vobsubfile))
        return -1;

      m_SelectionStreams.Update(nullptr, pDemuxVobsub.get(), vobsubfile);
      pDemux = pDemuxVobsub;
    }
    else // .sup file
    {
      CFileItem item(filename, false);
      std::shared_ptr<CDVDInputStream> pInput;
      pInput = CDVDFactoryInputStream::CreateInputStream(nullptr, item);
      if (!pInput || !pInput->Open())
        return -1;

      auto pDemuxFFmpeg = std::make_shared<CDVDDemuxFFmpeg>();
      if (!pDemuxFFmpeg->Open(pInput, false))
        return -1;

      m_SelectionStreams.Update(nullptr, pDemuxFFmpeg.get(), filename);
      pDemux = pDemuxFFmpeg;
    }

    ExternalStreamInfo info =
        CUtil::GetExternalStreamDetailsFromFilename(m_item.GetDynPath(), filename);

    for (auto sub : pDemux->GetStreams())
    {
      if (sub->type != STREAM_SUBTITLE)
        continue;

      int index = m_SelectionStreams.TypeIndexOf(STREAM_SUBTITLE,
        m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename),
        sub->demuxerId, sub->uniqueId);
      SelectionStream& stream = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

      if (stream.name.empty())
        stream.name = info.name;

      if (stream.language.empty())
        stream.language = info.language;

      if (static_cast<StreamFlags>(info.flag) != StreamFlags::FLAG_NONE)
        stream.flags = static_cast<StreamFlags>(info.flag);
    }

    UpdateContent();
    // the demuxer id is unique
    m_subtitleDemuxerMap[pDemux->GetDemuxerId()] = pDemux;
    return m_SelectionStreams.TypeIndexOf(
        STREAM_SUBTITLE, m_SelectionStreams.Source(STREAM_SOURCE_DEMUX_SUB, filename),
        pDemux->GetDemuxerId(), 0);
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
  ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(m_item.GetDynPath(), filename);
  s.name = info.name;
  s.language = info.language;
  if (static_cast<StreamFlags>(info.flag) != StreamFlags::FLAG_NONE)
    s.flags = static_cast<StreamFlags>(info.flag);

  m_SelectionStreams.Update(s);
  UpdateContent();
  return m_SelectionStreams.TypeIndexOf(STREAM_SUBTITLE, s.source, s.demuxerId, s.id);
}

void CVideoPlayer::UpdatePlayState(double timeout)
{
  if (m_State.timestamp != 0 &&
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

  std::shared_ptr<CDVDInputStream::IMenus> pMenu = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInputStream);

  if (m_pDemuxer)
  {
    if (IsInMenuInternal() && pMenu && !pMenu->CanSeek())
      state.chapter = 0;
    else
      state.chapter = m_pDemuxer->GetChapter();

    state.chapters.clear();
    if (m_pDemuxer->GetChapterCount() > 0)
    {
      for (int i = 0, ie = m_pDemuxer->GetChapterCount(); i < ie; ++i)
      {
        std::string name;
        m_pDemuxer->GetChapterName(name, i + 1);
        state.chapters.emplace_back(name, m_pDemuxer->GetChapterPos(i + 1));
      }
    }
    CServiceBroker::GetDataCacheCore().SetChapters(state.chapters);

    state.time = m_clock.GetClock(false) * 1000 / DVD_TIME_BASE;
    state.timeMax = m_pDemuxer->GetStreamLength();
  }

  state.canpause = false;
  state.canseek = false;
  state.cantempo = false;
  state.isInMenu = false;
  state.menuType = MenuType::NONE;

  if (m_pInputStream)
  {
    CDVDInputStream::IChapter* pChapter = m_pInputStream->GetIChapter();
    if (pChapter)
    {
      if (IsInMenuInternal() && pMenu && !pMenu->CanSeek())
        state.chapter = 0;
      else
        state.chapter = pChapter->GetChapter();

      state.chapters.clear();
      if (pChapter->GetChapterCount() > 0)
      {
        for (int i = 0, ie = pChapter->GetChapterCount(); i < ie; ++i)
        {
          std::string name;
          pChapter->GetChapterName(name, i + 1);
          state.chapters.push_back(make_pair(name, pChapter->GetChapterPos(i + 1)));
        }
      }
      CServiceBroker::GetDataCacheCore().SetChapters(state.chapters);
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
      state.time_offset = -times.ptsStart;
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
      state.time += state.time_offset * 1000 / DVD_TIME_BASE;
      state.timeMax = pDisplayTime->GetTotalTime();
    }
    else
    {
      state.time_offset = 0;
    }

    if (pMenu)
    {
      if (!pMenu->GetState(state.player_state))
        state.player_state = "";

      if (m_dvd.state == DVDSTATE_STILL)
      {
        const auto now = std::chrono::steady_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_dvd.iDVDStillStartTime);
        state.time = duration.count();
        state.timeMax = m_dvd.iDVDStillTime.count();
        state.isInMenu = true;
      }
      else if (IsInMenuInternal())
      {
        state.time = pDisplayTime->GetTime();
        state.isInMenu = true;
        if (!pMenu->CanSeek())
          state.time_offset = 0;
      }
      state.menuType = pMenu->GetSupportedMenuType();
    }

    state.canpause = m_pInputStream->CanPause();

    bool realtime = m_pInputStream->IsRealtime();

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK) &&
        !realtime)
    {
      state.cantempo = true;
    }
    else
    {
      state.cantempo = false;
    }

    m_processInfo->SetStateRealtime(realtime);
  }

  if (m_Edl.HasCuts())
  {
    state.time = static_cast<double>(m_Edl.GetTimeWithoutCuts(state.time));
    state.timeMax = state.timeMax - static_cast<double>(m_Edl.GetTotalCutTime());
  }

  if (m_caching > CACHESTATE_DONE && m_caching < CACHESTATE_PLAY)
    state.caching = true;
  else
    state.caching = false;

  double queueTime = GetQueueTime();
  CacheInfo cache = GetCachingTimes();

  if (cache.valid)
  {
    state.cache_delay = std::max(0.0, cache.delay);
    state.cache_level = std::max(0.0, std::min(1.0, cache.level));
    state.cache_offset = cache.offset;
    state.cache_time = cache.time;
  }
  else
  {
    state.cache_delay = 0.0;
    state.cache_level = std::min(1.0, queueTime / 8000.0);
    state.cache_offset = queueTime / state.timeMax;
    state.cache_time = queueTime / 1000.0;
  }

  XFILE::SCacheStatus status;
  if (m_pInputStream && m_pInputStream->GetCacheStatus(&status))
  {
    state.cache_bytes = status.forward;
    if(state.timeMax)
      state.cache_bytes += m_pInputStream->GetLength() * (int64_t)(queueTime / state.timeMax);
  }
  else
    state.cache_bytes = 0;

  state.timestamp = m_clock.GetAbsoluteClock();

  if (state.timeMax <= 0)
  {
    state.timeMax = state.time;
    state.timeMin = state.time;
  }
  if (state.timeMin == state.timeMax)
  {
    state.canseek = false;
    state.cantempo = false;
  }
  else
  {
    state.canseek = true;
    state.canpause = true;
  }

  m_processInfo->SetPlayTimes(state.startTime, state.time, state.timeMin, state.timeMax);

  std::unique_lock<CCriticalSection> lock(m_StateSection);
  m_State = state;
}

int64_t CVideoPlayer::GetUpdatedTime()
{
  UpdatePlayState(0);
  return llrint(m_State.time);
}

void CVideoPlayer::SetDynamicRangeCompression(long drc)
{
  m_processInfo->GetVideoSettingsLocked().SetVolumeAmplification(static_cast<float>(drc) / 100);
  m_VideoPlayerAudio->SetDynamicRangeCompression(drc);
}

CVideoSettings CVideoPlayer::GetVideoSettings() const
{
  return m_processInfo->GetVideoSettings();
}

void CVideoPlayer::SetVideoSettings(CVideoSettings& settings)
{
  m_processInfo->SetVideoSettings(settings);
  m_renderManager.SetVideoSettings(settings);
  m_renderManager.SetDelay(static_cast<int>(settings.m_AudioDelay * 1000.0f));
  m_renderManager.SetSubtitleVerticalPosition(settings.m_subtitleVerticalPosition,
                                              settings.m_subtitleVerticalPositionSave);
  m_VideoPlayerVideo->EnableSubtitle(settings.m_SubtitleOn);
  m_VideoPlayerVideo->SetSubtitleDelay(static_cast<int>(-settings.m_SubtitleDelay * DVD_TIME_BASE));
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
  m_renderManager.Flush(true, true);
}

void CVideoPlayer::SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch)
{
  m_processInfo->GetVideoSettingsLocked().SetViewMode(mode, zoom, par, shift, stretch);
  m_renderManager.SetVideoSettings(m_processInfo->GetVideoSettings());
  m_renderManager.SetViewMode(mode);
}

float CVideoPlayer::GetRenderAspectRatio() const
{
  return m_renderManager.GetAspectRatio();
}

void CVideoPlayer::TriggerUpdateResolution()
{
  std::string stereomode;
  m_renderManager.TriggerUpdateResolution(0, 0, 0, stereomode);
}

bool CVideoPlayer::IsRenderingVideo() const
{
  return m_renderManager.IsConfigured();
}

bool CVideoPlayer::Supports(EINTERLACEMETHOD method) const
{
  if (!m_processInfo)
    return false;
  return m_processInfo->Supports(method);
}

EINTERLACEMETHOD CVideoPlayer::GetDeinterlacingMethodDefault() const
{
  if (!m_processInfo)
    return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE;
  return m_processInfo->GetDeinterlacingMethodDefault();
}

bool CVideoPlayer::Supports(ESCALINGMETHOD method) const
{
  return m_renderManager.Supports(method);
}

bool CVideoPlayer::Supports(ERENDERFEATURE feature) const
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
  m_messenger.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_AVCHANGE));
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
  CLog::Log(LOGINFO, "VideoPlayer: OnLostDisplay received");
  m_VideoPlayerAudio->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, true), 1);
  m_VideoPlayerVideo->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, true), 1);
  m_clock.Pause(true);
  m_displayLost = true;
  FlushRenderer();
}

void CVideoPlayer::OnResetDisplay()
{
  if (!m_displayLost)
    return;

  CLog::Log(LOGINFO, "VideoPlayer: OnResetDisplay received");
  m_VideoPlayerAudio->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, false), 1);
  m_VideoPlayerVideo->SendMessage(std::make_shared<CDVDMsgBool>(CDVDMsg::GENERAL_PAUSE, false), 1);
  m_clock.Pause(false);
  m_displayLost = false;
  m_VideoPlayerAudio->SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_DISPLAY_RESET), 1);
}

void CVideoPlayer::UpdateFileItemStreamDetails(CFileItem& item)
{
  if (!m_UpdateStreamDetails)
    return;
  m_UpdateStreamDetails = false;

  CLog::Log(LOGDEBUG, "CVideoPlayer: updating file item stream details with available streams");

  VideoStreamInfo videoInfo;
  AudioStreamInfo audioInfo;
  SubtitleStreamInfo subtitleInfo;
  CVideoInfoTag* info = item.GetVideoInfoTag();
  GetVideoStreamInfo(CURRENT_STREAM, videoInfo);
  info->m_streamDetails.SetStreams(videoInfo, m_processInfo->GetMaxTime() / 1000, audioInfo,
                                   subtitleInfo);

  //grab all the audio and subtitle info and save it

  for (int i = 0; i < GetAudioStreamCount(); i++)
  {
    GetAudioStreamInfo(i, audioInfo);
    info->m_streamDetails.AddStream(new CStreamDetailAudio(audioInfo));
  }

  for (int i = 0; i < GetSubtitleCount(); i++)
  {
    GetSubtitleStreamInfo(i, subtitleInfo);
    info->m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitleInfo));
  }
}

//------------------------------------------------------------------------------
// content related methods
//------------------------------------------------------------------------------

void CVideoPlayer::UpdateContent()
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  m_content.m_selectionStreams = m_SelectionStreams;
  m_content.m_programs = m_programs;
}

void CVideoPlayer::UpdateContentState()
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);

  m_content.m_videoIndex = m_SelectionStreams.TypeIndexOf(STREAM_VIDEO, m_CurrentVideo.source,
                                                      m_CurrentVideo.demuxerId, m_CurrentVideo.id);
  m_content.m_audioIndex = m_SelectionStreams.TypeIndexOf(STREAM_AUDIO, m_CurrentAudio.source,
                                                      m_CurrentAudio.demuxerId, m_CurrentAudio.id);
  m_content.m_subtitleIndex = m_SelectionStreams.TypeIndexOf(STREAM_SUBTITLE, m_CurrentSubtitle.source,
                                                         m_CurrentSubtitle.demuxerId, m_CurrentSubtitle.id);

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) && m_content.m_videoIndex == -1 &&
      m_content.m_audioIndex == -1)
  {
    std::shared_ptr<CDVDInputStreamNavigator> nav =
          std::static_pointer_cast<CDVDInputStreamNavigator>(m_pInputStream);

    m_content.m_videoIndex = m_SelectionStreams.TypeIndexOf(STREAM_VIDEO, STREAM_SOURCE_NAV, -1,
                                                            nav->GetActiveAngle());
    m_content.m_audioIndex = m_SelectionStreams.TypeIndexOf(STREAM_AUDIO, STREAM_SOURCE_NAV, -1,
                                                            nav->GetActiveAudioStream());

    // only update the subtitle index in libdvdnav if the subtitle is provided by the dvd itself,
    // i.e. for external subtitles the index is always greater than the subtitlecount in dvdnav
    if (m_content.m_subtitleIndex < nav->GetSubTitleStreamCount())
    {
      m_content.m_subtitleIndex = m_SelectionStreams.TypeIndexOf(
          STREAM_SUBTITLE, STREAM_SOURCE_NAV, -1, nav->GetActiveSubtitleStream());
    }
  }
}

void CVideoPlayer::GetVideoStreamInfo(int streamId, VideoStreamInfo& info) const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);

  if (streamId == CURRENT_STREAM)
    streamId = m_content.m_videoIndex;

  if (streamId < 0 || streamId > GetVideoStreamCount() - 1)
  {
    info.valid = false;
    return;
  }

  const SelectionStream& s = m_content.m_selectionStreams.Get(STREAM_VIDEO, streamId);
  if (s.language.length() > 0)
    info.language = s.language;

  if (s.name.length() > 0)
    info.name = s.name;

  m_renderManager.GetVideoRect(info.SrcRect, info.DestRect, info.VideoRect);

  info.valid = true;
  info.bitrate = s.bitrate;
  info.width = s.width;
  info.height = s.height;
  info.codecName = s.codec;
  info.videoAspectRatio = s.aspect_ratio;
  info.stereoMode = s.stereo_mode;
  info.flags = s.flags;
  info.hdrType = s.hdrType;
}

int CVideoPlayer::GetVideoStreamCount() const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_selectionStreams.CountType(STREAM_VIDEO);
}

int CVideoPlayer::GetVideoStream() const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_videoIndex;
}

void CVideoPlayer::SetVideoStream(int iStream)
{
  m_messenger.Put(std::make_shared<CDVDMsgPlayerSetVideoStream>(iStream));
  m_processInfo->GetVideoSettingsLocked().SetVideoStream(iStream);
  SynchronizeDemuxer();
}

void CVideoPlayer::GetAudioStreamInfo(int index, AudioStreamInfo& info) const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);

  if (index == CURRENT_STREAM)
    index = m_content.m_audioIndex;

  if (index < 0 || index > GetAudioStreamCount() - 1)
  {
    info.valid = false;
    return;
  }

  const SelectionStream& s = m_content.m_selectionStreams.Get(STREAM_AUDIO, index);
  info.language = s.language;
  info.name = s.name;

  if (s.type == STREAM_NONE)
    info.name += " (Invalid)";

  info.valid = true;
  info.bitrate = s.bitrate;
  info.channels = s.channels;
  info.codecName = s.codec;
  info.flags = s.flags;
}

int CVideoPlayer::GetAudioStreamCount() const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_selectionStreams.CountType(STREAM_AUDIO);
}

int CVideoPlayer::GetAudioStream()
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_audioIndex;
}

void CVideoPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(std::make_shared<CDVDMsgPlayerSetAudioStream>(iStream));
  m_processInfo->GetVideoSettingsLocked().SetAudioStream(iStream);
  SynchronizeDemuxer();
}

void CVideoPlayer::GetSubtitleStreamInfo(int index, SubtitleStreamInfo& info) const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);

  if (index == CURRENT_STREAM)
    index = m_content.m_subtitleIndex;

  if (index < 0 || index > GetSubtitleCount() - 1)
  {
    info.valid = false;
    info.language.clear();
    info.flags = StreamFlags::FLAG_NONE;
    return;
  }

  const SelectionStream& s = m_content.m_selectionStreams.Get(STREAM_SUBTITLE, index);
  info.name = s.name;

  if (s.type == STREAM_NONE)
    info.name += "(Invalid)";

  info.language = s.language;
  info.flags = s.flags;
}

void CVideoPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(std::make_shared<CDVDMsgPlayerSetSubtitleStream>(iStream));
  m_processInfo->GetVideoSettingsLocked().SetSubtitleStream(iStream);
}

int CVideoPlayer::GetSubtitleCount() const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_selectionStreams.CountType(STREAM_SUBTITLE);
}

int CVideoPlayer::GetSubtitle()
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_content.m_subtitleIndex;
}

int CVideoPlayer::GetPrograms(std::vector<ProgramInfo>& programs)
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  programs = m_programs;
  return programs.size();
}

void CVideoPlayer::SetProgram(int progId)
{
  m_messenger.Put(std::make_shared<CDVDMsgInt>(CDVDMsg::PLAYER_SET_PROGRAM, progId));
}

int CVideoPlayer::GetProgramsCount() const
{
  std::unique_lock<CCriticalSection> lock(m_content.m_section);
  return m_programs.size();
}

void CVideoPlayer::SetUpdateStreamDetails()
{
  m_messenger.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_SET_UPDATE_STREAM_DETAILS));
}
