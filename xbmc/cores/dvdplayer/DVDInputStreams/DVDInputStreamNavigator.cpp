
#include "../../../stdafx.h"
#include "DVDInputStreamNavigator.h"
#include "..\DVDPlayerDLL.h"
#include "..\..\..\util.h"
#include "..\..\..\LangCodeExpander.h"
#include "..\DVDDemuxSPU.h"
#include "..\DVDOverlay.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DVDNAV_COMPILE
 #include "dvdnav/dvdnav.h"

#ifndef WIN32
 #  define WIN32
 #endif // WIN32
 #define HAVE_CONFIG_H
 #include "dvdnav/dvdnav_internal.h"
 #include "dvdnav/vm.h"

  // forward declarations
  vm_t* dvdnav_get_vm(dvdnav_t *self);
  int dvdnav_get_nr_of_subtitle_streams(dvdnav_t *self);
  int dvdnav_get_nr_of_audio_streams(dvdnav_t *self);
  int dvdnav_get_button_info(dvdnav_t* self, int alpha[2][4], int color[2][4]);

#ifdef __cplusplus
}
#endif

CDVDInputStreamNavigator::CDVDInputStreamNavigator(IDVDPlayer* player) : CDVDInputStream()
{
  m_streamType = DVDSTREAM_TYPE_DVD;
  m_dvdnav = 0;
  m_pBufferSize = 0;
  m_pDVDPlayer = player;
  InitializeCriticalSection(&m_critSection);
  m_pDLLlibdvdnav = NULL; // DLL Handle for libdvdnav.dll
  m_pDLLlibdvdcss = NULL; // DLL Handle for libdvdcss.dll

  m_iTotalTime = 0;
  m_iCurrentTime = 0;
}

CDVDInputStreamNavigator::~CDVDInputStreamNavigator()
{
  Close();
  DeleteCriticalSection(&m_critSection);
  UnloadDLL();
}

bool CDVDInputStreamNavigator::LoadDLL()
{
  if (!m_pDLLlibdvdcss)
  {
    m_pDLLlibdvdcss = new DllLoader("Q:\\system\\players\\dvdplayer\\libdvdcss-2.dll", true);
    if (!m_pDLLlibdvdcss)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamNavigator::LoadDLL() loading dll failed");
      UnloadDLL();
      return false;
    }

    if (!m_pDLLlibdvdcss->Parse())
    {
      CLog::Log(LOGERROR, "CDVDInputStreamNavigator::LoadDLL() parse failed");
      UnloadDLL();
      return false;
    }
    if (!m_pDLLlibdvdcss->ResolveImports())
    {
      CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::LoadDLL() resolve imports failed");
      UnloadDLL();
      return false;
    }
  }

  if (!m_pDLLlibdvdnav)
  {
    m_pDLLlibdvdnav = new DllLoader("Q:\\system\\players\\dvdplayer\\libdvdnav.dll", true);
    if (!m_pDLLlibdvdnav)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamNavigator::LoadDLL() loading dll failed");
      UnloadDLL();
      return false;
    }

    if (!m_pDLLlibdvdnav->Parse())
    {
      CLog::Log(LOGERROR, "CDVDInputStreamNavigator::LoadDLL() parse failed");
      UnloadDLL();
      return false;
    }
    if (!m_pDLLlibdvdnav->ResolveImports())
    {
      CLog::Log(LOGWARNING, "CDVDInputStreamNavigator::LoadDLL() resolve imports failed");
      UnloadDLL();
      return false;
    }
    if (!dvdplayer_load_dll_libdvdnav(*m_pDLLlibdvdnav))
    {
      // if we can't find all functions in our dll we just quit
      // cause we don't want any NULL pointer errors
      UnloadDLL();
      return false;
    }
  }

  return true;
}

void CDVDInputStreamNavigator::UnloadDLL()
{
  if (m_pDLLlibdvdnav)
  {
    delete m_pDLLlibdvdnav;
    m_pDLLlibdvdnav = NULL;
  }
  if (m_pDLLlibdvdcss)
  {
    delete m_pDLLlibdvdcss;
    m_pDLLlibdvdcss = NULL;
  }
}

