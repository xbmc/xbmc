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

#include "AMLPlayer.h"
#include "Application.h"
#include "FileItem.h"
#include "FileURLProtocol.h"
#include "GUIInfoManager.h"
#include "video/VideoThumbLoader.h"
#include "Util.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderFormats.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/VideoSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/Variant.h"
#include "windowing/WindowingFactory.h"

// for external subtitles
#include "xbmc/cores/dvdplayer/DVDClock.h"
#include "xbmc/cores/dvdplayer/DVDPlayerSubtitle.h"
#include "xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxVobsub.h"
#include "settings/VideoSettings.h"

// amlogic libplayer
#include "AMLUtils.h"
#include "DllLibamplayer.h"

struct AMLChapterInfo
{
  std::string name;
  int64_t     seekto_ms;
};

struct AMLPlayerStreamInfo
{
  void Clear()
  {
    id                = 0;
    width             = 0;
    height            = 0;
    aspect_ratio_num  = 0;
    aspect_ratio_den  = 0;
    frame_rate_num    = 0;
    frame_rate_den    = 0;
    bit_rate          = 0;
    duration          = 0;
    channel           = 0;
    sample_rate       = 0;
    language          = "";
    type              = STREAM_NONE;
    source            = STREAM_SOURCE_NONE;
    name              = "";
    filename          = "";
    filename2         = "";
  }

  int           id;
  StreamType    type;
  StreamSource  source;
  int           width;
  int           height;
  int           aspect_ratio_num;
  int           aspect_ratio_den;
  int           frame_rate_num;
  int           frame_rate_den;
  int           bit_rate;
  int           duration;
  int           channel;
  int           sample_rate;
  int           format;
  std::string   language;
  std::string   name;
  std::string   filename;
  std::string   filename2;  // for vobsub subtitles, 2 files are necessary (idx/sub) 
};

