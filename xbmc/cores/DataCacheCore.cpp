/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DataCacheCore.h"

#include "DVDStreamInfo.h"
#include "ServiceBroker.h"
#include "cores/EdlEdit.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "utils/AgedMap.h"
#include "utils/BitstreamConverter.h"

#include <mutex>
#include <utility>

CDataCacheCore::CDataCacheCore() :
  m_playerVideoInfo {},
  m_playerAudioInfo {},
  m_contentInfo {},
  m_renderInfo {},
  m_stateInfo {}
{
}

CDataCacheCore::~CDataCacheCore() = default;

CDataCacheCore& CDataCacheCore::GetInstance()
{
  return CServiceBroker::GetDataCacheCore();
}

void CDataCacheCore::Reset()
{
  {
    std::lock_guard lock(m_stateSection);

    m_stateInfo = {};
    m_playerStateChanged = false;
  }
  {
    std::lock_guard lock(m_videoPlayerSection);

    m_playerVideoInfo = {};
  }
  {
    std::lock_guard lock(m_audioPlayerSection);

    m_playerAudioInfo = {};
  }
  m_hasAVInfoChanges = false;
  {
    std::lock_guard lock(m_renderSection);

    m_renderInfo = {};
  }
  {
    std::lock_guard lock(m_contentSection);

    m_contentInfo.Reset();
  }
  m_timeInfo = {};
}

bool CDataCacheCore::HasAVInfoChanges()
{
  bool ret = m_hasAVInfoChanges;
  m_hasAVInfoChanges = false;
  return ret;
}

void CDataCacheCore::SignalVideoInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SignalAudioInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SignalSubtitleInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SetAVChange(bool value)
{
  m_AVChange = value;
}

bool CDataCacheCore::GetAVChange()
{
  return m_AVChange;
}

void CDataCacheCore::SetAVChangeExtended(bool value)
{
  m_AVChangeExtended = value;
}

bool CDataCacheCore::GetAVChangeExtended()
{
  return m_AVChangeExtended;
}

void CDataCacheCore::SetVideoDecoderName(std::string name, bool isHw)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.decoderName = std::move(name);
  m_playerVideoInfo.isHwDecoder = isHw;
}

std::string CDataCacheCore::GetVideoDecoderName()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.decoderName;
}

bool CDataCacheCore::IsVideoHwDecoder()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.isHwDecoder;
}

void CDataCacheCore::SetVideoDeintMethod(std::string method)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.deintMethod = std::move(method);
}

std::string CDataCacheCore::GetVideoDeintMethod()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.deintMethod;
}

void CDataCacheCore::SetVideoPixelFormat(std::string pixFormat)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.pixFormat = std::move(pixFormat);
}

std::string CDataCacheCore::GetVideoPixelFormat()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.pixFormat;
}

void CDataCacheCore::SetVideoStereoMode(std::string mode)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.stereoMode = std::move(mode);
}

std::string CDataCacheCore::GetVideoStereoMode()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.stereoMode;
}

void CDataCacheCore::SetVideoDimensions(int width, int height)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.width = width;
  m_playerVideoInfo.height = height;
}

int CDataCacheCore::GetVideoWidth()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.width;
}

int CDataCacheCore::GetVideoHeight()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.height;
}

void CDataCacheCore::SetVideoBitDepth(int bitDepth)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.bitDepth = bitDepth;
}

int CDataCacheCore::GetVideoBitDepth()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.bitDepth;
}

void CDataCacheCore::SetVideoHdrType(StreamHdrType hdrType)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.hdrType = hdrType;
}

StreamHdrType CDataCacheCore::GetVideoHdrType()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.hdrType;
}

void CDataCacheCore::SetVideoSourceHdrType(StreamHdrType hdrType)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.sourceHdrType = hdrType;
}

StreamHdrType CDataCacheCore::GetVideoSourceHdrType()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.sourceHdrType;
}

void CDataCacheCore::SetVideoSourceAdditionalHdrType(StreamHdrType hdrType)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.sourceAdditionalHdrType = hdrType;
}

StreamHdrType CDataCacheCore::GetVideoSourceAdditionalHdrType()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.sourceAdditionalHdrType;
}

