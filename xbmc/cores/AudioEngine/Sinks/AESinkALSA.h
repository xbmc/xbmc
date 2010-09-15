#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AESink.h"
#include <stdint.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "CriticalSection.h"

class CAESinkALSA : public IAESink
{
public:
  virtual const char *GetName() { return "ALSA"; }

  CAESinkALSA();
  virtual ~CAESinkALSA();

  virtual bool Initialize  (AEAudioFormat &format, CStdString &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const CStdString device);

  virtual void         Stop            ();
  virtual float        GetDelay        ();
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames);
  static void          EnumerateDevices(AEDeviceList &devices, bool passthrough);
private:
  unsigned int GetChannelCount(const AEAudioFormat format);
  CStdString   GetDeviceUse   (const AEAudioFormat format, CStdString device);

  AEAudioFormat     m_initFormat;
  AEAudioFormat     m_format;
  bool              m_passthrough;
  enum AEChannel   *m_channelLayout;
  CStdString        m_device;
  snd_pcm_t        *m_pcm;
  CCriticalSection  m_runLock;
  int               m_timeout;

  snd_pcm_format_t AEFormatToALSAFormat(const enum AEDataFormat format);

  bool InitializeHW(AEAudioFormat &format);
  bool InitializeSW(AEAudioFormat &format);

  static bool SoundDeviceExists(const CStdString& device);
  static void GenSoundLabel(AEDeviceList& devices, CStdString sink, CStdString card, CStdString readableCard);
};

