/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamNavigator.h"

#include "../DVDDemuxSPU.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "filesystem/IFileTypes.h"
#if defined(TARGET_WINDOWS_STORE)
#include "filesystem/SpecialProtocol.h"
#endif
#include "guilib/LocalizeStrings.h"
#if defined(TARGET_WINDOWS_STORE)
#include "platform/Environment.h"
#endif
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Geometry.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/CocoaInterface.h"
#endif

#include <memory>

namespace
{
constexpr int HOLDMODE_NONE = 0;
/* set internally when we wish to flush demuxer */
constexpr int HOLDMODE_HELD = 1;
/* set by inputstream user, when they wish to skip the held mode */
constexpr int HOLDMODE_SKIP = 2;
/* set after hold mode has been exited, and action that inited it has been executed */
constexpr int HOLDMODE_DATA = 3;

// DVD Subpicture types
constexpr int DVD_SUBPICTURE_TYPE_NOTSPECIFIED = 0;
constexpr int DVD_SUBPICTURE_TYPE_LANGUAGE = 1;

// DVD Subpicture language extensions
constexpr int DVD_SUBPICTURE_LANG_EXT_NOTSPECIFIED = 0;
constexpr int DVD_SUBPICTURE_LANG_EXT_NORMALCAPTIONS = 1;
constexpr int DVD_SUBPICTURE_LANG_EXT_BIGCAPTIONS = 2;
constexpr int DVD_SUBPICTURE_LANG_EXT_CHILDRENSCAPTIONS = 3;
constexpr int DVD_SUBPICTURE_LANG_EXT_NORMALCC = 5;
constexpr int DVD_SUBPICTURE_LANG_EXT_BIGCC = 6;
constexpr int DVD_SUBPICTURE_LANG_EXT_CHILDRENSCC = 7;
constexpr int DVD_SUBPICTURE_LANG_EXT_FORCED = 9;
constexpr int DVD_SUBPICTURE_LANG_EXT_NORMALDIRECTORSCOMMENTS = 13;
constexpr int DVD_SUBPICTURE_LANG_EXT_BIGDIRECTORSCOMMENTS = 14;
constexpr int DVD_SUBPICTURE_LANG_EXT_CHILDRENDIRECTORSCOMMENTS = 15;

// DVD Audio language extensions
constexpr int DVD_AUDIO_LANG_EXT_NOTSPECIFIED = 0;
constexpr int DVD_AUDIO_LANG_EXT_NORMALCAPTIONS = 1;
constexpr int DVD_AUDIO_LANG_EXT_VISUALLYIMPAIRED = 2;
constexpr int DVD_AUDIO_LANG_EXT_DIRECTORSCOMMENTS1 = 3;
constexpr int DVD_AUDIO_LANG_EXT_DIRECTORSCOMMENTS2 = 4;
} // namespace

static int dvd_inputstreamnavigator_cb_seek(void * p_stream, uint64_t i_pos);
static int dvd_inputstreamnavigator_cb_read(void * p_stream, void * buffer, int i_read);
static int dvd_inputstreamnavigator_cb_readv(void * p_stream, void * p_iovec, int i_blocks);
static void dvd_logger(void* priv, dvdnav_logger_level_t level, const char* fmt, va_list va);

CDVDInputStreamNavigator::CDVDInputStreamNavigator(IVideoPlayer* player, const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_DVD, fileitem), m_pstream(nullptr)
{
  m_dvdnav = 0;
  m_pVideoPlayer = player;
  m_bCheckButtons = false;
  m_iCellStart = 0;
  m_iVobUnitStart = 0LL;
  m_iVobUnitStop = 0LL;
  m_iVobUnitCorrection = 0LL;
  m_bInMenu = false;
  m_holdmode = HOLDMODE_NONE;
  m_iTitle = m_iTitleCount = 0;
  m_iPart = m_iPartCount = 0;
  m_iTime = m_iTotalTime = 0;
  m_bEOF = false;
  m_lastevent = DVDNAV_NOP;
  m_dvdnav_stream_cb.pf_read = dvd_inputstreamnavigator_cb_read;
  m_dvdnav_stream_cb.pf_readv = dvd_inputstreamnavigator_cb_readv;
  m_dvdnav_stream_cb.pf_seek = dvd_inputstreamnavigator_cb_seek;

  memset(m_lastblock, 0, sizeof(m_lastblock));
}

CDVDInputStreamNavigator::~CDVDInputStreamNavigator()
{
  Close();
}

