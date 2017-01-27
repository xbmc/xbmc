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
#include "AudioDecoder.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/TagLoaderTagLib.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

namespace ADDON
{

CAudioDecoder::CAudioDecoder(CAddonInfo addonInfo)
  : CAddonDll(std::move(addonInfo))
{
  m_extension = AddonInfo()->GetValue("@extension").asString();
  m_mimetype = AddonInfo()->GetValue("@mimetype").asString();
  m_tags = AddonInfo()->GetValue("@tags").asBoolean();
  m_tracks = AddonInfo()->GetValue("@tracks").asBoolean();
  m_CodecName = AddonInfo()->GetValue("@name").asString();
  m_strExt = AddonInfo()->GetValue("@name").asString() + "stream";
}

CAudioDecoder::~CAudioDecoder()
{
  DeInit();
}

bool CAudioDecoder::Create()
{
  if (CAddonDll::Create(ADDON_INSTANCE_AUDIODECODER, &m_struct, &m_info) == ADDON_STATUS_OK)
    return true;

  return false;
}

bool CAudioDecoder::Init(const CFileItem& file, unsigned int filecache)
{
  if (!Initialized())
    return false;

  // for replaygain
  CTagLoaderTagLib tag;
  tag.Load(file.GetPath(), XFILE::CMusicFileDirectory::m_tag, NULL);

  int channels;
  int sampleRate;

  m_context = m_struct.Init(file.GetPath().c_str(), filecache,
                              &channels, &sampleRate,
                              &m_bitsPerSample, &m_TotalTime,
                              &m_bitRate, &m_format.m_dataFormat, &m_channel);

  m_format.m_sampleRate = sampleRate;
  if (m_channel)
    m_format.m_channelLayout = CAEChannelInfo(m_channel);
  else
    m_format.m_channelLayout = CAEUtil::GuessChLayout(channels);

  return (m_context != NULL);
}

int CAudioDecoder::ReadPCM(uint8_t* buffer, int size, int* actualsize)
{
  if (!Initialized())
    return 0;

  return m_struct.ReadPCM(m_context, buffer, size, actualsize);
}

bool CAudioDecoder::Seek(int64_t time)
{
  if (!Initialized())
    return false;

  m_struct.Seek(m_context, time);
  return true;
}

void CAudioDecoder::DeInit()
{
  if (!Initialized())
    return;

  m_struct.DeInit(m_context);
}

bool CAudioDecoder::Load(const std::string& fileName,
                         MUSIC_INFO::CMusicInfoTag& tag,
                         MUSIC_INFO::EmbeddedArt* art)
{
  if (!Initialized())
    return false;

  char title[256];
  char artist[256];
  int length;
  if (m_struct.ReadTag(fileName.c_str(), title, artist, &length))
  {
    tag.SetTitle(title);
    tag.SetArtist(artist);
    tag.SetDuration(length);
    return true;
  }

  return false;
}

int CAudioDecoder::GetTrackCount(const std::string& strPath)
{
  if (!Initialized())
    return 0;

  int result = m_struct.TrackCount(strPath.c_str());

  if (result > 1 && !Load(strPath, XFILE::CMusicFileDirectory::m_tag, NULL))
    return 0;

  XFILE::CMusicFileDirectory::m_tag.SetLoaded(true);
  return result;
}

void CAudioDecoder::Destroy()
{
  CAddonDll::Destroy();
}

} /*namespace ADDON*/

