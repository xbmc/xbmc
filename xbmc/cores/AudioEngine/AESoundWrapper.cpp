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
  IAESound         (filename),
  m_filename       (filename),
  m_sound          (NULL    ),
  m_volume         (1.0f    ),
  m_freeCallback   (NULL    ),
  m_freeCallbackArg(NULL    )
{
  m_filename = filename;
  
  Load();
}

CAESoundWrapper::~CAESoundWrapper()
{
  if (m_freeCallback)
    m_freeCallback(this, m_freeCallbackArg);
}

void CAESoundWrapper::UnLoad()
{
  CExclusiveLock lock(m_lock);

  if (!m_sound) return;
  if (m_ae)
  {
    m_sound->SetFreeCallback(NULL, NULL);
    m_ae->FreeSound(m_sound);
  }
  m_sound = NULL;
  m_ae    = NULL;
}

void CAESoundWrapper::Load()
{
  CExclusiveLock lock(m_lock);

  m_ae = AE.GetEngine();
  
  if (m_ae   ) m_sound = m_ae->GetSound(m_filename);
  if (m_sound)
  {
    m_sound->SetVolume(m_volume);
    m_sound->SetFreeCallback(StaticSoundOnFree, this);
  }

}

void CAESoundWrapper::StaticSoundOnFree(IAESound *sender, void *arg)
{
  /* delete ourself */
  CAESoundWrapper *s = (CAESoundWrapper*)arg;
  delete s;
}

void CAESoundWrapper::Play()
{
  CSharedLock lock(m_lock);
  if (m_ae && m_sound)
    m_ae->PlaySound(m_sound);
}

void CAESoundWrapper::Stop()
{
  CSharedLock lock(m_lock);

  if (m_ae && m_sound)
    m_ae->StopSound(m_sound);
}

bool CAESoundWrapper::IsPlaying()
{
  bool play = false;

  CSharedLock lock(m_lock);
  if(m_sound) play = m_sound->IsPlaying();
  return play;
}

void CAESoundWrapper::SetVolume(float volume)
{
  CSharedLock lock(m_lock);
  if(m_sound) m_sound->SetVolume(volume);
  m_volume = volume;
}

float CAESoundWrapper::GetVolume()
{
  float volume = 0.0f;

  CSharedLock lock(m_lock);
  if(m_sound) m_sound->GetVolume();
  return volume;
}

void CAESoundWrapper::SetFreeCallback(AECBFunc *callback, void *arg)
{
  m_freeCallback    = callback;
  m_freeCallbackArg = arg;
}

