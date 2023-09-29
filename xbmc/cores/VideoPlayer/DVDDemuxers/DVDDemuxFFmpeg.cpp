/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxFFmpeg.h"

#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamFFmpeg.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "commons/Exception.h"
#include "cores/FFmpeg.h"
#include "cores/MenuType.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h" // for DVD_TIME_BASE
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/FontUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <mutex>
#include <sstream>
#include <tuple>
#include <utility>

extern "C"
{
#include "libavutil/channel_layout.h"
#include "libavutil/display.h"
#include "libavutil/pixdesc.h"
}

#ifdef HAVE_LIBBLURAY
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef TARGET_POSIX
#include <stdint.h>
#endif

extern "C"
{
#include <libavutil/dict.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/opt.h>
}

using namespace std::chrono_literals;

struct StereoModeConversionMap
{
  const char*          name;
  const char*          mode;
};

// we internally use the matroska string representation of stereoscopic modes.
// This struct is a conversion map to convert stereoscopic mode values
// from asf/wmv to the internally used matroska ones
static const struct StereoModeConversionMap WmvToInternalStereoModeMap[] =
{
  { "SideBySideRF",             "right_left" },
  { "SideBySideLF",             "left_right" },
  { "OverUnderRT",              "bottom_top" },
  { "OverUnderLT",              "top_bottom" },
  {}
};

namespace
{
const std::vector<std::string> font_mimetypes = {"application/x-truetype-font",
                                                 "application/vnd.ms-opentype",
                                                 "application/x-font-ttf",
                                                 "application/x-font", // probably incorrect
                                                 "application/font-sfnt",
                                                 "font/collection",
                                                 "font/otf",
                                                 "font/sfnt",
                                                 "font/ttf"};

bool AttachmentIsFont(const AVDictionaryEntry* dict)
{
  if (dict)
  {
    const std::string mimeType = dict->value;
    return std::find_if(font_mimetypes.begin(), font_mimetypes.end(),
                        [&mimeType](const std::string& str) { return str == mimeType; }) !=
           font_mimetypes.end();
  }
  return false;
}
} // namespace

std::string CDemuxStreamAudioFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return CDemuxStream::GetStreamName();
}

std::string CDemuxStreamSubtitleFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return CDemuxStream::GetStreamName();
}

std::string CDemuxStreamVideoFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return CDemuxStream::GetStreamName();
}

CDemuxParserFFmpeg::~CDemuxParserFFmpeg()
{
  if (m_codecCtx)
    avcodec_free_context(&m_codecCtx);
  if (m_parserCtx)
  {
    av_parser_close(m_parserCtx);
    m_parserCtx = nullptr;
  }
}

