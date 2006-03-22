
#include "../../../stdafx.h"
#include "DVDInputStreamNavigator.h"
#include "..\..\..\util.h"
#include "..\..\..\LangCodeExpander.h"
#include "..\DVDDemuxSPU.h"
#include "..\DVDOverlay.h"
#include "DVDStateSerializer.h"

CDVDInputStreamNavigator::CDVDInputStreamNavigator(IDVDPlayer* player) : CDVDInputStream()
{
  m_streamType = DVDSTREAM_TYPE_DVD;
  m_dvdnav = 0;
  m_pDVDPlayer = player;
  InitializeCriticalSection(&m_critSection);
  m_bCheckButtons = false;
  m_iCellStart = 0;
}

CDVDInputStreamNavigator::~CDVDInputStreamNavigator()
{
  Close();
  DeleteCriticalSection(&m_critSection);
}

bool CDVDInputStreamNavigator::Open(const char* strFile)
{
  char* strDVDFile;
  
  if (!CDVDInputStream::Open(strFile)) return false;
  
  // load libdvdnav.dll
  if (!m_dll.Load())
    return false;
  
  // load the dvd language codes
  // g_LangCodeExpander.LoadStandardCodes();

  // since libdvdnav automaticly play's the dvd if the directory contains VIDEO_TS.IFO
  // we strip it here.
  strDVDFile = strdup(strFile);
  if (strnicmp(strDVDFile + strlen(strDVDFile) - 12, "VIDEO_TS.IFO", 12) == 0)
  {
    strDVDFile[strlen(strDVDFile) - 13] = '\0';
  }

  // open up the DVD device
  if (m_dll.dvdnav_open(&m_dvdnav, strDVDFile) != DVDNAV_STATUS_OK)
  {
    free(strDVDFile);

    CLog::DebugLog("Error on dvdnav_open\n");
    Close();
    return false;
  }
  free(strDVDFile);

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (vm && vm->vmgi && vm->vmgi->vmgi_mat)
  {
    // find out what region dvd reports itself to be from, and use that as mask if available
    int mask = ~(vm->vmgi->vmgi_mat->vmg_category >> 16);
    if( mask != 0 )
      m_dll.dvdnav_set_region_mask(m_dvdnav, mask);
    else
      m_dll.dvdnav_set_region_mask(m_dvdnav, 0xff);
  }
  else
  {
    // set region flag (0xff = all regions ?)
    m_dll.dvdnav_set_region_mask(m_dvdnav, 0xff);
  }

  // get default language settings
  const char* language_menu     = g_langInfo.GetDVDMenuLanguage().c_str();
  const char* language_audio    = g_langInfo.GetDVDAudioLanguage().c_str();
  const char* language_subtitle = g_langInfo.GetDVDSubtitleLanguage().c_str();
  
  // set language settings in case they are not set in xbmc's configuration
  if (!language_menu || language_menu[0] == '\0') language_menu = "en";
  if (!language_audio || language_audio[0] == '\0') language_audio = "en";
  if (!language_subtitle || language_subtitle[0] == '\0') language_subtitle = "en";
  
  // set default language settings
  if (m_dll.dvdnav_menu_language_select(m_dvdnav, (char*)language_menu) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default menu language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_menu_language_select(m_dvdnav, "en");
  }

  if (m_dll.dvdnav_audio_language_select(m_dvdnav, (char*)language_audio) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default audio language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_audio_language_select(m_dvdnav, "en");
  }
  
  if (m_dll.dvdnav_spu_language_select(m_dvdnav, (char*)language_subtitle) != DVDNAV_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Error on setting default subtitle language: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    CLog::Log(LOGERROR, "Defaulting to \"en\"");
    m_dll.dvdnav_spu_language_select(m_dvdnav, "en");
  }

  // set read ahead cache usage
  if (m_dll.dvdnav_set_readahead_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_readahead_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // set the PGC positioning flag to have position information relatively to the
  // whole feature instead of just relatively to the current chapter
  if (m_dll.dvdnav_set_PGC_positioning_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_PGC_positioning_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  m_bEOF = false;
  
  return true;
}

void CDVDInputStreamNavigator::Close()
{
  if (!m_dvdnav) return;

  // finish off by closing the dvdnav device
  if (m_dll.dvdnav_close(m_dvdnav) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_close: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    return ;
  }

  CDVDInputStream::Close();
  m_dvdnav = NULL;
  m_bEOF = true;
}

int CDVDInputStreamNavigator::Read(BYTE* buf, int buf_size)
{
  if (!m_dvdnav || m_bEOF) return -1;
  if (buf_size < DVD_VIDEO_BLOCKSIZE)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamNavigator: buffer size is to small, %d bytes, should be 2048 bytes", buf_size);
    return -1;
  }
  
  int navresult;
  int iBytesRead;

  while(true) {
    navresult = ProcessBlock(buf, &iBytesRead);
    if (navresult == NAVRESULT_HOLD)       return 0; // return 0 bytes read;
    else if (navresult == NAVRESULT_ERROR) return -1;
    else if (navresult == NAVRESULT_DATA)  return iBytesRead;
  }

  return iBytesRead;
}

// not working yet, but it is the recommanded way for seeking
__int64 CDVDInputStreamNavigator::Seek(__int64 offset, int whence)
{
  if (!m_dvdnav) return -1;  
  uint32_t pos=0, len=1;
  if (m_dll.dvdnav_sector_search(m_dvdnav, (uint64_t)(offset / DVD_VIDEO_LB_LEN), (int32_t)whence) != DVDNAV_STATUS_ERR)
  {
    m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
  }
  else
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", m_dll.dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  
  return (int)(pos * DVD_VIDEO_LB_LEN);
}

int CDVDInputStreamNavigator::ProcessBlock(BYTE* dest_buffer, int* read)
{
  if (!m_dvdnav) return -1;

  int result;
  int event;
  int len;
  int iNavresult = NAVRESULT_NOP;

  // m_tempbuffer will be used for anything that isn't a normal data block
  uint8_t* buf = m_tempbuffer;
  iNavresult = -1;
  
  try
  {
    // the main reading function
    result = m_dll.dvdnav_get_next_cache_block(m_dvdnav, &buf, &event, &len);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamNavigator::ProcessBlock - exception thrown in dvdnav_get_next_cache_block.");

    // okey, we are probably holding a vm_lock here so leave it.. this could potentialy cause problems if we aren't holding it
    // but it's more likely that we do
    LeaveCriticalSection((LPCRITICAL_SECTION)&(m_dvdnav->vm_lock));
    m_bEOF = true;
    return NAVRESULT_ERROR;
  }

  if (result == DVDNAV_STATUS_ERR)
  {
    CLog::DebugLog("Error getting next block: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    return NAVRESULT_ERROR;
  }

    switch (event)
    {
    case DVDNAV_BLOCK_OK:
      {
        // We have received a regular block of the currently playing MPEG stream.
        // buf contains the data and len its length (obviously!) (which is always 2048 bytes btw)
        fast_memcpy(dest_buffer, buf, len);
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
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_STILL_FRAME);
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

        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_WAIT);
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
        iNavresult = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_VTS_CHANGE);        
      }
      break;

    case DVDNAV_CELL_CHANGE:
      {
        // Some status information like the current Title and Part numbers do not
        // change inside a cell. Therefore this event can be used to query such
        // information only when necessary and update the decoding/displaying
        // accordingly.
        int tt = 0, ptt = 0;
        uint32_t pos, len;
        char input = '\0';

        m_dll.dvdnav_current_title_info(m_dvdnav, &tt, &ptt);
        m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
        CLog::DebugLog("Cell change: Title %d, Chapter %d\n", tt, ptt);
        CLog::DebugLog("At position %.0f%% inside the feature\n", 100 * (double)pos / (double)len);
        
        //Get total segment time        
        
        
        // this was not correct.. the values stored in hour/minute/second is stored quite weird
        // (hour >> 4) * 10 + hour & 0x0f would have been correct for them all.
        //vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);        
        //if (vm)
        //{
        //  m_iTotalTime = vm->state.pgc->playback_time.hour * 3600;
        //  m_iTotalTime += vm->state.pgc->playback_time.minute * 60;
        //  m_iTotalTime += vm->state.pgc->playback_time.second;
        //  m_iTotalTime *= 1000;
        //}

        dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)buf;
        m_iCellStart = cell_change_event->cell_start; // store cell time as we need that for time later
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
        //m_iTime = (int)(((__int64)m_iTotalTime * pos) / len);

        pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
        if( pci )
        {
          m_iTime = (int) ( m_dll.dvdnav_convert_time( &(pci->pci_gi.e_eltm) ) + m_iCellStart ) / 90;
        }        

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
        CLog::DebugLog("CDVDInputStreamNavigator: Unknown event (%i)\n", event);
      }
      break;

    }
    
    // check if libdvdnav gave us some other buffer to work with
    // probably not needed since function will check if buf
    // is part of the internal cache, but do it for good measure
    if( buf != m_tempbuffer )
      m_dll.dvdnav_free_cache_block(m_dvdnav, buf);
  
  return iNavresult;
}

