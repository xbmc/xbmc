/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <samplerate.h>

#include "CoreAudioAESound.h"

#include "AEFactory.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEUtil.h"
#include "CoreAudioAE.h"
#include "Interfaces/AESound.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"

/* typecast AE to CCoreAudioAE */
#define AE (*(CCoreAudioAE*)CAEFactory::GetEngine())

CCoreAudioAESound::CCoreAudioAESound(const std::string &filename) :
  IAESound         (filename),
  m_filename       (filename),
  m_volume         (1.0f    ),
  m_inUse          (0       )
{
}

CCoreAudioAESound::~CCoreAudioAESound()
{
  DeInitialize();
}


std::string CCoreAudioAESound::GetFileName()
{
  return m_filename;
}

void CCoreAudioAESound::DeInitialize()
{
  m_wavLoader.DeInitialize();
}

bool CCoreAudioAESound::Initialize()
{
  DeInitialize();

  if (!m_wavLoader.Initialize(m_filename, AE.GetSampleRate()))
    return false;

  return m_wavLoader.Remap(AE.GetChannelLayout());
}

void CCoreAudioAESound::SetVolume(float volume)
{
  m_volume = std::max(0.0f, std::min(1.0f, volume));
}

float CCoreAudioAESound::GetVolume()
{
  return m_volume;
}

unsigned int CCoreAudioAESound::GetSampleCount()
{
  CSingleLock cs(m_critSection);
  if (m_wavLoader.IsValid())
    return m_wavLoader.GetSampleCount();
  return 0;
}

float* CCoreAudioAESound::GetSamples()
{
  CSingleLock cs(m_critSection);
  if (!m_wavLoader.IsValid())
    return NULL;

  ++m_inUse;
  return m_wavLoader.GetSamples();
}

void CCoreAudioAESound::ReleaseSamples()
{
  CSingleLock cs(m_critSection);
  ASSERT(m_inUse > 0);
  --m_inUse;
}

bool CCoreAudioAESound::IsPlaying()
{
  CSingleLock cs(m_critSection);
  return (m_inUse > 0);
}

void CCoreAudioAESound::Play()
{
  AE.PlaySound(this);
}

void CCoreAudioAESound::Stop()
{
  AE.StopSound(this);
}
