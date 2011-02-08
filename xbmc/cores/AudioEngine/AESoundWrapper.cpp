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
  m_lock.EnterExclusive();

  if (!m_sound) return;
  IAE *ae = AE.GetEngine();
  if (ae) ae->FreeSound(m_sound);
  m_sound = NULL;

  m_lock.LeaveExclusive();
}

void CAESoundWrapper::Load()
{
  m_lock.EnterExclusive();

  IAE *ae = AE.GetEngine();
  if (ae     ) m_sound = ae->GetSound(m_filename);
  if (m_sound) m_sound->SetVolume(m_volume);

  m_lock.LeaveExclusive();
}

void CAESoundWrapper::Play()
{
  m_lock.EnterShared();
  m_sound->Play();
  m_lock.LeaveShared();
}

void CAESoundWrapper::Stop()
{
  m_lock.EnterShared();
  m_sound->Stop();
  m_lock.LeaveShared();
}

bool CAESoundWrapper::IsPlaying()
{
  m_lock.EnterShared();
  bool play = m_sound->IsPlaying();
  m_lock.LeaveShared();
  return play;
}

void CAESoundWrapper::SetVolume(float volume)
{
  m_lock.EnterShared();
  m_sound->SetVolume(volume);
  m_volume = volume;
  m_lock.LeaveShared();
}

float CAESoundWrapper::GetVolume()
{
  m_lock.EnterShared();
  float volume = m_sound->GetVolume();
  m_lock.LeaveShared();
  return volume;
}

