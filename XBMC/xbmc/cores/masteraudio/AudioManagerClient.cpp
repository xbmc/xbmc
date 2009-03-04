/*
 *      Copyright (C) 2009 Team XBMC
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

#include "stdafx.h"
#include "AudioManagerClient.h"


CAudioManagerClient::CAudioManagerClient(CAudioManager* pManager) :
  m_pManager(pManager),
  m_StreamId(MA_STREAM_NONE)
{

}

CAudioManagerClient::~CAudioManagerClient()
{
  if (VALID_STREAM_ID(m_StreamId))
    CloseStream();
}

void CAudioManagerClient::Stop()
{
  TRY_CONTROL_STREAM(MA_CONTROL_STOP);    
}

void CAudioManagerClient::Play()
{
  TRY_CONTROL_STREAM(MA_CONTROL_PLAY);    
}

void CAudioManagerClient::Pause()
{
  TRY_CONTROL_STREAM(MA_CONTROL_PAUSE);    
}

void CAudioManagerClient::Resume()
{
  TRY_CONTROL_STREAM(MA_CONTROL_RESUME);    
}

bool CAudioManagerClient::SetVolume(long vol)
{
  if (m_pManager)
    return m_pManager->SetStreamVolume(m_StreamId ,vol);

  return false;
}

void CAudioManagerClient::CloseStream()
{
  if (m_pManager)
    m_pManager->CloseStream(m_StreamId);
  m_StreamId = MA_STREAM_NONE;
}

float CAudioManagerClient::GetDelay()
{
  if (!m_pManager)
    return 0.0f;
  return m_pManager->GetMaxStreamLatency(m_StreamId);
}

size_t CAudioManagerClient::AddDataToStream(void* pData, size_t len)
{
  if (m_pManager)
    return m_pManager->AddDataToStream(m_StreamId, pData, len);
  
  return 0;
}

void CAudioManagerClient::DrainStream(int maxWaitTime)
{
  if (m_pManager)
    m_pManager->DrainStream(m_StreamId,maxWaitTime);
}

void CAudioManagerClient::FlushStream()
{
  if (m_pManager)
    m_pManager->FlushStream(m_StreamId);  
}

////////////////////////////////////////////////////////////////////////////////////////
// Private Members
////////////////////////////////////////////////////////////////////////////////////////
bool CAudioManagerClient::OpenStream(CStreamDescriptor* pDesc, size_t blockSize)
{
  if (!m_pManager)
    return false;
  m_StreamId = m_pManager->OpenStream(pDesc, blockSize);
  return (m_StreamId != MA_STREAM_NONE);
}