////////////////////////////////////////////////////////////////////////////////////////////
static int media_info_dump(media_info_t* minfo)
{
  int i = 0;
  CLog::Log(LOGDEBUG, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  CLog::Log(LOGDEBUG, "======||file size:%lld",                          minfo->stream_info.file_size);
  CLog::Log(LOGDEBUG, "======||file type:%d",                            minfo->stream_info.type);
  CLog::Log(LOGDEBUG, "======||duration:%d",                             minfo->stream_info.duration);
  CLog::Log(LOGDEBUG, "======||has video track?:%s",                     minfo->stream_info.has_video>0?"YES!":"NO!");
  CLog::Log(LOGDEBUG, "======||has audio track?:%s",                     minfo->stream_info.has_audio>0?"YES!":"NO!");
  CLog::Log(LOGDEBUG, "======||has internal subtitle?:%s",               minfo->stream_info.has_sub>0?"YES!":"NO!");
  CLog::Log(LOGDEBUG, "======||internal subtile counts:%d",              minfo->stream_info.total_sub_num);
  if (minfo->stream_info.has_video && minfo->stream_info.total_video_num > 0)
  {
    CLog::Log(LOGDEBUG, "======||video index:%d",                        minfo->stream_info.cur_video_index);
    CLog::Log(LOGDEBUG, "======||video counts:%d",                       minfo->stream_info.total_video_num);
    CLog::Log(LOGDEBUG, "======||video width :%d",                       minfo->video_info[0]->width);
    CLog::Log(LOGDEBUG, "======||video height:%d",                       minfo->video_info[0]->height);
    CLog::Log(LOGDEBUG, "======||video ratio :%d:%d",                    minfo->video_info[0]->aspect_ratio_num,minfo->video_info[0]->aspect_ratio_den);
    CLog::Log(LOGDEBUG, "======||frame_rate  :%.2f",                     (float)minfo->video_info[0]->frame_rate_num/minfo->video_info[0]->frame_rate_den);
    CLog::Log(LOGDEBUG, "======||video bitrate:%d",                      minfo->video_info[0]->bit_rate);
    CLog::Log(LOGDEBUG, "======||video format:%d",                       minfo->video_info[0]->format);
    CLog::Log(LOGDEBUG, "======||video duration:%d",                     minfo->video_info[0]->duartion);
  }
  if (minfo->stream_info.has_audio && minfo->stream_info.total_audio_num > 0)
  {
    CLog::Log(LOGDEBUG, "======||audio index:%d",                        minfo->stream_info.cur_audio_index);
    CLog::Log(LOGDEBUG, "======||audio counts:%d",                       minfo->stream_info.total_audio_num);
    for (i = 0; i < minfo->stream_info.total_audio_num; i++)
    {
      CLog::Log(LOGDEBUG, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      CLog::Log(LOGDEBUG, "======||audio track(%d) id:%d",               i, minfo->audio_info[i]->id);
      CLog::Log(LOGDEBUG, "======||audio track(%d) codec type:%d",       i, minfo->audio_info[i]->aformat);
      CLog::Log(LOGDEBUG, "======||audio track(%d) audio_channel:%d",    i, minfo->audio_info[i]->channel);
      CLog::Log(LOGDEBUG, "======||audio track(%d) bit_rate:%d",         i, minfo->audio_info[i]->bit_rate);
      CLog::Log(LOGDEBUG, "======||audio track(%d) audio_samplerate:%d", i, minfo->audio_info[i]->sample_rate);
      CLog::Log(LOGDEBUG, "======||audio track(%d) duration:%d",         i, minfo->audio_info[i]->duration);
      CLog::Log(LOGDEBUG, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      if (NULL != minfo->audio_info[i]->audio_tag)
      {
        CLog::Log(LOGDEBUG, "======||audio track title:%s",              minfo->audio_info[i]->audio_tag->title!=NULL?minfo->audio_info[i]->audio_tag->title:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track album:%s",              minfo->audio_info[i]->audio_tag->album!=NULL?minfo->audio_info[i]->audio_tag->album:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track author:%s",             minfo->audio_info[i]->audio_tag->author!=NULL?minfo->audio_info[i]->audio_tag->author:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track year:%s",               minfo->audio_info[i]->audio_tag->year!=NULL?minfo->audio_info[i]->audio_tag->year:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track comment:%s",            minfo->audio_info[i]->audio_tag->comment!=NULL?minfo->audio_info[i]->audio_tag->comment:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track genre:%s",              minfo->audio_info[i]->audio_tag->genre!=NULL?minfo->audio_info[i]->audio_tag->genre:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track copyright:%s",          minfo->audio_info[i]->audio_tag->copyright!=NULL?minfo->audio_info[i]->audio_tag->copyright:"unknown");
        CLog::Log(LOGDEBUG, "======||audio track track:%d",              minfo->audio_info[i]->audio_tag->track);
      }
    }
  }
  if (minfo->stream_info.has_sub && minfo->stream_info.total_sub_num > 0)
  {
    CLog::Log(LOGDEBUG, "======||subtitle index:%d",                     minfo->stream_info.cur_sub_index);
    CLog::Log(LOGDEBUG, "======||subtitle counts:%d",                    minfo->stream_info.total_sub_num);
    for (i = 0; i < minfo->stream_info.total_sub_num; i++)
    {
      if (0 == minfo->sub_info[i]->internal_external){
        CLog::Log(LOGDEBUG, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) pid:%d",      i, minfo->sub_info[i]->id);
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) language:%s", i, minfo->sub_info[i]->sub_language?minfo->sub_info[i]->sub_language:"unknown");
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) width:%d",    i, minfo->sub_info[i]->width);
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) height:%d",   i, minfo->sub_info[i]->height);
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) resolution:%d", i, minfo->sub_info[i]->resolution);
        CLog::Log(LOGDEBUG, "======||internal subtitle(%d) subtitle size:%lld", i, minfo->sub_info[i]->subtitle_size);
        CLog::Log(LOGDEBUG, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      }
    }
  }
  CLog::Log(LOGDEBUG, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  return 0;
}

static const char* VideoCodecName(int vformat)
{
  const char *format = "";

  switch(vformat)
  {
    case VFORMAT_MPEG12:
      format = "mpeg12";
      break;
    case VFORMAT_MPEG4:
      format = "mpeg4";
      break;
    case VFORMAT_H264:
      format = "h264";
      break;
    case VFORMAT_MJPEG:
      format = "mjpeg";
      break;
    case VFORMAT_REAL:
      format = "real";
      break;
    case VFORMAT_JPEG:
      format = "jpeg";
      break;
    case VFORMAT_VC1:
      format = "vc1";
      break;
    case VFORMAT_AVS:
      format = "avs";
      break;
    case VFORMAT_SW:
      format = "sw";
      break;
    case VFORMAT_H264MVC:
      format = "h264mvc";
      break;
    default:
      format = "unknown";
      break;
  }
  return format;
}

static const char* AudioCodecName(int aformat)
{
  const char *format = "";

  switch(aformat)
  {
    case AFORMAT_MPEG:
      format = "mpeg";
      break;
    case AFORMAT_PCM_S16LE:
      format = "pcm";
      break;
    case AFORMAT_AAC:
      format = "aac";
      break;
    case AFORMAT_AC3:
      format = "ac3";
      break;
    case AFORMAT_ALAW:
      format = "alaw";
      break;
    case AFORMAT_MULAW:
      format = "mulaw";
      break;
    case AFORMAT_DTS:
      format = "dts";
      break;
    case AFORMAT_PCM_S16BE:
      format = "pcm";
      break;
    case AFORMAT_FLAC:
      format = "flac";
      break;
    case AFORMAT_COOK:
      format = "cook";
      break;
    case AFORMAT_PCM_U8:
      format = "pcm";
      break;
    case AFORMAT_ADPCM:
      format = "adpcm";
      break;
    case AFORMAT_AMR:
      format = "amr";
      break;
    case AFORMAT_RAAC:
      format = "raac";
      break;
    case AFORMAT_WMA:
      format = "wma";
      break;
    case AFORMAT_WMAPRO:
      format = "wmapro";
      break;
    case AFORMAT_PCM_BLURAY:
      format = "lpcm";
      break;
    case AFORMAT_ALAC:
      format = "alac";
      break;
    case AFORMAT_VORBIS:
      format = "vorbis";
      break;
    case AFORMAT_AAC_LATM:
      format = "aac-latm";
      break;
    case AFORMAT_APE:
      format = "ape";
      break;
    default:
      format = "unknown";
      break;
  }

  return format;
}

////////////////////////////////////////////////////////////////////////////////////////////
CAMLSubTitleThread::CAMLSubTitleThread(DllLibAmplayer *dll) :
  CThread("CAMLSubTitleThread"),
  m_dll(dll),
  m_subtitle_codec(-1)
{
}

CAMLSubTitleThread::~CAMLSubTitleThread()
{
  StopThread();
}

void CAMLSubTitleThread::UpdateSubtitle(CStdString &subtitle, int64_t elapsed_ms)
{
  CSingleLock lock(m_subtitle_csection);
  if (m_subtitle_strings.size())
  {
    AMLSubtitle *amlsubtitle;
    // remove any expired subtitles
    std::deque<AMLSubtitle*>::iterator it = m_subtitle_strings.begin();
    while (it != m_subtitle_strings.end())
    {
      amlsubtitle = *it;
      if (elapsed_ms > amlsubtitle->endtime)
        it = m_subtitle_strings.erase(it);
      else
        it++;
    }

    // find the current subtitle
    it = m_subtitle_strings.begin();
    while (it != m_subtitle_strings.end())
    {
      amlsubtitle = *it;
      if (elapsed_ms > amlsubtitle->bgntime && elapsed_ms < amlsubtitle->endtime)
      {
        subtitle = amlsubtitle->string;
        break;
      }
      it++;
    }
  }
}

void CAMLSubTitleThread::Process(void)
{
  CLog::Log(LOGDEBUG, "CAMLSubTitleThread::Process begin");

  m_subtitle_codec = m_dll->codec_open_sub_read();
  if (m_subtitle_codec < 0)
    CLog::Log(LOGERROR, "CAMLSubTitleThread::Process: codec_open_sub_read failed");

  while (!m_bStop)
  {
    if (m_subtitle_codec > 0)
    {
      // poll sub codec, we return on timeout or when a sub gets loaded
      // TODO: codec_poll_sub_fd has a bug in kernel driver, it trashes
      // subs in certain conditions so we read garbage, manual poll for now.
      //codec_poll_sub_fd(m_subtitle_codec, 1000);
      int sub_size = m_dll->codec_get_sub_size_fd(m_subtitle_codec);
      if (sub_size > 0)
      {
        int sub_type = 0, sub_pts = 0;
        // calloc sub_size + 1 so we auto terminate the string
        char *sub_buffer = (char*)calloc(sub_size + 1, 1);
        m_dll->codec_read_sub_data_fd(m_subtitle_codec, sub_buffer, sub_size);

        // check subtitle header stamp
        if ((sub_buffer[0] == 0x41) && (sub_buffer[1] == 0x4d) &&
            (sub_buffer[2] == 0x4c) && (sub_buffer[3] == 0x55) &&
            (sub_buffer[4] == 0xaa))
        {
          // 20 byte header, then subtitle string
          if (sub_size >= 20)
          {
            // csection lock it now as we are diddling shared vars
            CSingleLock lock(m_subtitle_csection);

            AMLSubtitle *subtitle = new AMLSubtitle;

            sub_type  = (sub_buffer[5] << 16)  | (sub_buffer[6] << 8)   | sub_buffer[7];
            // sub_pts are in ffmpeg timebase, not ms timebase, convert it.
            sub_pts = (sub_buffer[12] << 24) | (sub_buffer[13] << 16) | (sub_buffer[14] << 8) | sub_buffer[15];

            /* TODO: handle other subtitle codec types
            // subtitle codecs
            CODEC_ID_DVD_SUBTITLE= 0x17000,
            CODEC_ID_DVB_SUBTITLE,
            CODEC_ID_TEXT,  ///< raw UTF-8 text
            CODEC_ID_XSUB,
            CODEC_ID_SSA,
            CODEC_ID_MOV_TEXT,
            CODEC_ID_HDMV_PGS_SUBTITLE,
            CODEC_ID_DVB_TELETEXT,
            CODEC_ID_SRT,
            CODEC_ID_MICRODVD,
            */
            switch(sub_type)
            {
              default:
                CLog::Log(LOGDEBUG, "CAMLSubTitleThread::Process: fixme :) "
                  "sub_type(0x%x), size(%d), bgntime(%lld), endtime(%lld), string(%s)",
                  sub_type, sub_size-20, subtitle->bgntime, subtitle->endtime, &sub_buffer[20]);
                break;
              case CODEC_ID_TEXT:
                subtitle->bgntime = sub_pts/ 90;
                subtitle->endtime = subtitle->bgntime + 4000;
                subtitle->string  = &sub_buffer[20];
                break;
              case CODEC_ID_SSA:
                if (strncmp((const char*)&sub_buffer[20], "Dialogue:", 9) == 0)
                {
                  int  vars_found, hour1, min1, sec1, hunsec1, hour2, min2, sec2, hunsec2, nothing;
                  char line3[sub_size];
                  char *line = &sub_buffer[20];

                  memset(line3, 0x00, sub_size);
                  vars_found = sscanf(line, "Dialogue: Marked=%d,%d:%d:%d.%d,%d:%d:%d.%d,%[^\n\r]",
                    &nothing, &hour1, &min1, &sec1, &hunsec1, &hour2, &min2, &sec2, &hunsec2, line3);
                  if (vars_found < 10)
                    vars_found = sscanf(line, "Dialogue: %d,%d:%d:%d.%d,%d:%d:%d.%d,%[^\n\r]",
                      &nothing, &hour1, &min1, &sec1, &hunsec1, &hour2, &min2, &sec2, &hunsec2, line3);

                  if (vars_found > 9)
                  {
                    char *tmp, *line2 = strchr(line3, ',');
                    // use 32 for the case that the amount of commas increase with newer SSA versions
                    for (int comma = 4; comma < 32; comma++)
                    {
                      tmp = strchr(line2 + 1, ',');
                      if (!tmp)
                        break;
                      if (*(++tmp) == ' ')
                        break;
                      // a space after a comma means we are already in a sentence
                      line2 = tmp;
                    }
                    // eliminate the trailing comma
                    if (*line2 == ',')
                      line2++;
                    subtitle->bgntime = 10 * (360000 * hour1 + 6000 * min1 + 100 * sec1 + hunsec1);
                    subtitle->endtime = 10 * (360000 * hour2 + 6000 * min2 + 100 * sec2 + hunsec2);
                    subtitle->string  = line2;
                    // convert tags to what we understand
                    if (subtitle->string.Replace("{\\i1}","[I]"))
                      subtitle->string.Replace("{\\i0}","[/I]");
                    if (subtitle->string.Replace("{\\b1}","[B]"))
                      subtitle->string.Replace("{\\b0}","[/B]");
                    // remove anything other tags
                    for (std::string::const_iterator it = subtitle->string.begin(); it != subtitle->string.end(); ++it)
                    {
                      size_t beg = subtitle->string.find("{\\");
                      if (beg != std::string::npos)
                      {
                        size_t end = subtitle->string.find("}", beg);
                        if (end != std::string::npos)
                          subtitle->string.erase(beg, end-beg+1);
                      }
                    }
                  }
                }
                break;
            }
            free(sub_buffer);
            
            if (subtitle->string.length())
            {
              // quirks
              subtitle->string.Replace("&apos;","\'");
              m_subtitle_strings.push_back(subtitle);
              // fixup existing endtimes so they do not exceed bgntime of previous subtitle
              for (size_t i = 0; i < m_subtitle_strings.size() - 1; i++)
              {
                if (m_subtitle_strings[i]->endtime > m_subtitle_strings[i+1]->bgntime)
                  m_subtitle_strings[i]->endtime = m_subtitle_strings[i+1]->bgntime;
              }
            }
          }
        }
      }
      else
      {
        usleep(100 * 1000);
      }
    }
    else
    {
      usleep(250 * 1000);
    }
  }
  m_subtitle_strings.clear();
  if (m_subtitle_codec > 0)
    m_dll->codec_close_sub_fd(m_subtitle_codec);
  m_subtitle_codec = -1;

  CLog::Log(LOGDEBUG, "CAMLSubTitleThread::Process end");
}
////////////////////////////////////////////////////////////////////////////////////////////
CAMLPlayer::CAMLPlayer(IPlayerCallback &callback)
  : IPlayer(callback),
  CThread("CAMLPlayer"),
  m_ready(true)
{
  m_dll = new DllLibAmplayer;
  m_dll->Load();
  m_pid = -1;
  m_speed = 0;
  m_paused = false;
#if defined(_DEBUG)
  m_log_level = 5;
#else
  m_log_level = 3;
#endif
  m_bAbortRequest = false;

  // for external subtitles
  m_dvdOverlayContainer = new CDVDOverlayContainer;
  m_dvdPlayerSubtitle = new CDVDPlayerSubtitle(m_dvdOverlayContainer);
}

CAMLPlayer::~CAMLPlayer()
{
  CloseFile();

  delete m_dvdPlayerSubtitle;
  delete m_dvdOverlayContainer;
  delete m_dll, m_dll = NULL;
}

bool CAMLPlayer::OpenFile(const CFileItem &file, const CPlayerOptions &options)
{
  try
  {
    CLog::Log(LOGNOTICE, "CAMLPlayer: Opening: %s", file.GetPath().c_str());
    // if playing a file close it first
    // this has to be changed so we won't have to close it.
    if (IsRunning())
      CloseFile();

    m_bAbortRequest = false;

    m_item = file;
    m_options = options;

    m_elapsed_ms  =  0;
    m_duration_ms =  0;

    m_audio_info  = "none";
    m_audio_delay = 0;
    m_audio_passthrough_ac3 = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    m_audio_passthrough_dts = g_guiSettings.GetBool("audiooutput.dtspassthrough");

    m_video_info  = "none";
    m_video_width    =  0;
    m_video_height   =  0;
    m_video_fps_numerator = 25;
    m_video_fps_denominator = 1;

    m_subtitle_delay =  0;
    m_subtitle_thread = NULL;

    m_chapter_index  =  0;
    m_chapter_count  =  0;

    m_show_mainvideo = -1;
    m_dst_rect.SetRect(0, 0, 0, 0);
    m_zoom           = -1;
    m_contrast       = -1;
    m_brightness     = -1;

    ClearStreamInfos();

    // setup to spin the busy dialog until we are playing
    m_ready.Reset();

    g_renderManager.PreInit();

    // create the playing thread
    Create();
    if (!m_ready.WaitMSec(100))
    {
      CGUIDialogBusy *dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
      dialog->Show();
      while (!m_ready.WaitMSec(1))
        g_windowManager.ProcessRenderLoop(false);
      dialog->Close();
    }

    // Playback might have been stopped due to some error.
    if (m_bStop || m_bAbortRequest)
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown on open", __FUNCTION__);
    return false;
  }
}

bool CAMLPlayer::CloseFile()
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::CloseFile");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  CLog::Log(LOGDEBUG, "CAMLPlayer: waiting for threads to exit");
  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();

  CLog::Log(LOGDEBUG, "CAMLPlayer: finished waiting");

  g_renderManager.UnInit();

  return true;
}