static int interrupt_cb(void* ctx)
{
  CDVDDemuxFFmpeg* demuxer = static_cast<CDVDDemuxFFmpeg*>(ctx);
  if (demuxer && demuxer->Aborted())
    return 1;
  return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int dvd_file_open(URLContext* h, const char* filename, int flags)
{
  return -1;
}
*/

static int dvd_file_read(void* h, uint8_t* buf, int size)
{
  if (interrupt_cb(h))
    return AVERROR_EXIT;

  std::shared_ptr<CDVDInputStream> pInputStream = static_cast<CDVDDemuxFFmpeg*>(h)->m_pInput;
  int len = pInputStream->Read(buf, size);
  if (len == 0)
    return AVERROR_EOF;
  else
    return len;
}
/*
static int dvd_file_write(URLContext* h, uint8_t* buf, int size)
{
  return -1;
}
*/
static int64_t dvd_file_seek(void* h, int64_t pos, int whence)
{
  if (interrupt_cb(h))
    return AVERROR_EXIT;

  std::shared_ptr<CDVDInputStream> pInputStream = static_cast<CDVDDemuxFFmpeg*>(h)->m_pInput;
  if (whence == AVSEEK_SIZE)
    return pInputStream->GetLength();
  else
    return pInputStream->Seek(pos, whence & ~AVSEEK_FORCE);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxFFmpeg::CDVDDemuxFFmpeg() : CDVDDemux()
{
  m_pFormatContext = NULL;
  m_ioContext = NULL;
  m_currentPts = DVD_NOPTS_VALUE;
  m_bMatroska = false;
  m_bAVI = false;
  m_bSup = false;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  m_pkt.result = -1;
  memset(&m_pkt.pkt, 0, sizeof(AVPacket));
  m_streaminfo = true; /* set to true if we want to look for streams before playback */
  m_checkTransportStream = false;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  ff_flush_avutil_log_buffers();
}

bool CDVDDemuxFFmpeg::Aborted()
{
  if (m_timeout.IsTimePast())
    return true;

  std::shared_ptr<CDVDInputStreamFFmpeg> input = std::dynamic_pointer_cast<CDVDInputStreamFFmpeg>(m_pInput);
  if (input && input->Aborted())
    return true;

  return false;
}

bool CDVDDemuxFFmpeg::Open(const std::shared_ptr<CDVDInputStream>& pInput, bool fileinfo)
{
  const AVInputFormat* iformat = nullptr;
  std::string strFile;
  m_streaminfo = !pInput->IsRealtime() && !m_reopen;
  m_reopen = false;
  m_currentPts = DVD_NOPTS_VALUE;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  m_seekToKeyFrame = false;

  const AVIOInterruptCB int_cb = { interrupt_cb, this };

  if (!pInput)
    return false;

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  if (m_pInput->GetContent().length() > 0)
  {
    std::string content = m_pInput->GetContent();
    StringUtils::ToLower(content);

    /* check if we can get a hint from content */
    if (content.compare("video/x-vobsub") == 0)
      iformat = av_find_input_format("mpeg");
    else if (content.compare("video/x-dvd-mpeg") == 0)
      iformat = av_find_input_format("mpeg");
    else if (content.compare("video/mp2t") == 0)
      iformat = av_find_input_format("mpegts");
    else if (content.compare("multipart/x-mixed-replace") == 0)
      iformat = av_find_input_format("mjpeg");
  }

  // open the demuxer
  m_pFormatContext  = avformat_alloc_context();
  m_pFormatContext->interrupt_callback = int_cb;

  // try to abort after 30 seconds
  m_timeout.Set(30s);

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    // special stream type that makes avformat handle file opening
    // allows internal ffmpeg protocols to be used
    AVDictionary* options = GetFFMpegOptionsFromInput();

    CURL url = m_pInput->GetURL();

    int result = -1;
    if (url.IsProtocol("mms"))
    {
      // try mmsh, then mmst
      url.SetProtocol("mmsh");
      url.SetProtocolOptions("");
      result = avformat_open_input(&m_pFormatContext, url.Get().c_str(), iformat, &options);
      if (result < 0)
      {
        url.SetProtocol("mmst");
        strFile = url.Get();
      }
    }
    else if (url.IsProtocol("udp") || url.IsProtocol("rtp"))
    {
      std::string strURL = url.Get();
      CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::Open() UDP/RTP Original URL '{}'", strURL);
      size_t found = strURL.find("://");
      if (found != std::string::npos)
      {
        size_t start = found + 3;
        found = strURL.find('@');

        if (found != std::string::npos && found > start)
        {
          // sourceip found
          std::string strSourceIp = strURL.substr(start, found - start);

          strFile = strURL.substr(0, start);
          strFile += strURL.substr(found);
          if (strFile.back() == '/')
            strFile.pop_back();
          strFile += "?sources=";
          strFile += strSourceIp;
          CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::Open() UDP/RTP URL '{}'", strFile);
        }
      }
    }
    if (result < 0)
    {
      if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
      {
        CLog::Log(LOGDEBUG, "Error, could not open file {}", CURL::GetRedacted(strFile));
        Dispose();
        av_dict_free(&options);
        return false;
      }
      av_dict_free(&options);
      avformat_close_input(&m_pFormatContext);
      m_pFormatContext = avformat_alloc_context();
      m_pFormatContext->interrupt_callback = int_cb;
      AVDictionary* options = GetFFMpegOptionsFromInput();
      av_dict_set_int(&options, "load_all_variants", 0, AV_OPT_SEARCH_CHILDREN);
      if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
      {
        CLog::Log(LOGDEBUG, "Error, could not open file (2) {}", CURL::GetRedacted(strFile));
        Dispose();
        av_dict_free(&options);
        return false;
      }
    }
    av_dict_free(&options);
  }
  else
  {
    bool seekable = true;
    if (m_pInput->Seek(0, SEEK_POSSIBLE) == 0)
    {
      seekable = false;
    }
    int bufferSize = 4096;
    int blockSize = m_pInput->GetBlockSize();

    if (blockSize > 1 && seekable) // non seakable input streams are not supposed to set block size
      bufferSize = blockSize;

    unsigned char* buffer = (unsigned char*)av_malloc(bufferSize);
    m_ioContext = avio_alloc_context(buffer, bufferSize, 0, this, dvd_file_read, NULL, dvd_file_seek);

    if (blockSize > 1 && seekable)
      m_ioContext->max_packet_size = bufferSize;

    if (!seekable)
      m_ioContext->seekable = 0;

    std::string content = m_pInput->GetContent();
    StringUtils::ToLower(content);
    if (StringUtils::StartsWith(content, "audio/l16"))
      iformat = av_find_input_format("s16be");

    if (iformat == nullptr)
    {
      // let ffmpeg decide which demuxer we have to open
      bool trySPDIFonly = (m_pInput->GetContent() == "audio/x-spdif-compressed");

      if (!trySPDIFonly)
        av_probe_input_buffer(m_ioContext, &iformat, strFile.c_str(), NULL, 0, 0);

      // Use the more low-level code in case we have been built against an old
      // FFmpeg without the above av_probe_input_buffer(), or in case we only
      // want to probe for spdif (DTS or IEC 61937) compressed audio
      // specifically, or in case the file is a wav which may contain DTS or
      // IEC 61937 (e.g. ac3-in-wav) and we want to check for those formats.
      if (trySPDIFonly || (iformat && strcmp(iformat->name, "wav") == 0))
      {
        AVProbeData pd;
        int probeBufferSize = 32768;
        std::unique_ptr<uint8_t[]> probe_buffer (new uint8_t[probeBufferSize + AVPROBE_PADDING_SIZE]);

        // init probe data
        pd.buf = probe_buffer.get();
        pd.filename = strFile.c_str();

        // read data using avformat's buffers
        pd.buf_size = avio_read(m_ioContext, pd.buf, probeBufferSize);
        if (pd.buf_size <= 0)
        {
          CLog::Log(LOGERROR, "{} - error reading from input stream, {}", __FUNCTION__,
                    CURL::GetRedacted(strFile));
          return false;
        }
        memset(pd.buf + pd.buf_size, 0, AVPROBE_PADDING_SIZE);

        // restore position again
        avio_seek(m_ioContext , 0, SEEK_SET);

        // the advancedsetting is for allowing the user to force outputting the
        // 44.1 kHz DTS wav file as PCM, so that an A/V receiver can decode
        // it (this is temporary until we handle 44.1 kHz passthrough properly)
        if (trySPDIFonly || (iformat && strcmp(iformat->name, "wav") == 0 && !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_VideoPlayerIgnoreDTSinWAV))
        {
          // check for spdif and dts
          // This is used with wav files and audio CDs that may contain
          // a DTS or AC3 track padded for S/PDIF playback. If neither of those
          // is present, we assume it is PCM audio.
          // AC3 is always wrapped in iec61937 (ffmpeg "spdif"), while DTS
          // may be just padded.
          const AVInputFormat* iformat2 = av_find_input_format("spdif");
          if (iformat2 && iformat2->read_probe(&pd) > AVPROBE_SCORE_MAX / 4)
          {
            iformat = iformat2;
          }
          else
          {
            // not spdif or no spdif demuxer, try dts
            iformat2 = av_find_input_format("dts");

            if (iformat2 && iformat2->read_probe(&pd) > AVPROBE_SCORE_MAX / 4)
            {
              iformat = iformat2;
            }
            else if (trySPDIFonly)
            {
              // not dts either, return false in case we were explicitly
              // requested to only check for S/PDIF padded compressed audio
              CLog::Log(LOGDEBUG, "{} - not spdif or dts file, falling back", __FUNCTION__);
              return false;
            }
          }
        }
      }

      if (!iformat)
      {
        std::string content = m_pInput->GetContent();

        /* check if we can get a hint from content */
        if (content.compare("audio/aacp") == 0)
          iformat = av_find_input_format("aac");
        else if (content.compare("audio/aac") == 0)
          iformat = av_find_input_format("aac");
        else if (content.compare("video/flv") == 0)
          iformat = av_find_input_format("flv");
        else if (content.compare("video/x-flv") == 0)
          iformat = av_find_input_format("flv");
      }

      if (!iformat)
      {
        CLog::Log(LOGERROR, "{} - error probing input format, {}", __FUNCTION__,
                  CURL::GetRedacted(strFile));
        return false;
      }
      else
      {
        if (iformat->name)
          CLog::Log(LOGDEBUG, "{} - probing detected format [{}]", __FUNCTION__, iformat->name);
        else
          CLog::Log(LOGDEBUG, "{} - probing detected unnamed format", __FUNCTION__);
      }
    }


    m_pFormatContext->pb = m_ioContext;

    AVDictionary* options = NULL;
    if (iformat->name && (strcmp(iformat->name, "mp3") == 0 || strcmp(iformat->name, "mp2") == 0))
    {
      CLog::Log(LOGDEBUG, "{} - setting usetoc to 0 for accurate VBR MP3 seek", __FUNCTION__);
      av_dict_set(&options, "usetoc", "0", 0);
    }

    if (StringUtils::StartsWith(content, "audio/l16"))
    {
      int channels = 2;
      int samplerate = 44100;
      GetL16Parameters(channels, samplerate);
      av_dict_set_int(&options, "channels", channels, 0);
      av_dict_set_int(&options, "sample_rate", samplerate, 0);
    }

    if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
    {
      CLog::Log(LOGERROR, "{} - Error, could not open file {}", __FUNCTION__,
                CURL::GetRedacted(strFile));
      Dispose();
      av_dict_free(&options);
      return false;
    }
    av_dict_free(&options);
  }

  // Avoid detecting framerate if advancedsettings.xml says so
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoFpsDetect == 0)
      m_pFormatContext->fps_probe_size = 0;

  // analyse very short to speed up mjpeg playback start
  if (iformat && (strcmp(iformat->name, "mjpeg") == 0) && m_ioContext->seekable == 0)
    av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

  bool skipCreateStreams = false;
  bool isBluray = pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY);

  // this should never happen. Log it to inform about the error.
  if (m_pFormatContext->nb_streams > 0 && m_pFormatContext->streams == nullptr)
  {
    CLog::LogF(LOGERROR, "Detected number of streams is greater than zero but AVStream array is "
                         "empty. Please report this bug.");
  }

  // don't re-open mpegts streams with hevc encoding as the params are not correctly detected again
  if (iformat && (strcmp(iformat->name, "mpegts") == 0) && !fileinfo && !isBluray &&
      m_pFormatContext->nb_streams > 0 && m_pFormatContext->streams != nullptr &&
      m_pFormatContext->streams[0]->codecpar->codec_id != AV_CODEC_ID_HEVC)
  {
    av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);
    m_checkTransportStream = true;
    skipCreateStreams = true;
  }
  else if (!iformat || ((strcmp(iformat->name, "mpegts") != 0) ||
                        ((strcmp(iformat->name, "mpegts") == 0) &&
                         m_pFormatContext->nb_streams > 0 && m_pFormatContext->streams != nullptr &&
                         m_pFormatContext->streams[0]->codecpar->codec_id == AV_CODEC_ID_HEVC)))
  {
    m_streaminfo = true;
  }

  // we need to know if this is matroska, avi or sup later
  m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0;	// for "matroska.webm"
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;
  m_bSup = strcmp(m_pFormatContext->iformat->name, "sup") == 0;

  if (m_streaminfo)
  {
    /* to speed up dvd switches, only analyse very short */
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
      av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

    CLog::Log(LOGDEBUG, "{} - avformat_find_stream_info starting", __FUNCTION__);
    int iErr = avformat_find_stream_info(m_pFormatContext, NULL);
    if (iErr < 0)
    {
      CLog::Log(LOGWARNING, "could not find codec parameters for {}", CURL::GetRedacted(strFile));
      if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) ||
          m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY) ||
          (m_pFormatContext->nb_streams == 1 &&
           m_pFormatContext->streams[0]->codecpar->codec_id == AV_CODEC_ID_AC3) ||
          m_checkTransportStream)
      {
        // special case, our codecs can still handle it.
      }
      else
      {
        Dispose();
        return false;
      }
    }
    CLog::Log(LOGDEBUG, "{} - av_find_stream_info finished", __FUNCTION__);

    // print some extra information
    av_dump_format(m_pFormatContext, 0, CURL::GetRedacted(strFile).c_str(), 0);

    if (m_checkTransportStream)
    {
      // make sure we start video with an i-frame
      ResetVideoStreams();
    }
  }
  else
  {
    m_program = 0;
    m_checkTransportStream = true;
    skipCreateStreams = true;
  }

  // reset any timeout
  m_timeout.SetInfinite();

  // if format can be nonblocking, let's use that
  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  // select the correct program if requested
  m_initialProgramNumber = UINT_MAX;
  CVariant programProp(pInput->GetProperty("program"));
  if (!programProp.isNull())
    m_initialProgramNumber = static_cast<int>(programProp.asInteger());

  // in case of mpegts and we have not seen pat/pmt, defer creation of streams
  if (!skipCreateStreams || m_pFormatContext->nb_programs > 0)
  {
    unsigned int nProgram = UINT_MAX;
    if (m_pFormatContext->nb_programs > 0)
    {
      // select the correct program if requested
      if (m_initialProgramNumber != UINT_MAX)
      {
        for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
        {
          if (m_pFormatContext->programs[i]->program_num == static_cast<int>(m_initialProgramNumber))
          {
            nProgram = i;
            m_initialProgramNumber = UINT_MAX;
            break;
          }
        }
      }
      else if (m_pFormatContext->iformat && strcmp(m_pFormatContext->iformat->name, "hls") == 0)
      {
        nProgram = HLSSelectProgram();
      }
      else
      {
        // skip programs without or empty audio/video streams
        for (unsigned int i = 0; nProgram == UINT_MAX && i < m_pFormatContext->nb_programs; i++)
        {
          for (unsigned int j = 0; j < m_pFormatContext->programs[i]->nb_stream_indexes; j++)
          {
            int idx = m_pFormatContext->programs[i]->stream_index[j];
            AVStream* st = m_pFormatContext->streams[idx];
            // Related to https://patchwork.ffmpeg.org/project/ffmpeg/patch/20210429143825.53040-1-jamrial@gmail.com/
            // has been replaced with AVSTREAM_EVENT_FLAG_NEW_PACKETS.
            if ((st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
                 (st->event_flags & AVSTREAM_EVENT_FLAG_NEW_PACKETS)) ||
                (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && st->codecpar->sample_rate > 0))
            {
              nProgram = i;
              break;
            }
          }
        }
      }
    }
    CreateStreams(nProgram);
  }

  m_newProgram = m_program;

  // allow IsProgramChange to return true
  if (skipCreateStreams && GetNrOfStreams() == 0)
    m_program = 0;

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  m_startTime = 0;
  m_seekStream = -1;

  if (m_checkTransportStream && m_streaminfo)
  {
    int64_t duration = m_pFormatContext->duration;
    std::shared_ptr<CDVDInputStream> pInputStream = m_pInput;
    Dispose();
    m_reopen = true;
    if (!Open(pInputStream, false))
      return false;
    m_pFormatContext->duration = duration;
  }

  return true;
}

