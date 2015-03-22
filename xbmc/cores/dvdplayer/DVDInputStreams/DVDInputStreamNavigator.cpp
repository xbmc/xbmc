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

#include "DVDInputStreamNavigator.h"
#include "utils/LangCodeExpander.h"
#include "../DVDDemuxSPU.h"
#include "DVDStateSerializer.h"
#include "settings/Settings.h"
#include "LangInfo.h"
#include "utils/log.h"
#include "guilib/Geometry.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#if defined(TARGET_DARWIN)
#include "osx/CocoaInterface.h"
#endif

#define HOLDMODE_NONE 0
#define HOLDMODE_HELD 1 /* set internally when we wish to flush demuxer */
#define HOLDMODE_SKIP 2 /* set by inputstream user, when they wish to skip the held mode */
#define HOLDMODE_DATA 3 /* set after hold mode has been exited, and action that inited it has been executed */

CDVDInputStreamNavigator::CDVDInputStreamNavigator(IDVDPlayer* player) : CDVDInputStream(DVDSTREAM_TYPE_DVD)
{
  m_dvdnav = 0;
  m_pDVDPlayer = player;
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

  memset(m_lastblock, 0, sizeof(m_lastblock));
}

CDVDInputStreamNavigator::~CDVDInputStreamNavigator()
{
  Close();
}

bool CDVDInputStreamNavigator::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, "video/x-dvd-mpeg"))
    return false;

  // load libdvdnav.dll
  if (!m_dll.Load())
    return false;

  // load the dvd language codes
  // g_LangCodeExpander.LoadStandardCodes();

  // libdvdcss fails if the file path contains VIDEO_TS.IFO or VIDEO_TS/VIDEO_TS.IFO
  // libdvdnav is still able to play without, so strip them.

  std::string path = strFile;
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

  // open up the DVD device
  if (m_dll.dvdnav_open(&m_dvdnav, path.c_str()) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR,"Error on dvdnav_open\n");
    Close();
    return false;
  }

  int region = CSettings::Get().GetInt("dvds.playerregion");
  int mask = 0;
  if(region > 0)
    mask = 1 << (region-1);
  else
  {
    // find out what region dvd reports itself to be from, and use that as mask if available
    vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
    if (vm && vm->vmgi && vm->vmgi->vmgi_mat)
      mask = ((vm->vmgi->vmgi_mat->vmg_category >> 16) & 0xff) ^ 0xff;
  }
  if(!mask)
    mask = 0xff;

  CLog::Log(LOGDEBUG, "%s - Setting region mask %02x", __FUNCTION__, mask);
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
    CLog::Log(LOGERROR, "Error on setting default menu language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_menu_language_select(m_dvdnav, (char*)"en");
  }

  if (m_dll.dvdnav_audio_language_select(m_dvdnav, (char*)language_audio) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default audio language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_audio_language_select(m_dvdnav, (char*)"en");
  }

  if (m_dll.dvdnav_spu_language_select(m_dvdnav, (char*)language_subtitle) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default subtitle language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_spu_language_select(m_dvdnav, (char*)"en");
  }

  // set read ahead cache usage
  if (m_dll.dvdnav_set_readahead_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR,"Error on dvdnav_set_readahead_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // set the PGC positioning flag to have position information relatively to the
  // whole feature instead of just relatively to the current chapter
  if (m_dll.dvdnav_set_PGC_positioning_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR,"Error on dvdnav_set_PGC_positioning_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // jump directly to title menu
  if(CSettings::Get().GetBool("dvds.automenu"))
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
      CLog::Log(LOGERROR,"Error on dvdnav_menu_call(Title): %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
      // next try root menu
      if(m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Root) != DVDNAV_STATUS_OK )
        CLog::Log(LOGERROR,"Error on dvdnav_menu_call(Root): %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
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
    CLog::Log(LOGERROR,"Error on dvdnav_close: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    return ;
  }

  CDVDInputStream::Close();
  m_dvdnav = NULL;
  m_bEOF = true;
}

int CDVDInputStreamNavigator::Read(uint8_t* buf, int buf_size)
{
  if (!m_dvdnav || m_bEOF) return 0;
  if (buf_size < DVD_VIDEO_BLOCKSIZE)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamNavigator: buffer size is to small, %d bytes, should be 2048 bytes", buf_size);
    return -1;
  }

  int iBytesRead;

  int NOPcount = 0;
  while(true) {
    int navresult = ProcessBlock(buf, &iBytesRead);
    if (navresult == NAVRESULT_HOLD)       return 0; // return 0 bytes read;
    else if (navresult == NAVRESULT_ERROR) return -1;
    else if (navresult == NAVRESULT_DATA)  return iBytesRead;
    else if (navresult == NAVRESULT_NOP)
    {
      NOPcount++;
      if (NOPcount == 1000) 
      {
        m_bEOF = true;
        CLog::Log(LOGERROR,"CDVDInputStreamNavigator: Stopping playback due to infinite loop, caused by badly authored DVD navigation structure. Try enabling 'Attempt to skip introduction before DVD menu'.");
        m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_STOP);
        return -1; // fail and stop playback.
      }
    }
  }

  return iBytesRead;
}

