/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioEncoder.h"

namespace ADDON
{

CAudioEncoder::CAudioEncoder(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_AUDIOENCODER, addonInfo)
{
  // Create "C" interface structures, used as own parts to prevent API problems on update
  m_struct.props = new AddonProps_AudioEncoder();
  m_struct.toAddon = new KodiToAddonFuncTable_AudioEncoder();
  m_struct.toKodi = new AddonToKodiFuncTable_AudioEncoder();
}

CAudioEncoder::~CAudioEncoder()
{
  // Delete "C" interface structures
  delete m_struct.toAddon;
  delete m_struct.toKodi;
  delete m_struct.props;
}

bool CAudioEncoder::Init(AddonToKodiFuncTable_AudioEncoder& callbacks)
{
  *m_struct.toKodi = callbacks;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK || !m_struct.toAddon->start)
    return false;

  return m_struct.toAddon->start(&m_struct, m_iInChannels, m_iInSampleRate, m_iInBitsPerSample,
                                 m_strTitle.c_str(), m_strArtist.c_str(), m_strAlbumArtist.c_str(),
                                 m_strAlbum.c_str(), m_strYear.c_str(), m_strTrack.c_str(),
                                 m_strGenre.c_str(), m_strComment.c_str(), m_iTrackLength);
}

int CAudioEncoder::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  if (m_struct.toAddon->encode)
    return m_struct.toAddon->encode(&m_struct, nNumBytesRead, pbtStream);
  return 0;
}

bool CAudioEncoder::Close()
{
  bool ret = false;
  if (m_struct.toAddon->finish)
    ret = m_struct.toAddon->finish(&m_struct);

  DestroyInstance();

  return ret;
}

} /*namespace ADDON*/