void CDVDDemuxFFmpeg::Dispose()
{
  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  if (m_pFormatContext)
  {
    if (m_ioContext && m_pFormatContext->pb && m_pFormatContext->pb != m_ioContext)
    {
      CLog::Log(LOGWARNING, "CDVDDemuxFFmpeg::Dispose - demuxer changed our byte context behind our back, possible memleak");
      m_ioContext = m_pFormatContext->pb;
    }
    avformat_close_input(&m_pFormatContext);
  }

  if (m_ioContext)
  {
    av_free(m_ioContext->buffer);
    av_free(m_ioContext);
  }

  m_ioContext = NULL;
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  DisposeStreams();

  m_pInput = NULL;
}

bool CDVDDemuxFFmpeg::Reset()
{
  std::shared_ptr<CDVDInputStream> pInputStream = m_pInput;
  Dispose();
  return Open(pInputStream, false);
}

void CDVDDemuxFFmpeg::Flush()
{
  if (m_pFormatContext)
  {
    if (m_pFormatContext->pb)
      avio_flush(m_pFormatContext->pb);
    avformat_flush(m_pFormatContext);
  }

  m_currentPts = DVD_NOPTS_VALUE;

  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  m_seekToKeyFrame = false;
}

void CDVDDemuxFFmpeg::Abort()
{
  m_timeout.SetExpired();
}

void CDVDDemuxFFmpeg::SetSpeed(int iSpeed)
{
  if (!m_pFormatContext)
    return;

  if (m_speed == iSpeed)
    return;

  if (m_speed != DVD_PLAYSPEED_PAUSE && iSpeed == DVD_PLAYSPEED_PAUSE)
    av_read_pause(m_pFormatContext);
  else if (m_speed == DVD_PLAYSPEED_PAUSE && iSpeed != DVD_PLAYSPEED_PAUSE)
    av_read_play(m_pFormatContext);
  m_speed = iSpeed;

  AVDiscard discard = AVDISCARD_NONE;
  if (m_speed > 4 * DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_NONKEY;
  else if (m_speed > 2 * DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_BIDIR;
  else if (m_speed < DVD_PLAYSPEED_PAUSE)
    discard = AVDISCARD_NONKEY;


  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    if (m_pFormatContext->streams[i])
    {
      if (m_pFormatContext->streams[i]->discard != AVDISCARD_ALL)
        m_pFormatContext->streams[i]->discard = discard;
    }
  }
}

AVDictionary* CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput()
{
  const std::shared_ptr<CDVDInputStreamFFmpeg> input =
    std::dynamic_pointer_cast<CDVDInputStreamFFmpeg>(m_pInput);

  CURL url = m_pInput->GetURL();
  AVDictionary* options = nullptr;

  // For a local file we need the following protocol whitelist
  if (url.GetProtocol().empty() || url.IsProtocol("file"))
    av_dict_set(&options, "protocol_whitelist", "file,http,https,tcp,tls,crypto", 0);

  if (url.IsProtocol("http") || url.IsProtocol("https"))
  {
    std::map<std::string, std::string> protocolOptions;
    url.GetProtocolOptions(protocolOptions);
    std::string headers;
    bool hasUserAgent = false;
    bool hasCookies = false;
    for(std::map<std::string, std::string>::const_iterator it = protocolOptions.begin(); it != protocolOptions.end(); ++it)
    {
      std::string name = it->first;
      StringUtils::ToLower(name);
      const std::string &value = it->second;

      // set any of these ffmpeg options
      if (name == "seekable" || name == "reconnect" || name == "reconnect_at_eof" ||
          name == "reconnect_streamed" || name == "reconnect_delay_max" ||
          name == "icy" || name == "icy_metadata_headers" || name == "icy_metadata_packet")
      {
        CLog::Log(LOGDEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option '{}: {}'",
                  it->first, value);
        av_dict_set(&options, name.c_str(), value.c_str(), 0);
      }
      // map some standard http headers to the ffmpeg related options
      else if (name == "user-agent")
      {
        av_dict_set(&options, "user_agent", value.c_str(), 0);
        CLog::Log(
            LOGDEBUG,
            "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option 'user_agent: {}'",
            value);
        hasUserAgent = true;
      }
      else if (name == "cookies")
      {
        // in the plural option expect multiple Set-Cookie values. They are passed \n delimited to FFMPEG
        av_dict_set(&options, "cookies", value.c_str(), 0);
        CLog::Log(LOGDEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option 'cookies: {}'",
                  value);
        hasCookies = true;
      }
      else if (name == "cookie")
      {
        CLog::Log(
            LOGDEBUG,
            "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg header value 'cookie: {}'",
            value);
        headers.append(it->first).append(": ").append(value).append("\r\n");
        hasCookies = true;
      }
      // other standard headers (see https://en.wikipedia.org/wiki/List_of_HTTP_header_fields) are appended as actual headers
      else if (name == "accept" || name == "accept-language" || name == "accept-datetime" ||
               name == "authorization" || name == "cache-control" || name == "connection" || name == "content-md5" ||
               name == "date" || name == "expect" || name == "forwarded" || name == "from" || name == "if-match" ||
               name == "if-modified-since" || name == "if-none-match" || name == "if-range" || name == "if-unmodified-since" ||
               name == "max-forwards" || name == "origin" || name == "pragma" || name == "range" || name == "referer" ||
               name == "te" || name == "upgrade" || name == "via" || name == "warning" || name == "x-requested-with" ||
               name == "dnt" || name == "x-forwarded-for" || name == "x-forwarded-host" || name == "x-forwarded-proto" ||
               name == "front-end-https" || name == "x-http-method-override" || name == "x-att-deviceid" ||
               name == "x-wap-profile" || name == "x-uidh" || name == "x-csrf-token" || name == "x-request-id" ||
               name == "x-correlation-id")
      {
        if (name == "authorization")
        {
          CLog::Log(LOGDEBUG,
                    "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding custom header option '{}: "
                    "***********'",
                    it->first);
        }
        else
        {
          CLog::Log(
              LOGDEBUG,
              "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding custom header option '{}: {}'",
              it->first, value);
        }
        headers.append(it->first).append(": ").append(value).append("\r\n");
      }
      // Any other headers that need to be sent would be user defined and should be prefixed
      // by a `!`. We mask these values so we don't log anything we shouldn't
      else if (name.length() > 0 && name[0] == '!')
      {
        CLog::Log(LOGDEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding user custom header option "
                  "'{}: ***********'",
                  it->first);
        headers.append(it->first.substr(1)).append(": ").append(value).append("\r\n");
      }
      // for everything else we ignore the headers options if not specified above
      else
      {
        CLog::Log(LOGDEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() ignoring header option '{}'",
                  it->first);
      }
    }
    if (!hasUserAgent)
    {
      // set default xbmc user-agent.
      av_dict_set(&options, "user_agent", CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_userAgent.c_str(), 0);
    }

    if (!headers.empty())
      av_dict_set(&options, "headers", headers.c_str(), 0);

    if (!hasCookies)
    {
      std::string cookies;
      if (XFILE::CCurlFile::GetCookies(url, cookies))
        av_dict_set(&options, "cookies", cookies.c_str(), 0);
    }
  }

  if (input)
  {
    const std::string host = input->GetProxyHost();
    if (!host.empty() && input->GetProxyType() == "http")
    {
      std::ostringstream urlStream;

      const uint16_t port = input->GetProxyPort();
      const std::string user = input->GetProxyUser();
      const std::string password = input->GetProxyPassword();

      urlStream << "http://";

      if (!user.empty()) {
        urlStream << user;
        if (!password.empty())
          urlStream << ":" << password;
        urlStream << "@";
      }

      urlStream << host << ':' << port;

      av_dict_set(&options, "http_proxy", urlStream.str().c_str(), 0);
    }

    // rtsp options
    if (url.IsProtocol("rtsp"))
    {
      CVariant transportProp{m_pInput->GetProperty("rtsp_transport")};
      if (!transportProp.isNull() &&
          (transportProp == "tcp" || transportProp == "udp" || transportProp == "udp_multicast"))
      {
        CLog::LogF(LOGDEBUG, "GetFFMpegOptionsFromInput() Forcing rtsp transport protocol to '{}'",
                   transportProp.asString());
        av_dict_set(&options, "rtsp_transport", transportProp.asString().c_str(), 0);
      }
    }

    // rtmp options
    if (url.IsProtocol("rtmp")  || url.IsProtocol("rtmpt")  ||
        url.IsProtocol("rtmpe") || url.IsProtocol("rtmpte") ||
        url.IsProtocol("rtmps"))
    {
      static const std::map<std::string,std::string> optionmap =
      {{{"SWFPlayer", "rtmp_swfurl"},
        {"swfplayer", "rtmp_swfurl"},
        {"PageURL", "rtmp_pageurl"},
        {"pageurl", "rtmp_pageurl"},
        {"PlayPath", "rtmp_playpath"},
        {"playpath", "rtmp_playpath"},
        {"TcUrl",    "rtmp_tcurl"},
        {"tcurl",    "rtmp_tcurl"},
        {"IsLive",   "rtmp_live"},
        {"islive",   "rtmp_live"},
        {"swfurl",   "rtmp_swfurl"},
        {"swfvfy",   "rtmp_swfverify"},
      }};

      for (const auto& it : optionmap)
      {
        if (input->GetItem().HasProperty(it.first))
        {
          av_dict_set(&options, it.second.c_str(),
                      input->GetItem().GetProperty(it.first).asString().c_str(),0);
        }
      }

      CURL tmpUrl = url;
      std::vector<std::string> opts = StringUtils::Split(tmpUrl.Get(), " ");
      if (opts.size() > 1) // inline rtmp options
      {
        std::string swfurl;
        bool swfvfy=false;
        for (size_t i = 1; i < opts.size(); ++i)
        {
          std::vector<std::string> value = StringUtils::Split(opts[i], "=", 2);
          StringUtils::ToLower(value[0]);
          auto it = optionmap.find(value[0]);
          if (it != optionmap.end())
          {
            if (value[0] == "swfurl" || value[0] == "SWFPlayer")
              swfurl = value[1];
            if (value[0] == "swfvfy" && (value[1] == "true" || value[1] == "1"))
              swfvfy = true;
            else
              av_dict_set(&options, it->second.c_str(), value[1].c_str(), 0);
          }
          if (swfvfy)
            av_dict_set(&options, "rtmp_swfverify", swfurl.c_str(), 0);
        }
        tmpUrl = CURL(opts.front());
      }
    }
  }

  return options;
}

double CDVDDemuxFFmpeg::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completely exact timestamp anyway
  double timestamp = (double)pts * num / den;
  double starttime = 0.0;

  const std::shared_ptr<CDVDInputStream::IMenus> menuInterface =
      std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInput);
  if ((!menuInterface || menuInterface->GetSupportedMenuType() != MenuType::NATIVE) &&
      m_pFormatContext->start_time != static_cast<int64_t>(AV_NOPTS_VALUE))
  {
    starttime = static_cast<double>(m_pFormatContext->start_time) / AV_TIME_BASE;
  }

  if (m_checkTransportStream)
    starttime = m_startTime;

  if (!m_bSup)
  {
    if (timestamp > starttime || m_checkTransportStream)
      timestamp -= starttime;
    // allow for largest possible difference in pts and dts for a single packet
    else if (timestamp + 0.5 > starttime)
      timestamp = 0;
  }

  return timestamp * DVD_TIME_BASE;
}