// not working yet, but it is the recommanded way for seeking
int64_t CDVDInputStreamNavigator::Seek(int64_t offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

int CDVDInputStreamNavigator::ProcessBlock(uint8_t* dest_buffer, int* read)
{
  if (!m_dvdnav) return -1;

  int result;
  int len = 2048;

  // m_tempbuffer will be used for anything that isn't a normal data block
  uint8_t* buf = m_lastblock;
  int iNavresult = -1;

  if(m_holdmode == HOLDMODE_HELD)
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
    CLog::Log(LOGERROR,"Error getting next block: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
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
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_STILL_FRAME);

        /* if user didn't care for action, just skip it */
        if(iNavresult == NAVRESULT_NOP)
          SkipStill();
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
          iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_WAIT);

        /* if user didn't care for action, just skip it */
        if(iNavresult == NAVRESULT_NOP)
          SkipWait();
      }
      break;

    case DVDNAV_SPU_CLUT_CHANGE:
      // Player applications should pass the new colour lookup table to their
      // SPU decoder. The CLUT is given as 16 uint32_t's in the buffer.
      {
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_CLUT_CHANGE);
      }
      break;

    case DVDNAV_SPU_STREAM_CHANGE:
      // Player applications should inform their SPU decoder to switch channels
      {
        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)buf;

        //libdvdnav never sets logical, why.. don't know..
        event->logical = GetActiveSubtitleStream();

        /* correct stream ids for disabled subs if needed */
        if(!IsSubtitleStreamEnabled())
        {
          event->physical_letterbox |= 0x80;
          event->physical_pan_scan |= 0x80;
          event->physical_wide |= 0x80;
        }

        if(event->logical<0 && GetSubTitleStreamCount()>0)
        {
          /* this will not take effect in this event */
          CLog::Log(LOGINFO, "%s - none or invalid subtitle stream selected, defaulting to first", __FUNCTION__);
          SetActiveSubtitleStream(0);
        }
        m_bCheckButtons = true;
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_STREAM_CHANGE);
      }
      break;

    case DVDNAV_AUDIO_STREAM_CHANGE:
      // Player applications should inform their audio decoder to switch channels
      {

        //dvdnav_get_audio_logical_stream actually does the oposite to the docs..
        //taking a audiostream as given on dvd, it gives the physical stream that
        //refers to in the mpeg file

        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)buf;

        //wroong... stupid docs..
        //event->logical = dvdnav_get_audio_logical_stream(m_dvdnav, event->physical);
        //logical should actually be set to the (vm->state).AST_REG

        event->logical = GetActiveAudioStream();
        if(event->logical<0)
        {
          /* this will not take effect in this event */
          CLog::Log(LOGINFO, "%s - none or invalid audio stream selected, defaulting to first", __FUNCTION__);
          SetActiveAudioStream(0);
        }

        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_AUDIO_STREAM_CHANGE);
      }

      break;

    case DVDNAV_HIGHLIGHT:
      {
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_HIGHLIGHT);
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
          m_bInMenu   = (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav));
          iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_VTS_CHANGE);
        }
      }
      break;

    case DVDNAV_CELL_CHANGE:
      {
        // Some status information like the current Title and Part numbers do not
        // change inside a cell. Therefore this event can be used to query such
        // information only when necessary and update the decoding/displaying
        // accordingly.

        // this may lead to a discontinuity, but it's also the end of the
        // vobunit, so make sure everything in demuxer is output
        if(m_holdmode == HOLDMODE_NONE)
        {
          CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE (HOLDING)");
          m_holdmode = HOLDMODE_HELD;
          iNavresult = NAVRESULT_HOLD;
          break;
        }

        uint32_t pos, len;

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
          int entries = static_cast<int>(m_dll.dvdnav_describe_title_chapters(m_dvdnav, m_iTitle, &times, &duration));

          if (entries != m_iPartCount)
            CLog::Log(LOGDEBUG, "%s - Number of chapters/positions differ: Chapters %d, positions %d\n", __FUNCTION__, m_iPartCount, entries);

          if (times)
          {
            // the times array stores the end timestampes of the chapters, e.g., times[0] stores the position/beginning of chapter 2
            m_mapTitleChapters[m_iTitle][1] = 0;
            for (int i = 0; i < entries - 1; ++i)
            {
              m_mapTitleChapters[m_iTitle][i + 2] = times[i] / 90000;
            }
            m_dll.dvdnav_free(times);
          }
        }
        CLog::Log(LOGDEBUG, "%s - Cell change: Title %d, Chapter %d\n", __FUNCTION__, m_iTitle, m_iPart);
        CLog::Log(LOGDEBUG, "%s - At position %.0f%% inside the feature\n", __FUNCTION__, 100 * (double)pos / (double)len);
        //Get total segment time

        dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)buf;
        m_iCellStart = cell_change_event->cell_start; // store cell time as we need that for time later
        m_iTime      = (int) (m_iCellStart / 90);
        m_iTotalTime = (int) (cell_change_event->pgc_length / 90);

        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_CELL_CHANGE);
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

        /* if we have any buttons or are not in vts domain we assume we are in meny */
        m_bInMenu = pci->hli.hl_gi.hli_ss || (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav));

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

          CLog::Log(LOGDEBUG, "DVDNAV_NAV_PACKET - DISCONTINUITY FROM:%" PRId64" TO:%" PRId64" DIFF:%" PRId64, (m_iVobUnitStop * 1000)/90, ((int64_t)pci->pci_gi.vobu_s_ptm*1000)/90, (gap*1000)/90);
        }

        m_iVobUnitStart = pci->pci_gi.vobu_s_ptm;
        m_iVobUnitStop = pci->pci_gi.vobu_e_ptm;

        m_iTime = (int) ( m_dll.dvdnav_convert_time( &(pci->pci_gi.e_eltm) ) + m_iCellStart ) / 90;

        if (m_bCheckButtons)
        {
          CheckButtons();
          m_bCheckButtons = false;
        }

        iNavresult = m_pDVDPlayer->OnDVDNavResult((void*)pci, DVDNAV_NAV_PACKET);
      }
      break;

    case DVDNAV_HOP_CHANNEL:
      // This event is issued whenever a non-seamless operation has been executed.
      // Applications with fifos should drop the fifos content to speed up responsiveness.
      {
        iNavresult = m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_HOP_CHANNEL);
      }
      break;

    case DVDNAV_STOP:
      {
        // Playback should end here.

        // don't read any further, it could be libdvdnav had some problems reading
        // the disc. reading further results in a crash
        m_bEOF = true;

        m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_STOP);
        iNavresult = NAVRESULT_ERROR;
      }
      break;

    default:
      {
        CLog::Log(LOGDEBUG,"CDVDInputStreamNavigator: Unknown event (%i)\n", m_lastevent);
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
  int streamId = ConvertAudioStreamId_XBMCToExternal(iId);
  CLog::Log(LOGDEBUG, "%s - id: %d, stream: %d", __FUNCTION__, iId, streamId);

  if (!m_dvdnav)
    return false;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (!vm)
    return false;
  if (!vm->state.pgc)
    return false;

  /* make sure stream is valid, if not don't allow it */
  if (streamId < 0 || streamId >= 8)
    return false;
  else if ( !(vm->state.pgc->audio_control[streamId] & (1<<15)) )
    return false;

  if (vm->state.domain != VTS_DOMAIN && streamId != 0)
    return false;

  vm->state.AST_REG = streamId;
  return true;
}

