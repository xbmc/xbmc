/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#include "AudioRendererFactory.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "NullDirectSound.h"

#ifdef HAS_PULSEAUDIO
#include "PulseAudioDirectSound.h"
#endif

#ifdef _WIN32
#include "Win32WASAPI.h"
#include "Win32DirectSound.h"
#endif
#if defined(__APPLE__)
#if defined(__arm__)
  #include "IOSAudioRenderer.h"
#else
  #include "CoreAudioRenderer.h"
#endif
#elif defined(USE_ALSA)
#include "ALSADirectSound.h"
#endif

#define ReturnOnValidInitialize(rendererName)    \
{                                                \
  if (audioSink->Initialize(pCallback, device, iChannels, channelMap, uiSamplesPerSec, uiBitsPerSample, bResample, bIsMusic, encoded)) \
  {                                              \
    CLog::Log(LOGDEBUG, "%s::Initialize"         \
      " - Channels: %i"                          \
      " - SampleRate: %i"                        \
      " - SampleBit: %i"                         \
      " - Resample %s"                           \
      " - IsMusic %s"                            \
      " - IsPassthrough %d"                      \
      " - audioDevice: %s",                      \
      rendererName,                              \
      iChannels,                                 \
      uiSamplesPerSec,                           \
      uiBitsPerSample,                           \
      bResample ? "true" : "false",              \
      bIsMusic ? "true" : "false",               \
      encoded,                                   \
      device.c_str()                             \
    ); \
    return audioSink;                      \
  }                                        \
  else                                     \
  {                                        \
    audioSink->Deinitialize();             \
    delete audioSink;                      \
    audioSink = NULL;                      \
  }                                        \
}

#define CreateAndReturnOnValidInitialize(rendererClass) \
{ \
  audioSink = new rendererClass(); \
  ReturnOnValidInitialize(#rendererClass); \
}

#define ReturnNewRenderer(rendererClass) \
{ \
  renderer = #rendererClass; \
  return new rendererClass(); \
}

/* windows channel order */
static const enum PCMChannels dsound_default_channel_layout[][8] =
{
  {PCM_FRONT_CENTER},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT},
  {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_SIDE_LEFT, PCM_SIDE_RIGHT}
};

IAudioRenderer* CAudioRendererFactory::Create(IAudioCallback* pCallback, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, IAudioRenderer::EEncoded encoded)
{
  IAudioRenderer* audioSink = NULL;
  CStdString renderer;

  if(channelMap == NULL)
  {
    CLog::Log(LOGINFO, "CAudioRendererFactory: no input channel map specified assume windows\n");
    channelMap = (enum PCMChannels *)dsound_default_channel_layout[iChannels - 1];
  }

  CStdString deviceString, device;
  if (encoded)
  {
#if defined(_LINUX) && !defined(__APPLE__)
    deviceString = g_guiSettings.GetString("audiooutput.passthroughdevice");
    if (deviceString.Equals("custom"))
      deviceString = g_guiSettings.GetString("audiooutput.custompassthrough");
#else
    // osx/win platforms do not have an "audiooutput.passthroughdevice" setting but can do passthrough
    deviceString = g_guiSettings.GetString("audiooutput.audiodevice");
#endif
  }
  else
  {
    deviceString = g_guiSettings.GetString("audiooutput.audiodevice");
    if (deviceString.Equals("custom"))
      deviceString = g_guiSettings.GetString("audiooutput.customdevice");
  }
  int iPos = deviceString.Find(":");
  if (iPos > 0)
  {
    audioSink = CreateFromUri(deviceString.Left(iPos), renderer);
    if (audioSink)
    {
      device = deviceString.Right(deviceString.length() - iPos - 1);
      ReturnOnValidInitialize(renderer.c_str());

#ifdef _WIN32
      //If WASAPI failed try DirectSound.
      if(deviceString.Left(iPos).Equals("wasapi"))
      {
        audioSink = CreateFromUri("directsound", renderer);
        ReturnOnValidInitialize(renderer.c_str());
      }
#endif

      CreateAndReturnOnValidInitialize(CNullDirectSound);
      /* should never get here */
      assert(false);
    }
  }
  CLog::Log(LOGINFO, "AudioRendererFactory: %s not a explicit device, trying to autodetect.", device.c_str());

  device = deviceString;

/* First pass creation */
#ifdef HAS_PULSEAUDIO
  CreateAndReturnOnValidInitialize(CPulseAudioDirectSound);
#endif

/* incase none in the first pass was able to be created, fall back to os specific */
#ifdef WIN32
  CreateAndReturnOnValidInitialize(CWin32DirectSound);
#endif
#if defined(__APPLE__)
  #if defined(__arm__)
    CreateAndReturnOnValidInitialize(CIOSAudioRenderer);
  #else
    CreateAndReturnOnValidInitialize(CCoreAudioRenderer);
  #endif
#elif defined(USE_ALSA)
  CreateAndReturnOnValidInitialize(CALSADirectSound);
#endif

  CreateAndReturnOnValidInitialize(CNullDirectSound);
  /* should never get here */
  assert(false);
  return NULL;
}

void CAudioRendererFactory::EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough)
{
#ifdef HAS_PULSEAUDIO
  CPulseAudioDirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
#endif

#ifdef WIN32
  CWin32DirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
  CWin32WASAPI::EnumerateAudioSinks(vAudioSinks, passthrough);
#endif

#if defined(__APPLE__)
  #if !defined(__arm__)
    CCoreAudioRenderer::EnumerateAudioSinks(vAudioSinks, passthrough);
  #endif
#elif defined(USE_ALSA)
  CALSADirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
#endif
}

IAudioRenderer *CAudioRendererFactory::CreateFromUri(const CStdString &soundsystem, CStdString &renderer)
{
#ifdef HAS_PULSEAUDIO
  if (soundsystem.Equals("pulse"))
    ReturnNewRenderer(CPulseAudioDirectSound);
#endif

#ifdef WIN32
  if (soundsystem.Equals("wasapi"))
    ReturnNewRenderer(CWin32WASAPI)
  else if (soundsystem.Equals("directsound"))
    ReturnNewRenderer(CWin32DirectSound);
#endif

#if defined(__APPLE__)
  #if defined(__arm__)
    if (soundsystem.Equals("ioscoreaudio"))
      ReturnNewRenderer(CIOSAudioRenderer);
  #else
    if (soundsystem.Equals("coreaudio"))
      ReturnNewRenderer(CCoreAudioRenderer);
  #endif
#elif defined(USE_ALSA)
  if (soundsystem.Equals("alsa"))
    ReturnNewRenderer(CALSADirectSound);
#endif

  if (soundsystem.Equals("null"))
    ReturnNewRenderer(CNullDirectSound);

  return NULL;
}