DemuxPacket* CDVDDemuxFFmpeg::ReadInternal(bool keep)
{
  DemuxPacket* pPacket = NULL;
  // on some cases where the received packet is invalid we will need to return an empty packet (0 length) otherwise the main loop (in CVideoPlayer)
  // would consider this the end of stream and stop.
  bool bReturnEmpty = false;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection); // open lock scope
    if (m_pFormatContext)
    {
      // assume we are not eof
      if (m_pFormatContext->pb)
        m_pFormatContext->pb->eof_reached = 0;

      // check for saved packet after a program change
      if (m_pkt.result < 0)
      {
        // keep track if ffmpeg doesn't always set these
        m_pkt.pkt.size = 0;
        m_pkt.pkt.data = NULL;

        // timeout reads after 100ms
        m_timeout.Set(20s);
        m_pkt.result = av_read_frame(m_pFormatContext, &m_pkt.pkt);
        m_timeout.SetInfinite();
      }

      if (m_pkt.result == AVERROR(EINTR) || m_pkt.result == AVERROR(EAGAIN))
      {
        // timeout, probably no real error, return empty packet
        bReturnEmpty = true;
      }
      else if (m_pkt.result == AVERROR_EOF)
      {
      }
      else if (m_pkt.result < 0)
      {
        Flush();
      }
      // check size and stream index for being in a valid range
      else if (m_pkt.pkt.size < 0 || m_pkt.pkt.stream_index < 0 ||
               m_pkt.pkt.stream_index >= (int)m_pFormatContext->nb_streams)
      {
        // XXX, in some cases ffmpeg returns a negative packet size
        if (m_pFormatContext->pb && !m_pFormatContext->pb->eof_reached)
        {
          CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
          bReturnEmpty = true;
          Flush();
        }
        else
          CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() returned invalid packet and eof reached");

        m_pkt.result = -1;
        av_packet_unref(&m_pkt.pkt);
      }
      else
      {
        ParsePacket(&m_pkt.pkt);

        if (IsProgramChange())
        {
          CLog::Log(LOGINFO, "CDVDDemuxFFmpeg::Read() stream change");
          av_dump_format(m_pFormatContext, 0, CURL::GetRedacted(m_pInput->GetFileName()).c_str(),
                         0);

          // update streams
          CreateStreams(m_program);

          pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
          pPacket->iStreamId = DMX_SPECIALID_STREAMCHANGE;
          pPacket->demuxerId = m_demuxerId;

          return pPacket;
        }

        AVStream* stream = m_pFormatContext->streams[m_pkt.pkt.stream_index];

        if (IsTransportStreamReady())
        {
          if (m_program != UINT_MAX)
          {
            /* check so packet belongs to selected program */
            for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes;
                 i++)
            {
              if (m_pkt.pkt.stream_index ==
                  (int)m_pFormatContext->programs[m_program]->stream_index[i])
              {
                pPacket = CDVDDemuxUtils::AllocateDemuxPacket(m_pkt.pkt.size);
                break;
              }
            }

            if (!pPacket)
              bReturnEmpty = true;
          }
          else
            pPacket = CDVDDemuxUtils::AllocateDemuxPacket(m_pkt.pkt.size);
        }
        else
          bReturnEmpty = true;

        if (pPacket)
        {
          if (m_bAVI && stream->codecpar && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
          {
            // AVI's always have borked pts, specially if m_pFormatContext->flags includes
            // AVFMT_FLAG_GENPTS so always use dts
            m_pkt.pkt.pts = AV_NOPTS_VALUE;
          }

          // copy contents into our own packet
          pPacket->iSize = m_pkt.pkt.size;

          // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
          if (m_pkt.pkt.data)
            memcpy(pPacket->pData, m_pkt.pkt.data, pPacket->iSize);

          pPacket->pts =
              ConvertTimestamp(m_pkt.pkt.pts, stream->time_base.den, stream->time_base.num);
          pPacket->dts =
              ConvertTimestamp(m_pkt.pkt.dts, stream->time_base.den, stream->time_base.num);
          pPacket->duration = DVD_SEC_TO_TIME((double)m_pkt.pkt.duration * stream->time_base.num /
                                              stream->time_base.den);

          CDVDDemuxUtils::StoreSideData(pPacket, &m_pkt.pkt);

          CDVDInputStream::IDisplayTime* inputStream = m_pInput->GetIDisplayTime();
          if (inputStream)
          {
            int dispTime = inputStream->GetTime();
            if (m_displayTime != dispTime)
            {
              m_displayTime = dispTime;
              if (pPacket->dts != DVD_NOPTS_VALUE)
              {
                m_dtsAtDisplayTime = pPacket->dts;
              }
            }
            if (m_dtsAtDisplayTime != DVD_NOPTS_VALUE && pPacket->dts != DVD_NOPTS_VALUE)
            {
              pPacket->dispTime = m_displayTime;
              pPacket->dispTime += DVD_TIME_TO_MSEC(pPacket->dts - m_dtsAtDisplayTime);
            }
          }

          // used to guess streamlength
          if (pPacket->dts != DVD_NOPTS_VALUE &&
              (pPacket->dts > m_currentPts || m_currentPts == DVD_NOPTS_VALUE))
            m_currentPts = pPacket->dts;
          else if (pPacket->pts != DVD_NOPTS_VALUE &&
              (pPacket->pts > m_currentPts || m_currentPts == DVD_NOPTS_VALUE))
            m_currentPts = pPacket->pts;

          // store internal id until we know the continuous id presented to player
          // the stream might not have been created yet
          pPacket->iStreamId = m_pkt.pkt.stream_index;
        }
        if (!keep)
        {
          m_pkt.result = -1;
          av_packet_unref(&m_pkt.pkt);
        }
      }
    }
  } // end of lock scope
  if (bReturnEmpty && !pPacket)
    pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);

  if (!pPacket)
    return nullptr;

  // check streams, can we make this a bit more simple?
  if (pPacket->iStreamId >= 0)
  {
    CDemuxStream* stream = GetStream(pPacket->iStreamId);
    if (!stream ||
        stream->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        stream->codec != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->codec_id)
    {
      // content has changed, or stream did not yet exist
      stream = AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (stream->type == STREAM_AUDIO)
    {
      CDemuxStreamAudioFFmpeg* audiostream = dynamic_cast<CDemuxStreamAudioFFmpeg*>(stream);
      int codecparChannels =
          m_pFormatContext->streams[pPacket->iStreamId]->codecpar->ch_layout.nb_channels;
      if (audiostream && (audiostream->iChannels != codecparChannels ||
                          audiostream->iSampleRate !=
                              m_pFormatContext->streams[pPacket->iStreamId]->codecpar->sample_rate))
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
    }
    else if (stream->type == STREAM_VIDEO)
    {
      if (static_cast<CDemuxStreamVideo*>(stream)->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->width ||
          static_cast<CDemuxStreamVideo*>(stream)->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->height)
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
      if (stream && stream->codec == AV_CODEC_ID_H264)
        pPacket->recoveryPoint = m_seekToKeyFrame;
      m_seekToKeyFrame = false;
    }
    if (!stream)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
      return pPacket;
    }

    pPacket->iStreamId = stream->uniqueId;
    pPacket->demuxerId = m_demuxerId;
  }
  return pPacket;
}

DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  return ReadInternal(false);
}