bool CDVDInputStreamNavigator::Open()
{
  m_item.SetMimeType("video/x-dvd-mpeg");
  if (!CDVDInputStream::Open())
    return false;

#if defined(TARGET_WINDOWS_STORE)
  // libdvdcss
  CEnvironment::putenv("DVDCSS_METHOD=key");
  CEnvironment::putenv("DVDCSS_VERBOSE=3");
  CEnvironment::putenv("DVDCSS_CACHE=" + CSpecialProtocol::TranslatePath("special://masterprofile/cache"));
#endif

  // load libdvdnav.dll
  if (!m_dll.Load())
    return false;

  // load the dvd language codes
  // g_LangCodeExpander.LoadStandardCodes();

  // libdvdcss fails if the file path contains VIDEO_TS.IFO or VIDEO_TS/VIDEO_TS.IFO
  // libdvdnav is still able to play without, so strip them.

  std::string path = m_item.GetDynPath();
  if(URIUtils::GetFileName(path) == "VIDEO_TS.IFO")
    path = URIUtils::GetParentPath(path);
  URIUtils::RemoveSlashAtEnd(path);
  if(URIUtils::GetFileName(path) == "VIDEO_TS")
    path = URIUtils::GetParentPath(path);
  URIUtils::RemoveSlashAtEnd(path);

#if defined(TARGET_DARWIN_OSX)
  // if physical DVDs, libdvdnav wants "/dev/rdiskN" device name for OSX,
  // strDVDFile will get realloc'ed and replaced IF this is a physical DVD.
  char* strDVDFile = Cocoa_MountPoint2DeviceName(strdup(path.c_str()));
  path = strDVDFile;
  free(strDVDFile);
#endif

#if DVDNAV_VERSION >= 60100
  dvdnav_logger_cb loggerCallback;
  loggerCallback.pf_log = dvd_logger;
#endif

  // open up the DVD device
  if (m_item.IsDiscImage())
  {
    // if dvd image file (ISO or alike) open using libdvdnav stream callback functions
    m_pstream = std::make_unique<CDVDInputStreamFile>(
        m_item, XFILE::READ_TRUNCATED | XFILE::READ_BITRATE | XFILE::READ_CHUNKED);
#if DVDNAV_VERSION >= 60100
    if (!m_pstream->Open() || m_dll.dvdnav_open_stream2(&m_dvdnav, m_pstream.get(), &loggerCallback,
                                                        &m_dvdnav_stream_cb) != DVDNAV_STATUS_OK)
#else
    if (!m_pstream->Open() || m_dll.dvdnav_open_stream(&m_dvdnav, m_pstream.get(), &m_dvdnav_stream_cb) != DVDNAV_STATUS_OK)
#endif
    {
      CLog::Log(LOGERROR, "Error opening image file or Error on dvdnav_open_stream");
      Close();
      return false;
    }
  }
#if DVDNAV_VERSION >= 60100
  else if (m_dll.dvdnav_open2(&m_dvdnav, nullptr, &loggerCallback, path.c_str()) !=
           DVDNAV_STATUS_OK)
#else
  else if (m_dll.dvdnav_open(&m_dvdnav, path.c_str()) != DVDNAV_STATUS_OK)
#endif
  {
    CLog::Log(LOGERROR, "Error on dvdnav_open");
    Close();
    return false;
  }

  int region = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_DVDS_PLAYERREGION);
  int mask = 0;
  if(region > 0)
    mask = 1 << (region-1);
  else
  {
    // find out what region dvd reports itself to be from, and use that as mask if available
    if (m_dll.dvdnav_get_disk_region_mask(m_dvdnav, &mask) == DVDNAV_STATUS_ERR)
    {
      CLog::LogF(LOGERROR, "Error getting DVD region code: {}",
                 m_dll.dvdnav_err_to_string(m_dvdnav));
      mask = 0xff;
    }
  }

  CLog::Log(LOGDEBUG, "{} - Setting region mask {:02x}", __FUNCTION__, mask);
  m_dll.dvdnav_set_region_mask(m_dvdnav, mask);

  // get default language settings
  char language_menu[3];
  strncpy(language_menu, g_langInfo.GetDVDMenuLanguage().c_str(), sizeof(language_menu)-1);
  language_menu[2] = '\0';

  char language_audio[3];
  strncpy(language_audio, g_langInfo.GetDVDAudioLanguage().c_str(), sizeof(language_audio)-1);
  language_audio[2] = '\0';

  char language_subtitle[3];
  strncpy(language_subtitle, g_langInfo.GetDVDSubtitleLanguage().c_str(), sizeof(language_subtitle)-1);
  language_subtitle[2] = '\0';

  // set language settings in case they are not set in xbmc's configuration
  if (language_menu[0] == '\0') strcpy(language_menu, "en");
  if (language_audio[0] == '\0') strcpy(language_audio, "en");
  if (language_subtitle[0] == '\0') strcpy(language_subtitle, "en");

  // set default language settings
  if (m_dll.dvdnav_menu_language_select(m_dvdnav, (char*)language_menu) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default menu language: {}",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    //! @bug libdvdnav isn't const correct
    m_dll.dvdnav_menu_language_select(m_dvdnav, const_cast<char*>("en"));
  }

  if (m_dll.dvdnav_audio_language_select(m_dvdnav, (char*)language_audio) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default audio language: {}",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    //! @bug libdvdnav isn't const correct
    m_dll.dvdnav_audio_language_select(m_dvdnav, const_cast<char*>("en"));
  }

  if (m_dll.dvdnav_spu_language_select(m_dvdnav, (char*)language_subtitle) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default subtitle language: {}",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    //! @bug libdvdnav isn't const correct
    m_dll.dvdnav_spu_language_select(m_dvdnav, const_cast<char*>("en"));
  }

  // set read ahead cache usage
  if (m_dll.dvdnav_set_readahead_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on dvdnav_set_readahead_flag: {}",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // set the PGC positioning flag to have position information relatively to the
  // whole feature instead of just relatively to the current chapter
  if (m_dll.dvdnav_set_PGC_positioning_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on dvdnav_set_PGC_positioning_flag: {}",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // jump directly to title menu
  if(CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_DVDS_AUTOMENU))
  {
    int len, event;
    uint8_t buf[2048];
    uint8_t* buf_ptr = buf;

    // must startup vm and pgc
    m_dll.dvdnav_get_next_cache_block(m_dvdnav,&buf_ptr,&event,&len);
    m_dll.dvdnav_sector_search(m_dvdnav, 0, SEEK_SET);

    // first try title menu
    if(m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Title) != DVDNAV_STATUS_OK)
    {
      CLog::Log(LOGERROR, "Error on dvdnav_menu_call(Title): {}",
                m_dll.dvdnav_err_to_string(m_dvdnav));
      // next try root menu
      if(m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Root) != DVDNAV_STATUS_OK )
        CLog::Log(LOGERROR, "Error on dvdnav_menu_call(Root): {}",
                  m_dll.dvdnav_err_to_string(m_dvdnav));
    }
  }

  m_bEOF = false;
  m_bCheckButtons = false;
  m_iCellStart = 0;
  m_iVobUnitStart = 0LL;
  m_iVobUnitStop = 0LL;
  m_iVobUnitCorrection = 0LL;
  m_bInMenu = false;
  m_holdmode = HOLDMODE_NONE;
  m_iTitle = m_iTitleCount = 0;
  m_iPart = m_iPartCount = 0;
  m_iTime = m_iTotalTime = 0;

  return true;
}

void CDVDInputStreamNavigator::Close()
{
  if (!m_dvdnav) return;

  // finish off by closing the dvdnav device
  if (m_dll.dvdnav_close(m_dvdnav) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on dvdnav_close: {}", m_dll.dvdnav_err_to_string(m_dvdnav));
    return ;
  }

  CDVDInputStream::Close();
  m_dvdnav = NULL;
  m_bEOF = true;

  if (m_pstream != nullptr)
  {
    m_pstream->Close();
    m_pstream.reset();
  }
}

int CDVDInputStreamNavigator::Read(uint8_t* buf, int buf_size)
{
  if (!m_dvdnav || m_bEOF) return 0;
  if (buf_size < DVD_VIDEO_BLOCKSIZE)
  {
    CLog::Log(LOGERROR,
              "CDVDInputStreamNavigator: buffer size is to small, {} bytes, should be 2048 bytes",
              buf_size);
    return -1;
  }

  int iBytesRead = 0;

  int NOPcount = 0;
  while (true)
  {
    int navresult = ProcessBlock(buf, &iBytesRead);

    if (navresult == NAVRESULT_HOLD)
      return 0; // return 0 bytes read;
    else if (navresult == NAVRESULT_ERROR)
      return -1;
    else if (navresult == NAVRESULT_DATA)
      return iBytesRead;
    else if (navresult == NAVRESULT_NOP)
    {
      NOPcount++;
      if (NOPcount == 1000)
      {
        m_bEOF = true;
        CLog::Log(LOGERROR,"CDVDInputStreamNavigator: Stopping playback due to infinite loop, caused by badly authored DVD navigation structure. Try enabling 'Attempt to skip introduction before DVD menu'.");
        m_pVideoPlayer->OnDiscNavResult(nullptr, DVDNAV_ERROR);
        return -1; // fail and stop playback.
      }
    }
  }

  return iBytesRead;
}