bool CDVDInputStreamNavigator::SetActiveAudioStream(int iId)
{
  if (m_dvdnav)
  {
    int streamId = ConvertAudioStreamId_XBMCToExternal(iId);
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_audio_change(m_dvdnav, streamId));
  }
  return false; 
}

bool CDVDInputStreamNavigator::SetActiveSubtitleStream(int iId, bool bDisplay)
{
  if (m_dvdnav)
  {
    int streamId = ConvertSubtitleStreamId_XBMCToExternal(iId);
    if (!bDisplay) streamId |= 0x80;
    
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_subpicture_change(m_dvdnav, iId));
  }
  return false; 
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

void CDVDInputStreamNavigator::OnMenu()
{
  if (m_dvdnav) m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Escape);
}

void CDVDInputStreamNavigator::OnBack()
{
  if (!m_dvdnav) m_dll.dvdnav_go_up(m_dvdnav);
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnNext()
{
  if (m_dvdnav && !IsInMenu())
  {
    m_dll.dvdnav_next_pg_search(m_dvdnav);
  }
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnPrevious()
{
  if (m_dvdnav && !IsInMenu())
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

bool CDVDInputStreamNavigator::IsInMenu()
{
  if (!m_dvdnav) return false;

  //unsigned int iButton = (m_dll.dvdnav_get_vm(m_dvdnav)->state.HL_BTNN_REG >> 10);
  pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
  if (pci->hli.hl_gi.hli_ss > 0) return true;

  int iTitle, iPart;

  //if (DVDNAV_STATUS_OK != m_dll.dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart))
  //return (iTitle == 0);
  m_dll.dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart);

  // if we are not in a vts domain, we are probably in a menu
  return (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav) || iTitle == 0);
  return false;
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
        
        // If no such stream, then select the first one that exists.
        if (subpN < 0 || subpN >= 32)
        {
          subpN = 0;
          for (int i = 0; i < 32; i++)
          {
            if (vm->state.pgc->subp_control[i] & (1<<31))
            {
              subpN = i;
              break;
            }
          }
        }
      }
      
      activeStream = ConvertSubtitleStreamId_ExternalToXBMC(subpN);
    }
  }
  
  return activeStream;
}