bool CDVDDemuxFFmpeg::SeekTime(double time, bool backwards, double* startpts)
{
  bool hitEnd = false;

  if (!m_pInput)
    return false;

  if (time < 0)
  {
    time = 0;
    hitEnd = true;
  }

  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  CDVDInputStream::IPosTime* ist = m_pInput->GetIPosTime();
  if (ist)
  {
    if (!ist->PosTime(static_cast<int>(time)))
      return false;

    if (startpts)
      *startpts = DVD_NOPTS_VALUE;

    Flush();

    return true;
  }

  if (!m_pInput->Seek(0, SEEK_POSSIBLE) &&
      !m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    CLog::Log(LOGDEBUG, "{} - input stream reports it is not seekable", __FUNCTION__);
    return false;
  }

  int64_t seek_pts = (int64_t)time * (AV_TIME_BASE / 1000);
  bool ismp3 = m_pFormatContext->iformat && (strcmp(m_pFormatContext->iformat->name, "mp3") == 0);

  if (m_checkTransportStream)
  {
    XbmcThreads::EndTime<> timer(1000ms);

    while (!IsTransportStreamReady())
    {
      DemuxPacket* pkt = Read();
      if (pkt)
        CDVDDemuxUtils::FreeDemuxPacket(pkt);
      else
        KODI::TIME::Sleep(10ms);
      m_pkt.result = -1;
      av_packet_unref(&m_pkt.pkt);

      if (timer.IsTimePast())
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::{} - Timed out waiting for video to be ready",
                  __FUNCTION__);
        return false;
      }
    }

    AVStream* st = m_pFormatContext->streams[m_seekStream];
    seek_pts = av_rescale(static_cast<int64_t>(m_startTime + time / 1000), st->time_base.den,
                          st->time_base.num);
  }
  else if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE && !ismp3 && !m_bSup)
    seek_pts += m_pFormatContext->start_time;

  int ret;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    ret = av_seek_frame(m_pFormatContext, m_seekStream, seek_pts, backwards ? AVSEEK_FLAG_BACKWARD : 0);

    if (ret < 0)
    {
      int64_t starttime = m_pFormatContext->start_time;
      if (m_checkTransportStream)
      {
        AVStream* st = m_pFormatContext->streams[m_seekStream];
        starttime =
            av_rescale(static_cast<int64_t>(m_startTime), st->time_base.num, st->time_base.den);
      }

      // demuxer can return failure, if seeking behind eof
      if (m_pFormatContext->duration &&
          seek_pts >= (m_pFormatContext->duration + starttime))
      {
        // force eof
        // files of realtime streams may grow
        if (!m_pInput->IsRealtime())
          m_pInput->Close();
        else
          ret = 0;
      }
      else if (m_pInput->IsEOF())
        ret = 0;
    }

    if (ret >= 0)
    {
      if (m_pFormatContext->iformat->read_seek)
        m_seekToKeyFrame = true;
      m_currentPts = DVD_NOPTS_VALUE;
    }
  }

  if (ret >= 0)
  {
    XbmcThreads::EndTime<> timer(1000ms);
    while (m_currentPts == DVD_NOPTS_VALUE && !timer.IsTimePast())
    {
      m_pkt.result = -1;
      av_packet_unref(&m_pkt.pkt);

      DemuxPacket* pkt = ReadInternal(true);
      if (!pkt)
      {
        KODI::TIME::Sleep(10ms);
        continue;
      }
      CDVDDemuxUtils::FreeDemuxPacket(pkt);
    }
  }

  if (m_currentPts == DVD_NOPTS_VALUE)
    CLog::Log(LOGDEBUG, "{} - unknown position after seek", __FUNCTION__);
  else
    CLog::Log(LOGDEBUG, "{} - seek ended up on time {}", __FUNCTION__,
              (int)(m_currentPts / DVD_TIME_BASE * 1000));

  // in this case the start time is requested time
  if (startpts)
    *startpts = DVD_MSEC_TO_TIME(time);

  if (ret >= 0)
  {
    if (!hitEnd)
      return true;
    else
      return false;
  }
  else
    return false;
}

bool CDVDDemuxFFmpeg::SeekByte(int64_t pos)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  int ret = av_seek_frame(m_pFormatContext, -1, pos, AVSEEK_FLAG_BYTE);

  if (ret >= 0)
    m_currentPts = DVD_NOPTS_VALUE;

  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  return (ret >= 0);
}

int CDVDDemuxFFmpeg::GetStreamLength()
{
  if (!m_pFormatContext)
    return 0;

  if (m_pFormatContext->duration < 0 ||
      m_pFormatContext->duration == AV_NOPTS_VALUE)
    return 0;

  return (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
}

/**
 * @brief Finds stream based on unique id
 */
CDemuxStream* CDVDDemuxFFmpeg::GetStream(int iStreamId) const
{
  auto it = m_streams.find(iStreamId);
  if (it != m_streams.end())
    return it->second;

  return nullptr;
}

std::vector<CDemuxStream*> CDVDDemuxFFmpeg::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  streams.reserve(m_streams.size());
  for (auto& iter : m_streams)
    streams.push_back(iter.second);

  return streams;
}

int CDVDDemuxFFmpeg::GetNrOfStreams() const
{
  return static_cast<int>(m_streams.size());
}

