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

#include <list>
#include <map>

#include "system.h"
#include "threads/CriticalSection.h"

#include "AEAudioFormat.h"
#include "AEStream.h"
#include "AESound.h"

typedef std::pair<CStdString, CStdString> AEDevice;
typedef std::vector<AEDevice> AEDeviceList;

/* forward declarations */
class IAEStream;
class IAESound;
class IAEPacketizer;

/**
 * IAE Interface
 */
class IAE
{
protected:
  friend class CAEWrapper;

  IAE() {}
  virtual ~IAE() {}

  /**
   * Initializes the AudioEngine, called by CFactory when it is time to initialize the audio engine.
   * Do not call this directly, CApplication will call this when it is ready
   */
  virtual bool Initialize() = 0;
public:

  /**
   * Callback to alert the AudioEngine of setting changes
   * @param setting The name of the setting that was changed
   */
  virtual void OnSettingsChange(CStdString setting) {}

  /**
   * Returns the current master volume level of the AudioEngine
   * @return The volume level between 0.0 and 1.0
   */
  virtual float GetVolume() = 0;

  /**
   * Sets the master volume level of the AudioEngine
   * @param volume The new volume level between 0.0 and 1.0
   */
  virtual void  SetVolume(float volume) = 0;

  /**
   * Creates and returns a new IAEStream in the format specified, this function should never fail
   * @param dataFormat The data format the incoming audio will be in (eg, AE_FMT_S16LE)
   * @param sampleRate The sample rate of the audio data (eg, 48000)
   * @param channelCount The number of channels in the audio data
   * @param channelLayout The order of the channels in the audio data
   * @param options A bit field of stream options (see: enum AEStreamOptions)
   * @return a new IAEStream that will accept data in the requested format
   */
  virtual IAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0) = 0;

  /**
   * This method will re-configure the specified stream to accept data in the specified format.
   * If the AudioEngine does not support stream reconfiguration, it will drain and destroy the old stream and return a new stream in the requested format.
   * @param stream The stream to be altered
   * @param dataFormat The data format the incoming audio will be in (eg, AE_FMT_S16LE)
   * @param sampleRate The sample rate of the audio data (eg, 48000)
   * @param channelCount The number of channels in the audio data
   * @param channelLayout The order of the channels in the audio data
   * @param options A bit field of stream options (see: enum AEStreamOptions)
   * @return the stream supplied, or a new IAEStream that will accept data in the requested format if the AudioEngine does not support stream reconfiguration.
   */
  virtual IAEStream *AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0) = 0;

  /**
   * This method will remove the specifyed stream from the engine.
   * For OSX/IOS this is essential to reconfigure the audio output.
   * @param stream The stream to be altered
   * @return NULL
   */
  virtual IAEStream *FreeStream(IAEStream *stream) = 0;

  /**
   * Creates a new IAESound that is ready to play the specified file
   * @param file The WAV file to load, this supports XBMC's VFS
   * @return A new IAESound if the file could be loaded, otherwise NULL
   */
  virtual IAESound *GetSound(CStdString file) = 0;

  /**
   * Free the supplied IAESound object
   * @param sound The IAESound object to free
   */
  virtual void FreeSound(IAESound *sound) = 0;

  /**
   * Play the supplied IAESound object
   * @param sound The IAESound object to play
   */
  virtual void PlaySound(IAESound *sound) = 0;

  /**
   * Stop play the supplied IAESound object
   * @param sound The IAESound object to stop playing
   */
  virtual void StopSound(IAESound *sound) = 0;

  /**
   * Callback by CApplication for Garbage Collection. This method is called by CApplication every 500ms and can be used to clean up and free no-longer used resources.
   */
  virtual void GarbageCollect() = 0;

  /**
   * Enumerate the supported audio output devices
   * @param devices The device list to append supported devices to
   * @param passthrough True if only passthrough devices are wanted
   */
  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough) = 0;

  /**
   * Returns true if the AudioEngine supports AE_FMT_RAW streams for use with formats such as IEC61937
   * @see CAEPackIEC61937::CAEPackIEC61937()
   * @returns true if the AudioEngine is capable of RAW output
   */
  virtual bool SupportsRaw() { return false; }
};