// not working yet, but it is the recommended way for seeking
int64_t CDVDInputStreamNavigator::Seek(int64_t offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

int CDVDInputStreamNavigator::ProcessBlock(uint8_t* dest_buffer, int* read)
{
  if (!m_dvdnav)
    return -1;

  int result;
  int len = 2048;

  // m_tempbuffer will be used for anything that isn't a normal data block
  uint8_t* buf = m_lastblock;
  int iNavresult = -1;

  if (m_holdmode == HOLDMODE_HELD)
    return NAVRESULT_HOLD;

  // the main reading function
  if(m_holdmode == HOLDMODE_SKIP)
  { /* we where holding data, return the data held */
    m_holdmode = HOLDMODE_DATA;
    result = DVDNAV_STATUS_OK;
  }
  else
    result = m_dll.dvdnav_get_next_cache_block(m_dvdnav, &buf, &m_lastevent, &len);

  if (result == DVDNAV_STATUS_ERR)
  {
    CLog::Log(LOGERROR, "Error getting next block: {}", m_dll.dvdnav_err_to_string(m_dvdnav));
    m_bEOF = true;
    return NAVRESULT_ERROR;
  }

    switch (m_lastevent)
    {
    case DVDNAV_BLOCK_OK:
      {
        // We have received a regular block of the currently playing MPEG stream.
        // buf contains the data and len its length (obviously!) (which is always 2048 bytes btw)
        m_holdmode = HOLDMODE_NONE;
        memcpy(dest_buffer, buf, len);
        *read = len;
        iNavresult = NAVRESULT_DATA;
      }
      break;

    case DVDNAV_NOP:
      // Nothing to do here.
      break;

    case DVDNAV_STILL_FRAME:
      {
        // We have reached a still frame. A real player application would wait
        // the amount of time specified by the still's length while still handling
        // user input to make menus and other interactive stills work.
        // A length of 0xff means an indefinite still which has to be skipped
        // indirectly by some user interaction.
        m_holdmode = HOLDMODE_NONE;
        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_STILL_FRAME);
      }
      break;

    case DVDNAV_WAIT:
      {
        // We have reached a point in DVD playback, where timing is critical.
        // Player application with internal fifos can introduce state
        // inconsistencies, because libdvdnav is always the fifo's length
        // ahead in the stream compared to what the application sees.
        // Such applications should wait until their fifos are empty
        // when they receive this type of event.
        if(m_holdmode == HOLDMODE_NONE)
        {
          CLog::Log(LOGDEBUG, " - DVDNAV_WAIT (HOLDING)");
          m_holdmode = HOLDMODE_HELD;
          iNavresult = NAVRESULT_HOLD;
        }
        else
          iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_WAIT);

        /* if user didn't care for action, just skip it */
        if(iNavresult == NAVRESULT_NOP)
          SkipWait();
      }
      break;

    case DVDNAV_SPU_CLUT_CHANGE:
      // Player applications should pass the new colour lookup table to their
      // SPU decoder. The CLUT is given as 16 uint32_t's in the buffer.
      {
        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_SPU_CLUT_CHANGE);
      }
      break;

    case DVDNAV_SPU_STREAM_CHANGE:
      // Player applications should inform their SPU decoder to switch channels
      {
        m_bCheckButtons = true;
        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_SPU_STREAM_CHANGE);
      }
      break;

    case DVDNAV_AUDIO_STREAM_CHANGE:
      // Player applications should inform their audio decoder to switch channels
      {
        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_AUDIO_STREAM_CHANGE);
      }

      break;

    case DVDNAV_HIGHLIGHT:
      {
        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_HIGHLIGHT);
      }
      break;

    case DVDNAV_VTS_CHANGE:
      // Some status information like video aspect and video scale permissions do
      // not change inside a VTS. Therefore this event can be used to query such
      // information only when necessary and update the decoding/displaying
      // accordingly.
      {
        if(m_holdmode == HOLDMODE_NONE)
        {
          CLog::Log(LOGDEBUG, " - DVDNAV_VTS_CHANGE (HOLDING)");
          m_holdmode = HOLDMODE_HELD;
          iNavresult = NAVRESULT_HOLD;
        }
        else
        {
          bool menu = (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav));
          if (menu != m_bInMenu)
          {
            m_bInMenu = menu;
          }
          iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_VTS_CHANGE);
        }
      }
      break;

    case DVDNAV_CELL_CHANGE:
      {
        // Some status information like the current Title and Part numbers do not
        // change inside a cell. Therefore this event can be used to query such
        // information only when necessary and update the decoding/displaying
        // accordingly.

        uint32_t pos = 0;
        uint32_t len = 0;

        m_dll.dvdnav_current_title_info(m_dvdnav, &m_iTitle, &m_iPart);
        m_dll.dvdnav_get_number_of_titles(m_dvdnav, &m_iTitleCount);
        if(m_iTitle > 0)
          m_dll.dvdnav_get_number_of_parts(m_dvdnav, m_iTitle, &m_iPartCount);
        else
          m_iPartCount = 0;
        m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);

        // get chapters' timestamps if we have not cached them yet
        if (m_mapTitleChapters.find(m_iTitle) == m_mapTitleChapters.end())
        {
          uint64_t* times = NULL;
          uint64_t duration;
          //dvdnav_describe_title_chapters returns 0 on failure and NULL for times
          int entries = m_dll.dvdnav_describe_title_chapters(m_dvdnav, m_iTitle, &times, &duration);

          if (entries != m_iPartCount)
            CLog::Log(LOGDEBUG,
                      "{} - Number of chapters/positions differ: Chapters {}, positions {}",
                      __FUNCTION__, m_iPartCount, entries);

          if (times)
          {
            // the times array stores the end timestamps of the chapters, e.g., times[0] stores the position/beginning of chapter 2
            m_mapTitleChapters[m_iTitle][1] = 0;
            for (int i = 0; i < entries - 1; ++i)
            {
              m_mapTitleChapters[m_iTitle][i + 2] = times[i] / 90000;
            }
            free(times);
          }
        }
        CLog::Log(LOGDEBUG, "{} - Cell change: Title {}, Chapter {}", __FUNCTION__, m_iTitle,
                  m_iPart);
        CLog::Log(LOGDEBUG, "{} - At position {:.0f}% inside the feature", __FUNCTION__,
                  100 * (double)pos / (double)len);
        //Get total segment time

        dvdnav_cell_change_event_t* cell_change_event = reinterpret_cast<dvdnav_cell_change_event_t*>(buf);
        m_iCellStart = cell_change_event->cell_start; // store cell time as we need that for time later
        m_iTime      = (int) (m_iCellStart / 90);
        m_iTotalTime = (int) (cell_change_event->pgc_length / 90);

        iNavresult = m_pVideoPlayer->OnDiscNavResult(buf, DVDNAV_CELL_CHANGE);
      }
      break;

    case DVDNAV_NAV_PACKET:
      {
        // A NAV packet provides PTS discontinuity information, angle linking information and
        // button definitions for DVD menus. Angles are handled completely inside libdvdnav.
        // For the menus to work, the NAV packet information has to be passed to the overlay
        // engine of the player so that it knows the dimensions of the button areas.

        // Applications with fifos should not use these functions to retrieve NAV packets,
        // they should implement their own NAV handling, because the packet you get from these
        // functions will already be ahead in the stream which can cause state inconsistencies.
        // Applications with fifos should therefore pass the NAV packet through the fifo
        // and decoding pipeline just like any other data.

        // Calculate current time
        //unsigned int pos, len;
        //m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
        //m_iTime = (int)(((int64_t)m_iTotalTime * pos) / len);

        pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
        m_dll.dvdnav_get_current_nav_dsi(m_dvdnav);

        if(!pci)
        {
          iNavresult = NAVRESULT_NOP;
          break;
        }

        /* if we have any buttons or are not in vts domain we assume we are in menu */
        bool menu = pci->hli.hl_gi.hli_ss || (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav));
        if (menu != m_bInMenu)
        {
          m_bInMenu = menu;
        }

        /* check for any gap in the stream, this is likely a discontinuity */
        int64_t gap = (int64_t)pci->pci_gi.vobu_s_ptm - m_iVobUnitStop;
        if(gap)
        {
          /* make sure demuxer is flushed before we change any correction */
          if(m_holdmode == HOLDMODE_NONE)
          {
            CLog::Log(LOGDEBUG, "DVDNAV_NAV_PACKET (HOLDING)");
            m_holdmode = HOLDMODE_HELD;
            iNavresult = NAVRESULT_HOLD;
            break;
          }
          m_iVobUnitCorrection += gap;

          CLog::Log(LOGDEBUG, "DVDNAV_NAV_PACKET - DISCONTINUITY FROM:{} TO:{} DIFF:{}",
                    (m_iVobUnitStop * 1000) / 90, ((int64_t)pci->pci_gi.vobu_s_ptm * 1000) / 90,
                    (gap * 1000) / 90);
        }

        m_iVobUnitStart = pci->pci_gi.vobu_s_ptm;
        m_iVobUnitStop = pci->pci_gi.vobu_e_ptm;

        m_iTime = (int) ( m_dll.dvdnav_get_current_time(m_dvdnav)  / 90 );

        iNavresult = m_pVideoPlayer->OnDiscNavResult((void*)pci, DVDNAV_NAV_PACKET);
      }
      break;

    case DVDNAV_HOP_CHANNEL:
      // This event is issued whenever a non-seamless operation has been executed.
      // Applications with fifos should drop the fifos content to speed up responsiveness.
      {
        iNavresult = m_pVideoPlayer->OnDiscNavResult(NULL, DVDNAV_HOP_CHANNEL);
      }
      break;

    case DVDNAV_STOP:
      {
        // Playback should end here.

        // don't read any further, it could be libdvdnav had some problems reading
        // the disc. reading further results in a crash
        m_bEOF = true;

        m_pVideoPlayer->OnDiscNavResult(NULL, DVDNAV_STOP);
        iNavresult = NAVRESULT_ERROR;
      }
      break;

    default:
      {
        CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator: Unknown event ({})", m_lastevent);
      }
      break;

    }

    // check if libdvdnav gave us some other buffer to work with
    // probably not needed since function will check if buf
    // is part of the internal cache, but do it for good measure
    if( buf != m_lastblock )
      m_dll.dvdnav_free_cache_block(m_dvdnav, buf);

  return iNavresult;
}