int CDVDDemuxFFmpeg::GetPrograms(std::vector<ProgramInfo>& programs)
{
  programs.clear();
  if (!m_pFormatContext || m_pFormatContext->nb_programs <= 1)
    return 0;

  for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
  {
    std::ostringstream os;
    ProgramInfo prog;
    prog.id = i;
    os << i;
    prog.name = os.str();
    if (i == m_program)
      prog.playing = true;

    if (!m_pFormatContext->programs[i]->metadata)
      continue;

    AVDictionaryEntry* tag = av_dict_get(m_pFormatContext->programs[i]->metadata, "", nullptr, AV_DICT_IGNORE_SUFFIX);
    while (tag)
    {
      os << " - " << tag->key << ": " << tag->value;
      tag = av_dict_get(m_pFormatContext->programs[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
    }
    prog.name = os.str();
    programs.push_back(prog);
  }
  return static_cast<int>(programs.size());
}

void CDVDDemuxFFmpeg::SetProgram(int progId)
{
  m_newProgram = progId;
}

double CDVDDemuxFFmpeg::SelectAspect(AVStream* st, bool& forced)
{
  // trust matroska container
  if (m_bMatroska && st->sample_aspect_ratio.num != 0)
  {
    forced = true;
    double dar = av_q2d(st->sample_aspect_ratio);
    // for stereo modes, use codec aspect ratio
    AVDictionaryEntry* entry = av_dict_get(st->metadata, "stereo_mode", NULL, 0);
    if (entry)
    {
      if (strcmp(entry->value, "left_right") == 0 || strcmp(entry->value, "right_left") == 0)
        dar /= 2;
      else if (strcmp(entry->value, "top_bottom") == 0 || strcmp(entry->value, "bottom_top") == 0)
        dar *= 2;
    }
    return dar;
  }

  /* if stream aspect is 1:1 or 0:0 use codec aspect */
  if ((st->sample_aspect_ratio.den == 1 || st->sample_aspect_ratio.den == 0) &&
     (st->sample_aspect_ratio.num == 1 || st->sample_aspect_ratio.num == 0) &&
      st->codecpar->sample_aspect_ratio.num != 0)
  {
    forced = false;
    return av_q2d(st->codecpar->sample_aspect_ratio);
  }

  if (st->sample_aspect_ratio.num != 0)
  {
    forced = true;
    return av_q2d(st->sample_aspect_ratio);
  }

  forced = false;
  return 0.0;
}

void CDVDDemuxFFmpeg::CreateStreams(unsigned int program)
{
  DisposeStreams();

  // add the ffmpeg streams to our own stream map
  if (m_pFormatContext->nb_programs)
  {
    // check if desired program is available
    if (program < m_pFormatContext->nb_programs)
    {
      m_program = program;
      m_streamsInProgram = m_pFormatContext->programs[program]->nb_stream_indexes;
      m_pFormatContext->programs[program]->discard = AVDISCARD_NONE;
    }
    else
      m_program = UINT_MAX;

    // look for first non empty stream and discard nonselected programs
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      if (m_program == UINT_MAX && m_pFormatContext->programs[i]->nb_stream_indexes > 0)
      {
        m_program = i;
      }

      if (i != m_program)
        m_pFormatContext->programs[i]->discard = AVDISCARD_ALL;
    }
    if (m_program != UINT_MAX)
    {
      m_pFormatContext->programs[m_program]->discard = AVDISCARD_NONE;

      // add streams from selected program
      for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
      {
        int streamIdx = m_pFormatContext->programs[m_program]->stream_index[i];
        m_pFormatContext->streams[streamIdx]->discard = AVDISCARD_NONE;
        AddStream(streamIdx);
      }

      // discard all unneeded streams
      for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      {
        m_pFormatContext->streams[i]->discard = AVDISCARD_NONE;
        if (GetStream(i) == nullptr)
          m_pFormatContext->streams[i]->discard = AVDISCARD_ALL;
      }
    }
  }
  else
    m_program = UINT_MAX;

  // if there were no programs or they were all empty, add all streams
  if (m_program == UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      AddStream(i);
  }
}

void CDVDDemuxFFmpeg::DisposeStreams()
{
  std::map<int, CDemuxStream*>::iterator it;
  for(it = m_streams.begin(); it != m_streams.end(); ++it)
    delete it->second;
  m_streams.clear();
  m_parsers.clear();
}

CDemuxStream* CDVDDemuxFFmpeg::AddStream(int streamIdx)
{
  AVStream* pStream = m_pFormatContext->streams[streamIdx];
  if (pStream && pStream->discard != AVDISCARD_ALL)
  {
    // Video (mp4) from GoPro cameras can have a 'meta' track used for a file repair containing
    // 'fdsc' data, this is also called the SOS track.
    if (pStream->codecpar->codec_tag == MKTAG('f','d','s','c'))
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream - discarding fdsc stream");
      pStream->discard = AVDISCARD_ALL;
      return nullptr;
    }

    CDemuxStream* stream = nullptr;

    switch (pStream->codecpar->codec_type)
    {
      case AVMEDIA_TYPE_AUDIO:
      {
        CDemuxStreamAudioFFmpeg* st = new CDemuxStreamAudioFFmpeg(pStream);
        stream = st;
        int codecparChannels = pStream->codecpar->ch_layout.nb_channels;
        int codecparChannelLayout = pStream->codecpar->ch_layout.u.mask;
        st->iChannels = codecparChannels;
        st->iChannelLayout = codecparChannelLayout;
        st->iSampleRate = pStream->codecpar->sample_rate;
        st->iBlockAlign = pStream->codecpar->block_align;
        st->iBitRate = static_cast<int>(pStream->codecpar->bit_rate);
        st->iBitsPerSample = pStream->codecpar->bits_per_raw_sample;
        char buf[32] = {};
        // https://github.com/FFmpeg/FFmpeg/blob/6ccc3989d15/doc/APIchanges#L50-L53
        AVChannelLayout layout = {};
        av_channel_layout_from_mask(&layout, st->iChannelLayout);
        av_channel_layout_describe(&layout, buf, sizeof(buf));
        av_channel_layout_uninit(&layout);
        st->m_channelLayoutName = buf;
        if (st->iBitsPerSample == 0)
          st->iBitsPerSample = pStream->codecpar->bits_per_coded_sample;

        if (av_dict_get(pStream->metadata, "title", NULL, 0))
          st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
      case AVMEDIA_TYPE_VIDEO:
      {
        CDemuxStreamVideoFFmpeg* st = new CDemuxStreamVideoFFmpeg(pStream);
        stream = st;
        if (strcmp(m_pFormatContext->iformat->name, "flv") == 0)
          st->bVFR = true;
        else
          st->bVFR = false;

        // never trust pts in avi files with h264.
        if (m_bAVI && pStream->codecpar->codec_id == AV_CODEC_ID_H264)
          st->bPTSInvalid = true;

        AVRational r_frame_rate = pStream->r_frame_rate;

        //average fps is more accurate for mkv files
        if (m_bMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
        {
          st->iFpsRate = pStream->avg_frame_rate.num;
          st->iFpsScale = pStream->avg_frame_rate.den;
        }
        else if (r_frame_rate.den && r_frame_rate.num)
        {
          st->iFpsRate = r_frame_rate.num;
          st->iFpsScale = r_frame_rate.den;
        }
        else
        {
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }

        st->iWidth = pStream->codecpar->width;
        st->iHeight = pStream->codecpar->height;
        st->fAspect = SelectAspect(pStream, st->bForcedAspect);
        if (pStream->codecpar->height)
          st->fAspect *= (double)pStream->codecpar->width / pStream->codecpar->height;
        st->iOrientation = 0;
        st->iBitsPerPixel = pStream->codecpar->bits_per_coded_sample;
        st->iBitRate = static_cast<int>(pStream->codecpar->bit_rate);
        st->bitDepth = 8;
        const AVPixFmtDescriptor* desc =
            av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pStream->codecpar->format));
        if (desc != nullptr)
          st->bitDepth = desc->comp[0].depth;

        st->colorPrimaries = pStream->codecpar->color_primaries;
        st->colorSpace = pStream->codecpar->color_space;
        st->colorTransferCharacteristic = pStream->codecpar->color_trc;
        st->colorRange = pStream->codecpar->color_range;
        st->hdr_type = DetermineHdrType(pStream);

        // https://github.com/FFmpeg/FFmpeg/blob/release/5.0/doc/APIchanges
        size_t size = 0;
        uint8_t* side_data = nullptr;

        if (st->hdr_type == StreamHdrType::HDR_TYPE_DOLBYVISION)
        {
          side_data = av_stream_get_side_data(pStream, AV_PKT_DATA_DOVI_CONF, &size);
          if (side_data && size)
          {
            st->dovi = *reinterpret_cast<AVDOVIDecoderConfigurationRecord*>(side_data);
          }
        }

        side_data = av_stream_get_side_data(pStream, AV_PKT_DATA_MASTERING_DISPLAY_METADATA, &size);
        if (side_data && size)
        {
          st->masteringMetaData = std::make_shared<AVMasteringDisplayMetadata>(
              *reinterpret_cast<AVMasteringDisplayMetadata*>(side_data));
        }

        side_data = av_stream_get_side_data(pStream, AV_PKT_DATA_CONTENT_LIGHT_LEVEL, &size);
        if (side_data && size)
        {
          st->contentLightMetaData = std::make_shared<AVContentLightMetadata>(
              *reinterpret_cast<AVContentLightMetadata*>(side_data));
        }

        uint8_t* displayMatrixSideData =
            av_stream_get_side_data(pStream, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
        if (displayMatrixSideData)
        {
          const double tetha =
              av_display_rotation_get(reinterpret_cast<int32_t*>(displayMatrixSideData));
          if (!std::isnan(tetha))
          {
            st->iOrientation = ((static_cast<int>(-tetha) % 360) + 360) % 360;
          }
        }

        // detect stereoscopic mode
        std::string stereoMode = GetStereoModeFromMetadata(pStream->metadata);
          // check for metadata in file if detection in stream failed
        if (stereoMode.empty())
          stereoMode = GetStereoModeFromMetadata(m_pFormatContext->metadata);
        if (!stereoMode.empty())
          st->stereo_mode = stereoMode;


        if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          if (pStream->codecpar->codec_id == AV_CODEC_ID_PROBE)
          {
            // fix MPEG-1/MPEG-2 video stream probe returning AV_CODEC_ID_PROBE for still frames.
            // ffmpeg issue 1871, regression from ffmpeg r22831.
            if ((pStream->id & 0xF0) == 0xE0)
            {
              pStream->codecpar->codec_id = AV_CODEC_ID_MPEG2VIDEO;
              pStream->codecpar->codec_tag = MKTAG('M','P','2','V');
              CLog::Log(LOGERROR, "{} - AV_CODEC_ID_PROBE detected, forcing AV_CODEC_ID_MPEG2VIDEO",
                        __FUNCTION__);
            }
          }
        }
        if (av_dict_get(pStream->metadata, "title", NULL, 0))
          st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
      case AVMEDIA_TYPE_DATA:
      {
        stream = new CDemuxStream();
        stream->type = STREAM_DATA;
        break;
      }
      case AVMEDIA_TYPE_SUBTITLE:
      {
        if (pStream->codecpar->codec_id == AV_CODEC_ID_DVB_TELETEXT && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_TELETEXTENABLED))
        {
          CDemuxStreamTeletext* st = new CDemuxStreamTeletext();
          stream = st;
          stream->type = STREAM_TELETEXT;
          break;
        }
        else
        {
          CDemuxStreamSubtitleFFmpeg* st = new CDemuxStreamSubtitleFFmpeg(pStream);
          stream = st;

          if (av_dict_get(pStream->metadata, "title", NULL, 0))
            st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

          break;
        }
      }
      case AVMEDIA_TYPE_ATTACHMENT:
      {
        // MKV attachments. Only bothering with fonts for now.
        AVDictionaryEntry* attachmentMimetype =
            av_dict_get(pStream->metadata, "mimetype", nullptr, 0);

        if (pStream->codecpar->codec_id == AV_CODEC_ID_TTF ||
            pStream->codecpar->codec_id == AV_CODEC_ID_OTF || AttachmentIsFont(attachmentMimetype))
        {
          // Temporary fonts are extracted to the temporary fonts path
          //! @todo: temporary font file management should be completely
          //! removed, by sending font data to the subtitle renderer and
          //! using libass ass_add_font to add the fonts directly in memory.
          std::string filePath{UTILS::FONT::FONTPATH::TEMP};
          XFILE::CDirectory::Create(filePath);

          AVDictionaryEntry* nameTag = av_dict_get(pStream->metadata, "filename", NULL, 0);
          if (nameTag)
          {
            filePath += CUtil::MakeLegalFileName(nameTag->value, LEGAL_WIN32_COMPAT);
            XFILE::CFile file;
            if (pStream->codecpar->extradata && file.OpenForWrite(filePath))
            {
              if (file.Write(pStream->codecpar->extradata, pStream->codecpar->extradata_size) !=
                  pStream->codecpar->extradata_size)
              {
                file.Close();
                XFILE::CFile::Delete(filePath);
                CLog::LogF(LOGDEBUG, "Error saving font file \"{}\"", filePath);
              }
            }
          }
          else
          {
            CLog::LogF(LOGERROR, "Attached font has no name");
          }
        }
        stream = new CDemuxStream();
        stream->type = STREAM_NONE;
        break;
      }
      default:
      {
        // if analyzing streams is skipped, unknown streams may become valid later
        if (m_streaminfo && IsTransportStreamReady())
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream - discarding unknown stream with id: {}",
                    pStream->index);
          pStream->discard = AVDISCARD_ALL;
          return nullptr;
        }
        stream = new CDemuxStream();
        stream->type = STREAM_NONE;
      }
    }

    // generic stuff
    if (pStream->duration != (int64_t)AV_NOPTS_VALUE)
      stream->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    stream->codec = pStream->codecpar->codec_id;
    stream->codec_fourcc = pStream->codecpar->codec_tag;
    stream->profile = pStream->codecpar->profile;
    stream->level = pStream->codecpar->level;

    stream->source = STREAM_SOURCE_DEMUX;
    stream->pPrivate = pStream;
    stream->flags = (StreamFlags)pStream->disposition;

    AVDictionaryEntry* langTag = av_dict_get(pStream->metadata, "language", NULL, 0);
    if (!langTag)
    {
      // only for avi audio streams
      if ((strcmp(m_pFormatContext->iformat->name, "avi") == 0) && (pStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO))
      {
        // only defined for streams 1 to 9
        if ((streamIdx > 0) && (streamIdx < 10))
        {
          // search for language information in RIFF-Header ("IAS1": first language - "IAS9": ninth language)
          char riff_tag_string[5] = {'I', 'A', 'S', (char)(streamIdx + '0'), '\0'};
          langTag = av_dict_get(m_pFormatContext->metadata, riff_tag_string, NULL, 0);
          if (!langTag && (streamIdx == 1))
          {
            // search for language information in RIFF-Header ("ILNG": language)
            langTag = av_dict_get(m_pFormatContext->metadata, "language", NULL, 0);
          }
        }
      }
    }
    if (langTag)
    {
      stream->language = std::string(langTag->value, 3);
      //! @FIXME: Matroska v4 support BCP-47 language code with LanguageIETF element
      //! that have the priority over the Language element, but this is not currently
      //! implemented in to ffmpeg library. Since ffmpeg read only the Language element
      //! all tracks will be identified with same language (of Language element).
      //! As workaround to allow set the right language code we provide the possibility
      //! to set the language code in the title field, this allow to kodi to recognize
      //! the right language and select the right track to be played at playback starts.
      AVDictionaryEntry* title = av_dict_get(pStream->metadata, "title", NULL, 0);
      if (title && title->value)
      {
        const std::string langCode = g_LangCodeExpander.FindLanguageCodeWithSubtag(title->value);
        if (!langCode.empty())
          stream->language = langCode;
      }
    }

    if (stream->type != STREAM_NONE && pStream->codecpar->extradata && pStream->codecpar->extradata_size > 0)
    {
      stream->extraData =
          FFmpegExtraData(pStream->codecpar->extradata, pStream->codecpar->extradata_size);
    }

