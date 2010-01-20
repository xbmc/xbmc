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

#ifdef __APPLE__

#include "CoreAudioSoundManager.h"
#include "PlatformDefs.h"
#include "Log.h"

struct core_audio_sound
{
  UInt32 ref_count;
  UInt32 play_count;
  UInt32 total_frames;
  AudioBufferList* buffer_list;
};

typedef clock_t core_audio_timestamp;

struct core_audio_sound_event
{
  core_audio_sound* sound;
  core_audio_timestamp play_time; // Don't play before
  core_audio_timestamp expire_time; // Stop playing no later than
  UInt32 offset;
};

CCoreAudioSoundManager::CCoreAudioSoundManager() :
  m_pCurrentEvent(NULL),
  m_RestartOutputUnit(false)
{

}

CCoreAudioSoundManager::~CCoreAudioSoundManager()
{
  Stop();
}

bool CCoreAudioSoundManager::Initialize(CStdString deviceName)
{
  // Attempt to find the configured output device
  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(deviceName);
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    CLog::Log(LOGWARNING, "CCoreAudioSoundManager::Initialize: Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device.
    {
      CLog::Log(LOGERROR, "CCoreAudioSoundManager::Initialize: No default device found. Unable to initialize.");
      return false;
    }
  }
  
  // Attach our output object to the device
  if (!m_OutputDevice.Open(outputDevice))
    return false;
  
  // Create the Output AudioUnit Component  
  if (!m_OutputUnit.Open(kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
    return false;
  
  // Hook the Ouput AudioUnit to the selected device
  if (!m_OutputUnit.SetCurrentDevice(outputDevice))
    return false;
  
  // Set up output format (32-bit float, 2-channel, non-interleaved)
  m_OutputFormat.mSampleRate = 44100.0;
  m_OutputFormat.mChannelsPerFrame  = 2;
  m_OutputFormat.mFormatID = kAudioFormatLinearPCM;
  m_OutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
  m_OutputFormat.mBitsPerChannel = 8 * sizeof(float);
  m_OutputFormat.mBytesPerFrame =  sizeof(float);
  m_OutputFormat.mFramesPerPacket = 1;
  m_OutputFormat.mBytesPerPacket = m_OutputFormat.mBytesPerFrame * m_OutputFormat.mFramesPerPacket;  
  if (!m_OutputUnit.SetInputFormat(&m_OutputFormat))
    return false;
  
  // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
  // If this is not called, there is no guarantee that the callback will ever be called.
  UInt32 bufferFrames = m_OutputUnit.GetBufferFrameSize(); // Size of the output buffer, in Frames
  if (!m_OutputUnit.SetMaxFramesPerSlice(bufferFrames))
    return false;
  
  // Setup the callback function that the AudioUnit will use to request data	
  if (!m_OutputUnit.SetRenderProc(RenderCallback, this))
    return false;
  
  // Initialize the Output AudioUnit
  if (!m_OutputUnit.Initialize())
    return false;  
  
  // All went as planned
  return true;
}

void CCoreAudioSoundManager::Run()
{
  AudioDeviceAddPropertyListener(m_OutputDevice.GetId(), 0, false, kAudioDevicePropertyHogMode, PropertyChangeCallback, this); // If this fails, there is not a whole lot to be done
  m_OutputUnit.Start();
  CLog::Log(LOGDEBUG, "CCoreAudioSoundManager::Run: SoundManager is now running.");
}

void CCoreAudioSoundManager::Stop()
{
  if (!m_OutputUnit.IsRunning())
    return;
  
  m_OutputUnit.Stop();
  AudioDeviceRemovePropertyListener(m_OutputDevice.GetId(), 0, false, kAudioDevicePropertyHogMode, PropertyChangeCallback); // No longer need to know if the device is hogged
  // TODO: Clean up leftover event(s)
  CLog::Log(LOGDEBUG, "CCoreAudioSoundManager::Stop: SoundManager has been stopped.");
}

CoreAudioSoundRef CCoreAudioSoundManager::RegisterSound(const CStdString& fileName)
{  
  return LoadSoundFromFile(fileName);
}

void CCoreAudioSoundManager::UnregisterSound(CoreAudioSoundRef soundRef)
{
  if (!soundRef)
    return;
  
  core_audio_sound* pSound = soundRef;
  if (!--pSound->ref_count && !pSound->play_count) // Release the caller's reference and see if the object is ready to be freed
  {
    delete pSound->buffer_list;
    delete pSound;
  }
}

void CCoreAudioSoundManager::PlaySound(CoreAudioSoundRef soundRef)
{
  if (!soundRef)
    return;
  
  // Restart the output AudioUnit if necessary
  if (m_RestartOutputUnit) // This flag is set by the property change notification handler to work around an OSX 10.4 quirk/bug.
  {                        // It will re-init the callback thread for the AudioUnit after hog-mode has been released.
    m_OutputUnit.Stop();
    m_OutputUnit.Start();
    m_RestartOutputUnit = false;
  }
  
  if (!m_pCurrentEvent)
  {
    // TODO: Build queue and/or mix parallel sounds. For now just boot the currently playing sound and replace it.
    // TODO: Not thread-safe. Going to cause trouble.
    
    // Create a new 'event' object to hold playback info
    core_audio_sound_event* pEvent = new core_audio_sound_event;
    pEvent->sound = soundRef;
    pEvent->offset = 0; // Start at the beginning
    
    pEvent->sound->play_count++;
    m_pCurrentEvent = pEvent; // TODO: This can create a memory leak if we already had one
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////////////////
OSStatus CCoreAudioSoundManager::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  if (m_pCurrentEvent) // Do we have a sound event queued to play?
  {
    core_audio_sound_event* pEvent = m_pCurrentEvent; // Grab reference in case the event gets changed while we work 
    UInt32 framesAvailable = pEvent->sound->total_frames - pEvent->offset;
    // TODO: Find a better way to manage time references
    if (pEvent->offset < pEvent->sound->total_frames) // Do we have any (unexpired) data left to give.
    {
      if (framesAvailable < inNumberFrames) // Do we have enough data to fill the complete request
        inNumberFrames = framesAvailable; // Truncate request to available data length
      for (int i = 0; i < ioData->mNumberBuffers; i++)
      {
        UInt32 frameLen = m_OutputFormat.mBytesPerFrame;
        unsigned char* pIn = (unsigned char*)pEvent->sound->buffer_list->mBuffers[i].mData;
        memcpy(ioData->mBuffers[i].mData, &pIn[pEvent->offset * frameLen], inNumberFrames * frameLen); // Copy out the requested number of frames
        ioData->mBuffers[i].mDataByteSize = inNumberFrames * frameLen; // Update to reflect actual date
      }
      pEvent->offset += inNumberFrames; // Update position in current buffer
      if (m_pCurrentEvent != pEvent) // This sound has been canceled
        delete pEvent;
    }
    else // No (unexpired) data left in this buffer. Free it.
    {
      // TODO: reference counting needs work
      pEvent->sound->play_count--; // Allow the sound to be deleted. We are done with it.
      if (!pEvent->sound->play_count && !m_pCurrentEvent->sound->ref_count) // Try and help out if all references have been released
        UnregisterSound(pEvent->sound);
      delete pEvent;
      if (pEvent == m_pCurrentEvent)
        m_pCurrentEvent = NULL;
    }
  }
  else // No current buffer. notify caller and return.
  {
    ioData->mBuffers[0].mDataByteSize = ioData->mBuffers[0].mDataByteSize = 0;
  }
  return noErr;
}

OSStatus CCoreAudioSoundManager::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return ((CCoreAudioSoundManager*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData); // Hand over to instance memeber
}

OSStatus CCoreAudioSoundManager::PropertyChangeCallback(AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, void* inClientData)
{
  CCoreAudioSoundManager* pThis = (CCoreAudioSoundManager*)inClientData;
  pid_t hogPid = pThis->m_OutputDevice.GetHogStatus();
  if (hogPid > -1) // Device is hogged.
  {
    CLog::Log(LOGWARNING, "CCoreAudioSoundManager: Someone has hogged the output device. Stopping until it is released.");
    pThis->m_OutputUnit.Stop();
  }
  else // Device has been released.
  {
    CLog::Log(LOGWARNING, "CCoreAudioSoundManager: The output device has been released. Resuming.");
    pThis->m_RestartOutputUnit = true; // This will cause the PlaySound method to restart the AudioUnit before queuing up the next sound.
                                       // It is needed to work around a bug in the OSX 10.4 AudioUnit code that incorrectly reports a successful start.
  }
  return noErr;
}

core_audio_sound* CCoreAudioSoundManager::LoadSoundFromFile(const CStdString& fileName)
{
  FSRef fileRef;
  UInt32 size = 0;
  ExtAudioFileRef audioFile;
  OSStatus ret = FSPathMakeRef((const UInt8*) fileName.c_str(), &fileRef, false);
  ret = ExtAudioFileOpen(&fileRef, &audioFile);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to open source file (%s). Error = 0x%08x (%4.4s)", fileName.c_str(), ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }    
  
  core_audio_sound* pSound = new core_audio_sound;
  
  // Retrieve the format of the source file
  AudioStreamBasicDescription inputFormat;
  size = sizeof(inputFormat);
  ret = ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileDataFormat, &size, &inputFormat);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to fetch source file format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    delete pSound;
    return NULL;
  }  
  
  // Set up format conversion. This is the format that will be produced by Read/Write calls. 
  // Here we use the same format provided to the output AudioUnit
  ret = ExtAudioFileSetProperty(audioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(AudioStreamBasicDescription), &m_OutputFormat);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to set conversion format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    delete pSound;
    return NULL;
  }    
  
  // Retrieve the file size (in terms of the file's sample-rate, not the output sample-rate)
  UInt64 totalFrames;
  size = sizeof(totalFrames);
  ret = ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileLengthFrames, &size, &totalFrames);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to fetch source file size. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    delete pSound;
    return NULL;
  }  
  
  // Calculate the total number of converted frames to be read
  totalFrames *= (float)m_OutputFormat.mSampleRate / (float)inputFormat.mSampleRate; // TODO: Verify the accuracy of this
  
  // Allocate AudioBuffers
  UInt32 channelCount = m_OutputFormat.mChannelsPerFrame;
  pSound->buffer_list = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer) * (channelCount - kVariableLengthArray));
  pSound->buffer_list->mNumberBuffers = channelCount; // One buffer per channel for deinterlaced pcm
  float* buffers = (float*)calloc(1, sizeof(float) * totalFrames * channelCount);
  for(int i = 0; i < channelCount; i++)
  {
    pSound->buffer_list->mBuffers[i].mNumberChannels = 1; // One channel per buffer for deinterlaced pcm
    pSound->buffer_list->mBuffers[i].mData = buffers + (totalFrames * i);
    pSound->buffer_list->mBuffers[i].mDataByteSize = totalFrames * sizeof(float);
  }
  
  // Read the entire file
  // TODO: Should we limit the total file length?
  UInt32 readFrames = totalFrames;
  ret = ExtAudioFileRead(audioFile, &readFrames, pSound->buffer_list);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to read from file (%s). Error = 0x%08x (%4.4s)", fileName.c_str(), ret, CONVERT_OSSTATUS(ret));
    delete pSound;
    return NULL;
  }  
  pSound->total_frames = readFrames; // Store the actual number of frames read from the file. Rounding errors in calcuating the converted number of frames can truncate the read.
  
  // TODO: What do we do with files with more than 2 channels. Currently we just copy the first two and dump the rest.
  if (inputFormat.mChannelsPerFrame == 1) // Copy Left channel into Right if the source file is Mono
    memcpy(pSound->buffer_list->mBuffers[1].mData, pSound->buffer_list->mBuffers[0].mData, pSound->buffer_list->mBuffers[0].mDataByteSize);
  
  ret = ExtAudioFileDispose(audioFile); // Close the file. We have what we need. Not a lot to be done on failure.
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioSoundManager::LoadSoundFromFile: Unable to close file (%s). Error = 0x%08x (%4.4s)", fileName.c_str(), ret, CONVERT_OSSTATUS(ret));

  pSound->ref_count = 1; // The caller holds a reference to this object now
  pSound->play_count = 0;
  
  return pSound;  
}

#endif