bool CDVDInputStreamNavigator::SetActiveAudioStream(int iId)
{
  CLog::Log(LOGDEBUG, "Setting active audio stream id: {}", iId);

  if (!m_dvdnav)
    return false;

  dvdnav_status_t ret = m_dll.dvdnav_set_active_stream(m_dvdnav, iId, DVD_AUDIO_STREAM);
  if (ret == DVDNAV_STATUS_ERR)
  {
    CLog::LogF(LOGERROR, "dvdnav_set_active_stream (audio) failed: {}",
               m_dll.dvdnav_err_to_string(m_dvdnav));
  }

  return ret == DVDNAV_STATUS_OK;
}

bool CDVDInputStreamNavigator::SetActiveSubtitleStream(int iId)
{
  CLog::LogF(LOGDEBUG, "Setting active subtitle stream id: {}", iId);

  if (!m_dvdnav)
    return false;

  dvdnav_status_t ret = m_dll.dvdnav_set_active_stream(m_dvdnav, iId, DVD_SUBTITLE_STREAM);
  if (ret == DVDNAV_STATUS_ERR)
  {
    CLog::LogF(LOGERROR, "dvdnav_set_active_stream (subtitles) failed: {}",
               m_dll.dvdnav_err_to_string(m_dvdnav));
  }

  return ret == DVDNAV_STATUS_OK;
}

void CDVDInputStreamNavigator::ActivateButton()
{
  if (m_dvdnav)
  {
    m_dll.dvdnav_button_activate(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
  }
}

void CDVDInputStreamNavigator::SelectButton(int iButton)
{
  if (!m_dvdnav) return;
  m_dll.dvdnav_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav), iButton);
}

