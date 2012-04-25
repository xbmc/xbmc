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

#ifndef __COREAUDIOSOURCE_H__
#define __COREAUDIOSOURCE_H__

#include "AEAudioFormat.h"
#include "Interfaces/AE.h"
#include "utils/StdString.h"
#include <AudioUnit/AudioUnit.h>

class ICoreAudioSource;

/**
 * ICoreAudioSource Interface
 */
class ICoreAudioSource
{
private:
  std::string        m_inputName;
  AudioUnitElement  m_inputBus;
public:
  // Function to request rendered data from a data source
  virtual OSStatus Render(AudioUnitRenderActionFlags* actionFlags, 
                          const AudioTimeStamp* pTimeStamp, 
                          UInt32 busNumber, 
                          UInt32 frameCount, 
                          AudioBufferList* pBufList) = 0;
  //std::string InputName() { return m_inputName; };
  //void InputName(std::string inputName) { m_inputName = inputName; };

  //AudioUnitElement InputBus() { return m_inputBus; };
  //void InputBus(AudioUnitElement inputBus) { m_inputBus = m_inputBus; };
};
#endif
