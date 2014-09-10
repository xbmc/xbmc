/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#if defined(HAS_MMAL)
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecMMAL.h"
#include "settings/Settings.h"
#include "utils/log.h"

#define CLASSNAME "CDVDVideoCodecMMAL"
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecMMAL::CDVDVideoCodecMMAL()
 : m_decoder( new CMMALVideo )
{
  CLog::Log(LOGDEBUG, "%s::%s %p\n", CLASSNAME, __func__, this);
}

CDVDVideoCodecMMAL::~CDVDVideoCodecMMAL()
{
  CLog::Log(LOGDEBUG, "%s::%s %p\n", CLASSNAME, __func__, this);
  Dispose();
}

bool CDVDVideoCodecMMAL::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  return m_decoder->Open(hints, options, m_decoder);
}

const char* CDVDVideoCodecMMAL::GetName(void)
{
  return m_decoder ? m_decoder->GetName() : "mmal-xxx";
}

void CDVDVideoCodecMMAL::Dispose()
{
  m_decoder->Dispose();
}

void CDVDVideoCodecMMAL::SetDropState(bool bDrop)
{
  m_decoder->SetDropState(bDrop);
}

int CDVDVideoCodecMMAL::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  return m_decoder->Decode(pData, iSize, dts, pts);
}

unsigned CDVDVideoCodecMMAL::GetAllowedReferences()
{
  return m_decoder->GetAllowedReferences();
}

void CDVDVideoCodecMMAL::Reset(void)
{
  m_decoder->Reset();
}

bool CDVDVideoCodecMMAL::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_decoder->GetPicture(pDvdVideoPicture);
}

bool CDVDVideoCodecMMAL::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_decoder->ClearPicture(pDvdVideoPicture);
}

bool CDVDVideoCodecMMAL::GetCodecStats(double &pts, int &droppedPics)
{
  return m_decoder->GetCodecStats(pts, droppedPics);
}

#endif