#ifdef HAVE_LIBBLURAY
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
    {
      // UHD BD have a secondary video stream called by Dolby as enhancement layer.
      // This is not used by streaming services and devices (ATV, Nvidia Shield, XONE).
      if (pStream->id == 0x1015)
      {
        CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream - discarding Dolby Vision stream");
        pStream->discard = AVDISCARD_ALL;
        delete stream;
        return nullptr;
      }
      stream->dvdNavId = pStream->id;

      auto it = std::find_if(m_streams.begin(), m_streams.end(),
        [&stream](const std::pair<int, CDemuxStream*>& v)
        {return (v.second->dvdNavId == stream->dvdNavId) && (v.second->type == stream->type); });

      if (it != m_streams.end())
      {
        if (stream->codec == AV_CODEC_ID_AC3 && it->second->codec == AV_CODEC_ID_TRUEHD)
          CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream - discarding duplicated bluray stream (truehd ac3 core)");
        else
          CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream - discarding duplicate bluray stream {}",
                    stream->codecName);

        pStream->discard = AVDISCARD_ALL;
        delete stream;
        return nullptr;
      }
      std::static_pointer_cast<CDVDInputStreamBluray>(m_pInput)->GetStreamInfo(pStream->id, stream->language);
    }
#endif
    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      // this stuff is really only valid for dvd's.
      // this is so that the physicalid matches the
      // id's reported from libdvdnav
      switch (stream->codec)
      {
        case AV_CODEC_ID_AC3:
          stream->dvdNavId = pStream->id - 128;
          break;
        case AV_CODEC_ID_DTS:
          stream->dvdNavId = pStream->id - 136;
          break;
        case AV_CODEC_ID_MP2:
          stream->dvdNavId = pStream->id - 448;
          break;
        case AV_CODEC_ID_PCM_S16BE:
          stream->dvdNavId = pStream->id - 160;
          break;
        case AV_CODEC_ID_DVD_SUBTITLE:
          stream->dvdNavId = pStream->id - 0x20;
          break;
        default:
          stream->dvdNavId = pStream->id & 0x1f;
          break;
      }
    }

    stream->uniqueId = pStream->index;
    stream->demuxerId = m_demuxerId;

    AddStream(stream->uniqueId, stream);
    return stream;
  }
  else
    return nullptr;
}

/**
 * @brief Adds or updates a demux stream based in ffmpeg id
 */
void CDVDDemuxFFmpeg::AddStream(int streamIdx, CDemuxStream* stream)
{
  std::pair<std::map<int, CDemuxStream*>::iterator, bool> res;

  res = m_streams.insert(std::make_pair(streamIdx, stream));
  if (res.second)
  {
    /* was new stream */
    stream->uniqueId = streamIdx;
  }
  else
  {
    delete res.first->second;
    res.first->second = stream;
  }
  CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream ID: {}", streamIdx);
}


std::string CDVDDemuxFFmpeg::GetFileName()
{
  if (m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

int CDVDDemuxFFmpeg::GetChapterCount()
{
  std::shared_ptr<CDVDInputStream::IChapter> ich = std::dynamic_pointer_cast<CDVDInputStream::IChapter>(m_pInput);
  if (ich)
    return ich->GetChapterCount();

  if (m_pFormatContext == NULL)
    return 0;

  return m_pFormatContext->nb_chapters;
}

int CDVDDemuxFFmpeg::GetChapter()
{
  std::shared_ptr<CDVDInputStream::IChapter> ich = std::dynamic_pointer_cast<CDVDInputStream::IChapter>(m_pInput);
  if (ich)
    return ich->GetChapter();

  if (m_pFormatContext == NULL
  || m_currentPts == DVD_NOPTS_VALUE)
    return 0;

  for(unsigned i = 0; i < m_pFormatContext->nb_chapters; i++)
  {
    AVChapter* chapter = m_pFormatContext->chapters[i];
    if (m_currentPts >= ConvertTimestamp(chapter->start, chapter->time_base.den, chapter->time_base.num)
    && m_currentPts <  ConvertTimestamp(chapter->end,   chapter->time_base.den, chapter->time_base.num))
      return i + 1;
  }

  return 0;
}

void CDVDDemuxFFmpeg::GetChapterName(std::string& strChapterName, int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();
  std::shared_ptr<CDVDInputStream::IChapter> ich = std::dynamic_pointer_cast<CDVDInputStream::IChapter>(m_pInput);
  if (ich)
    ich->GetChapterName(strChapterName, chapterIdx);
  else
  {
    if (chapterIdx <= 0)
      return;

    AVDictionaryEntry* titleTag = av_dict_get(m_pFormatContext->chapters[chapterIdx - 1]->metadata,
                                                          "title", NULL, 0);
    if (titleTag)
      strChapterName = titleTag->value;
  }
}

int64_t CDVDDemuxFFmpeg::GetChapterPos(int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();
  if (chapterIdx <= 0)
    return 0;

  std::shared_ptr<CDVDInputStream::IChapter> ich = std::dynamic_pointer_cast<CDVDInputStream::IChapter>(m_pInput);
  if (ich)
    return ich->GetChapterPos(chapterIdx);

  return static_cast<int64_t>(m_pFormatContext->chapters[chapterIdx - 1]->start * av_q2d(m_pFormatContext->chapters[chapterIdx - 1]->time_base));
}

bool CDVDDemuxFFmpeg::SeekChapter(int chapter, double* startpts)
{
  if (chapter < 1)
    chapter = 1;

  std::shared_ptr<CDVDInputStream::IChapter> ich = std::dynamic_pointer_cast<CDVDInputStream::IChapter>(m_pInput);
  if (ich)
  {
    CLog::Log(LOGDEBUG, "{} - chapter seeking using input stream", __FUNCTION__);
    if (!ich->SeekChapter(chapter))
      return false;

    if (startpts)
    {
      *startpts = DVD_SEC_TO_TIME(static_cast<double>(ich->GetChapterPos(chapter)));
    }

    Flush();
    return true;
  }

  if (m_pFormatContext == NULL)
    return false;

  if (chapter < 1 || chapter > (int)m_pFormatContext->nb_chapters)
    return false;

  AVChapter* ch = m_pFormatContext->chapters[chapter - 1];
  double dts = ConvertTimestamp(ch->start, ch->time_base.den, ch->time_base.num);
  return SeekTime(DVD_TIME_TO_MSEC(dts), true, startpts);
}

std::string CDVDDemuxFFmpeg::GetStreamCodecName(int iStreamId)
{
  CDemuxStream* stream = GetStream(iStreamId);
  std::string strName;
  if (stream)
  {
    /* use profile to determine the DTS type */
    if (stream->codec == AV_CODEC_ID_DTS)
    {
      if (stream->profile == FF_PROFILE_DTS_HD_MA)
        strName = "dtshd_ma";
      else if (stream->profile == FF_PROFILE_DTS_HD_HRA)
        strName = "dtshd_hra";
      else
        strName = "dca";

      return strName;
    }

    const AVCodec* codec = avcodec_find_decoder(stream->codec);
    if (codec)
      strName = avcodec_get_name(codec->id);
  }
  return strName;
}

bool CDVDDemuxFFmpeg::IsProgramChange()
{
  if (m_program == UINT_MAX)
    return false;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  if (m_initialProgramNumber != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
    {
      if (m_pFormatContext->programs[i]->program_num == static_cast<int>(m_initialProgramNumber))
      {
        m_newProgram = i;
        m_initialProgramNumber = UINT_MAX;
        break;
      }
    }
    if (m_initialProgramNumber != UINT_MAX)
      return false;
  }

  if (m_program != m_newProgram)
  {
    m_program = m_newProgram;
    return true;
  }

  if (m_pFormatContext->programs[m_program]->nb_stream_indexes != m_streamsInProgram)
    return true;

  if (m_program >= m_pFormatContext->nb_programs)
    return true;

  for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
  {
    int idx = m_pFormatContext->programs[m_program]->stream_index[i];
    if (m_pFormatContext->streams[idx]->discard >= AVDISCARD_ALL)
      continue;
    CDemuxStream* stream = GetStream(idx);
    if (!stream)
      return true;
    if (m_pFormatContext->streams[idx]->codecpar->codec_id != stream->codec)
      return true;
    if (m_pFormatContext->streams[idx]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      CDemuxStreamAudioFFmpeg* audiostream = dynamic_cast<CDemuxStreamAudioFFmpeg*>(stream);
      int codecparChannels = m_pFormatContext->streams[idx]->codecpar->ch_layout.nb_channels;
      if (audiostream && codecparChannels != audiostream->iChannels)
      {
        return true;
      }
    }
    if (m_pFormatContext->streams[idx]->codecpar->extradata_size !=
        static_cast<int>(stream->extraData.GetSize()))
      return true;
  }
  return false;
}

unsigned int CDVDDemuxFFmpeg::HLSSelectProgram()
{
  unsigned int prog = UINT_MAX;

  int bandwidth = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_NETWORK_BANDWIDTH) * 1000;
  if (bandwidth <= 0)
    bandwidth = INT_MAX;

  int selectedBitrate = 0;
  int selectedRes = 0;
  for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
  {
    int strBitrate = 0;
    AVDictionaryEntry* tag = av_dict_get(m_pFormatContext->programs[i]->metadata, "variant_bitrate", NULL, 0);
    if (tag)
      strBitrate = atoi(tag->value);
    else
      continue;

    int strRes = 0;
    for (unsigned int j = 0; j < m_pFormatContext->programs[i]->nb_stream_indexes; j++)
    {
      int idx = m_pFormatContext->programs[i]->stream_index[j];
      AVStream* pStream = m_pFormatContext->streams[idx];
      if (pStream && pStream->codecpar &&
          pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        strRes = pStream->codecpar->width * pStream->codecpar->height;
      }
    }

    if ((strRes && strRes < selectedRes) && selectedBitrate < bandwidth)
      continue;

    bool want = false;

    if (strBitrate <= bandwidth)
    {
      if (strBitrate > selectedBitrate || strRes > selectedRes)
        want = true;
    }
    else
    {
      if (strBitrate < selectedBitrate)
        want = true;
    }

    if (want)
    {
      selectedRes = strRes;
      selectedBitrate = strBitrate;
      prog = i;
    }
  }
  return prog;
}

std::string CDVDDemuxFFmpeg::GetStereoModeFromMetadata(AVDictionary* pMetadata)
{
  std::string stereoMode;
  AVDictionaryEntry* tag = NULL;

  // matroska
  tag = av_dict_get(pMetadata, "stereo_mode", NULL, 0);
  if (tag && tag->value)
    stereoMode = tag->value;

  // asf / wmv
  if (stereoMode.empty())
  {
    tag = av_dict_get(pMetadata, "Stereoscopic", NULL, 0);
    if (tag && tag->value)
    {
      tag = av_dict_get(pMetadata, "StereoscopicLayout", NULL, 0);
      if (tag && tag->value)
        stereoMode = ConvertCodecToInternalStereoMode(tag->value, WmvToInternalStereoModeMap);
    }
  }

  return stereoMode;
}

std::string CDVDDemuxFFmpeg::ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap* conversionMap)
{
  size_t i = 0;
  while (conversionMap[i].name)
  {
    if (mode == conversionMap[i].name)
      return conversionMap[i].mode;
    i++;
  }
  return "";
}

