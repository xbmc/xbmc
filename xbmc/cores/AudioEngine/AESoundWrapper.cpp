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

#include "AEWrapper.h"
#include "AESoundWrapper.h"

CAESoundWrapper::CAESoundWrapper(const CStdString &filename) :
  IAESound  (filename),
  m_filename(filename),
  m_sound   (NULL    ),
  m_volume  (1.0f    )
{
  m_filename = filename;
  Load();
}

CAESoundWrapper::~CAESoundWrapper()
{
}

void CAESoundWrapper::UnLoad()
{
  CExclusiveLock lock(m_lock);

  if (!m_sound) return;
  IAE *ae = AE.GetEngine();
  if (ae) ae->FreeSound(m_sound);
  m_sound = NULL;

}

void CAESoundWrapper::Load()
{
  CExclusiveLock lock(m_lock);

  IAE *ae = AE.GetEngine();
  if (ae     ) m_sound = ae->GetSound(m_filename);
  if (m_sound) m_sound->SetVolume(m_volume);

}

void CAESoundWrapper::Play()
{
  CSharedLock lock(m_lock);
  m_sound->Play();
}

void CAESoundWrapper::Stop()
{
  CSharedLock lock(m_lock);
  m_sound->Stop();
}

bool CAESoundWrapper::IsPlaying()
{
  CSharedLock lock(m_lock);
  bool play = m_sound->IsPlaying();
  return play;
}

void CAESoundWrapper::SetVolume(float volume)
{
  CSharedLock lock(m_lock);
  m_sound->SetVolume(volume);
  m_volume = volume;
}

float CAESoundWrapper::GetVolume()
{
  CSharedLock lock(m_lock);
  float volume = m_sound->GetVolume();
  return volume;
}