std::string CDVDInputStreamNavigator::GetSubtitleStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;

  CStdString strLanguage;  

  subp_attr_t subp_attributes;
  int streamId = iId; //ConvertSubtitleStreamId_XBMCToExternal(iId);
  if( m_dll.dvdnav_get_stitle_info(m_dvdnav, iId, &subp_attributes) == DVDNAV_STATUS_OK )
  {
    
    if (subp_attributes.type == DVD_SUBPICTURE_TYPE_Language ||
        subp_attributes.type == DVD_SUBPICTURE_TYPE_NotSpecified)
    {
      if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, subp_attributes.lang_code)) strLanguage = "Unknown";

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
          strLanguage+= " (CC)";
          break;
        case DVD_SUBPICTURE_LANG_EXT_Forced:
          strLanguage+= " (Forced)";
          break;
        case DVD_SUBPICTURE_LANG_EXT_NormalDirectorsComments:
        case DVD_SUBPICTURE_LANG_EXT_BigDirectorsComments:
        case DVD_SUBPICTURE_LANG_EXT_ChildrensDirectorsComments:
          strLanguage+= " (Directors Comments)";
          break;
      }
    }
    else
    {
      strLanguage = "Unknown";
    }
  }

  return strLanguage;
}

int CDVDInputStreamNavigator::GetSubTitleStreamCount()
{
  int count = 0;
  
  if (m_dvdnav)
  {
    count = m_dll.dvdnav_get_nr_of_subtitle_streams(m_dvdnav);
  }
  
  return count;
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
        
        // If no such stream, then select the first one that exists.
        if (audioN < 0 || audioN >= 8)
        {
          audioN = 0;
          for (int i = 0; i < 8; i++)
          {
            if (vm->state.pgc->audio_control[i] & (1<<15))
            {
              audioN = i;
              break;
            }
          }
        }
      }
  
      activeStream = ConvertAudioStreamId_ExternalToXBMC(audioN);
    }
  }
  
  return activeStream;
}