bool CDVDInputStreamNavigator::Open(const char* strFile)
{
  char* strDVDFile;

  if (!CDVDInputStream::Open(strFile)) return false;

  // load the dll first
  if (!LoadDLL()) return false;

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
  if (dvdnav_open(&m_dvdnav, strDVDFile) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_open\n");
    UnloadDLL();
    return false;
  }

  // set region flag (0xff = all regions ?)
  dvdnav_set_region_mask(m_dvdnav, 0xff);

  // set defaults
  if (dvdnav_menu_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK ||
      dvdnav_audio_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK ||
      dvdnav_spu_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on setting languages: %s\n", dvdnav_err_to_string(m_dvdnav));
    UnloadDLL();
    return false;
  }

  // set read ahead cache usage
  if (dvdnav_set_readahead_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_readahead_flag: %s\n", dvdnav_err_to_string(m_dvdnav));
    return false;
  }

  // set the PGC positioning flag to have position information relatively to the
  // whole feature instead of just relatively to the current chapter
  if (dvdnav_set_PGC_positioning_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_PGC_positioning_flag: %s\n", dvdnav_err_to_string(m_dvdnav));
    UnloadDLL();
    return false;
  }

  free(strDVDFile);

  return true;
}

void CDVDInputStreamNavigator::Close()
{
  if (!m_dvdnav) return ;

  // finish off by closing the dvdnav device
  if (dvdnav_close(m_dvdnav) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_close: %s\n", dvdnav_err_to_string(m_dvdnav));
    return ;
  }

  CDVDInputStream::Close();
  m_dvdnav = NULL;
}

int CDVDInputStreamNavigator::Read(BYTE* buf, int buf_size)
{
  if (!m_dvdnav) return -1;

  int navresult;
  if (m_pBufferSize == 0)
  {
    do
    {
      navresult = ProcessBlock();
      if (navresult == DVDNAV_STILL_FRAME) return 0; // return 0 bytes read;
      if (navresult == DVDNAV_STOP || navresult == -1) return -1;
    }
    while (navresult != DVDNAV_BLOCK_OK);
  }

  if (m_pBufferSize > 0) fast_memcpy(buf, m_temp, m_pBufferSize);

  int temp = m_pBufferSize;
  m_pBufferSize = 0;
  return temp;
}

int CDVDInputStreamNavigator::Seek(__int64 offset, int whence)
{
  if (!m_dvdnav) return -1;  
  uint32_t pos=0, len=1;
  if (dvdnav_sector_search(m_dvdnav, (uint64_t)(offset / DVD_VIDEO_LB_LEN), (int32_t)whence) != DVDNAV_STATUS_ERR)
  {
    dvdnav_get_position(m_dvdnav, &pos, &len);    
  }
  else
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  
  return (int)(pos * DVD_VIDEO_LB_LEN);
}

