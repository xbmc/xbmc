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

CAudioDecoder::CAudioDecoder(const cp_extension_t* ext)
 : AudioDecoderDll(ext),
   m_extension(CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@extension")),
   m_mimetype(CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@mimetype")),
   m_context(NULL),
   m_tags(CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@tags") == "true"),
   m_tracks(CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@tracks") == "true"),
   m_channel(NULL)
{
  m_CodecName = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@name");
  m_strExt = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@name")+"stream";
}

AddonPtr CAudioDecoder::Clone() const
{
  // Copy constructor is generated by compiler and calls parent copy constructor
  return AddonPtr(new CAudioDecoder(*this));
}

bool CAudioDecoder::Init(const std::string& strFile, unsigned int filecache)
{
  if (!Initialized())
    return false;

  // for replaygain
  CTagLoaderTagLib tag;
  tag.Load(strFile, XFILE::CMusicFileDirectory::m_tag, NULL);

  int channels;
  int sampleRate;

  m_context = m_pStruct->Init(strFile.c_str(), filecache,
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

  return m_pStruct->ReadPCM(m_context, buffer, size, actualsize);
}

int64_t CAudioDecoder::Seek(int64_t time)
{
  if (!Initialized())
    return 0;

  return m_pStruct->Seek(m_context, time);
}

void CAudioDecoder::DeInit()
{
  if (!Initialized())
    return;

  m_pStruct->DeInit(m_context);
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
  if (m_pStruct->ReadTag(fileName.c_str(), title, artist, &length))
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

  int result = m_pStruct->TrackCount(strPath.c_str());

  if (result > 1 && !Load(strPath, XFILE::CMusicFileDirectory::m_tag, NULL))
    return 0;

  XFILE::CMusicFileDirectory::m_tag.SetLoaded(true);
  return result;
}

CAEChannelInfo CAudioDecoder::GetChannelInfo()
{
  return m_format.m_channelLayout;
}

void CAudioDecoder::Destroy()
{
  AudioDecoderDll::Destroy();
}

} /*namespace ADDON*/

