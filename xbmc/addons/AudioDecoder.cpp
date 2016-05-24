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

std::unique_ptr<CAudioDecoder> CAudioDecoder::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  std::string extension = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@extension");
  std::string mimetype = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@mimetype");
  bool tags = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@tags") == "true";
  bool tracks = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@tracks") == "true";
  std::string codecName = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@name");
  std::string strExt = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@name") + "stream";

  return std::unique_ptr<CAudioDecoder>(new CAudioDecoder(std::move(props), std::move(extension),
      std::move(mimetype), tags, tracks, std::move(codecName), std::move(strExt)));
}

CAudioDecoder::CAudioDecoder(AddonProps props, std::string extension, std::string mimetype,
    bool tags, bool tracks, std::string codecName, std::string strExt)
    : AudioDecoderDll(std::move(props)), m_extension(extension), m_mimetype(mimetype),
      m_context(nullptr), m_tags(tags), m_tracks(tracks), m_channel(nullptr)
{
  m_CodecName = std::move(codecName);
  m_strExt = std::move(strExt);
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

  m_context = m_pStruct->Init(file.GetPath().c_str(), filecache,
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

bool CAudioDecoder::Seek(int64_t time)
{
  if (!Initialized())
    return false;

  m_pStruct->Seek(m_context, time);
  return true;
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

