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

#include "utils/SingleLock.h"
#include "utils/log.h"
#include "AESound.h"

CAESound::CAESound(const CStdString &filename) :
  m_valid       (false),
  m_channelCount(0    ),
  m_samples     (NULL ),
  m_frameCount  (0    )
{
  m_filename = filename;
}

CAESound::~CAESound()
{
  DeInitialize();
}

void CAESound::DeInitialize()
{
  CSingleLock lock(m_critSection);
  AE.StopSound(this);

  delete[] m_samples;
  m_samples      = NULL;
  m_frameCount   = 0;
  m_channelCount = 0;
  m_valid        = false;
}

bool CAESound::Initialize()
{
  DeInitialize();
  CSingleLock lock(m_critSection);

  return false;
}

float* CAESound::GetFrame(unsigned int frame)
{
  if (!m_valid || frame > m_frameCount) return NULL;
  return &m_samples[frame * m_channelCount];
}

void CAESound::Play()
{
  if (!m_valid) return;
  AE.PlaySound(this);
}