bool CAMLPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CAMLPlayer::Pause()
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::Pause");
  CSingleLock lock(m_aml_csection);

  if ((m_pid < 0) && m_bAbortRequest)
    return;

  if (m_paused)
    m_dll->player_resume(m_pid);
  else
    m_dll->player_pause(m_pid);

  m_paused = !m_paused;
}

bool CAMLPlayer::IsPaused() const
{
  return m_paused;
}

bool CAMLPlayer::HasVideo() const
{
  return m_video_count > 0;
}

bool CAMLPlayer::HasAudio() const
{
  return m_audio_count > 0;
}

void CAMLPlayer::ToggleFrameDrop()
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::ToggleFrameDrop");
}

bool CAMLPlayer::CanSeek()
{
  return GetTotalTime() > 0;
}

void CAMLPlayer::Seek(bool bPlus, bool bLargeStep)
{
  // force updated to m_elapsed_ms, m_duration_ms.
  GetStatus();

  CSingleLock lock(m_aml_csection);

  // try chapter seeking first, chapter_index is ones based.
  int chapter_index = GetChapter();
  if (bLargeStep)
  {
    // seek to next chapter
    if (bPlus && (chapter_index < GetChapterCount()))
    {
      SeekChapter(chapter_index + 1);
      return;
    }
    // seek to previous chapter
    if (!bPlus && chapter_index)
    {
      SeekChapter(chapter_index - 1);
      return;
    }
  }

  int64_t seek_ms;
  if (g_advancedSettings.m_videoUseTimeSeeking)
  {
    if (bLargeStep && (GetTotalTime() > (2000 * g_advancedSettings.m_videoTimeSeekForwardBig)))
      seek_ms = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek_ms = bPlus ? g_advancedSettings.m_videoTimeSeekForward    : g_advancedSettings.m_videoTimeSeekBackward;
    // convert to milliseconds
    seek_ms *= 1000;
    seek_ms += m_elapsed_ms;
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForward    : g_advancedSettings.m_videoPercentSeekBackward;
    percent /= 100.0f;
    percent += (float)m_elapsed_ms/(float)m_duration_ms;
    // convert to milliseconds
    seek_ms = m_duration_ms * percent;
  }

  // handle stacked videos, dvdplayer does it so we do it too.
  if (g_application.CurrentFileItem().IsStack() &&
    (seek_ms > m_duration_ms || seek_ms < 0))
  {
    CLog::Log(LOGDEBUG, "CAMLPlayer::Seek: In mystery code, what did I do");
    g_application.SeekTime((seek_ms - m_elapsed_ms) * 0.001 + g_application.GetTime());
    // warning, don't access any object variables here as
    // the object may have been destroyed
    return;
  }

  if (seek_ms <= 1000)
    seek_ms = 1000;

  if (seek_ms > m_duration_ms)
    seek_ms = m_duration_ms;

  // do seek here
  g_infoManager.SetDisplayAfterSeek(100000);
  SeekTime(seek_ms);
  m_callback.OnPlayBackSeek((int)seek_ms, (int)(seek_ms - m_elapsed_ms));
  g_infoManager.SetDisplayAfterSeek();
}

bool CAMLPlayer::SeekScene(bool bPlus)
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::SeekScene");
  return false;
}

void CAMLPlayer::SeekPercentage(float fPercent)
{
  CSingleLock lock(m_aml_csection);

  // force updated to m_elapsed_ms, m_duration_ms.
  GetStatus();

  if (m_duration_ms)
  {
    int64_t seek_ms = fPercent * m_duration_ms / 100.0;
    if (seek_ms <= 1000)
      seek_ms = 1000;

    // do seek here
    g_infoManager.SetDisplayAfterSeek(100000);
    SeekTime(seek_ms);
    m_callback.OnPlayBackSeek((int)seek_ms, (int)(seek_ms - m_elapsed_ms));
    g_infoManager.SetDisplayAfterSeek();
  }
}

float CAMLPlayer::GetPercentage()
{
  GetStatus();
  if (m_duration_ms)
    return 100.0f * (float)m_elapsed_ms/(float)m_duration_ms;
  else
    return 0.0f;
}

void CAMLPlayer::SetVolume(float volume)
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::SetVolume(%f)", volume);
#if !defined(TARGET_ANDROID)
  CSingleLock lock(m_aml_csection);
  // volume is a float percent from 0.0 to 1.0
  if (m_dll->check_pid_valid(m_pid))
    m_dll->audio_set_volume(m_pid, volume);
#endif
}

void CAMLPlayer::GetAudioInfo(CStdString &strAudioInfo)
{
  CSingleLock lock(m_aml_csection);
  if (m_audio_streams.size() == 0 || m_audio_index > (int)(m_audio_streams.size() - 1))
    return;

  strAudioInfo.Format("Audio stream (%s) [Kb/s:%.2f]",
    AudioCodecName(m_audio_streams[m_audio_index]->format),
    (double)m_audio_streams[m_audio_index]->bit_rate / 1024.0);
}

void CAMLPlayer::GetVideoInfo(CStdString &strVideoInfo)
{
  CSingleLock lock(m_aml_csection);
  if (m_video_streams.size() == 0 || m_video_index > (int)(m_video_streams.size() - 1))
    return;

  strVideoInfo.Format("Video stream (%s) [fr:%.3f Mb/s:%.2f]",
    VideoCodecName(m_video_streams[m_video_index]->format),
    GetActualFPS(),
    (double)m_video_streams[m_video_index]->bit_rate / (1024.0*1024.0));
}

int CAMLPlayer::GetAudioStreamCount()
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetAudioStreamCount");
  return m_audio_count;
}

int CAMLPlayer::GetAudioStream()
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetAudioStream");
  return m_audio_index;
}

void CAMLPlayer::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetAudioStreamName");
  CSingleLock lock(m_aml_csection);

  strStreamName.Format("Undefined");

  if (iStream > (int)m_audio_streams.size() || iStream < 0)
    return;

  if ( m_audio_streams[iStream]->language.size())
  {
    CStdString name;
    g_LangCodeExpander.Lookup( name, m_audio_streams[iStream]->language);
    strStreamName = name;
  }

}

