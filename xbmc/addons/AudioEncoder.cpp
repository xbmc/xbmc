/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioEncoder.h"

namespace ADDON
{

CAudioEncoder::CAudioEncoder(BinaryAddonBasePtr addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_AUDIOENCODER, addonInfo)
{
  m_struct = {{ 0 }};
}

bool CAudioEncoder::Init(AddonToKodiFuncTable_AudioEncoder& callbacks)
{
  m_struct.toKodi = callbacks;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK || !m_struct.toAddon.start)
    return false;

  return m_struct.toAddon.start(&m_struct,
                                m_iInChannels,
                                m_iInSampleRate,
                                m_iInBitsPerSample,
                                m_strTitle.c_str(),
                                m_strArtist.c_str(),
                                m_strAlbumArtist.c_str(),
                                m_strAlbum.c_str(),
                                m_strYear.c_str(),
                                m_strTrack.c_str(),
                                m_strGenre.c_str(),
                                m_strComment.c_str(),
                                m_iTrackLength);
}

int CAudioEncoder::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  if (m_struct.toAddon.encode)
    return m_struct.toAddon.encode(&m_struct, nNumBytesRead, pbtStream);
  return 0;
}

bool CAudioEncoder::Close()
{
  bool ret = false;
  if (m_struct.toAddon.finish)
    ret = m_struct.toAddon.finish(&m_struct);

  DestroyInstance();
  m_struct = {{ 0 }};

  return ret;
}

} /*namespace ADDON*/

