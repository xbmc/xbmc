#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <list>
#include <map>
#include <vector>

#include "system.h"

#include "cores/AudioEngine/Utils/AEAudioFormat.h"

typedef std::pair<std::string, std::string> AEDevice;
typedef std::vector<AEDevice> AEDeviceList;

/* forward declarations */
class IAEStream;
class IAESound;
class IAEPacketizer;
class IAudioCallback;

/* sound options */
#define AE_SOUND_OFF    0 /* disable sounds */
#define AE_SOUND_IDLE   1 /* only play sounds while no streams are running */
#define AE_SOUND_ALWAYS 2 /* always play sounds */

/* config options */
#define AE_CONFIG_FIXED 1
#define AE_CONFIG_AUTO  2
#define AE_CONFIG_MATCH 3

enum AEQuality
{
  AE_QUALITY_UNKNOWN    = -1, /* Unset, unknown or incorrect quality level */
  AE_QUALITY_DEFAULT    =  0, /* Engine's default quality level */

  /* Basic quality levels */
  AE_QUALITY_LOW        = 20, /* Low quality level */
  AE_QUALITY_MID        = 30, /* Standard quality level */
  AE_QUALITY_HIGH       = 50, /* Best sound processing quality */

  /* Optional quality levels */
  AE_QUALITY_REALLYHIGH = 100, /* Uncompromised optional quality level,
                               usually with unmeasurable and unnoticeable improvement */ 
  AE_QUALITY_GPU        = 101, /* GPU acceleration */
};

/**
 * IAE Interface
 */
class IAE
{
protected:
  friend class CAEFactory;

  IAE() {}
  virtual ~IAE() {}

  /**
   * Returns true when it should be possible to initialize this engine, if it returns false
   * CAEFactory can possibly fall back to a different one
   */
  virtual bool CanInit() { return true; }

  /**
   * Initializes the AudioEngine, called by CFactory when it is time to initialize the audio engine.
   * Do not call this directly, CApplication will call this when it is ready
   */
  virtual bool Initialize() = 0;
public:
  /**
   * Called when the application needs to terminate the engine
   */
  virtual void Shutdown() { }

  /**
   * Suspends output and de-initializes sink
   * Used to avoid conflicts with external players or to reduce power consumption
   * @return True if successful
   */
  virtual bool Suspend() = 0;

  /**
   * Resumes output and re-initializes sink
   * Used to resume output from Suspend() state above
   * @return True if successful
   */
  virtual bool Resume() = 0;

  /**
   * Get the current Suspend() state
   * Used by players to determine if audio is being processed
   * Default is true so players drop audio or pause if engine unloaded
   * @return True if processing suspended
   */
  virtual bool IsSuspended() {return true;}
  
  /**
   * Callback to alert the AudioEngine of setting changes
   * @param setting The name of the setting that was changed
   */
  virtual void OnSettingsChange(const std::string& setting) {}

  /**
   * Returns the current master volume level of the AudioEngine
   * @return The volume level between 0.0 and 1.0
   */
  virtual float GetVolume() = 0;

  /**
   * Sets the master volume level of the AudioEngine
   * @param volume The new volume level between 0.0 and 1.0
   */
  virtual void SetVolume(const float volume) = 0;

  /**
   * Set the mute state (does not affect volume level value)
   * @param enabled The mute state
   */
  virtual void SetMute(const bool enabled) = 0;

  /**
   * Get the current mute state
   * @return The current mute state
   */
  virtual bool IsMuted() = 0;

  /**
   * Sets the sound mode
   * @param mode One of AE_SOUND_OFF, AE_SOUND_IDLE or AE_SOUND_ALWAYS
   */
  virtual void SetSoundMode(const int mode) = 0;

  /**
   * Creates and returns a new IAEStream in the format specified, this function should never fail
   * @param dataFormat The data format the incoming audio will be in (eg, AE_FMT_S16LE)
   * @param sampleRate The sample rate of the audio data (eg, 48000)
   * @prarm encodedSampleRate The sample rate of the encoded audio data if AE_IS_RAW(dataFormat)
   * @param channelLayout The order of the channels in the audio data
   * @param options A bit field of stream options (see: enum AEStreamOptions)
   * @return a new IAEStream that will accept data in the requested format
   */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo& channelLayout, unsigned int options = 0) = 0;

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
  virtual IAESound *MakeSound(const std::string &file) = 0;

  /**
   * Free the supplied IAESound object
   * @param sound The IAESound object to free
   */
  virtual void FreeSound(IAESound *sound) = 0;

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
   * Returns the default audio device
   * @param passthrough True if the default passthrough device is wanted
   * @return the default audio device
   */
  virtual std::string GetDefaultDevice(bool passthrough) { return "default"; }

  /**
   * Returns true if the AudioEngine supports AE_FMT_RAW streams for use with formats such as IEC61937
   * @see CAEPackIEC61937::CAEPackIEC61937()
   * @returns true if the AudioEngine is capable of RAW output
   */
  virtual bool SupportsRaw(AEDataFormat format, int samplerate) { return false; }

   /**
   * Returns true if the AudioEngine supports drain mode which is not streaming silence when idle
   * @returns true if the AudioEngine is capable of drain mode
   */
  virtual bool SupportsSilenceTimeout() { return false; }

  /**
   * Returns true if the AudioEngine is currently configured for stereo audio
   * @returns true if the AudioEngine is currently configured for stereo audio
   */
  virtual bool HasStereoAudioChannelCount() { return false; }

  /**
   * Returns true if the AudioEngine is currently configured for HD audio (more than 5.1)
   * @returns true if the AudioEngine is currently configured for HD audio (more than 5.1)
   */
  virtual bool HasHDAudioChannelCount() { return true; }

  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {}

  virtual void UnregisterAudioCallback() {}

  /**
   * Returns true if AudioEngine supports specified quality level
   * @return true if specified quality level is supported, otherwise false
   */
  virtual bool SupportsQualityLevel(enum AEQuality level) { return false; }

  /**
   * AE decides whether this settings should be displayed
   * @return true if AudioEngine wants to display this setting
   */
  virtual bool IsSettingVisible(const std::string &settingId) {return false; }

  /**
   * Instruct AE to keep configuration for a specified time
   * @param millis time for which old configuration should be kept
   */
  virtual void KeepConfiguration(unsigned int millis) {return; }

  /**
   * Instruct AE to re-initialize, e.g. after ELD change event
   */
  virtual void DeviceChange() {return; }
};