void CDataCacheCore::SetVideoColorSpace(AVColorSpace colorSpace)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.colorSpace = colorSpace;
}

AVColorSpace CDataCacheCore::GetVideoColorSpace()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.colorSpace;
}

void CDataCacheCore::SetVideoColorRange(AVColorRange colorRange)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.colorRange = colorRange;
}

AVColorRange CDataCacheCore::GetVideoColorRange()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.colorRange;
}

void CDataCacheCore::SetVideoColorPrimaries(AVColorPrimaries colorPrimaries)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.colorPrimaries = colorPrimaries;
}

AVColorPrimaries CDataCacheCore::GetVideoColorPrimaries()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.colorPrimaries;
}

void CDataCacheCore::SetVideoColorTransferCharacteristic(AVColorTransferCharacteristic colorTransferCharacteristic)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.colorTransferCharacteristic = colorTransferCharacteristic;
}

AVColorTransferCharacteristic CDataCacheCore::GetVideoColorTransferCharacteristic()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.colorTransferCharacteristic;
}

void CDataCacheCore::SetVideoDoViFrameMetadata(DOVIFrameMetadata value)
{
  std::lock_guard lock(m_videoPlayerSection);

  uint64_t pts = value.pts;
  logM(LOGDEBUG, "CDataCacheCore", "Set meta for pts [{}] [{}]", pts, value.level1_max_pq);
  m_playerVideoInfo.doviFrameMetadataMap.insert(pts, value);
}

DOVIFrameMetadata CDataCacheCore::GetVideoDoViFrameMetadata()
{
  std::lock_guard lock(m_videoPlayerSection);

  uint64_t pts = GetRenderPts();
  auto doviFrameMetadata = m_playerVideoInfo.doviFrameMetadataMap.findOrLatest(pts);
  if (doviFrameMetadata != m_playerVideoInfo.doviFrameMetadataMap.end())
  {
    logM(LOGDEBUG, "CDataCacheCore", "Get meta for pts [{}] [{}] (matched pts [{}])",
                                     pts, doviFrameMetadata->second.level1_max_pq, doviFrameMetadata->first);
    return doviFrameMetadata->second;
  }
  return {};
}

void CDataCacheCore::SetVideoDoViStreamMetadata(DOVIStreamMetadata value)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.doviStreamMetadata = value;
}

DOVIStreamMetadata CDataCacheCore::GetVideoDoViStreamMetadata()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.doviStreamMetadata;
}

void CDataCacheCore::SetVideoDoViStreamInfo(DOVIStreamInfo value)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.doviStreamInfo = value;
}

DOVIStreamInfo CDataCacheCore::GetVideoDoViStreamInfo()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.doviStreamInfo;
}

void CDataCacheCore::SetVideoSourceDoViStreamInfo(DOVIStreamInfo value)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.sourceDoViStreamInfo = value;
}

DOVIStreamInfo CDataCacheCore::GetVideoSourceDoViStreamInfo()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.sourceDoViStreamInfo;
}

void CDataCacheCore::SetVideoDoViCodecFourCC(std::string codecFourCC)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.doviCodecFourCC = codecFourCC;
}

std::string CDataCacheCore::GetVideoDoViCodecFourCC()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.doviCodecFourCC;
}

void CDataCacheCore::SetVideoHDRStaticMetadataInfo(HDRStaticMetadataInfo value)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.hdrStaticMetadataInfo = value;
}

HDRStaticMetadataInfo CDataCacheCore::GetVideoHDRStaticMetadataInfo()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.hdrStaticMetadataInfo;
}

void CDataCacheCore::SetVideoLiveBitRate(double bitRate)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.liveBitRate = bitRate;
}

double CDataCacheCore::GetVideoLiveBitRate()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.liveBitRate;
}

void CDataCacheCore::SetVideoQueueLevel(int level)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.queueLevel = level;
}

int CDataCacheCore::GetVideoQueueLevel()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.queueLevel;
}

void CDataCacheCore::SetVideoQueueDataLevel(int level)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.queueDataLevel = level;
}

int CDataCacheCore::GetVideoQueueDataLevel()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.queueDataLevel;
}

void CDataCacheCore::SetVideoFps(float fps)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.fps = fps;
}