std::string CDVDInputStreamNavigator::GetAudioStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;
  
  CStdString strLanguage;

  audio_attr_t audio_attributes;
  int streamId = iId; //ConvertAudioStreamId_XBMCToExternal(iId);
  if( m_dll.dvdnav_get_audio_info(m_dvdnav, streamId, &audio_attributes) == DVDNAV_STATUS_OK )
  {
    if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, audio_attributes.lang_code)) strLanguage = "Unknown";

    switch( audio_attributes.lang_extension )
    {
      case DVD_AUDIO_LANG_EXT_VisuallyImpaired:
        strLanguage+= " (Visually Impaired)";
        break;
      case DVD_AUDIO_LANG_EXT_DirectorsComments1:
        strLanguage+= " (Directors Comments)";
        break;
      case DVD_AUDIO_LANG_EXT_DirectorsComments2:
        strLanguage+= " (Directors Comments 2)";
        break;
      case DVD_AUDIO_LANG_EXT_NotSpecified:
      case DVD_AUDIO_LANG_EXT_NormalCaptions:
      default:
        break;
    }
  }

  return strLanguage;
}

int CDVDInputStreamNavigator::GetAudioStreamCount()
{
  int count = 0;
  
  if (m_dvdnav)
  {
    count = m_dll.dvdnav_get_nr_of_audio_streams(m_dvdnav);
  }
  
  return count;
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

    // check if libdvdnav provided us correct alpha values
    int* a = alpha[iButtonType];
    if (a[0] || a[1] || a[2] || a[3])
    {
      pOverlayPicture->alpha[0] = alpha[iButtonType][0];
      pOverlayPicture->alpha[1] = alpha[iButtonType][1];
      pOverlayPicture->alpha[2] = alpha[iButtonType][2];
      pOverlayPicture->alpha[3] = alpha[iButtonType][3];
    }
    
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 3; j++)
        pOverlayPicture->color[i][j] = pSPU->m_clut[color[iButtonType][i]][j];
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

bool CDVDInputStreamNavigator::Seek(int iTimeInMsec)
{

  if( m_dll.dvdnav_time_search(m_dvdnav, iTimeInMsec * 90) == DVDNAV_STATUS_ERR )
  {
    CLog::Log(LOGDEBUG, "dvdnav: dvdnav_time_search failed( %s )", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }
  return true;
}

float CDVDInputStreamNavigator::GetVideoAspectRatio()
{
  int iAspect = m_dll.dvdnav_get_video_aspect(m_dvdnav);
  int iPerm = m_dll.dvdnav_get_video_scale_permission(m_dvdnav);

  //The video scale permissions should give if the source is letterboxed
  //and such. should be able to give us info that we can zoom in automatically
  //not sure what to do with it currently

  CLog::Log(LOGINFO, __FUNCTION__" - Aspect wanted: %d, Scale permissions: %d", iAspect, iPerm);
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

int CDVDInputStreamNavigator::GetNrOfTitles()
{
  int iTitles = 0;
  
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_number_of_titles(m_dvdnav, &iTitles);
  }
  
  return iTitles;
}

int CDVDInputStreamNavigator::GetNrOfParts(int iTitle)
{
  int iParts = 0;
  
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_number_of_parts(m_dvdnav, iTitle, &iParts);
  }
  
  return iParts;
}