int CDVDInputStreamNavigator::ProcessBlock()
{
  if (!m_dvdnav) return -1;

  int result, event, len, iNavresult;
  uint8_t *buf;
  bool bFinished = false;

  if (m_pBufferSize > 0)
  {
    //if we still have unreaded mpeg data, just return
    CLog::DebugLog("CDVDInputStreamNavigator::ProcessBlock, unreaded mpeg data!!");
    return DVDNAV_BLOCK_OK;
  }

  iNavresult = -1;

  while (!bFinished)
  {
    buf = m_mem;
    // the main reading function
    result = dvdnav_get_next_cache_block(m_dvdnav, &buf, &event, &len);

    if (result == DVDNAV_STATUS_ERR)
    {
      CLog::DebugLog("Error getting next block: %s\n", dvdnav_err_to_string(m_dvdnav));
      iNavresult = DVDNAV_STATUS_ERR;
      bFinished = true;
    }

    switch (event)
    {
    case DVDNAV_BLOCK_OK:
      // We have received a regular block of the currently playing MPEG stream.
      // A real player application would now pass this block through demuxing
      // and decoding.
      // buf contains the data and len its length (obviously!) (which is always 2048 bytes btw)
      m_pBufferSize = len;
      fast_memcpy(m_temp, buf, len);
      iNavresult = DVDNAV_BLOCK_OK;
      bFinished = true;
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
        int navres = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_STILL_FRAME);
        iNavresult = DVDNAV_STILL_FRAME;
        if (navres == NAVRESULT_STILL_NOT_SKIPPED) bFinished = true;
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

        // xbmc, we don't use any fifos at this place
        SkipWait();
      }
      break;

    case DVDNAV_SPU_CLUT_CHANGE:
      // Player applications should pass the new colour lookup table to their
      // SPU decoder. The CLUT is given as 16 uint32_t's in the buffer.
      m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_CLUT_CHANGE);
      break;

    case DVDNAV_SPU_STREAM_CHANGE:
      // Player applications should inform their SPU decoder to switch channels
      m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_STREAM_CHANGE);
      break;

    case DVDNAV_AUDIO_STREAM_CHANGE:
      // Player applications should inform their audio decoder to switch channels
      {
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)buf;
        event->logical = dvdnav_get_audio_logical_stream(m_dvdnav, event->physical);
      }
      m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_AUDIO_STREAM_CHANGE);

      break;

    case DVDNAV_HIGHLIGHT:
      {
        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_HIGHLIGHT);
      }
      break;

    case DVDNAV_VTS_CHANGE:
      // Some status information like video aspect and video scale permissions do
      // not change inside a VTS. Therefore this event can be used to query such
      // information only when necessary and update the decoding/displaying
      // accordingly.
      m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_VTS_CHANGE);
      iNavresult = -1; // return read error
      bFinished = true;
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

        dvdnav_current_title_info(m_dvdnav, &tt, &ptt);
        dvdnav_get_position(m_dvdnav, &pos, &len);
        CLog::DebugLog("Cell change: Title %d, Chapter %d\n", tt, ptt);
        CLog::DebugLog("At position %.0f%% inside the feature\n", 100 * (double)pos / (double)len);

        dvdnav_cell_change_event_t* cell_event = (dvdnav_cell_change_event_t*)buf;
        m_iTotalTime = (int)((cell_event->pgc_length / 100000) & 0xFFFFFFFF);

        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_CELL_CHANGE);
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
        pci_t* pci = dvdnav_get_current_nav_pci(m_dvdnav);
        m_iCurrentTime = (int)((pci->pci_gi.vobu_s_ptm / 100000) & 0xFFFFFFFF);
        
        m_pDVDPlayer->OnDVDNavResult((void*)pci, DVDNAV_NAV_PACKET);

      }
      break;

    case DVDNAV_HOP_CHANNEL:
      // This event is issued whenever a non-seamless operation has been executed.
      // Applications with fifos should drop the fifos content to speed up responsiveness.
      m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_HOP_CHANNEL);
      break;

    case DVDNAV_STOP:
      {
        // Playback should end here.
        m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_STOP);
        iNavresult = DVDNAV_STOP;
        bFinished = true;
      }
      break;

    default:
      {
        CLog::DebugLog("Unknown event (%i)\n", event);
      }
      break;

    }

    dvdnav_free_cache_block(m_dvdnav, buf);
  }
  bFinished = false;
  return iNavresult;
}

bool CDVDInputStreamNavigator::SetActiveAudioStream(int iPhysicalId)
{
  vm_t* vm = dvdnav_get_vm(m_dvdnav);

  if((vm->state).domain != VTS_DOMAIN)
    return false;


  int iStreams = vm->vtsi->vtsi_mat->nr_of_vts_audio_streams;
  for( int audioN = 0; audioN < iStreams; audioN++ )
  {
    if((vm->state).pgc->audio_control[audioN] & (1<<15))
    {
      int streamN = ((vm->state).pgc->audio_control[audioN] >> 8) & 0x07;
      if( streamN == iPhysicalId )
      {
        (vm->state).AST_REG = audioN;
        return true;
      }
    }
  }
  return false;
}

