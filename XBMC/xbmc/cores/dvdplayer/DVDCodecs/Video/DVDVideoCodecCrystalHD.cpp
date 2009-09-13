/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#include "stdafx.h"
// FIXME: clean up HAVE_LIBCRYSTALHD later
#define HAVE_LIBCRYSTALHD
#if defined(HAVE_LIBCRYSTALHD)
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"

#define __MODULE_NAME__ "DVDVideoCodecCrystalHD"

class CExecTimer
{
public:
  CExecTimer() :
    m_StartTime(0),
    m_PunchInTime(0),
    m_PunchOutTime(0),
    m_LastCallInterval(0)
  {
    QueryPerformanceFrequency((LARGE_INTEGER*)&m_CounterFreq);
    m_CounterFreq /= 1000; // Scale to ms
  }
  
  void Start()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&m_StartTime);
  }
  
  void PunchIn()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&m_PunchInTime);
    if (m_PunchOutTime)
      m_LastCallInterval = m_PunchInTime - m_PunchOutTime;
    else
      m_LastCallInterval = 0;
    m_PunchOutTime = 0;
  }
  
  void PunchOut()
  {
    if (m_PunchInTime)
      QueryPerformanceCounter((LARGE_INTEGER*)&m_PunchOutTime);
  }
  
  void Reset()
  {
    m_StartTime = 0;
    m_PunchInTime = 0;
    m_PunchOutTime = 0;
    m_LastCallInterval = 0;
  }

  uint64_t GetTimeSincePunchIn()
  {
    if (m_PunchInTime)
    {
      uint64_t now;
      QueryPerformanceCounter((LARGE_INTEGER*)&now);
      return (now - m_PunchInTime)/m_CounterFreq;  
    }
    else
      return 0;
  }

  uint64_t GetElapsedTime()
  {
    if (m_StartTime)
    {
      uint64_t now;
      QueryPerformanceCounter((LARGE_INTEGER*)&now);
      return (now - m_StartTime)/m_CounterFreq;  
    }
    else
      return 0;
  }
  
  uint64_t GetExecTime()
  {
    if (m_PunchOutTime && m_PunchInTime)
      return (m_PunchOutTime - m_PunchInTime)/m_CounterFreq;  
    else
      return 0;
  }
  
  uint64_t GetIntervalTime()
  {
    return m_LastCallInterval/m_CounterFreq;
  }
  
protected:
  uint64_t m_StartTime;
  uint64_t m_PunchInTime;
  uint64_t m_PunchOutTime;
  uint64_t m_LastCallInterval;
  uint64_t m_CounterFreq;
};

CExecTimer g_InputTimer;
CExecTimer g_OutputTimer;
CExecTimer g_ClientTimer;

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(NULL),
  m_DropPictures(false),
  m_pFormatName("")
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  BCM_CODEC_TYPE codec_type;
  BCM_STREAM_TYPE stream_type;
  
  codec_type = hints.codec;
  stream_type = BC_STREAM_TYPE_ES;
  
  
  m_Device = new CCrystalHD();
  if (!m_Device)
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    return false;
  }
  
  if (!m_Device->Open(stream_type, codec_type))
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  if (m_Device)
  {
    m_Device->Close();
    delete m_Device;
    m_Device = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
  int ret = 0;
  bool inputFull = false;

  int maxWait = 40;
  unsigned int lastTime = GetTickCount();
  unsigned int maxTime = lastTime + maxWait;
  while ((lastTime = GetTickCount()) < maxTime)
  {
    // Handle Input
    if (pData)
    {
      if (m_Device->AddInput(pData, iSize, pts))
        pData = NULL;
      else
        CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full", __MODULE_NAME__);
    }
      // Handle Output
    if (m_Device->GetReadyCount())
      ret |= VC_PICTURE;
    
    if (m_Device->GetInputCount() < 10)
      ret |= VC_BUFFER;

    if (!pData && (ret & VC_PICTURE))
      break;
  }

  if (lastTime >= maxTime)
    CLog::Log(LOGDEBUG, "%s: Timeout in CDVDVideoCodecCrystalHD::Decode. ret: 0x%08x pData: %p", __MODULE_NAME__, ret, pData);

  if (!ret)
    ret = VC_ERROR;

  return ret;
}

void CDVDVideoCodecCrystalHD::Reset()
{
  m_Device->Flush();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  m_Device->GetPicture(pDvdVideoPicture);
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   

  return true;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_Device->SetDropState(bDrop);
}


/////////////////////////////////////////////////////////////////////


#endif
