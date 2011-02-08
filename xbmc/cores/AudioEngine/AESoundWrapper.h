#pragma once
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

#include "utils/StdString.h"
#include "threads/SharedSection.h"
#include "AESound.h"

class CAESoundWrapper : public IAESound
{
protected:
  friend class CAEWrapper;
  CAESoundWrapper(const CStdString &filename);
  virtual ~CAESoundWrapper();

  /* Un/Load the sound object, called when the AE is changed */ 
  void UnLoad();
  void Load();

public:
  virtual void Play();
  virtual void Stop();
  virtual bool IsPlaying();
  virtual void SetVolume(float volume);
  virtual float GetVolume();

private:
  CStdString     m_filename;
  CSharedSection m_lock;
  IAESound      *m_sound;
  float          m_volume;
};