void CAMLPlayer::SetAudioStream(int SetAudioStream)
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::SetAudioStream");
  CSingleLock lock(m_aml_csection);

  if (SetAudioStream > (int)m_audio_streams.size() || SetAudioStream < 0)
    return;

  m_audio_index = SetAudioStream;
  SetAudioPassThrough(m_audio_streams[m_audio_index]->format);

  if (m_dll->check_pid_valid(m_pid))
  {
    m_dll->player_aid(m_pid, m_audio_streams[m_audio_index]->id);
  }
}

void CAMLPlayer::SetAVDelay(float fValue)
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::SetAVDelay (%f)", fValue);
  m_audio_delay = fValue * 1000.0;

  if (m_audio_streams.size() && m_dll->check_pid_valid(m_pid))
  {
    CSingleLock lock(m_aml_csection);
    m_dll->audio_set_delay(m_pid, m_audio_delay);
  }
}

float CAMLPlayer::GetAVDelay()
{
  return (float)m_audio_delay / 1000.0;
}

void CAMLPlayer::SetSubTitleDelay(float fValue = 0.0f)
{
  if (GetSubtitleCount())
  {
    CSingleLock lock(m_aml_csection);
    m_subtitle_delay = fValue * 1000.0;
  }
}

float CAMLPlayer::GetSubTitleDelay()
{
  return (float)m_subtitle_delay / 1000.0;
}

int CAMLPlayer::GetSubtitleCount()
{
  return m_subtitle_count;
}

int CAMLPlayer::GetSubtitle()
{
  if (m_subtitle_show)
    return m_subtitle_index;
  else
    return -1;
}

void CAMLPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  CSingleLock lock(m_aml_csection);

  strStreamName = "";

  if (iStream > (int)m_subtitle_streams.size() || iStream < 0)
    return;

  if (m_subtitle_streams[m_subtitle_index]->source == STREAM_SOURCE_NONE)
  {
    if ( m_subtitle_streams[iStream]->language.size())
    {
      CStdString name;
      g_LangCodeExpander.Lookup(name, m_subtitle_streams[iStream]->language);
      strStreamName = name;
    }
    else
      strStreamName = g_localizeStrings.Get(13205); // Unknown
  }
  else
  {
    if(m_subtitle_streams[m_subtitle_index]->name.length() > 0)
      strStreamName = m_subtitle_streams[m_subtitle_index]->name;
    else
      strStreamName = g_localizeStrings.Get(13205); // Unknown
  }
  if (m_log_level > 5)
    CLog::Log(LOGDEBUG, "CAMLPlayer::GetSubtitleName, iStream(%d)", iStream);
}
 
void CAMLPlayer::SetSubtitle(int iStream)
{
  CSingleLock lock(m_aml_csection);

  if (iStream > (int)m_subtitle_streams.size() || iStream < 0)
    return;

  m_subtitle_index = iStream;

  // smells like a bug, if no showing subs and we get called
  // to set the subtitle, we are expected to update internal state
  // but not show the subtitle.
  if (!m_subtitle_show)
    return;

  if (m_dll->check_pid_valid(m_pid) && m_subtitle_streams[m_subtitle_index]->source == STREAM_SOURCE_NONE)
    m_dll->player_sid(m_pid, m_subtitle_streams[m_subtitle_index]->id);
  else
  {
    m_dvdPlayerSubtitle->CloseStream(true);
    OpenSubtitleStream(m_subtitle_index);
  }
}

bool CAMLPlayer::GetSubtitleVisible()
{
  return m_subtitle_show;
}

void CAMLPlayer::SetSubtitleVisible(bool bVisible)
{
  m_subtitle_show = (bVisible && m_subtitle_count);
  g_settings.m_currentVideoSettings.m_SubtitleOn = bVisible;

  if (m_subtitle_show  && m_subtitle_count)
  {
    // on startup, if asked to show subs and SetSubtitle has not
    // been called, we are expected to switch/show the 1st subtitle
    if (m_subtitle_index < 0)
      m_subtitle_index = 0;
    if (m_dll->check_pid_valid(m_pid) && m_subtitle_streams[m_subtitle_index]->source == STREAM_SOURCE_NONE)
      m_dll->player_sid(m_pid, m_subtitle_streams[m_subtitle_index]->id);
    else
      OpenSubtitleStream(m_subtitle_index);
  }
}

int CAMLPlayer::AddSubtitle(const CStdString& strSubPath)
{
  CSingleLock lock(m_aml_csection);

  return AddSubtitleFile(strSubPath);
}

void CAMLPlayer::Update(bool bPauseDrawing)
{
  g_renderManager.Update(bPauseDrawing);
}

void CAMLPlayer::GetVideoRect(CRect& SrcRect, CRect& DestRect)
{
  g_renderManager.GetVideoRect(SrcRect, DestRect);
}

void CAMLPlayer::GetVideoAspectRatio(float &fAR)
{
  fAR = g_renderManager.GetAspectRatio();
}

int CAMLPlayer::GetChapterCount()
{
  return m_chapter_count;
}

int CAMLPlayer::GetChapter()
{
  GetStatus();

  for (int i = 0; i < m_chapter_count - 1; i++)
  {
    if (m_elapsed_ms >= m_chapters[i]->seekto_ms && m_elapsed_ms < m_chapters[i + 1]->seekto_ms)
      return i + 1;
  }
  return 0;
}

void CAMLPlayer::GetChapterName(CStdString& strChapterName)
{
  if (m_chapter_count)
    strChapterName = m_chapters[GetChapter() - 1]->name;
}

int CAMLPlayer::SeekChapter(int chapter_index)
{
  CSingleLock lock(m_aml_csection);

  // chapter_index is a one based value.
  if (m_chapter_count > 1)
  {
    if (chapter_index < 1)
      chapter_index = 1;
    if (chapter_index > m_chapter_count)
      return 0;

    // time units are seconds,
    // so we add 1000ms to get into the chapter.
    int64_t seek_ms = m_chapters[chapter_index - 1]->seekto_ms + 1000;

    //  seek to 1 second and play is immediate.
    if (seek_ms <= 0)
      seek_ms = 1000;

    // seek to chapter here
    g_infoManager.SetDisplayAfterSeek(100000);
    SeekTime(seek_ms);
    m_callback.OnPlayBackSeekChapter(chapter_index);
    g_infoManager.SetDisplayAfterSeek();
  }
  else
  {
    // we do not have a chapter list so do a regular big jump.
    if (chapter_index > 0)
      Seek(true,  true);
    else
      Seek(false, true);
  }
  return 0;
}

float CAMLPlayer::GetActualFPS()
{
  float video_fps = m_video_fps_numerator / m_video_fps_denominator;
  CLog::Log(LOGDEBUG, "CAMLPlayer::GetActualFPS:m_video_fps(%f)", video_fps);
  return video_fps;
}

void CAMLPlayer::SeekTime(__int64 seek_ms)
{
  CSingleLock lock(m_aml_csection);

  // we cannot seek if paused
  if (m_paused)
    return;

  if (seek_ms <= 0)
    seek_ms = 100;

  // seek here
  if (m_dll->check_pid_valid(m_pid))
  {
    if (!CheckPlaying())
      return;
    // player_timesearch is seconds (float).
    m_dll->player_timesearch(m_pid, (float)seek_ms/1000.0);
    WaitForSearchOK(5000);
    WaitForPlaying(5000);
  }
}

__int64 CAMLPlayer::GetTime()
{
  return m_elapsed_ms;
}

__int64 CAMLPlayer::GetTotalTime()
{
  return m_duration_ms;
}

int CAMLPlayer::GetAudioBitrate()
{
  CSingleLock lock(m_aml_csection);
  if (m_audio_streams.size() == 0 || m_audio_index > (int)(m_audio_streams.size() - 1))
    return 0;

  return m_audio_streams[m_audio_index]->bit_rate;
}

int CAMLPlayer::GetVideoBitrate()
{
  CSingleLock lock(m_aml_csection);
  if (m_video_streams.size() == 0 || m_video_index > (int)(m_video_streams.size() - 1))
    return 0;

  return m_video_streams[m_video_index]->bit_rate;
}

int CAMLPlayer::GetSourceBitrate()
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::GetSourceBitrate");
  return 0;
}

int CAMLPlayer::GetChannels()
{
  CSingleLock lock(m_aml_csection);
  if (m_audio_streams.size() == 0 || m_audio_index > (int)(m_audio_streams.size() - 1))
    return 0;
  
  return m_audio_streams[m_audio_index]->channel;
}

int CAMLPlayer::GetBitsPerSample()
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::GetBitsPerSample");
  return 0;
}

int CAMLPlayer::GetSampleRate()
{
  CSingleLock lock(m_aml_csection);
  if (m_audio_streams.size() == 0 || m_audio_index > (int)(m_audio_streams.size() - 1))
    return 0;
  
  return m_audio_streams[m_audio_index]->sample_rate;
}