void CDVDDemuxFFmpeg::ParsePacket(AVPacket* pkt)
{
  AVStream* st = m_pFormatContext->streams[pkt->stream_index];

  if (st && st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
  {
    auto parser = m_parsers.find(st->index);
    if (parser == m_parsers.end())
    {
      m_parsers.insert(std::make_pair(st->index,
                                      std::unique_ptr<CDemuxParserFFmpeg>(new CDemuxParserFFmpeg())));
      parser = m_parsers.find(st->index);

      parser->second->m_parserCtx = av_parser_init(st->codecpar->codec_id);

      const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
      if (codec == nullptr)
      {
        CLog::Log(LOGERROR, "{} - can't find decoder", __FUNCTION__);
        m_parsers.erase(parser);
        return;
      }
      parser->second->m_codecCtx = avcodec_alloc_context3(codec);
    }

    CDemuxStream* stream = GetStream(st->index);
    if (!stream)
      return;

    if (parser->second->m_parserCtx &&
        parser->second->m_parserCtx->parser &&
        !st->codecpar->extradata)
    {
      FFmpegExtraData retExtraData = GetPacketExtradata(pkt, st->codecpar);
      if (retExtraData)
      {
        st->codecpar->extradata_size = retExtraData.GetSize();
        st->codecpar->extradata = retExtraData.TakeData();

        if (parser->second->m_parserCtx->parser->parser_parse)
        {
          parser->second->m_codecCtx->extradata = st->codecpar->extradata;
          parser->second->m_codecCtx->extradata_size = st->codecpar->extradata_size;
          const uint8_t* outbufptr;
          int bufSize;
          parser->second->m_parserCtx->flags |= PARSER_FLAG_COMPLETE_FRAMES;
          parser->second->m_parserCtx->parser->parser_parse(parser->second->m_parserCtx,
                                                            parser->second->m_codecCtx, &outbufptr,
                                                            &bufSize, pkt->data, pkt->size);
          parser->second->m_codecCtx->extradata = nullptr;
          parser->second->m_codecCtx->extradata_size = 0;

          if (parser->second->m_parserCtx->width != 0)
          {
            st->codecpar->width = parser->second->m_parserCtx->width;
            st->codecpar->height = parser->second->m_parserCtx->height;
          }
          else
          {
            CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::ParsePacket() invalid width/height");
          }
        }
      }
    }
  }
}

TRANSPORT_STREAM_STATE CDVDDemuxFFmpeg::TransportStreamAudioState()
{
  AVStream* st = nullptr;
  bool hasAudio = false;

  if (m_program != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
    {
      int idx = m_pFormatContext->programs[m_program]->stream_index[i];
      st = m_pFormatContext->streams[idx];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
      {
        if (idx == m_pkt.pkt.stream_index && m_pkt.pkt.dts != AV_NOPTS_VALUE)
        {
          if (!m_startTime)
          {
            m_startTime =
                av_rescale(m_pkt.pkt.dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = idx;
          }
          return TRANSPORT_STREAM_STATE::READY;
        }
        hasAudio = true;
      }
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      st = m_pFormatContext->streams[i];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
      {
        if (static_cast<int>(i) == m_pkt.pkt.stream_index && m_pkt.pkt.dts != AV_NOPTS_VALUE)
        {
          if (!m_startTime)
          {
            m_startTime =
                av_rescale(m_pkt.pkt.dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = i;
          }
          return TRANSPORT_STREAM_STATE::READY;
        }
        hasAudio = true;
      }
    }
  }
  if (hasAudio && m_startTime)
    return TRANSPORT_STREAM_STATE::READY;

  return (hasAudio) ? TRANSPORT_STREAM_STATE::NOTREADY : TRANSPORT_STREAM_STATE::NONE;
}

TRANSPORT_STREAM_STATE CDVDDemuxFFmpeg::TransportStreamVideoState()
{
  AVStream* st = nullptr;
  bool hasVideo = false;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return TRANSPORT_STREAM_STATE::NONE;

  if (m_program != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
    {
      int idx = m_pFormatContext->programs[m_program]->stream_index[i];
      st = m_pFormatContext->streams[idx];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (idx == m_pkt.pkt.stream_index && m_pkt.pkt.dts != AV_NOPTS_VALUE &&
            st->codecpar->extradata)
        {
          if (!m_startTime)
          {
            m_startTime =
                av_rescale(m_pkt.pkt.dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = idx;
          }
          return TRANSPORT_STREAM_STATE::READY;
        }
        hasVideo = true;
      }
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      st = m_pFormatContext->streams[i];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (static_cast<int>(i) == m_pkt.pkt.stream_index && m_pkt.pkt.dts != AV_NOPTS_VALUE &&
            st->codecpar->extradata)
        {
          if (!m_startTime)
          {
            m_startTime =
                av_rescale(m_pkt.pkt.dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = i;
          }
          return TRANSPORT_STREAM_STATE::READY;
        }
        hasVideo = true;
      }
    }
  }
  if (hasVideo && m_startTime)
    return TRANSPORT_STREAM_STATE::READY;

  return (hasVideo) ? TRANSPORT_STREAM_STATE::NOTREADY : TRANSPORT_STREAM_STATE::NONE;
}

bool CDVDDemuxFFmpeg::IsTransportStreamReady()
{
  if (!m_checkTransportStream)
    return true;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  TRANSPORT_STREAM_STATE state = TransportStreamVideoState();
  if (state == TRANSPORT_STREAM_STATE::NONE)
    state = TransportStreamAudioState();

  return state == TRANSPORT_STREAM_STATE::READY;
}

void CDVDDemuxFFmpeg::ResetVideoStreams()
{
  AVStream* st;
  for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    st = m_pFormatContext->streams[i];
    if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      av_freep(&st->codecpar->extradata);
      st->codecpar->extradata_size = 0;
    }
  }
}

void CDVDDemuxFFmpeg::GetL16Parameters(int &channels, int &samplerate)
{
  std::string content;
  if (XFILE::CCurlFile::GetContentType(m_pInput->GetURL(), content))
  {
    StringUtils::ToLower(content);
    const size_t len = content.length();
    size_t pos = content.find(';');
    while (pos < len)
    {
      // move to the next non-whitespace character
      pos = content.find_first_not_of(" \t", pos + 1);

      if (pos != std::string::npos)
      {
        if (content.compare(pos, 9, "channels=", 9) == 0)
        {
          pos += 9; // move position to char after 'channels='
          size_t len = content.find(';', pos);
          if (len != std::string::npos)
            len -= pos;
          std::string no_channels(content, pos, len);
          // as we don't support any charset with ';' in name
          StringUtils::Trim(no_channels, " \t");
          if (!no_channels.empty())
          {
            int val = strtol(no_channels.c_str(), NULL, 0);
            if (val > 0)
              channels = val;
            else
              CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::{} - no parameter for channels", __FUNCTION__);
          }
        }
        else if (content.compare(pos, 5, "rate=", 5) == 0)
        {
          pos += 5; // move position to char after 'rate='
          size_t len = content.find(';', pos);
          if (len != std::string::npos)
            len -= pos;
          std::string rate(content, pos, len);
          // as we don't support any charset with ';' in name
          StringUtils::Trim(rate, " \t");
          if (!rate.empty())
          {
            int val = strtol(rate.c_str(), NULL, 0);
            if (val > 0)
              samplerate = val;
            else
              CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::{} - no parameter for samplerate",
                        __FUNCTION__);
          }
        }
        pos = content.find(';', pos); // find next parameter
      }
    }
  }
}

StreamHdrType CDVDDemuxFFmpeg::DetermineHdrType(AVStream* pStream)
{
  StreamHdrType hdrType = StreamHdrType::HDR_TYPE_NONE;

  if (av_stream_get_side_data(pStream, AV_PKT_DATA_DOVI_CONF, nullptr)) // DoVi
    hdrType = StreamHdrType::HDR_TYPE_DOLBYVISION;
  else if (pStream->codecpar->color_trc == AVCOL_TRC_SMPTE2084) // HDR10
    hdrType = StreamHdrType::HDR_TYPE_HDR10;
  else if (pStream->codecpar->color_trc == AVCOL_TRC_ARIB_STD_B67) // HLG
    hdrType = StreamHdrType::HDR_TYPE_HLG;
  // file could be SMPTE2086 which FFmpeg currently returns as unknown
  // so use the presence of static metadata to detect it
  else if (av_stream_get_side_data(pStream, AV_PKT_DATA_MASTERING_DISPLAY_METADATA, nullptr))
    hdrType = StreamHdrType::HDR_TYPE_HDR10;

  return hdrType;
}
