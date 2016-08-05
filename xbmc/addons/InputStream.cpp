/*
 *      Copyright (C) 2016 Team Kodi
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
#include "InputStream.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "threads/SingleLock.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"

namespace ADDON
{

std::map<std::string, CInputStream::Config> CInputStream::m_configMap;
CCriticalSection CInputStream::m_parentSection;

std::unique_ptr<CInputStream> CInputStream::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  std::string listitemprops = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@listitemprops");
  std::string extensions = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@extension");
  std::string protocols = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@protocols");
  std::string name(ext->plugin->identifier);
  std::unique_ptr<CInputStream> istr(new CInputStream(props, name, listitemprops,
                                                      extensions, protocols));
  istr->CheckConfig();
  return istr;
}

CInputStream::CInputStream(const AddonProps& props,
                           const std::string& name,
                           const std::string& listitemprops,
                           const std::string& extensions,
                           const std::string& protocols)
: InputStreamDll(std::move(props))
{
  m_fileItemProps = StringUtils::Tokenize(listitemprops, "|");
  for (auto &key : m_fileItemProps)
  {
    StringUtils::Trim(key);
    key = name + "." + key;
  }

  m_extensionsList = StringUtils::Tokenize(extensions, "|");
  for (auto &ext : m_extensionsList)
  {
    StringUtils::Trim(ext);
  }

  m_protocolsList = StringUtils::Tokenize(protocols, "|");
  for (auto &ext : m_protocolsList)
  {
    StringUtils::Trim(ext);
  }
}

bool CInputStream::CheckAPIVersion()
{
  std::string dllVersion = m_pStruct->GetApiVersion();
  if (dllVersion.compare(INPUTSTREAM_API_VERSION) != 0)
  {
    CLog::Log(LOGERROR, "CInputStream::CheckAPIVersion - API version does not match");
    return false;
  }

  return true;
}

void CInputStream::SaveSettings()
{
  CAddon::SaveSettings();
  if (!m_bIsChild)
    UpdateConfig();
}

void CInputStream::CheckConfig()
{
  bool hasConfig = false;

  {
    CSingleLock lock(m_parentSection);
    auto it = m_configMap.find(ID());
    hasConfig = it != m_configMap.end();
  }

  if (!m_bIsChild && !hasConfig)
    UpdateConfig();
}

void CInputStream::UpdateConfig()
{
  std::string pathList;
  ADDON_STATUS status = Create();

  if (status != ADDON_STATUS_PERMANENT_FAILURE)
  {
    try
    {
      pathList = m_pStruct->GetPathList();
    }
    catch (std::exception &e)
    {
      CLog::Log(LOGERROR, "CInputStream::Supports - could not get a list of paths. Reason: %s", e.what());
    }
    Destroy();
  }

  Config config;
  config.m_pathList = StringUtils::Tokenize(pathList, "|");
  for (auto &path : config.m_pathList)
  {
    StringUtils::Trim(path);
  }

  CSingleLock lock(m_parentSection);
  auto it = m_configMap.find(ID());
  if (it == m_configMap.end())
    config.m_parentBusy = false;
  else
    config.m_parentBusy = it->second.m_parentBusy;

  config.m_ready = true;
  if (status == ADDON_STATUS_PERMANENT_FAILURE)
    config.m_ready = false;

  m_configMap[ID()] = config;
}

bool CInputStream::UseParent()
{
  CSingleLock lock(m_parentSection);

  auto it = m_configMap.find(ID());
  if (it == m_configMap.end())
    return false;
  if (it->second.m_parentBusy)
    return false;

  it->second.m_parentBusy = true;
  return true;
}

bool CInputStream::Supports(const CFileItem &fileitem)
{
  {
    CSingleLock lock(m_parentSection);

    auto it = m_configMap.find(ID());
    if (it == m_configMap.end())
      return false;
    if (!it->second.m_ready)
      return false;
  }

  // check if a specific inputstream addon is requested
  CVariant addon = fileitem.GetProperty("inputstreamaddon");
  if (!addon.isNull())
  {
    if (addon.asString() != ID())
      return false;
    else
      return true;
  }

  // check protocols
  std::string protocol = fileitem.GetURL().GetProtocol();
  if (!protocol.empty())
  {
    if (std::find(m_protocolsList.begin(),
                  m_protocolsList.end(), protocol) != m_protocolsList.end())
      return true;
  }

  // check paths
  CSingleLock lock(m_parentSection);
  auto it = m_configMap.find(ID());
  if (it == m_configMap.end())
    return false;

  bool match = false;
  for (auto &path : it->second.m_pathList)
  {
    if (path.empty())
      continue;

    CRegExp r(true, CRegExp::asciiOnly, path.c_str());
    if (r.RegFind(fileitem.GetPath().c_str()) == 0 && r.GetFindLen() > 5)
    {
      match = true;
      break;
    }
  }
  return match;
}

bool CInputStream::Open(CFileItem &fileitem)
{
  INPUTSTREAM props;
  std::map<std::string, std::string> propsMap;
  for (auto &key : m_fileItemProps)
  {
    if (fileitem.GetProperty(key).isNull())
      continue;
    propsMap[key] = fileitem.GetProperty(key).asString();
  }

  props.m_nCountInfoValues = 0;
  for (auto &pair : propsMap)
  {
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = pair.first.c_str();
    props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = pair.second.c_str();
    props.m_nCountInfoValues++;
  }

  props.m_strURL = fileitem.GetPath().c_str();
  
  std::string libFolder = URIUtils::GetDirectory(m_parentLib);
  std::string profileFolder = CSpecialProtocol::TranslatePath(Profile());
  props.m_libFolder = libFolder.c_str();
  props.m_profileFolder = profileFolder.c_str();

  bool ret = false;
  try
  {
    ret = m_pStruct->Open(props);
    if (ret)
      m_caps = m_pStruct->GetCapabilities();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::Open - could not open stream. Reason: %s", e.what());
    return false;
  }

  UpdateStreams();
  return ret;
}

void CInputStream::Close()
{
  try
  {
    m_pStruct->Close();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::Close - could not close stream. Reason: %s", e.what());
  }

  if (!m_bIsChild)
  {
    CSingleLock lock(m_parentSection);
    auto it = m_configMap.find(ID());
    if (it != m_configMap.end())
      it->second.m_parentBusy = false;
  }
}

// IDisplayTime
int CInputStream::GetTotalTime()
{
  int ret = 0;
  try
  {
    ret = m_pStruct->GetTotalTime();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::GetTotalTime - error. Reason: %s", e.what());
  }
  return ret;
}

int CInputStream::GetTime()
{
  int ret = 0;
  try
  {
    ret = m_pStruct->GetTime();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::GetTime - error. Reason: %s", e.what());
  }
  return ret;
}

// IPosTime
bool CInputStream::PosTime(int ms)
{
  bool ret = false;
  try
  {
    ret = m_pStruct->PosTime(ms);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::PosTime - error. Reason: %s", e.what());
  }
  return ret;
}

// IDemux
void CInputStream::UpdateStreams()
{
  DisposeStreams();

  INPUTSTREAM_IDS streamIDs;
  try
  {
    streamIDs = m_pStruct->GetStreamIds();
  }
  catch (std::exception &e)
  {
    DisposeStreams();
    CLog::Log(LOGERROR, "CInputStream::UpdateStreams - error GetStreamIds. Reason: %s", e.what());
    return;
  }

  if (streamIDs.m_streamCount > INPUTSTREAM_IDS::MAX_STREAM_COUNT)
  {
    DisposeStreams();
    return;
  }

  for (unsigned int i=0; i<streamIDs.m_streamCount; i++)
  {
    INPUTSTREAM_INFO stream;
    try
    {
      stream = m_pStruct->GetStream(streamIDs.m_streamIds[i]);
    }
    catch (std::exception &e)
    {
      DisposeStreams();
      CLog::Log(LOGERROR, "CInputStream::GetTotalTime - error GetStream. Reason: %s", e.what());
      return;
    }

    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_NONE)
      continue;

    std::string codecName(stream.m_codecName);
    StringUtils::ToLower(codecName);
    AVCodec *codec = avcodec_find_decoder_by_name(codecName.c_str());
    if (!codec)
      continue;

    CDemuxStream *demuxStream;

    if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_AUDIO)
    {
      CDemuxStreamAudio *audioStream = new CDemuxStreamAudio();

      audioStream->iChannels = stream.m_Channels;
      audioStream->iSampleRate = stream.m_SampleRate;
      audioStream->iBlockAlign = stream.m_BlockAlign;
      audioStream->iBitRate = stream.m_BitRate;
      audioStream->iBitsPerSample = stream.m_BitsPerSample;
      demuxStream = audioStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_VIDEO)
    {
      CDemuxStreamVideo *videoStream = new CDemuxStreamVideo();

      videoStream->iFpsScale = stream.m_FpsScale;
      videoStream->iFpsRate = stream.m_FpsRate;
      videoStream->iWidth = stream.m_Width;
      videoStream->iHeight = stream.m_Height;
      videoStream->fAspect = stream.m_Aspect;
      videoStream->stereo_mode = "mono";
      demuxStream = videoStream;
    }
    else if (stream.m_streamType == INPUTSTREAM_INFO::TYPE_SUBTITLE)
    {
      //! @todo needs identifier in INPUTSTREAM_INFO
      continue;
    }
    else
      continue;

    demuxStream->codec = codec->id;
    demuxStream->bandwidth = stream.m_Bandwidth;
    demuxStream->codecName = stream.m_codecInternalName;
    demuxStream->uniqueId = streamIDs.m_streamIds[i];
    demuxStream->language[0] = stream.m_language[0];
    demuxStream->language[1] = stream.m_language[1];
    demuxStream->language[2] = stream.m_language[2];
    demuxStream->language[3] = stream.m_language[3];

    if (stream.m_ExtraData && stream.m_ExtraSize)
    {
      demuxStream->ExtraData = new uint8_t[stream.m_ExtraSize];
      demuxStream->ExtraSize = stream.m_ExtraSize;
      for (unsigned int j=0; j<stream.m_ExtraSize; j++)
        demuxStream->ExtraData[j] = stream.m_ExtraData[j];
    }

    m_streams[demuxStream->uniqueId] = demuxStream;
  }
}

void CInputStream::DisposeStreams()
{
  for (auto &stream : m_streams)
    delete stream.second;
  m_streams.clear();
}

int CInputStream::GetNrOfStreams() const
{
  return m_streams.size();
}

CDemuxStream* CInputStream::GetStream(int iStreamId)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it != m_streams.end())
    return it->second;

  return nullptr;
}

std::vector<CDemuxStream*> CInputStream::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  for (auto &stream : m_streams)
  {
    streams.push_back(stream.second);
  }

  return streams;
}

void CInputStream::EnableStream(int iStreamId, bool enable)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iStreamId);
  if (it == m_streams.end())
    return;

  try
  {
    m_pStruct->EnableStream(it->second->uniqueId, enable);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::EnableStream - error. Reason: %s", e.what());
  }
}

DemuxPacket* CInputStream::ReadDemux()
{
  DemuxPacket* pPacket = nullptr;
  try
  {
    pPacket = m_pStruct->DemuxRead();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::ReadDemux - error. Reason: %s", e.what());
    return nullptr;
  }

  if (!pPacket)
  {
    return nullptr;
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    UpdateStreams();
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    UpdateStreams();
  }

  return pPacket;
}

bool CInputStream::SeekTime(int time, bool backward, double* startpts)
{
  bool ret = false;
  try
  {
    ret = m_pStruct->DemuxSeekTime(time, backward, startpts);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::SeekTime - error. Reason: %s", e.what());
  }
  return ret;
}

void CInputStream::AbortDemux()
{
  try
  {
    m_pStruct->DemuxAbort();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::AbortDemux - error. Reason: %s", e.what());
  }
}

void CInputStream::FlushDemux()
{
  try
  {
    m_pStruct->DemuxFlush();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::FlushDemux - error. Reason: %s", e.what());
  }
}

void CInputStream::SetSpeed(int iSpeed)
{
  try
  {
    m_pStruct->DemuxSetSpeed(iSpeed);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::SetSpeed - error. Reason: %s", e.what());
  }
}

int CInputStream::ReadStream(uint8_t* buf, unsigned int size)
{
  int ret = -1;
  try
  {
    ret = m_pStruct->ReadStream(buf, size);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::ReadStream - error. Reason: %s", e.what());
  }
  return ret;
}

int64_t CInputStream::SeekStream(int64_t offset, int whence)
{
  int64_t ret = -1;
  try
  {
    ret = m_pStruct->SeekStream(offset, whence);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::SeekStream - error. Reason: %s", e.what());
  }
  return ret;
}

int64_t CInputStream::PositionStream()
{
  int64_t ret = -1;
  try
  {
    ret = m_pStruct->PositionStream();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::PositionStream - error. Reason: %s", e.what());
  }
  return ret;
}

int64_t CInputStream::LengthStream()
{
  int64_t ret = -1;
  try
  {
    ret = m_pStruct->LengthStream();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::LengthStream - error. Reason: %s", e.what());
  }
  return ret;
}

void CInputStream::PauseStream(double time)
{
  try
  {
    m_pStruct->PauseStream(time);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::PauseStream - error. Reason: %s", e.what());
  }
}

bool CInputStream::IsRealTimeStream()
{
  bool ret = false;
  try
  {
    ret = m_pStruct->IsRealTimeStream();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::IsRealTimeStream - error. Reason: %s", e.what());
  }
  return ret;
}

void CInputStream::SetVideoResolution(int width, int height)
{
  try
  {
    m_pStruct->SetVideoResolution(width, height);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CInputStream::SetVideoResolution - error. Reason: %s", e.what());
  }
}

} /*namespace ADDON*/

