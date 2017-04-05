/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "games/addons/GameClientCallbacks.h"

#include <memory>

class CDVDAudioCodec;
class CProcessInfo;
class IAEStream;

namespace GAME
{
  class CRetroPlayerAudio : public IGameAudioCallback
  {
  public:
    CRetroPlayerAudio(CProcessInfo& processInfo);
    virtual ~CRetroPlayerAudio();

    // implementation of IGameAudioCallback
    virtual unsigned int NormalizeSamplerate(unsigned int samplerate) const override;
    virtual bool OpenPCMStream(AEDataFormat format, unsigned int samplerate, const CAEChannelInfo& channelLayout) override;
    virtual bool OpenEncodedStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout) override;
    virtual void AddData(const uint8_t* data, unsigned int size) override;
    virtual void CloseStream() override;

    void Enable(bool bEnabled) { m_bAudioEnabled = bEnabled; }

  private:
    CProcessInfo& m_processInfo;
    IAEStream* m_pAudioStream;
    std::unique_ptr<CDVDAudioCodec> m_pAudioCodec;
    bool       m_bAudioEnabled;
  };
}