float CDataCacheCore::GetVideoFps()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.fps;
}

void CDataCacheCore::SetVideoDAR(float dar)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.dar = dar;
}

float CDataCacheCore::GetVideoDAR()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.dar;
}

void CDataCacheCore::SetVideoInterlaced(bool isInterlaced)
{
  std::lock_guard lock(m_videoPlayerSection);

  m_playerVideoInfo.m_isInterlaced = isInterlaced;
}

bool CDataCacheCore::IsVideoInterlaced()
{
  std::lock_guard lock(m_videoPlayerSection);

  return m_playerVideoInfo.m_isInterlaced;
}

// player audio info
void CDataCacheCore::SetAudioDecoderName(std::string name)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.decoderName = std::move(name);
}

std::string CDataCacheCore::GetAudioDecoderName()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.decoderName;
}

void CDataCacheCore::SetAudioChannels(std::string channels)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.channels = std::move(channels);
}

std::string CDataCacheCore::GetAudioChannels()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.channels;
}

void CDataCacheCore::SetAudioSampleRate(int sampleRate)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.sampleRate = sampleRate;
}

int CDataCacheCore::GetAudioSampleRate()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.sampleRate;
}

void CDataCacheCore::SetAudioBitsPerSample(int bitsPerSample)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.bitsPerSample = bitsPerSample;
}

int CDataCacheCore::GetAudioBitsPerSample()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.bitsPerSample;
}

void CDataCacheCore::SetAudioLiveBitRate(double bitRate)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.liveBitRate = bitRate;
}

double CDataCacheCore::GetAudioLiveBitRate()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.liveBitRate;
}

void CDataCacheCore::SetAudioQueueLevel(int level)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.queueLevel = level;
}

int CDataCacheCore::GetAudioQueueLevel()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.queueLevel;
}

void CDataCacheCore::SetAudioQueueDataLevel(int level)
{
  std::lock_guard lock(m_audioPlayerSection);

  m_playerAudioInfo.queueDataLevel = level;
}

int CDataCacheCore::GetAudioQueueDataLevel()
{
  std::lock_guard lock(m_audioPlayerSection);

  return m_playerAudioInfo.queueDataLevel;
}

void CDataCacheCore::SetEditList(const std::vector<EDL::Edit>& editList)
{
  std::lock_guard lock(m_contentSection);

  m_contentInfo.SetEditList(editList);
}

const std::vector<EDL::Edit>& CDataCacheCore::GetEditList() const
{
  std::lock_guard lock(m_contentSection);

  return m_contentInfo.GetEditList();
}

void CDataCacheCore::SetCuts(const std::vector<int64_t>& cuts)
{
  std::lock_guard lock(m_contentSection);

  m_contentInfo.SetCuts(cuts);
}

const std::vector<int64_t>& CDataCacheCore::GetCuts() const
{
  std::lock_guard lock(m_contentSection);

  return m_contentInfo.GetCuts();
}

void CDataCacheCore::SetSceneMarkers(const std::vector<int64_t>& sceneMarkers)
{
  std::lock_guard lock(m_contentSection);

  m_contentInfo.SetSceneMarkers(sceneMarkers);
}

const std::vector<int64_t>& CDataCacheCore::GetSceneMarkers() const
{
  std::lock_guard lock(m_contentSection);

  return m_contentInfo.GetSceneMarkers();
}

void CDataCacheCore::SetChapters(const std::vector<std::pair<std::string, int64_t>>& chapters)
{
  std::lock_guard lock(m_contentSection);

  m_contentInfo.SetChapters(chapters);
}

const std::vector<std::pair<std::string, int64_t>>& CDataCacheCore::GetChapters() const
{
  std::lock_guard lock(m_contentSection);

  return m_contentInfo.GetChapters();
}

void CDataCacheCore::SetRenderClockSync(bool enable)
{
  std::lock_guard lock(m_renderSection);

  m_renderInfo.m_isClockSync = enable;
}

bool CDataCacheCore::IsRenderClockSync()
{
  std::lock_guard lock(m_renderSection);

  return m_renderInfo.m_isClockSync;
}

void CDataCacheCore::SetRenderPts(double pts)
{
  std::lock_guard lock(m_renderSection);

  m_renderInfo.pts = pts;
}