CStdString CAMLPlayer::GetAudioCodecName()
{
  CStdString strAudioCodec = "";
  if (m_audio_streams.size() == 0 || m_audio_index > (int)(m_audio_streams.size() - 1))
    return strAudioCodec;

  strAudioCodec = AudioCodecName(m_audio_streams[m_audio_index]->format);

  return strAudioCodec;
}

CStdString CAMLPlayer::GetVideoCodecName()
{
  CStdString strVideoCodec = "";
  if (m_video_streams.size() == 0 || m_video_index > (int)(m_video_streams.size() - 1))
    return strVideoCodec;
  
  strVideoCodec = VideoCodecName(m_video_streams[m_video_index]->format);

  return strVideoCodec;
}

int CAMLPlayer::GetPictureWidth()
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetPictureWidth(%d)", m_video_width);
  return m_video_width;
}

int CAMLPlayer::GetPictureHeight()
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetPictureHeight(%)", m_video_height);
  return m_video_height;
}

bool CAMLPlayer::GetStreamDetails(CStreamDetails &details)
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetStreamDetails");
  return false;
}

void CAMLPlayer::ToFFRW(int iSpeed)
{
  CLog::Log(LOGDEBUG, "CAMLPlayer::ToFFRW: iSpeed(%d), m_speed(%d)", iSpeed, m_speed);
  CSingleLock lock(m_aml_csection);

  if (!m_dll->check_pid_valid(m_pid) && m_bAbortRequest)
    return;

  if (m_speed != iSpeed)
  {
    // recover power of two value
    int ipower = 0;
    int ispeed = abs(iSpeed);
    while (ispeed >>= 1) ipower++;

    switch(ipower)
    {
      // regular playback
      case  0:
        m_dll->player_forward(m_pid, 0);
        break;
      default:
        // N x fast forward/rewind (I-frames)
        // speed playback 1,2,4,8
        if (iSpeed > 0)
          m_dll->player_forward(m_pid,   iSpeed);
        else
          m_dll->player_backward(m_pid, -iSpeed);
        break;
    }

    m_speed = iSpeed;
  }
}

bool CAMLPlayer::GetCurrentSubtitle(CStdString& strSubtitle)
{
  strSubtitle = "";

  if (m_subtitle_count)
  {
    // force updated to m_elapsed_ms.
    GetStatus();
    if (m_subtitle_streams[m_subtitle_index]->source == STREAM_SOURCE_NONE && m_subtitle_thread)
    {
      m_subtitle_thread->UpdateSubtitle(strSubtitle, m_elapsed_ms - m_subtitle_delay);
    }
    else
    {
      double pts = DVD_MSEC_TO_TIME(m_elapsed_ms) - DVD_MSEC_TO_TIME(m_subtitle_delay);
      m_dvdOverlayContainer->CleanUp(pts);
      m_dvdPlayerSubtitle->GetCurrentSubtitle(strSubtitle, pts);
    }
  }

  return !strSubtitle.IsEmpty();
}

void CAMLPlayer::OnStartup()
{
  //m_CurrentVideo.Clear();
  //m_CurrentAudio.Clear();
  //m_CurrentSubtitle.Clear();

  //CThread::SetName("CAMLPlayer");
}

void CAMLPlayer::OnExit()
{
  //CLog::Log(LOGNOTICE, "CAMLPlayer::OnExit()");
  Sleep(1000);

  m_bStop = true;
  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if (m_options.identify == false)
  {
    if (m_bAbortRequest)
      m_callback.OnPlayBackStopped();
    else
      m_callback.OnPlayBackEnded();
  }
  // set event to inform openfile something went wrong
  // in case openfile is still waiting for this event
  m_ready.Set();
}

