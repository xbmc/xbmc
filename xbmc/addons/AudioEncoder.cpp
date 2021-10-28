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
  m_struct.toAddon = new KodiToAddonFuncTable_AudioEncoder();
  m_struct.toKodi = new AddonToKodiFuncTable_AudioEncoder();
}

CAudioEncoder::~CAudioEncoder()
{
  // Delete "C" interface structures
  delete m_struct.toAddon;
  delete m_struct.toKodi;
}

bool CAudioEncoder::Init(AddonToKodiFuncTable_AudioEncoder& callbacks)
{
  *m_struct.toKodi = callbacks;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK || !m_struct.toAddon->start)
    return false;

  m_addonInstance = m_struct.toAddon->addonInstance;

  KODI_ADDON_AUDIOENCODER_INFO_TAG tag{};
  tag.channels = m_iInChannels;
  tag.samplerate = m_iInSampleRate;
  tag.bits_per_sample = m_iInBitsPerSample;
  tag.track_length = m_iTrackLength;
  tag.title = m_strTitle.c_str();
  tag.artist = m_strArtist.c_str();
  tag.album_artist = m_strAlbumArtist.c_str();
  tag.album = m_strAlbum.c_str();
  tag.release_date = m_strYear.c_str();
  tag.track = atoi(m_strTrack.c_str());
  tag.genre = m_strGenre.c_str();
  tag.comment = m_strComment.c_str();

  return m_struct.toAddon->start(m_addonInstance, &tag);
}

int CAudioEncoder::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  if (m_struct.toAddon->encode)
    return m_struct.toAddon->encode(m_addonInstance, nNumBytesRead, pbtStream);
  return 0;
}

bool CAudioEncoder::Close()
{
  bool ret = false;
  if (m_struct.toAddon->finish)
    ret = m_struct.toAddon->finish(m_addonInstance);

  DestroyInstance();

  return ret;
}

} /* namespace ADDON */