int CDVDInputStreamNavigator::GetCurrentButton()
{
  if (!m_dvdnav)
  {
    return -1;
  }

  int button = 0;
  if (m_dll.dvdnav_get_current_highlight(m_dvdnav, &button) == DVDNAV_STATUS_ERR)
  {
    CLog::LogF(LOGERROR, "dvdnav_get_current_highlight failed: {}",
               m_dll.dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  return button;
}

void CDVDInputStreamNavigator::CheckButtons()
{
  if (m_dvdnav && m_bCheckButtons)
  {
    m_bCheckButtons = false;
    pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
    int iCurrentButton = GetCurrentButton();

    if( iCurrentButton > 0 && iCurrentButton < 37 )
    {
      btni_t* button = &(pci->hli.btnit[iCurrentButton-1]);

      // menu buttons are always cropped overlays, so if there is no such information
      // we assume the button is invalid
      if ((button->x_start || button->x_end || button->y_start || button->y_end))
      {
        // button has info, it's valid
        return;
      }
    }

    // select first valid button.
    for (int i = 0; i < 36; i++)
    {
      if (pci->hli.btnit[i].x_start ||
          pci->hli.btnit[i].x_end ||
          pci->hli.btnit[i].y_start ||
          pci->hli.btnit[i].y_end)
      {
        CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: found invalid button({})", iCurrentButton);
        CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: switching to button({}) instead", i + 1);
        m_dll.dvdnav_button_select(m_dvdnav, pci, i + 1);
        break;
      }
    }
  }
}

int CDVDInputStreamNavigator::GetTotalButtons()
{
  if (!m_dvdnav) return 0;

  pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);

  int counter = 0;
  for (const btni_t& buttonInfo : pci->hli.btnit)
  {
    if (buttonInfo.x_start ||
        buttonInfo.x_end ||
        buttonInfo.y_start ||
        buttonInfo.y_end)
    {
      counter++;
    }
  }
  return counter;
}

void CDVDInputStreamNavigator::OnUp()
{
  if (m_dvdnav) m_dll.dvdnav_upper_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnDown()
{
  if (m_dvdnav) m_dll.dvdnav_lower_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnLeft()
{
  if (m_dvdnav) m_dll.dvdnav_left_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnRight()
{
  if (m_dvdnav) m_dll.dvdnav_right_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

bool CDVDInputStreamNavigator::OnMouseMove(const CPoint &point)
{
  if (m_dvdnav)
  {
    pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_mouse_select(m_dvdnav, pci, (int32_t)point.x, (int32_t)point.y));
  }
  return false;
}

bool CDVDInputStreamNavigator::OnMouseClick(const CPoint &point)
{
  if (m_dvdnav)
  {
    pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_mouse_activate(m_dvdnav, pci, (int32_t)point.x, (int32_t)point.y));
  }
  return false;
}

bool CDVDInputStreamNavigator::OnMenu()
{
  if (!m_dvdnav)
  {
    return false;
  }

  return m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Escape) == DVDNAV_STATUS_OK;
}

void CDVDInputStreamNavigator::OnBack()
{
  if (m_dvdnav) m_dll.dvdnav_go_up(m_dvdnav);
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnNext()
{
  if (m_dvdnav && !(IsInMenu() && GetTotalButtons() > 0))
  {
    m_dll.dvdnav_next_pg_search(m_dvdnav);
  }
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnPrevious()
{
  if (m_dvdnav && !(IsInMenu() && GetTotalButtons() > 0))
  {
    m_dll.dvdnav_prev_pg_search(m_dvdnav);
  }
}

void CDVDInputStreamNavigator::SkipStill()
{
  if (!m_dvdnav)
    return ;
  m_dll.dvdnav_still_skip(m_dvdnav);
}

void CDVDInputStreamNavigator::SkipWait()
{
  if (!m_dvdnav) return ;
  m_dll.dvdnav_wait_skip(m_dvdnav);
}

CDVDInputStream::ENextStream CDVDInputStreamNavigator::NextStream()
{
  if(m_holdmode == HOLDMODE_HELD)
    m_holdmode = HOLDMODE_SKIP;

  if(m_bEOF)
    return NEXTSTREAM_NONE;
  else if(m_lastevent == DVDNAV_VTS_CHANGE)
    return NEXTSTREAM_OPEN;
  else
    return NEXTSTREAM_RETRY;
}

int CDVDInputStreamNavigator::GetActiveSubtitleStream()
{
  int activeStream = 0;

  if (!m_dvdnav)
  {
    return activeStream;
  }

  const int8_t logicalSubStreamId = m_dll.dvdnav_get_active_spu_stream(m_dvdnav);
  if (logicalSubStreamId < 0)
  {
    return activeStream;
  }

  int subStreamCount = GetSubTitleStreamCount();
  for (int subpN = 0; subpN < subStreamCount; subpN++)
  {
    if (m_dll.dvdnav_get_spu_logical_stream(m_dvdnav, subpN) == logicalSubStreamId)
    {
      activeStream = subpN;
      break;
    }
  }

  return activeStream;
}

SubtitleStreamInfo CDVDInputStreamNavigator::GetSubtitleStreamInfo(const int iId)
{
  SubtitleStreamInfo info;
  if (!m_dvdnav)
    return info;

  subp_attr_t subp_attributes;

  if (m_dll.dvdnav_get_spu_attr(m_dvdnav, iId, &subp_attributes) == DVDNAV_STATUS_OK)
  {
    SetSubtitleStreamName(info, subp_attributes);

    char lang[3];
    lang[2] = 0;
    lang[1] = (subp_attributes.lang_code & 255);
    lang[0] = (subp_attributes.lang_code >> 8) & 255;

    info.language = g_LangCodeExpander.ConvertToISO6392B(lang);
  }

  return info;
}

void CDVDInputStreamNavigator::SetSubtitleStreamName(SubtitleStreamInfo &info, const subp_attr_t &subp_attributes)
{
  if (subp_attributes.type == DVD_SUBPICTURE_TYPE_LANGUAGE ||
      subp_attributes.type == DVD_SUBPICTURE_TYPE_NOTSPECIFIED)
  {
    switch (subp_attributes.code_extension)
    {
      case DVD_SUBPICTURE_LANG_EXT_NOTSPECIFIED:
      case DVD_SUBPICTURE_LANG_EXT_CHILDRENSCAPTIONS:
        break;

      case DVD_SUBPICTURE_LANG_EXT_NORMALCAPTIONS:
      case DVD_SUBPICTURE_LANG_EXT_NORMALCC:
      case DVD_SUBPICTURE_LANG_EXT_BIGCAPTIONS:
      case DVD_SUBPICTURE_LANG_EXT_BIGCC:
      case DVD_SUBPICTURE_LANG_EXT_CHILDRENSCC:
        info.flags = StreamFlags::FLAG_HEARING_IMPAIRED;
        break;
      case DVD_SUBPICTURE_LANG_EXT_FORCED:
        info.flags = StreamFlags::FLAG_FORCED;
        break;
      case DVD_SUBPICTURE_LANG_EXT_NORMALDIRECTORSCOMMENTS:
      case DVD_SUBPICTURE_LANG_EXT_BIGDIRECTORSCOMMENTS:
      case DVD_SUBPICTURE_LANG_EXT_CHILDRENDIRECTORSCOMMENTS:
        info.name = g_localizeStrings.Get(37001);
        break;
      default:
        break;
    }
  }
}

int CDVDInputStreamNavigator::GetSubTitleStreamCount()
{
  if (!m_dvdnav)
  {
    return 0;
  }
  return m_dll.dvdnav_get_number_of_streams(m_dvdnav, DVD_SUBTITLE_STREAM);
}

int CDVDInputStreamNavigator::GetActiveAudioStream()
{
  if (!m_dvdnav)
  {
    return -1;
  }

  const int8_t logicalAudioStreamId = m_dll.dvdnav_get_active_audio_stream(m_dvdnav);
  if (logicalAudioStreamId < 0)
  {
    return -1;
  }

  int activeStream = -1;
  int audioStreamCount = GetAudioStreamCount();
  for (int audioN = 0; audioN < audioStreamCount; audioN++)
  {
    if (m_dll.dvdnav_get_audio_logical_stream(m_dvdnav, audioN) == logicalAudioStreamId)
    {
      activeStream = audioN;
      break;
    }
  }

  return activeStream;
}

void CDVDInputStreamNavigator::SetAudioStreamName(AudioStreamInfo &info, const audio_attr_t &audio_attributes)
{
  switch( audio_attributes.code_extension )
  {
    case DVD_AUDIO_LANG_EXT_VISUALLYIMPAIRED:
      info.name = g_localizeStrings.Get(37000);
      info.flags = StreamFlags::FLAG_VISUAL_IMPAIRED;
      break;
    case DVD_AUDIO_LANG_EXT_DIRECTORSCOMMENTS1:
      info.name = g_localizeStrings.Get(37001);
      break;
    case DVD_AUDIO_LANG_EXT_DIRECTORSCOMMENTS2:
      info.name = g_localizeStrings.Get(37002);
      break;
    case DVD_AUDIO_LANG_EXT_NOTSPECIFIED:
    case DVD_AUDIO_LANG_EXT_NORMALCAPTIONS:
    default:
      break;
  }

  switch(audio_attributes.audio_format)
  {
  case DVD_AUDIO_FORMAT_AC3:
    info.name += " AC3";
    info.codecName = "ac3";
    break;
  case DVD_AUDIO_FORMAT_UNKNOWN_1:
    info.name += " UNKNOWN #1";
    break;
  case DVD_AUDIO_FORMAT_MPEG:
    info.name += " MPEG AUDIO";
    info.codecName = "mp1";
    break;
  case DVD_AUDIO_FORMAT_MPEG2_EXT:
    info.name += " MP2 Ext.";
    info.codecName = "mp2";
    break;
  case DVD_AUDIO_FORMAT_LPCM:
    info.name += " LPCM";
    info.codecName = "pcm";
    break;
  case DVD_AUDIO_FORMAT_UNKNOWN_5:
    info.name += " UNKNOWN #5";
    break;
  case DVD_AUDIO_FORMAT_DTS:
    info.name += " DTS";
    info.codecName = "dts";
    break;
  case DVD_AUDIO_FORMAT_SDDS:
    info.name += " SDDS";
    break;
  default:
    info.name += " Other";
    break;
  }

  switch(audio_attributes.channels + 1)
  {
  case 1:
    info.name += " Mono";
    break;
  case 2:
    info.name += " Stereo";
    break;
  case 6:
    info.name += " 5.1";
    break;
  case 7:
    info.name += " 6.1";
    break;
  default:
    char temp[32];
    snprintf(temp, sizeof(temp), " %d-chs", audio_attributes.channels + 1);
    info.name += temp;
  }

  StringUtils::TrimLeft(info.name);
}

AudioStreamInfo CDVDInputStreamNavigator::GetAudioStreamInfo(const int iId)
{
  AudioStreamInfo info;
  if (!m_dvdnav)
    return info;

  audio_attr_t audio_attributes;

  if (m_dll.dvdnav_get_audio_attr(m_dvdnav, iId, &audio_attributes) == DVDNAV_STATUS_OK)
  {
    SetAudioStreamName(info, audio_attributes);

    char lang[3];
    lang[2] = 0;
    lang[1] = (audio_attributes.lang_code & 255);
    lang[0] = (audio_attributes.lang_code >> 8) & 255;

    info.language = g_LangCodeExpander.ConvertToISO6392B(lang);
    info.channels = audio_attributes.channels + 1;
  }

  return info;
}

int CDVDInputStreamNavigator::GetAudioStreamCount()
{
  if (!m_dvdnav)
  {
    return 0;
  }
  return m_dll.dvdnav_get_number_of_streams(m_dvdnav, DVD_AUDIO_STREAM);
}

int CDVDInputStreamNavigator::GetAngleCount()
{
  if (!m_dvdnav)
    return -1;

  int number_of_angles;
  int current_angle;
  dvdnav_status_t status = m_dll.dvdnav_get_angle_info(m_dvdnav, &current_angle, &number_of_angles);

  if (status == DVDNAV_STATUS_OK)
    return number_of_angles;
  else
    return -1;
}

int CDVDInputStreamNavigator::GetActiveAngle()
{
  if (!m_dvdnav)
    return -1;

  int number_of_angles;
  int current_angle;
  if (m_dll.dvdnav_get_angle_info(m_dvdnav, &current_angle, &number_of_angles) == DVDNAV_STATUS_ERR)
  {
    CLog::LogF(LOGERROR, "Failed to get current angle: {}", m_dll.dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  return current_angle;
}

bool CDVDInputStreamNavigator::SetAngle(int angle)
{
  if (!m_dvdnav)
    return false;

  dvdnav_status_t status = m_dll.dvdnav_angle_change(m_dvdnav, angle);

  return (status == DVDNAV_STATUS_OK);
}

bool CDVDInputStreamNavigator::GetCurrentButtonInfo(CDVDOverlaySpu& pOverlayPicture,
                                                    CDVDDemuxSPU* pSPU,
                                                    int iButtonType)
{
  int colorAndAlpha[4][4];
  dvdnav_highlight_area_t hl;

  if (!m_dvdnav)
  {
    return false;
  }

  int button = GetCurrentButton();
  if (button < 0)
  {
    return false;
  }

  if (DVDNAV_STATUS_OK == m_dll.dvdnav_get_highlight_area(
                              m_dll.dvdnav_get_current_nav_pci(m_dvdnav), button, iButtonType, &hl))
  {
    // button cropping information
    pOverlayPicture.crop_i_x_start = hl.sx;
    pOverlayPicture.crop_i_x_end = hl.ex;
    pOverlayPicture.crop_i_y_start = hl.sy;
    pOverlayPicture.crop_i_y_end = hl.ey;
  }

  if (pSPU->m_bHasClut)
  {
    // get color stored in the highlight area palete using the previously stored clut
    for (unsigned i = 0; i < 4; i++)
    {
      uint8_t* yuvColor = pSPU->m_clut[(hl.palette >> (16 + i * 4)) & 0x0f];
      uint8_t alpha = (((hl.palette >> (i * 4)) & 0x0f));

      colorAndAlpha[i][0] = yuvColor[0];
      colorAndAlpha[i][1] = yuvColor[1];
      colorAndAlpha[i][2] = yuvColor[2];
      colorAndAlpha[i][3] = alpha;
    }

    for (int i = 0; i < 4; i++)
    {
      pOverlayPicture.highlight_alpha[i] = colorAndAlpha[i][3];
      for (int j = 0; j < 3; j++)
        pOverlayPicture.highlight_color[i][j] = colorAndAlpha[i][j];
    }
  }

  return true;
}

int CDVDInputStreamNavigator::GetTotalTime()
{
  //We use buffers of this as they can get called from multiple threads, and could block if we are currently reading data
  return m_iTotalTime;
}

int CDVDInputStreamNavigator::GetTime()
{
  //We use buffers of this as they can get called from multiple threads, and could block if we are currently reading data
  return m_iTime;
}

bool CDVDInputStreamNavigator::PosTime(int iTimeInMsec)
{
  if( m_dll.dvdnav_jump_to_sector_by_time(m_dvdnav, iTimeInMsec * 90, 0) == DVDNAV_STATUS_ERR )
  {
    CLog::Log(LOGDEBUG, "dvdnav: dvdnav_jump_to_sector_by_time failed( {} )",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }
  m_iTime = iTimeInMsec;
  return true;
}

bool CDVDInputStreamNavigator::SeekChapter(int iChapter)
{
  if (!m_dvdnav)
    return false;

  // cannot allow to return true in case of buttons (overlays) because otherwise back in VideoPlayer FlushBuffers will remove menu overlays
  // therefore we just skip the request in case there are buttons and return false
  if (IsInMenu() && GetTotalButtons() > 0)
  {
    CLog::Log(LOGDEBUG, "{} - Seeking chapter is not allowed in menu set with buttons",
              __FUNCTION__);
    return false;
  }

  bool enabled = IsSubtitleStreamEnabled();
  int audio    = GetActiveAudioStream();
  int subtitle = GetActiveSubtitleStream();

  if (iChapter == (m_iPart + 1))
  {
    if (m_dll.dvdnav_next_pg_search(m_dvdnav) == DVDNAV_STATUS_ERR)
    {
      CLog::Log(LOGERROR, "dvdnav: dvdnav_next_pg_search( {} )",
                m_dll.dvdnav_err_to_string(m_dvdnav));
      return false;
    }
  }
  else if (iChapter == (m_iPart - 1))
  {
    if (m_dll.dvdnav_prev_pg_search(m_dvdnav) == DVDNAV_STATUS_ERR)
    {
      CLog::Log(LOGERROR, "dvdnav: dvdnav_prev_pg_search( {} )",
                m_dll.dvdnav_err_to_string(m_dvdnav));
      return false;
    }
  }
  else if (m_dll.dvdnav_part_play(m_dvdnav, m_iTitle, iChapter) == DVDNAV_STATUS_ERR)
  {
    CLog::Log(LOGERROR, "dvdnav: dvdnav_part_play failed( {} )",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  SetActiveSubtitleStream(subtitle);
  SetActiveAudioStream(audio);
  EnableSubtitleStream(enabled);
  return true;
}

float CDVDInputStreamNavigator::GetVideoAspectRatio()
{
  int iAspect = m_dll.dvdnav_get_video_aspect(m_dvdnav);
  int iPerm = m_dll.dvdnav_get_video_scale_permission(m_dvdnav);

  //The video scale permissions should give if the source is letterboxed
  //and such. should be able to give us info that we can zoom in automatically
  //not sure what to do with it currently

  CLog::Log(LOGINFO, "{} - Aspect wanted: {}, Scale permissions: {}", __FUNCTION__, iAspect, iPerm);
  switch(iAspect)
  {
    case 0: //4:3
      return 4.0f / 3.0f;
    case 3: //16:9
      return 16.0f / 9.0f;
    default: //Unknown, use libmpeg2
      return 0.0f;
  }
}

void CDVDInputStreamNavigator::EnableSubtitleStream(bool bEnable)
{
  if (!m_dvdnav)
    return;

  m_dll.dvdnav_toggle_spu_stream(m_dvdnav, static_cast<uint8_t>(bEnable));
}

bool CDVDInputStreamNavigator::IsSubtitleStreamEnabled()
{
  if (!m_dvdnav)
    return false;

  return m_dll.dvdnav_get_active_spu_stream(m_dvdnav) >= 0;
}

bool CDVDInputStreamNavigator::FillDVDState(DVDState& dvdState)
{
  if (!m_dvdnav)
  {
    return false;
  }

  if (m_dll.dvdnav_current_title_program(m_dvdnav, &dvdState.title, &dvdState.pgcn,
                                         &dvdState.pgn) == DVDNAV_STATUS_ERR)
  {
    CLog::LogF(LOGERROR, "Failed to get current title info ({})",
               m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  int current_angle = GetActiveAngle();
  if (current_angle == -1)
  {
    CLog::LogF(LOGERROR, "Could not detect current angle, ignoring saved state");
    return false;
  }
  dvdState.current_angle = current_angle;
  dvdState.audio_num = GetActiveAudioStream();
  dvdState.subp_num = GetActiveSubtitleStream();
  dvdState.sub_enabled = IsSubtitleStreamEnabled();

  return true;
}

bool CDVDInputStreamNavigator::GetState(std::string& xmlstate)
{
  if( !m_dvdnav )
  {
    return false;
  }

  // do not save state if we are not playing a title stream (e.g. if we are in menus)
  if (!m_dll.dvdnav_is_domain_vts(m_dvdnav))
  {
    return false;
  }

  DVDState dvdState;
  if (!FillDVDState(dvdState))
  {
    CLog::LogF(LOGWARNING, "Failed to obtain current dvdnav state");
    return false;
  }

  if (!m_dvdStateSerializer.DVDStateToXML(xmlstate, dvdState))
  {
    CLog::Log(LOGWARNING,
              "CDVDInputStreamNavigator::SetNavigatorState - Failed to serialize state");
    return false;
  }

  return true;
}

bool CDVDInputStreamNavigator::SetState(const std::string& xmlstate)
{
  if (!m_dvdnav)
    return false;

  DVDState dvdState;
  if (!m_dvdStateSerializer.XMLToDVDState(dvdState, xmlstate))
  {
    CLog::LogF(LOGWARNING, "Failed to deserialize state");
    return false;
  }

  m_dll.dvdnav_program_play(m_dvdnav, dvdState.title, dvdState.pgcn, dvdState.pgn);
  m_dll.dvdnav_angle_change(m_dvdnav, dvdState.current_angle);
  SetActiveSubtitleStream(dvdState.subp_num);
  SetActiveAudioStream(dvdState.audio_num);
  EnableSubtitleStream(dvdState.sub_enabled);
  return true;
}

std::string CDVDInputStreamNavigator::GetDVDTitleString()
{
  if (!m_dvdnav)
    return "";

  const char* str = NULL;
  if (m_dll.dvdnav_get_title_string(m_dvdnav, &str) == DVDNAV_STATUS_OK)
    return str;
  else
    return "";
}

std::string CDVDInputStreamNavigator::GetDVDSerialString()
{
  if (!m_dvdnav)
    return "";

  const char* str = NULL;
  if (m_dll.dvdnav_get_serial_string(m_dvdnav, &str) == DVDNAV_STATUS_OK)
    return str;
  else
    return "";
}

std::string CDVDInputStreamNavigator::GetDVDVolIdString()
{
  if (!m_dvdnav)
    return "";

  const char* volIdTmp = m_dll.dvdnav_get_volid_string(m_dvdnav);
  if (volIdTmp)
  {
    std::string volId{volIdTmp};
    free(const_cast<char*>(volIdTmp));
    return volId;
  }
  return "";
}

int64_t CDVDInputStreamNavigator::GetChapterPos(int ch)
{
  if (ch == -1 || ch > GetChapterCount())
    ch = GetChapter();

  std::map<int, std::map<int, int64_t>>::iterator title = m_mapTitleChapters.find(m_iTitle);
  if (title != m_mapTitleChapters.end())
  {
    std::map<int, int64_t>::iterator chapter = title->second.find(ch);
    if (chapter != title->second.end())
      return chapter->second;
  }
  return 0;
}

void CDVDInputStreamNavigator::GetVideoResolution(uint32_t* width, uint32_t* height)
{
  if (!m_dvdnav) return;

  // for version <= 5.0.3 this functions returns 0 instead of DVDNAV_STATUS_OK and -1 instead of DVDNAV_STATUS_ERR
  int status = m_dll.dvdnav_get_video_resolution(m_dvdnav, width, height);
  if (status == -1)
  {
    CLog::Log(LOGWARNING,
              "CDVDInputStreamNavigator::GetVideoResolution - Failed to get resolution ({})",
              m_dll.dvdnav_err_to_string(m_dvdnav));
    *width = 0;
    *height = 0;
  }
}

VideoStreamInfo CDVDInputStreamNavigator::GetVideoStreamInfo()
{
  VideoStreamInfo info;
  if (!m_dvdnav)
    return info;

  info.angles = GetAngleCount();
  info.videoAspectRatio = GetVideoAspectRatio();
  uint32_t width = 0;
  uint32_t height = 0;
  GetVideoResolution(&width, &height);

  info.width = static_cast<int>(width);
  info.height = static_cast<int>(height);

  // Until we add get_video_attr or get_video_codec we can't distinguish MPEG-1 (h261)
  // from MPEG-2 (h262). The latter is far more common, so use this.
  info.codecName = "mpeg2";

  return info;
}

int dvd_inputstreamnavigator_cb_seek(void * p_stream, uint64_t i_pos)
{
  CDVDInputStreamFile *lpstream = reinterpret_cast<CDVDInputStreamFile*>(p_stream);
  if (lpstream->Seek(i_pos, SEEK_SET) >= 0)
    return 0;
  else
    return -1;
}

int dvd_inputstreamnavigator_cb_read(void * p_stream, void * buffer, int i_read)
{
  CDVDInputStreamFile *lpstream = reinterpret_cast<CDVDInputStreamFile*>(p_stream);

  int i_ret = 0;
  while (i_ret < i_read)
  {
    int i_r;
    i_r = lpstream->Read(reinterpret_cast<uint8_t *>(buffer) + i_ret, i_read - i_ret);
    if (i_r < 0)
    {
      CLog::Log(LOGERROR,"read error dvd_inputstreamnavigator_cb_read");
      return i_r;
    }
    if (i_r == 0)
      break;

    i_ret += i_r;
  }

  return i_ret;
}

void dvd_logger(void* priv, dvdnav_logger_level_t level, const char* fmt, va_list va)
{
  const std::string message = StringUtils::FormatV(fmt, va);
  auto logLevel = LOGDEBUG;
  switch (level)
  {
    case DVDNAV_LOGGER_LEVEL_INFO:
      logLevel = LOGINFO;
      break;
    case DVDNAV_LOGGER_LEVEL_ERROR:
      logLevel = LOGERROR;
      break;
    case DVDNAV_LOGGER_LEVEL_WARN:
      logLevel = LOGWARNING;
      break;
    case DVDNAV_LOGGER_LEVEL_DEBUG:
      logLevel = LOGDEBUG;
      break;
    default:
      break;
  };
  CLog::Log(logLevel, "Libdvd: {}", message);
}

int dvd_inputstreamnavigator_cb_readv(void * p_stream, void * p_iovec, int i_blocks)
{
  // NOTE/TODO: this vectored read callback somehow doesn't seem to be called by libdvdnav.
  // Therefore, the code below isn't really tested, but inspired from the libc_readv code for Win32 in libdvdcss (device.c:713).
  CDVDInputStreamFile *lpstream = reinterpret_cast<CDVDInputStreamFile*>(p_stream);
  const struct iovec* lpiovec = reinterpret_cast<const struct iovec*>(p_iovec);

  int i_index, i_len, i_total = 0;
  unsigned char *p_base;
  int i_bytes;

  for (i_index = i_blocks; i_index; i_index--, lpiovec++)
  {
    i_len = lpiovec->iov_len;
    p_base = reinterpret_cast<unsigned char*>(lpiovec->iov_base);

    if (i_len <= 0)
      continue;

    i_bytes = lpstream->Read(p_base, i_len);
    if (i_bytes < 0)
      return -1;
    else
      i_total += i_bytes;

    if (i_bytes != i_len)
    {
      /* We reached the end of the file or a signal interrupted
      * the read. Return a partial read. */
      int i_seek = lpstream->Seek(i_total,0);
      if (i_seek < 0)
        return i_seek;

      /* We have to return now so that i_pos isn't clobbered */
      return i_total;
    }
  }
  return i_total;
}
