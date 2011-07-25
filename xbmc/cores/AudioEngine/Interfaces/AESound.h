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
#include "AE.h"

class IAE;
class IAESound
{
protected:
  friend class IAE;
  IAESound(const CStdString &filename) {}
  virtual ~IAESound() {}

public:
  /* play the sound this object represents */
  virtual void Play() = 0;

  /* stop playing the sound this object represents */
  virtual void Stop() = 0;

  /* return true if the sound is currently playing */
  virtual bool IsPlaying() = 0;

  /* set the playback volume of this sound */
  virtual void SetVolume(float volume) = 0;

  /* get the current playback volume of this sound */
  virtual float GetVolume() = 0;
};