double CDataCacheCore::GetRenderPts()
{
  std::lock_guard lock(m_renderSection);

  return m_renderInfo.pts;
}

// player states
void CDataCacheCore::SeekFinished(int64_t offset)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_lastSeekTime = std::chrono::system_clock::now();
  m_stateInfo.m_lastSeekOffset = offset;
}

int64_t CDataCacheCore::GetSeekOffSet() const
{
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_lastSeekOffset;
}

bool CDataCacheCore::HasPerformedSeek(int64_t lastSecondInterval) const
{
  std::lock_guard lock(m_stateSection);

  if (m_stateInfo.m_lastSeekTime == std::chrono::time_point<std::chrono::system_clock>{})
  {
    return false;
  }
  return (std::chrono::system_clock::now() - m_stateInfo.m_lastSeekTime) <
         std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::duration<int64_t>(lastSecondInterval));
}

void CDataCacheCore::SetStateSeeking(bool active)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_stateSeeking = active;
  m_playerStateChanged = true;
}

bool CDataCacheCore::IsSeeking() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_stateSeeking;
}

void CDataCacheCore::SetSpeed(float tempo, float speed)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_tempo = tempo;
  m_stateInfo.m_speed = speed;
}

float CDataCacheCore::GetSpeed() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_speed;
}

float CDataCacheCore::GetTempo() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_tempo;
}

void CDataCacheCore::SetFrameAdvance(bool fa)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_frameAdvance = fa;
}

bool CDataCacheCore::IsFrameAdvance() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_frameAdvance;
}

bool CDataCacheCore::IsPlayerStateChanged()
{
  std::lock_guard lock(m_stateSection);

  bool ret(m_playerStateChanged);
  m_playerStateChanged = false;

  return ret;
}

void CDataCacheCore::SetGuiRender(bool gui)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_renderGuiLayer = gui;
  m_playerStateChanged = true;
}

bool CDataCacheCore::GetGuiRender() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_renderGuiLayer;
}

void CDataCacheCore::SetVideoRender(bool video)
{
  std::lock_guard lock(m_stateSection);

  m_stateInfo.m_renderVideoLayer = video;
  m_playerStateChanged = true;
}

bool CDataCacheCore::GetVideoRender() const {
  std::lock_guard lock(m_stateSection);

  return m_stateInfo.m_renderVideoLayer;
}

void CDataCacheCore::SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max)
{
  std::lock_guard lock(m_stateSection);

  m_timeInfo.m_startTime = start;
  m_timeInfo.m_time = current;
  m_timeInfo.m_timeMin = min;
  m_timeInfo.m_timeMax = max;
}

void CDataCacheCore::GetPlayTimes(time_t &start, int64_t &current, int64_t &min, int64_t &max) const {
  std::lock_guard lock(m_stateSection);

  start = m_timeInfo.m_startTime;
  current = m_timeInfo.m_time;
  min = m_timeInfo.m_timeMin;
  max = m_timeInfo.m_timeMax;
}

time_t CDataCacheCore::GetStartTime() const {
  std::lock_guard lock(m_stateSection);

  return m_timeInfo.m_startTime;
}

int64_t CDataCacheCore::GetPlayTime() const {
  std::lock_guard lock(m_stateSection);

  return m_timeInfo.m_time;
}

int64_t CDataCacheCore::GetMinTime() const {
  std::lock_guard lock(m_stateSection);

  return m_timeInfo.m_timeMin;
}

int64_t CDataCacheCore::GetMaxTime() const {
  std::lock_guard lock(m_stateSection);

  return m_timeInfo.m_timeMax;
}

float CDataCacheCore::GetPlayPercentage() const {
  std::lock_guard lock(m_stateSection);

  // Note: To calculate accurate percentage, all time data must be consistent,
  //       which is the case for data cache core. Calculation can not be done
  //       outside of data cache core or a possibility to lock the data cache
  //       core from outside would be needed.
  int64_t iTotalTime = m_timeInfo.m_timeMax - m_timeInfo.m_timeMin;
  if (iTotalTime <= 0)
    return 0;

  return m_timeInfo.m_time * 100 / static_cast<float>(iTotalTime);
}