bool CDVDInputStreamNavigator::SetActiveSubtitleStream(int iId)
{
  int streamId = ConvertSubtitleStreamId_XBMCToExternal(iId);
  CLog::Log(LOGDEBUG, "%s - id: %d, stream: %d", __FUNCTION__, iId, streamId);

  if (!m_dvdnav)
    return false;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (!vm)
    return false;
  if (!vm->state.pgc)
    return false;

  /* make sure stream is valid, if not don't allow it */
  if (streamId < 0 || streamId >= 32)
    return false;
  else if ( !(vm->state.pgc->subp_control[streamId] & (1<<31)) )
    return false;

  if (vm->state.domain != VTS_DOMAIN && streamId != 0)
    return false;

  /* set subtitle stream without modifying visibility */
  vm->state.SPST_REG = streamId | (vm->state.SPST_REG & 0x40);

  return true;
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
  int button;
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_current_highlight(m_dvdnav, &button);
    return button;
  }
  return -1;
}

void CDVDInputStreamNavigator::CheckButtons()
{
  if (m_dvdnav)
  {

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
        CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: found invalid button(%d)", iCurrentButton);
        CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: switching to button(%d) instead", i + 1);
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
  for (int i = 0; i < 36; i++)
  {
    if (pci->hli.btnit[i].x_start ||
        pci->hli.btnit[i].x_end ||
        pci->hli.btnit[i].y_start ||
        pci->hli.btnit[i].y_end)
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

void CDVDInputStreamNavigator::OnMenu()
{
  if (m_dvdnav) m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Escape);
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
  if (!m_dvdnav) return ;
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

  if (m_dvdnav)
  {
    vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
    if (vm && vm->state.pgc)
    {
      // get the current selected audiostream, for non VTS_DOMAIN it is always stream 0
      int subpN = 0;
      if (vm->state.domain == VTS_DOMAIN)
      {
        subpN = vm->state.SPST_REG & ~0x40;

        /* make sure stream is valid, if not don't allow it */
        if (subpN < 0 || subpN >= 32)
          subpN = -1;
        else if ( !(vm->state.pgc->subp_control[subpN] & (1<<31)) )
          subpN = -1;
      }

      activeStream = ConvertSubtitleStreamId_ExternalToXBMC(subpN);
    }
  }

  return activeStream;
}

bool CDVDInputStreamNavigator::GetSubtitleStreamInfo(const int iId, DVDNavStreamInfo &info)
{
  if (!m_dvdnav) return false;

  int streamId = ConvertSubtitleStreamId_XBMCToExternal(iId);
  subp_attr_t subp_attributes;

  if( m_dll.dvdnav_get_spu_attr(m_dvdnav, streamId, &subp_attributes) == DVDNAV_STATUS_OK )
  {
    SetSubtitleStreamName(info, subp_attributes);

    char lang[3];
    lang[2] = 0;
    lang[1] = (subp_attributes.lang_code & 255);
    lang[0] = (subp_attributes.lang_code >> 8) & 255;

    g_LangCodeExpander.ConvertToISO6392T(lang, info.language);
    return true;
  }
  return false;
}

void CDVDInputStreamNavigator::SetSubtitleStreamName(DVDNavStreamInfo &info, const subp_attr_t &subp_attributes)
{
  if (subp_attributes.type == DVD_SUBPICTURE_TYPE_Language ||
    subp_attributes.type == DVD_SUBPICTURE_TYPE_NotSpecified)
  {
    switch (subp_attributes.lang_extension)
    {
    case DVD_SUBPICTURE_LANG_EXT_NotSpecified:
    case DVD_SUBPICTURE_LANG_EXT_NormalCaptions:
    case DVD_SUBPICTURE_LANG_EXT_BigCaptions:
    case DVD_SUBPICTURE_LANG_EXT_ChildrensCaptions:
      break;

    case DVD_SUBPICTURE_LANG_EXT_NormalCC:
    case DVD_SUBPICTURE_LANG_EXT_BigCC:
    case DVD_SUBPICTURE_LANG_EXT_ChildrensCC:
      info.name += g_localizeStrings.Get(37011);
      break;
    case DVD_SUBPICTURE_LANG_EXT_Forced:
      info.name += g_localizeStrings.Get(37012);
      break;
    case DVD_SUBPICTURE_LANG_EXT_NormalDirectorsComments:
    case DVD_SUBPICTURE_LANG_EXT_BigDirectorsComments:
    case DVD_SUBPICTURE_LANG_EXT_ChildrensDirectorsComments:
      info.name += g_localizeStrings.Get(37013);
      break;
    default:
      break;
    }
  }
}

int CDVDInputStreamNavigator::GetSubTitleStreamCount()
{
  if (!m_dvdnav) return 0;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if (!vm) return 0;
  if (!vm->state.pgc) return 0;

  if (vm->state.domain == VTS_DOMAIN)
  {
    int streamN = 0;
    for (int i = 0; i < 32; i++)
    {
      if (vm->state.pgc->subp_control[i] & (1<<31))
        streamN++;
    }
    return streamN;
  }
  else
  {
    /* just for good measure say that non vts domain always has one */
    return 1;
  }
}

int CDVDInputStreamNavigator::GetActiveAudioStream()
{
  int activeStream = -1;

  if (m_dvdnav)
  {
    vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
    if (vm && vm->state.pgc)
    {
      // get the current selected audiostream, for non VTS_DOMAIN it is always stream 0
      int audioN = 0;
      if (vm->state.domain == VTS_DOMAIN)
      {
        audioN = vm->state.AST_REG;

        /* make sure stream is valid, if not don't allow it */
        if (audioN < 0 || audioN >= 8)
          audioN = -1;
        else if ( !(vm->state.pgc->audio_control[audioN] & (1<<15)) )
          audioN = -1;
      }

      activeStream = ConvertAudioStreamId_ExternalToXBMC(audioN);
    }
  }

  return activeStream;
}

void CDVDInputStreamNavigator::SetAudioStreamName(DVDNavStreamInfo &info, const audio_attr_t &audio_attributes)
{
  switch( audio_attributes.code_extension )
  {
  case DVD_AUDIO_LANG_EXT_VisuallyImpaired:
    info.name = g_localizeStrings.Get(37000);
    break;
  case DVD_AUDIO_LANG_EXT_DirectorsComments1:
    info.name = g_localizeStrings.Get(37001);
    break;
  case DVD_AUDIO_LANG_EXT_DirectorsComments2:
    info.name = g_localizeStrings.Get(37002);
    break;
  case DVD_AUDIO_LANG_EXT_NotSpecified:
  case DVD_AUDIO_LANG_EXT_NormalCaptions:
  default:
    break;
  }

  switch(audio_attributes.audio_format)
  {
  case DVD_AUDIO_FORMAT_AC3:
    info.name += " AC3";
    break;
  case DVD_AUDIO_FORMAT_UNKNOWN_1:
    info.name += " UNKNOWN #1";
    break;
  case DVD_AUDIO_FORMAT_MPEG:
    info.name += " MPEG AUDIO";
    break;
  case DVD_AUDIO_FORMAT_MPEG2_EXT:
    info.name += " MP2 Ext.";
    break;
  case DVD_AUDIO_FORMAT_LPCM:
    info.name += " LPCM";
    break;
  case DVD_AUDIO_FORMAT_UNKNOWN_5:
    info.name += " UNKNOWN #5";
    break;
  case DVD_AUDIO_FORMAT_DTS:
    info.name += " DTS";
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
    sprintf(temp, " %d-chs", audio_attributes.channels + 1);
    info.name += temp;
  }

  StringUtils::TrimLeft(info.name);

}

bool CDVDInputStreamNavigator::GetAudioStreamInfo(const int iId, DVDNavStreamInfo &info)
{
  if (!m_dvdnav) return false;

  int streamId = ConvertAudioStreamId_XBMCToExternal(iId);
  audio_attr_t audio_attributes;

  if( m_dll.dvdnav_get_audio_attr(m_dvdnav, streamId, &audio_attributes) == DVDNAV_STATUS_OK )
  {
    SetAudioStreamName(info, audio_attributes);

    char lang[3];
    lang[2] = 0;
    lang[1] = (audio_attributes.lang_code & 255);
    lang[0] = (audio_attributes.lang_code >> 8) & 255;

    g_LangCodeExpander.ConvertToISO6392T(lang, info.language);

    info.channels = audio_attributes.channels + 1;

    return true;
  }
  return false;
}

int CDVDInputStreamNavigator::GetAudioStreamCount()
{
  if (!m_dvdnav) return 0;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if (!vm) return 0;
  if (!vm->state.pgc) return 0;

  if (vm->state.domain == VTS_DOMAIN)
  {
    int streamN = 0;
    for (int i = 0; i < 8; i++)
    {
      if (vm->state.pgc->audio_control[i] & (1<<15))
        streamN++;
    }
    return streamN;
  }
  else
  {
    /* just for good measure say that non vts domain always has one */
    return 1;
  }
}

bool CDVDInputStreamNavigator::GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType)
{
  int alpha[2][4];
  int color[2][4];
  dvdnav_highlight_area_t hl;

  if (!m_dvdnav) return false;

  int iButton = GetCurrentButton();

  if (m_dll.dvdnav_get_button_info(m_dvdnav, alpha, color) == 0)
  {
    pOverlayPicture->highlight_alpha[0] = alpha[iButtonType][0];
    pOverlayPicture->highlight_alpha[1] = alpha[iButtonType][1];
    pOverlayPicture->highlight_alpha[2] = alpha[iButtonType][2];
    pOverlayPicture->highlight_alpha[3] = alpha[iButtonType][3];

    if (pSPU->m_bHasClut)
    {
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++)
          pOverlayPicture->highlight_color[i][j] = pSPU->m_clut[color[iButtonType][i]][j];
    }
  }

  if (DVDNAV_STATUS_OK == m_dll.dvdnav_get_highlight_area(m_dll.dvdnav_get_current_nav_pci(m_dvdnav), iButton, iButtonType, &hl))
  {
    // button cropping information
    pOverlayPicture->crop_i_x_start = hl.sx;
    pOverlayPicture->crop_i_x_end = hl.ex;
    pOverlayPicture->crop_i_y_start = hl.sy;
    pOverlayPicture->crop_i_y_end = hl.ey;
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

bool CDVDInputStreamNavigator::SeekTime(int iTimeInMsec)
{
  if( m_dll.dvdnav_jump_to_sector_by_time(m_dvdnav, iTimeInMsec * 90, 0) == DVDNAV_STATUS_ERR )
  {
    CLog::Log(LOGDEBUG, "dvdnav: dvdnav_time_search failed( %s )", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }
  return true;
}

bool CDVDInputStreamNavigator::SeekChapter(int iChapter)
{
  if (!m_dvdnav)
    return false;

  // cannot allow to return true in case of buttons (overlays) because otherwise back in DVDPlayer FlushBuffers will remove menu overlays
  // therefore we just skip the request in case there are buttons and return false
  if (IsInMenu() && GetTotalButtons() > 0)
  {
    CLog::Log(LOGDEBUG, "%s - Seeking chapter is not allowed in menu set with buttons", __FUNCTION__);
    return false;
  }

  bool enabled = IsSubtitleStreamEnabled();
  int audio    = GetActiveAudioStream();
  int subtitle = GetActiveSubtitleStream();

  if (iChapter == (m_iPart + 1))
  {
    if (m_dll.dvdnav_next_pg_search(m_dvdnav) == DVDNAV_STATUS_ERR)
    {
      CLog::Log(LOGERROR, "dvdnav: dvdnav_next_pg_search( %s )", m_dll.dvdnav_err_to_string(m_dvdnav));
      return false;
    }
  }
  else
  if (iChapter == (m_iPart - 1))
  {
    if (m_dll.dvdnav_prev_pg_search(m_dvdnav) == DVDNAV_STATUS_ERR)
    {
      CLog::Log(LOGERROR, "dvdnav: dvdnav_prev_pg_search( %s )", m_dll.dvdnav_err_to_string(m_dvdnav));
      return false;
    }
  }
  else  
  if (m_dll.dvdnav_part_play(m_dvdnav, m_iTitle, iChapter) == DVDNAV_STATUS_ERR)
  {
    CLog::Log(LOGERROR, "dvdnav: dvdnav_part_play failed( %s )", m_dll.dvdnav_err_to_string(m_dvdnav));
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

  CLog::Log(LOGINFO, "%s - Aspect wanted: %d, Scale permissions: %d", __FUNCTION__, iAspect, iPerm);
  switch(iAspect)
  {
    case 2: //4:3
      return 4.0f / 3.0f;
    case 3: //16:9
      return 16.0f / 9.0f;
    case 4:
      return 2.11f / 1.0f;
    default: //Unknown, use libmpeg2
      return 0.0f;
  }
}

void CDVDInputStreamNavigator::EnableSubtitleStream(bool bEnable)
{
  if (!m_dvdnav)
    return;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (!vm)
    return;

  if(bEnable)
    vm->state.SPST_REG |= 0x40;
  else
    vm->state.SPST_REG &= ~0x40;
}

bool CDVDInputStreamNavigator::IsSubtitleStreamEnabled()
{
  if (!m_dvdnav)
    return false;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (!vm)
    return false;


  if(vm->state.SPST_REG & 0x40)
    return true;
  else
    return false;
}

bool CDVDInputStreamNavigator::GetState(std::string &xmlstate)
{
  if( !m_dvdnav )
    return false;

  dvd_state_t save_state;
  if( DVDNAV_STATUS_ERR == m_dll.dvdnav_get_state(m_dvdnav, &save_state) )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::GetNavigatorState - Failed to get state (%s)", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  if( !CDVDStateSerializer::DVDToXMLState(xmlstate, &save_state) )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to serialize state");
    return false;
  }

  return true;
}

bool CDVDInputStreamNavigator::SetState(const std::string &xmlstate)
{
  if( !m_dvdnav )
    return false;

  dvd_state_t save_state;
  memset( &save_state, 0, sizeof( save_state ) );

  if( !CDVDStateSerializer::XMLToDVDState(&save_state, xmlstate)  )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to deserialize state");
    return false;
  }

  if( DVDNAV_STATUS_ERR == m_dll.dvdnav_set_state(m_dvdnav, &save_state) )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to set state (%s), retrying after read", m_dll.dvdnav_err_to_string(m_dvdnav));

    /* vm won't be started until after first read, this should really be handled internally */
    uint8_t buffer[DVD_VIDEO_BLOCKSIZE];
    Read(buffer,DVD_VIDEO_BLOCKSIZE);

    if( DVDNAV_STATUS_ERR == m_dll.dvdnav_set_state(m_dvdnav, &save_state) )
    {
      CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to set state (%s)", m_dll.dvdnav_err_to_string(m_dvdnav));
      return false;
    }
  }
  return true;
}