void CAMLPlayer::Process()
{
  CLog::Log(LOGNOTICE, "CAMLPlayer::Process");
  try
  {
    CJobManager::GetInstance().Pause(kJobTypeMediaFlags);

    if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
    {
      if (!WaitForPausedThumbJobs(20000))
      {
        CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);
        throw "CAMLPlayer::Process:thumbgen jobs still running !!!";
      }
    }

    static AML_URLProtocol vfs_protocol = {
      "vfs",
      CFileURLProtocol::Open,
      CFileURLProtocol::Read,
      CFileURLProtocol::Write,
      CFileURLProtocol::Seek,
      CFileURLProtocol::SeekEx,
      CFileURLProtocol::Close,
    };

    CStdString url = m_item.GetPath();
    if (url.Left(strlen("smb://")).Equals("smb://"))
    {
      // the name string needs to persist
      static const char *smb_name = "smb";
      vfs_protocol.name = smb_name;
    }
    else if (url.Left(strlen("afp://")).Equals("afp://"))
    {
      // the name string needs to persist
      static const char *afp_name = "afp";
      vfs_protocol.name = afp_name;
    }
    else if (url.Left(strlen("nfs://")).Equals("nfs://"))
    {
      // the name string needs to persist
      static const char *nfs_name = "nfs";
      vfs_protocol.name = nfs_name;
    }
    else if (url.Left(strlen("rar://")).Equals("rar://"))
    {
      // the name string needs to persist
      static const char *rar_name = "rar";
      vfs_protocol.name = rar_name;
    }
    else if (url.Left(strlen("ftp://")).Equals("ftp://"))
    {
      // the name string needs to persist
      static const char *http_name = "xb-ftp";
      vfs_protocol.name = http_name;
      url = "xb-" + url;
    }
    else if (url.Left(strlen("ftps://")).Equals("ftps://"))
    {
      // the name string needs to persist
      static const char *http_name = "xb-ftps";
      vfs_protocol.name = http_name;
      url = "xb-" + url;
    }
    else if (url.Left(strlen("http://")).Equals("http://"))
    {
      // the name string needs to persist
      static const char *http_name = "xb-http";
      vfs_protocol.name = http_name;
      url = "xb-" + url;
    }
    else if (url.Left(strlen("https://")).Equals("https://"))
    {
      // the name string needs to persist
      static const char *http_name = "xb-https";
      vfs_protocol.name = http_name;
      url = "xb-" + url;
    }
    else if (url.Left(strlen("hdhomerun://")).Equals("hdhomerun://"))
    {
      // the name string needs to persist
      static const char *http_name = "xb-hdhomerun";
      vfs_protocol.name = http_name;
      url = "xb-" + url;
    }
    CLog::Log(LOGDEBUG, "CAMLPlayer::Process: URL=%s", url.c_str());

    if (m_dll->player_init() != PLAYER_SUCCESS)
    {
      CLog::Log(LOGDEBUG, "player init failed");
      throw "CAMLPlayer::Process:player init failed";
    }
    CLog::Log(LOGDEBUG, "player init......");
    usleep(250 * 1000);

    // must be after player_init
    m_dll->av_register_protocol2(&vfs_protocol, sizeof(vfs_protocol));

    static play_control_t play_control;
    memset(&play_control, 0, sizeof(play_control_t));
    // if we do not register a callback,
    // then the libamplayer will free run checking status.
    m_dll->player_register_update_callback(&play_control.callback_fn, &UpdatePlayerInfo, 1000);
    // amlplayer owns file_name and will release on exit
    play_control.file_name = (char*)strdup(url.c_str());
    //play_control->nosound   = 1; // if disable audio...,must call this api
    play_control.video_index = -1; //MUST
    play_control.audio_index = -1; //MUST
    play_control.sub_index   = -1; //MUST
    play_control.hassub      =  1;
    if (m_options.starttime > 0)   // player start position in seconds as is starttime
      play_control.t_pos = m_options.starttime;
    else
      play_control.t_pos     = -1;
    play_control.need_start  =  1; // if 0,you can omit player_start_play API.
                                   // just play video/audio immediately.
                                   // if 1,then need call "player_start_play" API;
    //play_control.auto_buffing_enable = 1;
    //play_control.buffing_min        = 0.2;
    //play_control.buffing_middle     = 0.5;
    //play_control.buffing_max        = 0.8;
    //play_control.byteiobufsize      =; // maps to av_open_input_file buffer size
    //play_control.loopbufsize        =;
    //play_control.enable_rw_on_pause =;
    m_aml_state.clear();
    m_aml_state.push_back(0);
    m_pid = m_dll->player_start(&play_control, 0);
    if (m_pid < 0)
    {
      if (m_log_level > 5)
        CLog::Log(LOGDEBUG, "player start failed! error = %d", m_pid);
      throw "CAMLPlayer::Process:player start failed";
    }

    // wait for media to open with 30 second timeout.
    if (WaitForFormatValid(30000))
    {
      // start the playback.
      int res = m_dll->player_start_play(m_pid);
      if (res != PLAYER_SUCCESS)
        throw "CAMLPlayer::Process:player_start_play() failed";
    }
    else
    {
      throw "CAMLPlayer::Process:WaitForFormatValid timeout";
    }

    // hide the mainvideo layer so we can get stream info
    // and setup/transition to gui video playback
    // without having video playback blended into it.
    if (m_item.IsVideo())
      ShowMainVideo(false);

    // wait for playback to start with 20 second timeout
    if (WaitForPlaying(20000))
    {
      m_speed = 1;
      m_callback.OnPlayBackSpeedChanged(m_speed);

      // get our initial status.
      GetStatus();

      // restore system volume setting.
      SetVolume(g_settings.m_fVolumeLevel);

      // the default staturation is to high, drop it
      SetVideoSaturation(110);

      // drop CGUIDialogBusy dialog and release the hold in OpenFile.
      m_ready.Set();

      // we are playing but hidden and all stream fields are valid.
      // check for video in media content
      if (GetVideoStreamCount() > 0)
      {
        SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);

        // turn on/off subs
        SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
        SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);

        // setup renderer for bypass. This tell renderer to get out of the way as
        // hw decoder will be doing the actual video rendering in a video plane
        // that is under the GUI layer.
        int width  = GetPictureWidth();
        int height = GetPictureHeight();
        double fFrameRate = GetActualFPS();
        unsigned int flags = 0;

        flags |= CONF_FLAGS_FULLSCREEN;
        CStdString formatstr = "BYPASS";
        CLog::Log(LOGDEBUG,"%s - change configuration. %dx%d. framerate: %4.2f. format: %s",
          __FUNCTION__, width, height, fFrameRate, formatstr.c_str());
        g_renderManager.IsConfigured();
        if (!g_renderManager.Configure(width, height, width, height, fFrameRate, flags, RENDER_FMT_BYPASS, 0, 0))
        {
          CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
        }
        if (!g_renderManager.IsStarted())
        {
          CLog::Log(LOGERROR, "%s - renderer not started", __FUNCTION__);
        }

        g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);

        m_subtitle_thread = new CAMLSubTitleThread(m_dll);
        m_subtitle_thread->Create();
      }

      if (m_options.identify == false)
        m_callback.OnPlayBackStarted();

      bool stopPlaying = false;
      while (!m_bAbortRequest && !stopPlaying)
      {
        player_status pstatus = (player_status)GetPlayerSerializedState();
        switch(pstatus)
        {
          case PLAYER_INITING:
          case PLAYER_TYPE_REDY:
          case PLAYER_INITOK:
            if (m_log_level > 5)
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process: %s", m_dll->player_status2str(pstatus));
            // player is parsing file, decoder not running
            break;

          default:
          case PLAYER_RUNNING:
            GetStatus();
            // playback status, decoder is running
            break;

          case PLAYER_START:
          case PLAYER_BUFFERING:
          case PLAYER_PAUSE:
          case PLAYER_SEARCHING:
          case PLAYER_SEARCHOK:
          case PLAYER_FF_END:
          case PLAYER_FB_END:
          case PLAYER_PLAY_NEXT:
          case PLAYER_BUFFER_OK:
            if (m_log_level > 5)
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process: %s", m_dll->player_status2str(pstatus));
            break;

          case PLAYER_FOUND_SUB:
            // found a NEW subtitle in stream.
            // TODO: reload m_subtitle_streams
            if (m_log_level > 5)
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process: %s", m_dll->player_status2str(pstatus));
            break;

          case PLAYER_PLAYEND:
            GetStatus();
            if (m_log_level > 5)
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process: %s", m_dll->player_status2str(pstatus));
            break;

          case PLAYER_ERROR:
            if (m_log_level > 5)
            {
              printf("CAMLPlayer::Process PLAYER_ERROR\n");
              printf("CAMLPlayer::Process: %s\n", m_dll->player_status2str(pstatus));
            }
            m_bAbortRequest = true;
            break;

          case PLAYER_STOPED:
          case PLAYER_EXIT:
            if (m_log_level > 5)
            {
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process PLAYER_STOPED");
              CLog::Log(LOGDEBUG, "CAMLPlayer::Process: %s", m_dll->player_status2str(pstatus));
            }
            stopPlaying = true;
            break;
        }
        usleep(250 * 1000);
      }
    }
  }
  catch(char* error)
  {
    CLog::Log(LOGERROR, "%s", error);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CAMLPlayer::Process Exception thrown");
  }

  if (m_log_level > 5)
    CLog::Log(LOGDEBUG, "CAMLPlayer::Process stopped");
  if (m_dll->check_pid_valid(m_pid))
  {
    delete m_subtitle_thread;
    m_subtitle_thread = NULL;
    m_dll->player_stop(m_pid);
    m_dll->player_exit(m_pid);
    m_pid = -1;
  }

  // we are done, hide the mainvideo layer.
  ShowMainVideo(false);
  m_ready.Set();

  ClearStreamInfos();

  // reset ac3/dts passthough
  SetAudioPassThrough(AFORMAT_UNKNOWN);
  // let thumbgen jobs resume.
  CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);

  if (m_log_level > 5)
    CLog::Log(LOGDEBUG, "CAMLPlayer::Process exit");
}
/*
void CAMLPlayer::GetRenderFeatures(Features* renderFeatures)
{
  renderFeatures->push_back(RENDERFEATURE_ZOOM);
  renderFeatures->push_back(RENDERFEATURE_CONTRAST);
  renderFeatures->push_back(RENDERFEATURE_BRIGHTNESS);
  renderFeatures->push_back(RENDERFEATURE_STRETCH);
  return;
}

void CAMLPlayer::GetDeinterlaceMethods(Features* deinterlaceMethods)
{
  deinterlaceMethods->push_back(VS_INTERLACEMETHOD_DEINTERLACE);
  return;
}

void CAMLPlayer::GetDeinterlaceModes(Features* deinterlaceModes)
{
  deinterlaceModes->push_back(VS_DEINTERLACEMODE_AUTO);
  return;
}

void CAMLPlayer::GetScalingMethods(Features* scalingMethods)
{
  return;
}

void CAMLPlayer::GetAudioCapabilities(Features* audioCaps)
{
  audioCaps->push_back(IPC_AUD_OFFSET);
  audioCaps->push_back(IPC_AUD_SELECT_STREAM);
  return;
}

void CAMLPlayer::GetSubtitleCapabilities(Features* subCaps)
{
  subCaps->push_back(IPC_SUBS_EXTERNAL);
  subCaps->push_back(IPC_SUBS_OFFSET);
  subCaps->push_back(IPC_SUBS_SELECT);
  return;
}
*/

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
int CAMLPlayer::GetVideoStreamCount()
{
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetVideoStreamCount(%d)", m_video_count);
  return m_video_count;
}

void CAMLPlayer::ShowMainVideo(bool show)
{
  if (m_show_mainvideo == show)
    return;

  aml_set_sysfs_int("/sys/class/video/disable_video", show ? 0:1);

  m_show_mainvideo = show;
}

void CAMLPlayer::SetVideoZoom(float zoom)
{
  // input zoom range is 0.5 to 2.0 with a default of 1.0.
  // output zoom range is 2 to 300 with default of 100.
  // we limit that to a range of 50 to 200 with default of 100.
  aml_set_sysfs_int("/sys/class/video/zoom", (int)(100 * zoom));
}

void CAMLPlayer::SetVideoContrast(int contrast)
{
  // input contrast range is 0 to 100 with default of 50.
  // output contrast range is -255 to 255 with default of 0.
  contrast = (255 * (contrast - 50)) / 50;
  aml_set_sysfs_int("/sys/class/video/contrast", contrast);
}
void CAMLPlayer::SetVideoBrightness(int brightness)
{
  // input brightness range is 0 to 100 with default of 50.
  // output brightness range is -127 to 127 with default of 0.
  brightness = (127 * (brightness - 50)) / 50;
  aml_set_sysfs_int("/sys/class/video/brightness", brightness);
}
void CAMLPlayer::SetVideoSaturation(int saturation)
{
  // output saturation range is -127 to 127 with default of 127.
  aml_set_sysfs_int("/sys/class/video/saturation", saturation);
}

void CAMLPlayer::SetAudioPassThrough(int format)
{
  aml_set_audio_passthrough(
    (m_audio_passthrough_ac3 && format == AFORMAT_AC3) ||
    (m_audio_passthrough_dts && format == AFORMAT_DTS));
}

bool CAMLPlayer::WaitForPausedThumbJobs(int timeout_ms)
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

int CAMLPlayer::GetPlayerSerializedState(void)
{
  CSingleLock lock(m_aml_state_csection);

  int playerstate;
  int dequeue_size = m_aml_state.size();

  if (dequeue_size > 0)
  {
    // serialized state is the front element.
    playerstate = m_aml_state.front();
    // pop the front element if there are
    // more present.
    if (dequeue_size > 1)
      m_aml_state.pop_front();
  }
  else
  {
    // if queue is empty (only at startup),
    // pull the player state directly. this should
    // really never happen but we need to cover it.
    playerstate = m_dll->player_get_state(m_pid);
    m_aml_state.push_back(playerstate);
  }

  return playerstate;
}


int CAMLPlayer::UpdatePlayerInfo(int pid, player_info_t *info)
{
  // we get called when status changes or after update time expires.
  // static callback from libamplayer, since it does not pass an opaque,
  // we have to retreve our player class reference the hard way.
  CAMLPlayer *amlplayer = dynamic_cast<CAMLPlayer*>(g_application.m_pPlayer);
  if (amlplayer)
  {
    CSingleLock lock(amlplayer->m_aml_state_csection);
    if (amlplayer->m_aml_state.back() != info->status)
    {
      //CLog::Log(LOGDEBUG, "update_player_info: %s, old state %s", player_status2str(info->status), player_status2str(info->last_sta));
      amlplayer->m_aml_state.push_back(info->status);
    }
  }
  return 0;
}

