/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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

#ifndef __CORE_AUDIO_SOUND_MANAGER_H__
#define __CORE_AUDIO_SOUND_MANAGER_H__

#include "CoreAudio.h"

struct core_audio_sound;
struct core_audio_sound_event;

typedef core_audio_sound* CoreAudioSoundRef; // Opaque reference for clients

class CCoreAudioSoundManager
{
public:
  CCoreAudioSoundManager();
  ~CCoreAudioSoundManager();
  bool Initialize(CStdString deviceName);
  void Run();
  void Stop();
  CoreAudioSoundRef RegisterSound(const CStdString& fileName);
  void UnregisterSound(CoreAudioSoundRef soundRef);
  void PlaySound(CoreAudioSoundRef soundRef);
protected:
  core_audio_sound_event* m_pCurrentEvent;

  CCoreAudioDevice m_OutputDevice;
  CCoreAudioUnit m_OutputUnit;
  AudioStreamBasicDescription m_OutputFormat;
  bool m_RestartOutputUnit;
  
  core_audio_sound* LoadSoundFromFile(const CStdString& fileName);
  static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
  OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);  
  static OSStatus PropertyChangeCallback(AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, void* inClientData);
};

#endif // __CORE_AUDIO_SOUND_MANAGER_H__