int CDVDInputStreamNavigator::ConvertAudioStreamId_XBMCToExternal(int id)
{
  if (!m_dvdnav)
    return -1;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if(!vm)
    return -1;

  if (vm->state.domain == VTS_DOMAIN)
  {
    if(!vm->state.pgc)
      return -1;

    int stream = -1;
    for (int i = 0; i < 8; i++)
    {
      if (vm->state.pgc->audio_control[i] & (1<<15)) stream++;
      if (stream == id) return i;
    }
  }
  else if(id == 0)
    return 0;

  return -1;
}

int CDVDInputStreamNavigator::ConvertAudioStreamId_ExternalToXBMC(int id)
{
  if  (!m_dvdnav) return -1;
  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if (!vm) return -1;
  if (!vm->state.pgc) return -1;
  if (id < 0) return -1;

  if( vm->state.domain == VTS_DOMAIN )
  {
    /* VTS domain can only have limited number of streams */
    if (id >= 8)
    {
      CLog::Log(LOGWARNING, "%s - incorrect id : %d", __FUNCTION__, id);
      return -1;
    }

    /* make sure this is a valid id, otherwise the count below will be very wrong */
    if ((vm->state.pgc->audio_control[id] & (1<<15)))
    {
      int stream = -1;
      for (int i = 0; i <= id; i++)
      {
        if (vm->state.pgc->audio_control[i] & (1<<15)) stream++;
      }
      return stream;
    }
    else
    {
      CLog::Log(LOGWARNING, "%s - non existing id %d", __FUNCTION__, id);
      return -1;
    }
  }
  else
  {
    if( id != 0 )
      CLog::Log(LOGWARNING, "%s - non vts domain can't have id %d", __FUNCTION__, id);

    // non VTS_DOMAIN, only one stream is available
    return 0;
  }
}

