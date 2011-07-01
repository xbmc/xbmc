/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AESound.h"

#include <samplerate.h>
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"

#include "AEFactory.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEUtil.h"
#include "DllAvCore.h"

#include "SoftAE.h"
#include "SoftAESound.h"

/* typecast AE to CSoftAE */
#define AE (*(CSoftAE*)AE.GetEngine())

typedef struct
{
  char     chunk_id[4];
  uint32_t chunksize;
} WAVE_CHUNK;

CSoftAESound::CSoftAESound(const CStdString &filename) :
  IAESound         (filename),
  m_volume         (1.0f    ),
  m_inUse          (0       ),
  m_freeCallback   (NULL    ),
  m_freeCallbackArg(NULL    )
{
  m_filename = filename;
}

CSoftAESound::~CSoftAESound()
{
  DeInitialize();
}

void CSoftAESound::DeInitialize()
{
  m_wavLoader.DeInitialize();
}

bool CSoftAESound::Initialize()
{
  DeInitialize();

  if (!m_wavLoader.Initialize(m_filename, AE.GetSampleRate()))
    return false;

  return m_wavLoader.Remap(AE.GetChannelLayout());
}

unsigned int CSoftAESound::GetSampleCount()
{
  m_sampleLock.lock_shared();
  int sampleCount = 0;
  if (m_wavLoader.IsValid())
    sampleCount = m_wavLoader.GetSampleCount();
  m_sampleLock.unlock_shared();
  return sampleCount;
}

float* CSoftAESound::GetSamples()
{
  m_sampleLock.lock_shared();
  if (!m_wavLoader.IsValid())
  {
    m_sampleLock.unlock_shared();
    return NULL;
  }

  ++m_inUse;
  return m_wavLoader.GetSamples();
}

void CSoftAESound::ReleaseSamples()
{
  --m_inUse;
  m_sampleLock.unlock_shared();
}

bool CSoftAESound::IsPlaying()
{
  m_sampleLock.lock_shared();
  bool playing = m_inUse > 0;
  m_sampleLock.unlock_shared();

  return playing;
}

void CSoftAESound::SetFreeCallback(AECBFunc *callback, void *arg)
{
  m_freeCallback    = callback;
  m_freeCallbackArg = arg;
}

void CSoftAESound::Play()
{
  AE.PlaySound(this);
}

void CSoftAESound::Stop()
{
  AE.StopSound(this);
}