bool CAMLPlayer::CheckPlaying()
{
  return ((player_status)GetPlayerSerializedState() == PLAYER_RUNNING);
}

bool CAMLPlayer::WaitForStopped(int timeout_ms)
{
  while (!m_bAbortRequest && (timeout_ms > 0))
  {
    player_status pstatus = (player_status)GetPlayerSerializedState();
    if (m_log_level > 5)
      CLog::Log(LOGDEBUG, "CAMLPlayer::WaitForStopped: %s", m_dll->player_status2str(pstatus));
    switch(pstatus)
    {
      default:
        usleep(100 * 1000);
        timeout_ms -= 100;
        break;
      case PLAYER_PLAYEND:
      case PLAYER_STOPED:
      case PLAYER_ERROR:
      case PLAYER_EXIT:
        m_bAbortRequest = true;
        return true;
        break;
    }
  }

  return false;
}

bool CAMLPlayer::WaitForSearchOK(int timeout_ms)
{
  while (!m_bAbortRequest && (timeout_ms > 0))
  {
    player_status pstatus = (player_status)GetPlayerSerializedState();
    if (m_log_level > 5)
      CLog::Log(LOGDEBUG, "CAMLPlayer::WaitForSearchOK: %s", m_dll->player_status2str(pstatus));
    switch(pstatus)
    {
      default:
        usleep(100 * 1000);
        timeout_ms -= 100;
        break;
      case PLAYER_STOPED:
        return false;
      case PLAYER_ERROR:
      case PLAYER_EXIT:
        m_bAbortRequest = true;
        return false;
        break;
      case PLAYER_SEARCHOK:
        return true;
        break;
    }
  }

  return false;
}

bool CAMLPlayer::WaitForPlaying(int timeout_ms)
{
  while (!m_bAbortRequest && (timeout_ms > 0))
  {
    player_status pstatus = (player_status)GetPlayerSerializedState();
    if (m_log_level > 5)
      CLog::Log(LOGDEBUG, "CAMLPlayer::WaitForPlaying: %s", m_dll->player_status2str(pstatus));
    switch(pstatus)
    {
      default:
        usleep(100 * 1000);
        timeout_ms -= 100;
        break;
      case PLAYER_ERROR:
      case PLAYER_EXIT:
        m_bAbortRequest = true;
        return false;
        break;
      case PLAYER_RUNNING:
        return true;
        break;
    }
  }

  return false;
}

bool CAMLPlayer::WaitForFormatValid(int timeout_ms)
{
  while (timeout_ms > 0)
  {
    player_status pstatus = (player_status)GetPlayerSerializedState();
    if (m_log_level > 5)
      CLog::Log(LOGDEBUG, "CAMLPlayer::WaitForFormatValid: %s", m_dll->player_status2str(pstatus));
    switch(pstatus)
    {
      default:
        usleep(100 * 1000);
        timeout_ms -= 100;
        break;
      case PLAYER_ERROR:
      case PLAYER_EXIT:
        m_bAbortRequest = true;
        return false;
        break;
      case PLAYER_INITOK:

        ClearStreamInfos();

        media_info_t media_info;
        int res = m_dll->player_get_media_info(m_pid, &media_info);
        if (res != PLAYER_SUCCESS)
          return false;

        if (m_log_level > 5)
        {
          media_info_dump(&media_info);

          // m_video_index, m_audio_index, m_subtitle_index might be -1 eventhough
          // total_video_xxx is > 0, not sure why, they should be set to zero or
          // some other sensible value.
          CLog::Log(LOGDEBUG, "CAMLPlayer::WaitForFormatValid: "
            "m_video_index(%d), m_audio_index(%d), m_subtitle_index(%d), m_chapter_count(%d)",
            media_info.stream_info.cur_video_index,
            media_info.stream_info.cur_audio_index,
#if !defined(TARGET_ANDROID)
            media_info.stream_info.cur_sub_index,
            media_info.stream_info.total_chapter_num);
#else
            media_info.stream_info.cur_sub_index,
            0);
#endif
        }

        // video info
        if (media_info.stream_info.has_video && media_info.stream_info.total_video_num > 0)
        {
          for (int i = 0; i < media_info.stream_info.total_video_num; i++)
          {
            AMLPlayerStreamInfo *info = new AMLPlayerStreamInfo;
            info->Clear();

            info->id              = media_info.video_info[i]->id;
            info->type            = STREAM_VIDEO;
            info->width           = media_info.video_info[i]->width;
            info->height          = media_info.video_info[i]->height;
            info->aspect_ratio_num= media_info.video_info[i]->aspect_ratio_num;
            info->aspect_ratio_den= media_info.video_info[i]->aspect_ratio_den;
            info->frame_rate_num  = media_info.video_info[i]->frame_rate_num;
            info->frame_rate_den  = media_info.video_info[i]->frame_rate_den;
            info->bit_rate        = media_info.video_info[i]->bit_rate;
            info->duration        = media_info.video_info[i]->duartion;
            info->format          = media_info.video_info[i]->format;

            m_video_streams.push_back(info);
          }

          m_video_index	= media_info.stream_info.cur_video_index;
          m_video_count	= media_info.stream_info.total_video_num;
          if (m_video_index != 0)
            m_video_index = 0;
          m_video_width	= media_info.video_info[m_video_index]->width;
          m_video_height= media_info.video_info[m_video_index]->height;
          m_video_fps_numerator	= media_info.video_info[m_video_index]->frame_rate_num;
          m_video_fps_denominator = media_info.video_info[m_video_index]->frame_rate_den;

          // bail if we do not get a valid width/height
          if (m_video_width == 0 || m_video_height == 0)
            return false;
        }

        // audio info
        if (media_info.stream_info.has_audio && media_info.stream_info.total_audio_num > 0)
        {
          for (int i = 0; i < media_info.stream_info.total_audio_num; i++)
          {
            AMLPlayerStreamInfo *info = new AMLPlayerStreamInfo;
            info->Clear();

            info->id              = media_info.audio_info[i]->id;
            info->type            = STREAM_AUDIO;
            info->channel         = media_info.audio_info[i]->channel;
            info->sample_rate     = media_info.audio_info[i]->sample_rate;
            info->bit_rate        = media_info.audio_info[i]->bit_rate;
            info->duration        = media_info.audio_info[i]->duration;
            info->format          = media_info.audio_info[i]->aformat;
#if !defined(TARGET_ANDROID)
            if (media_info.audio_info[i]->audio_language[0] != 0)
              info->language = std::string(media_info.audio_info[i]->audio_language, 3);
#endif
            m_audio_streams.push_back(info);
          }

          m_audio_index	= media_info.stream_info.cur_audio_index;
          if (m_audio_index != 0)
            m_audio_index = 0;
          m_audio_count	= media_info.stream_info.total_audio_num;
          // setup ac3/dts passthough if required
          SetAudioPassThrough(m_audio_streams[m_audio_index]->format);
        }

        // subtitle info
        if (media_info.stream_info.has_sub && media_info.stream_info.total_sub_num > 0)
        {
          for (int i = 0; i < media_info.stream_info.total_sub_num; i++)
          {
            AMLPlayerStreamInfo *info = new AMLPlayerStreamInfo;
            info->Clear();

            info->id   = media_info.sub_info[i]->id;
            info->type = STREAM_SUBTITLE;
            if (media_info.sub_info[i]->sub_language && media_info.sub_info[i]->sub_language[0] != 0)
              info->language = std::string(media_info.sub_info[i]->sub_language, 3);
            m_subtitle_streams.push_back(info);
          }
          m_subtitle_index = media_info.stream_info.cur_sub_index;
        }
        // find any external subs
        FindSubtitleFiles();
        // setup count and index
        m_subtitle_count = m_subtitle_streams.size();
        if (m_subtitle_count && m_subtitle_index != 0)
          m_subtitle_index = 0;

#if !defined(TARGET_ANDROID)
        // chapter info
        if (media_info.stream_info.total_chapter_num > 0)
        {
          m_chapter_count = media_info.stream_info.total_chapter_num;
          for (int i = 0; i < m_chapter_count; i++)
          {
            if (media_info.chapter_info[i] != NULL)
            {
              AMLChapterInfo *info = new AMLChapterInfo;

              info->name = media_info.chapter_info[i]->name;
              info->seekto_ms = media_info.chapter_info[i]->seekto_ms;
              m_chapters.push_back(info);
            }
          }
        }
#endif
        return true;
        break;
    }
  }

  return false;
}

