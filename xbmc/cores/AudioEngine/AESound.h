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

#include "StdString.h"

class IAESound
{
public:
  /* this should NEVER be called directly, use AE.GetSound */
  IAESound(const CStdString &filename) {}
  virtual ~IAESound() {}

  virtual void DeInitialize() = 0;
  virtual bool Initialize() = 0;

  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual bool IsPlaying() = 0;

  virtual void         SetVolume(float volume) = 0;
  virtual float        GetVolume() = 0;
  virtual unsigned int GetSampleCount() = 0;
  virtual float*       GetSamples    () = 0;
};