bool CDVDInputStreamNavigator::SetActiveSubtitleStream(int iPhysicalId)
{
  vm_t* vm = dvdnav_get_vm(m_dvdnav);

  if((vm->state).domain != VTS_DOMAIN)
    return false;


  int iStreams = vm->vtsi->vtsi_mat->nr_of_vts_subp_streams;
  for( int subpN = 0; subpN < iStreams; subpN++ )
  {
    if((vm->state).pgc->subp_control[subpN] & (1<<31))
    {
      int streamN0 = ((vm->state).pgc->subp_control[subpN] >> 16) & 0x1f;
      int streamN1 = ((vm->state).pgc->subp_control[subpN] >> 8) & 0x1f;
      int streamN2 = (vm->state).pgc->subp_control[subpN] & 0x1f;
      if( streamN0 == iPhysicalId || streamN1 == iPhysicalId || streamN2 == iPhysicalId )
      {
        (vm->state).SPST_REG = subpN;
        return true;
      }
    }
  }
  return false;


}
void CDVDInputStreamNavigator::ActivateButton()
{
  if (!m_dvdnav) return ;
  dvdnav_button_activate(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::SelectButton(int iButton)
{
  if (!m_dvdnav) return ;
  dvdnav_button_select(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav), iButton);
}

int CDVDInputStreamNavigator::GetCurrentButton()
{
  int button;
  if (!m_dvdnav) return -1;
  dvdnav_get_current_highlight(m_dvdnav, &button);
  return button;
}

int CDVDInputStreamNavigator::GetTotalButtons()
{
  if (!m_dvdnav) return 0;
  return dvdnav_get_current_nav_pci(m_dvdnav)->hli.hl_gi.nsl_btn_ns;
}

void CDVDInputStreamNavigator::OnUp()
{
  if (!m_dvdnav) return ;
  dvdnav_upper_button_select(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnDown()
{
  if (!m_dvdnav) return ;
  dvdnav_lower_button_select(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnLeft()
{
  if (!m_dvdnav) return ;
  dvdnav_left_button_select(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnRight()
{
  if (!m_dvdnav) return ;
  dvdnav_right_button_select(m_dvdnav, dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnMenu()
{
  if (!m_dvdnav) return ;
  dvdnav_menu_call(m_dvdnav, DVD_MENU_Escape);
}

void CDVDInputStreamNavigator::OnBack()
{
  if (!m_dvdnav) return ;
  dvdnav_go_up(m_dvdnav);
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnNext()
{
  if (m_dvdnav && !IsInMenu())
  {
    dvdnav_next_pg_search(m_dvdnav);
  }
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnPrevious()
{
  if (m_dvdnav && !IsInMenu())
  {
    dvdnav_prev_pg_search(m_dvdnav);
  }
}

void CDVDInputStreamNavigator::SkipStill()
{
  if (!m_dvdnav) return ;
  dvdnav_still_skip(m_dvdnav);
}

void CDVDInputStreamNavigator::SkipWait()
{
  if (!m_dvdnav) return ;
  dvdnav_wait_skip(m_dvdnav);
}

void CDVDInputStreamNavigator::Lock()
{
  EnterCriticalSection(&m_critSection);
}


void CDVDInputStreamNavigator::Unlock()
{
  LeaveCriticalSection(&m_critSection);
}

bool CDVDInputStreamNavigator::IsInMenu()
{
  if (!m_dvdnav) return false;

  //unsigned int iButton = (dvdnav_get_vm(m_dvdnav)->state.HL_BTNN_REG >> 10);
  pci_t* pci = dvdnav_get_current_nav_pci(m_dvdnav);
  if (pci->hli.hl_gi.hli_ss > 0) return true;

  int iTitle, iPart;

  //if (DVDNAV_STATUS_OK != dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart))
  //return (iTitle == 0);
  dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart);

  // if we are not in a vts domain, we are probably in a menu
  return (0 == dvdnav_is_domain_vts(m_dvdnav) || iTitle == 0);
  return false;
}

int CDVDInputStreamNavigator::GetActiveSubtitleStream()
{
  int iStream;
  if (!m_dvdnav) return -1;

  iStream = (dvdnav_get_active_spu_stream(m_dvdnav) & 0xff);
  if (iStream == 0x80) return -1;
  return (iStream & ~0x80);
}

std::string CDVDInputStreamNavigator::GetSubtitleStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;

  CStdString strLanguage;

  uint16_t lang = dvdnav_spu_stream_to_lang(m_dvdnav, iId);
  if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, lang)) strLanguage = "Unknown";

  return strLanguage;
}

int CDVDInputStreamNavigator::GetSubTitleStreamCount()
{
  if (!m_dvdnav) return 0;
  return dvdnav_get_nr_of_subtitle_streams(m_dvdnav);
}

int CDVDInputStreamNavigator::GetActiveAudioStream()
{
  if (!m_dvdnav) return -1;

  return dvdnav_get_active_audio_stream(m_dvdnav);
}

std::string CDVDInputStreamNavigator::GetAudioStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;

  CStdString strLanguage;

  uint16_t lang = dvdnav_audio_stream_to_lang(m_dvdnav, iId);
  if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, lang)) strLanguage = "Unknown";

  return strLanguage;
}

int CDVDInputStreamNavigator::GetAudioStreamCount()
{
  if (!m_dvdnav) return 0;
  return dvdnav_get_nr_of_audio_streams(m_dvdnav);
}

bool CDVDInputStreamNavigator::GetHighLightArea(int* iXStart, int* iXEnd, int* iYStart, int* iYEnd, int iButton)
{
  dvdnav_highlight_area_t hl;
  int iCurrentButton = 0;

  if (!m_dvdnav) return false;

  if (DVDNAV_STATUS_OK == dvdnav_get_highlight_area(dvdnav_get_current_nav_pci(m_dvdnav), iButton, 0 /*mode? spu stream?*/, &hl))
  {
    // button cropping information
    *iXStart = hl.sx;
    *iXEnd = hl.ex;
    *iYStart = hl.sy;
    *iYEnd = hl.ey;
    return true;
  }

  return false;
}

bool CDVDInputStreamNavigator::GetButtonInfo(DVDOverlayPicture* pOverlayPicture, CDVDDemuxSPU* pSPU)
{
  int alpha[2][4];
  int color[2][4];

  int iButtonType = 0; // 0 = selection, 1 = action (clicked)

  if (!m_dvdnav || dvdnav_get_button_info(m_dvdnav, alpha, color) < 0) return false;

  pOverlayPicture->alpha[0] = alpha[iButtonType][0];
  pOverlayPicture->alpha[1] = alpha[iButtonType][1];
  pOverlayPicture->alpha[2] = alpha[iButtonType][2];
  pOverlayPicture->alpha[3] = alpha[iButtonType][3];

  int i;
  for (i = 0; i < 3; i++) pOverlayPicture->color[0][i] = pSPU->m_clut[color[iButtonType][0]][i];
  for (i = 0; i < 3; i++) pOverlayPicture->color[1][i] = pSPU->m_clut[color[iButtonType][1]][i];
  for (i = 0; i < 3; i++) pOverlayPicture->color[2][i] = pSPU->m_clut[color[iButtonType][2]][i];
  for (i = 0; i < 3; i++) pOverlayPicture->color[3][i] = pSPU->m_clut[color[iButtonType][3]][i];

  return true;
}

int CDVDInputStreamNavigator::GetTotalTime()
{
  return m_iTotalTime;
}

int CDVDInputStreamNavigator::GetTime()
{
  return m_iCurrentTime;
}

float CDVDInputStreamNavigator::GetPercentage()
{
  uint32_t pos=0, len=0;
  if( dvdnav_get_position(m_dvdnav, &pos, &len) == DVDNAV_STATUS_ERR )
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  return (pos * 100.0f / len);
}

bool CDVDInputStreamNavigator::SeekPercentage(float fPercent)
{
  uint32_t pos=0, len=0;
  uint64_t newpos=0;
  dvdnav_get_position(m_dvdnav, &pos, &len);
    
  newpos = (uint64_t)(fPercent * (uint64_t)len / 100.0f);
  if (dvdnav_sector_search(m_dvdnav, newpos, SEEK_SET) == DVDNAV_STATUS_ERR)
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", dvdnav_err_to_string(m_dvdnav));
    return false;
  }
  return true;
}