void CAMLPlayer::ClearStreamInfos()
{
  CSingleLock lock(m_aml_csection);

  if (!m_audio_streams.empty())
  {
    for (unsigned int i = 0; i < m_audio_streams.size(); i++)
      delete m_audio_streams[i];
    m_audio_streams.clear();
  }
  m_audio_count = 0;
  m_audio_index = -1;

  if (!m_video_streams.empty())
  {
    for (unsigned int i = 0; i < m_video_streams.size(); i++)
      delete m_video_streams[i];
    m_video_streams.clear();
  }
  m_video_count = 0;
  m_video_index = -1;

  if (!m_subtitle_streams.empty())
  {
    for (unsigned int i = 0; i < m_subtitle_streams.size(); i++)
      delete m_subtitle_streams[i];
    m_subtitle_streams.clear();
  }
  m_subtitle_count = 0;
  m_subtitle_index = -1;

  if (!m_chapters.empty())
  {
    for (unsigned int i = 0; i < m_chapters.size(); i++)
      delete m_chapters[i];
    m_chapters.clear();
  }
  m_chapter_count = 0;
}

bool CAMLPlayer::GetStatus()
{
  CSingleLock lock(m_aml_csection);

  if (!m_dll->check_pid_valid(m_pid))
    return false;

  player_info_t player_info;
  int res = m_dll->player_get_play_info(m_pid, &player_info);
  if (res != PLAYER_SUCCESS)
    return false;

  m_elapsed_ms  = player_info.current_ms;
  m_duration_ms = 1000 * player_info.full_time;
  //CLog::Log(LOGDEBUG, "CAMLPlayer::GetStatus: audio_bufferlevel(%f), video_bufferlevel(%f), bufed_time(%d), bufed_pos(%lld)",
  //  player_info.audio_bufferlevel, player_info.video_bufferlevel, player_info.bufed_time, player_info.bufed_pos);

  return true;
}

void CAMLPlayer::FindSubtitleFiles()
{
  // find any available external subtitles
  std::vector<CStdString> filenames;
  CUtil::ScanForExternalSubtitles(m_item.GetPath(), filenames);

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
  }
}

int CAMLPlayer::AddSubtitleFile(const std::string &filename, const std::string &subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;

  if(ext == ".idx")
  {
    /* TODO: we do not handle idx/sub binary subs yet.
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
    */
    return -1;
  }
  if(ext == ".sub")
  {
    // check for texual sub, if this is a idx/sub pair, ignore it.
    CStdString strReplace(URIUtils::ReplaceExtension(filename,".idx"));
    if (XFILE::CFile::Exists(strReplace))
      return -1;
  }

  AMLPlayerStreamInfo *info = new AMLPlayerStreamInfo;
  info->Clear();

  info->id       = 0;
  info->type     = STREAM_SUBTITLE;
  info->source   = STREAM_SOURCE_TEXT;
  info->filename = filename;
  info->name     = URIUtils::GetFileName(filename);
  info->frame_rate_num = m_video_fps_numerator;
  info->frame_rate_den = m_video_fps_denominator;
  m_subtitle_streams.push_back(info);

  return m_subtitle_streams.size();
}

bool CAMLPlayer::OpenSubtitleStream(int index)
{
  CLog::Log(LOGNOTICE, "Opening external subtitle stream: %i", index);

  CDemuxStream* pStream = NULL;
  std::string filename;
  CDVDStreamInfo hint;

  if (m_subtitle_streams[index]->source == STREAM_SOURCE_DEMUX_SUB)
  {
    /*
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, source, iStream);
    if(index < 0)
      return false;
    SelectionStream st = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

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
    double pts = m_dvdPlayerVideo.GetCurrentPts();
    if(pts == DVD_NOPTS_VALUE)
      pts = m_CurrentVideo.dts;
    if(pts == DVD_NOPTS_VALUE)
      pts = 0;
    pts += m_offset_pts;
    m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE));

    hint.Assign(*pStream, true);
    */
    return false;
  }
  else if (m_subtitle_streams[index]->source == STREAM_SOURCE_TEXT)
  {
    filename = m_subtitle_streams[index]->filename;

    hint.Clear();
    hint.fpsscale = m_subtitle_streams[index]->frame_rate_den;
    hint.fpsrate  = m_subtitle_streams[index]->frame_rate_num;
  }

  m_dvdPlayerSubtitle->CloseStream(true);
  if (!m_dvdPlayerSubtitle->OpenStream(hint, filename))
  {
    CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, index);
    if(pStream)
    {
      pStream->disabled = true;
      pStream->SetDiscard(AVDISCARD_ALL);
    }
    return false;
  }

  return true;
}

void CAMLPlayer::SetVideoRect(const CRect &SrcRect, const CRect &DestRect)
{
  // this routine gets called every video frame
  // and is in the context of the renderer thread so
  // do not do anything stupid here.

  // video zoom adjustment.
  float zoom = g_settings.m_currentVideoSettings.m_CustomZoomAmount;
  if ((int)(zoom * 1000) != (int)(m_zoom * 1000))
  {
    m_zoom = zoom;
  }
  // video contrast adjustment.
  int contrast = g_settings.m_currentVideoSettings.m_Contrast;
  if (contrast != m_contrast)
  {
    SetVideoContrast(contrast);
    m_contrast = contrast;
  }
  // video brightness adjustment.
  int brightness = g_settings.m_currentVideoSettings.m_Brightness;
  if (brightness != m_brightness)
  {
    SetVideoBrightness(brightness);
    m_brightness = brightness;
  }

  // check if destination rect or video view mode has changed
  if ((m_dst_rect != DestRect) || (m_view_mode != g_settings.m_currentVideoSettings.m_ViewMode))
  {
    m_dst_rect  = DestRect;
    m_view_mode = g_settings.m_currentVideoSettings.m_ViewMode;
  }
  else
  {
    // mainvideo 'should' be showing already if we get here, make sure.
    ShowMainVideo(true);
    return;
  }

  CRect gui, display, dst_rect;
  gui = g_graphicsContext.GetViewWindow();
  // when display is at 1080p, we have freescale enabled
  // and that scales all layers into 1080p display including video,
  // so we have to setup video axis for 720p instead of 1080p... Boooo.
  display = g_graphicsContext.GetViewWindow();
  //RESOLUTION res = g_graphicsContext.GetVideoResolution();
  //display.SetRect(0, 0, g_settings.m_ResInfo[res].iScreenWidth, g_settings.m_ResInfo[res].iScreenHeight);
  dst_rect = m_dst_rect;
  if (gui != display)
  {
    float xscale = display.Width()  / gui.Width();
    float yscale = display.Height() / gui.Height();
    dst_rect.x1 *= xscale;
    dst_rect.x2 *= xscale;
    dst_rect.y1 *= yscale;
    dst_rect.y2 *= yscale;
  }

  ShowMainVideo(false);

  // goofy 0/1 based difference in aml axis coordinates.
  // fix them.
  dst_rect.x2--;
  dst_rect.y2--;

  char video_axis[256] = {0};
  sprintf(video_axis, "%d %d %d %d", (int)dst_rect.x1, (int)dst_rect.y1, (int)dst_rect.x2, (int)dst_rect.y2);
  aml_set_sysfs_str("/sys/class/video/axis", video_axis);
/*
  CStdString rectangle;
  rectangle.Format("%i,%i,%i,%i",
    (int)dst_rect.x1, (int)dst_rect.y1,
    (int)dst_rect.Width(), (int)dst_rect.Height());
  CLog::Log(LOGDEBUG, "CAMLPlayer::SetVideoRect:dst_rect(%s)", rectangle.c_str());
*/
  // we only get called once gui has changed to something
  // that would show video playback, so show it.
  ShowMainVideo(true);
}

void CAMLPlayer::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  CAMLPlayer *player = (CAMLPlayer*)ctx;
  player->SetVideoRect(SrcRect, DestRect);
}

void CAMLPlayer::GetRenderFeatures(std::vector<int> &renderFeatures)
{
  renderFeatures.push_back(RENDERFEATURE_ZOOM);
  renderFeatures.push_back(RENDERFEATURE_CONTRAST);
  renderFeatures.push_back(RENDERFEATURE_BRIGHTNESS);
  renderFeatures.push_back(RENDERFEATURE_STRETCH);
}

void CAMLPlayer::GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods)
{
  deinterlaceMethods.push_back(VS_INTERLACEMETHOD_DEINTERLACE);
}

void CAMLPlayer::GetDeinterlaceModes(std::vector<int> &deinterlaceModes)
{
  deinterlaceModes.push_back(VS_DEINTERLACEMODE_AUTO);
}

void CAMLPlayer::GetScalingMethods(std::vector<int> &scalingMethods)
{
}

void CAMLPlayer::GetAudioCapabilities(std::vector<int> &audioCaps)
{
  audioCaps.push_back(IPC_AUD_SELECT_STREAM);
  audioCaps.push_back(IPC_AUD_SELECT_OUTPUT);
#if !defined(TARGET_ANDROID)
  audioCaps.push_back(IPC_AUD_OFFSET);
#endif
}

void CAMLPlayer::GetSubtitleCapabilities(std::vector<int> &subCaps)
{
  subCaps.push_back(IPC_SUBS_EXTERNAL);
  subCaps.push_back(IPC_SUBS_SELECT);
  subCaps.push_back(IPC_SUBS_OFFSET);
}