int CDVDInputStreamNavigator::ConvertSubtitleStreamId_XBMCToExternal(int id)
{
  if (!m_dvdnav)
    return -1;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if(!vm)
    return -1;

  if (vm->state.domain == VTS_DOMAIN)
  {
    if(!vm->state.pgc)
      return -1;

    int stream = -1;
    for (int i = 0; i < 32; i++)
    {
      if (vm->state.pgc->subp_control[i] & (1<<31)) stream++;
      if (stream == id) return i;
    }
  }
  else if(id == 0)
    return 0;

  return -1;
}

int CDVDInputStreamNavigator::ConvertSubtitleStreamId_ExternalToXBMC(int id)
{
  if  (!m_dvdnav) return -1;
  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if (!vm) return -1;
  if (!vm->state.pgc) return -1;
  if (id < 0) return -1;

  if( vm->state.domain == VTS_DOMAIN )
  {
    /* VTS domain can only have limited number of streams */
    if (id >= 32)
    {
      CLog::Log(LOGWARNING, "%s - incorrect id : %d", __FUNCTION__, id);
      return -1;
    }

    /* make sure this is a valid id, otherwise the count below will be very wrong */
    if ((vm->state.pgc->subp_control[id] & (1<<31)))
    {
      int stream = -1;
      for (int i = 0; i <= id; i++)
      {
        if (vm->state.pgc->subp_control[i] & (1<<31)) stream++;
      }
      return stream;
    }
    else
    {
      CLog::Log(LOGWARNING, "%s - non existing id %d", __FUNCTION__, id);
      return -1;
    }
  }
  else
  {
    if( id != 0 )
      CLog::Log(LOGWARNING, "%s - non vts domain can't have id %d", __FUNCTION__, id);

    // non VTS_DOMAIN, only one stream is available
    return 0;
  }
}

bool CDVDInputStreamNavigator::GetDVDTitleString(std::string& titleStr)
{
  if (!m_dvdnav) return false;
  const char* str = NULL;
  m_dll.dvdnav_get_title_string(m_dvdnav, &str);
  titleStr.assign(str);
  return true;
}

bool CDVDInputStreamNavigator::GetDVDSerialString(std::string& serialStr)
{
  if (!m_dvdnav) return false;
  const char* str = NULL;
  m_dll.dvdnav_get_serial_string(m_dvdnav, &str);
  serialStr.assign(str);
  return true;
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
