/*
 *      Copyright (C) 2009 Team XBMC
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
#include "AudioStream.h"

///////////////////////////////////////////////////////////////////////////////
// CAudioStream
///////////////////////////////////////////////////////////////////////////////
CAudioStream::CAudioStream() :
  m_pInput(NULL),
  m_pDSPChain(NULL),
  m_pRenderingAdapter(NULL),
  m_Open(false)
{
  m_ProcessTimer.reset();
  m_IntervalTimer.reset();
}

CAudioStream::~CAudioStream()
{
    Destroy();
}

bool CAudioStream::Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, IRenderingAdapter* pRenderAdapter)
{
  if (m_Open || !pInput || !pDSPChain || !pRenderAdapter)
    return false;

  m_pInput = pInput;
  m_pDSPChain = pDSPChain;
  m_pRenderingAdapter = pRenderAdapter;

  // Hook-up interconnections: Input <- DSPChain <- Mixer
  // It is assumed at this point that the input/output stream formats are compatible
  m_pDSPChain->SetSource(m_pInput);
  m_pRenderingAdapter->SetSource(m_pDSPChain);

  m_Open = true;
  return m_Open;
}

void CAudioStream::Destroy()
{
  if (!m_Open)
    return;

  m_Open = false;
  m_pDSPChain->Close();
  delete m_pDSPChain;
  m_pDSPChain = NULL;

  delete m_pInput;
  m_pInput = NULL;

  CLog::Log(LOGINFO,"MasterAudio:AudioStream: Closing. Average time to process = %0.2fms / %0.2fms (%0.2f%% duty cycle)", m_ProcessTimer.average_time/1000.0f, m_IntervalTimer.average_time/1000.0f, (m_ProcessTimer.average_time / m_IntervalTimer.average_time) * 100);
}

bool CAudioStream::Render()
{
  m_IntervalTimer.lap_end();
  bool ret = true;
  
  m_ProcessTimer.lap_start();

  m_pRenderingAdapter->Render();

  m_ProcessTimer.lap_end(); // Time per call

  m_IntervalTimer.lap_start(); // Time between calls
  return ret;
}

float CAudioStream::GetDelay()
{
  // TODO: This can be more elegant...
  return m_pRenderingAdapter->GetDelay() + m_pDSPChain->GetDelay() + m_pInput->GetDelay();
}

void CAudioStream::Flush()
{
  if (!m_Open)
    return;

  m_pInput->Reset();
  m_pDSPChain->Flush();
  m_pRenderingAdapter->Flush();
  
  CLog::Log(LOGDEBUG,"MasterAudio:AudioStream: Flushed stream.");
}

bool CAudioStream::Drain(unsigned int timeout)
{
  if (!m_Open)
    return true;

  if (m_pRenderingAdapter->Drain(timeout))
    return true;

  // We ran out of time. Clean up.
  Flush();  // Abandon any remaining data
  return false;
}

void CAudioStream::SendCommand(int command)
{
  if (!m_Open)
    return;

  switch(command)
  {
    case MA_CONTROL_STOP:
      m_pRenderingAdapter->Stop();
      break;
    case MA_CONTROL_PLAY:
      m_pRenderingAdapter->Play();
      break;
    case MA_CONTROL_PAUSE:
      m_pRenderingAdapter->Pause();
      break;
    case MA_CONTROL_RESUME:
      m_pRenderingAdapter->Resume();
      break;
  }
}

void CAudioStream::SetLevel(float level)
{
  if (!m_Open)
    return;

  m_pRenderingAdapter->SetVolume((long)((level - 1.0f) * 6000.0f));
}

unsigned int CAudioStream::AddData(void* pData, unsigned int len)
{
  if (!m_Open)
    return 0;

  return m_pInput->AddData(pData, len);
}