bool CDVDInputStreamNavigator::PlayTitle(int iTitle)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_title_play(m_dvdnav, iTitle));
  }
  
  return false;
}

bool CDVDInputStreamNavigator::PlayPart(int iTitle, int iPart)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_part_play(m_dvdnav, iTitle, iPart));
  }
  
  return false;
}

void CDVDInputStreamNavigator::EnableSubtitleStream(bool bEnable)
{
  int iCurrentStream = GetActiveSubtitleStream();
  if (bEnable)
  {
    SetActiveSubtitleStream(iCurrentStream);
  }
  else
  {
    // hide subtitles
    SetActiveSubtitleStream(iCurrentStream, false);
  }
}

bool CDVDInputStreamNavigator::GetNavigatorState(std::string &xmlstate)
{
  if( !m_dvdnav ) 
    return false;

  dvd_state_t save_state;
  if( DVDNAV_STATUS_ERR == m_dll.dvdnav_get_state(m_dvdnav, &save_state) )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::GetNavigatorState - Failed to get state (%s)", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  CDVDStateSerializer::test( &save_state );

  if( !CDVDStateSerializer::DVDToXMLState(xmlstate, &save_state) )
  {
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to serialize state");
    return false;
  }

  return true;
}

bool CDVDInputStreamNavigator::SetNavigatorState(std::string &xmlstate)
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
    CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::SetNavigatorState - Failed to set state (%s)", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  return true;
}

int CDVDInputStreamNavigator::ConvertAudioStreamId_XBMCToExternal(int id)
{
  if (m_dvdnav)
  {
    vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
    if (vm && vm->state.pgc && vm->state.domain == VTS_DOMAIN)
    {
      int stream = -1;
      for (int i = 0; i < 8; i++)
      {
        if (vm->state.pgc->audio_control[i] & (1<<15)) stream++;
        if (stream == id) return i;
      }
    }
  }
  
  return 0;
}

int CDVDInputStreamNavigator::ConvertAudioStreamId_ExternalToXBMC(int id)
{
  int stream = 0;
  
  if (id >= 0 && id < 8)
  {
    if (m_dvdnav)
    {
      vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
      if (vm && vm->state.pgc && vm->state.domain == VTS_DOMAIN)
      {
        for (int i = 0; i < id; i++)
        {
          if (vm->state.pgc->audio_control[i] & (1<<15)) stream++;
        }
      }
      else
      {
        // non VTS_DOMAIN, only one stream is available
        stream = 0;
      }
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "ConvertAudioStreamId_XBMCToExternal, incorrect id : %d", id);
  }
  
  return stream;
}

int CDVDInputStreamNavigator::ConvertSubtitleStreamId_XBMCToExternal(int id)
{
  if (m_dvdnav)
  {
    vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
    if (vm && vm->state.pgc && vm->state.domain == VTS_DOMAIN)
    {
      int stream = -1;
      for (int i = 0; i < 32; i++)
      {
        if (vm->state.pgc->subp_control[i] & (1<<31)) stream++;
        if (stream == id) return i;
      }
    }
  }
  
  return 0;
}

int CDVDInputStreamNavigator::ConvertSubtitleStreamId_ExternalToXBMC(int id)
{
  int stream = 0;
  
  if (id >= 0 && id < 8)
  {
    if (m_dvdnav)
    {
      vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
      if (vm && vm->state.pgc && vm->state.domain == VTS_DOMAIN)
      {
        for (int i = 0; i < id; i++)
        {
          if (vm->state.pgc->subp_control[i] & (1<<31)) stream++;
        }
      }
      else
      {
        // non VTS_DOMAIN, this function should not have been called anyway
        stream = 0;
      }
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "ConvertAudioStreamId_XBMCToExternal, incorrect id : %d", id);
  }
  
  return stream;
}
