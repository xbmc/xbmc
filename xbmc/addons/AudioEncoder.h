/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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
#pragma once

#include "AddonDll.h"
#include "include/xbmc_audioenc_types.h"
#include "cdrip/IEncoder.h"

typedef DllAddon<AudioEncoder, AUDIOENC_PROPS> DllAudioEncoder;
namespace ADDON
{
  typedef CAddonDll<DllAudioEncoder,
                    AudioEncoder, AUDIOENC_PROPS> AudioEncoderDll;

  class CAudioEncoder : public AudioEncoderDll, public IEncoder
  {
  public:
    CAudioEncoder(const AddonProps &props) : AudioEncoderDll(props), m_context{nullptr} {};
    CAudioEncoder(const cp_extension_t *ext);
    virtual ~CAudioEncoder() {}
    virtual AddonPtr Clone() const;

    // Things that MUST be supplied by the child classes
    bool Init(audioenc_callbacks &callbacks);
    int Encode(int nNumBytesRead, uint8_t* pbtStream);
    bool Close();
    void Destroy();

    const std::string extension;

  private:
    void *m_context; ///< audio encoder context
  };

} /*namespace ADDON*/
