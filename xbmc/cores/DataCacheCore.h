/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDStreamInfo.h"
#include "EdlEdit.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "threads/CriticalSection.h"
#include "utils/AgedMap.h"
#include "utils/BitstreamConverter.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

class CDataCacheCore
{
public:
  CDataCacheCore();
  virtual ~CDataCacheCore();
  static CDataCacheCore& GetInstance();
  void Reset();
  void ResetAudioCache();
  bool HasAVInfoChanges();
  void SignalVideoInfoChange();
  void SignalAudioInfoChange();
  void SignalSubtitleInfoChange();

  void SetAVChange(bool value);
  bool GetAVChange();
  void SetAVChangeExtended(bool value);
  bool GetAVChangeExtended();

  // player video info
  void SetVideoDecoderName(std::string name, bool isHw);
  std::string GetVideoDecoderName();
  bool IsVideoHwDecoder();
  void SetVideoDeintMethod(std::string method);
  std::string GetVideoDeintMethod();
  void SetVideoPixelFormat(std::string pixFormat);
  std::string GetVideoPixelFormat();
  void SetVideoStereoMode(std::string mode);
  std::string GetVideoStereoMode();
  void SetVideoDimensions(int width, int height);
  int GetVideoWidth();
  int GetVideoHeight();
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();

  // Additional Player Process Info data (Only set in Data Core Cache)
  void SetVideoPts(double pts);
  double GetVideoPts();
  void SetVideoBitDepth(int bitDepth);
  int GetVideoBitDepth();
  void SetVideoHdrType(StreamHdrType hdrType);
  StreamHdrType GetVideoHdrType();
  void SetVideoSourceHdrType(StreamHdrType hdrType);
  StreamHdrType GetVideoSourceHdrType();
  void SetVideoSourceAdditionalHdrType(StreamHdrType hdrType);
  StreamHdrType GetVideoSourceAdditionalHdrType();
  void SetVideoColorSpace(AVColorSpace colorSpace);
  AVColorSpace GetVideoColorSpace();
  void SetVideoColorRange(AVColorRange colorRange);
  AVColorRange GetVideoColorRange();
  void SetVideoColorPrimaries(AVColorPrimaries colorPrimaries);
  AVColorPrimaries GetVideoColorPrimaries();
  void SetVideoColorTransferCharacteristic(AVColorTransferCharacteristic colorTransferCharacteristic);
  AVColorTransferCharacteristic GetVideoColorTransferCharacteristic();
  void SetVideoDoViFrameMetadata(DOVIFrameMetadata value);
  DOVIFrameMetadata GetVideoDoViFrameMetadata();
  void SetVideoDoViStreamMetadata(DOVIStreamMetadata value);
  DOVIStreamMetadata GetVideoDoViStreamMetadata();
  void SetVideoDoViStreamInfo(DOVIStreamInfo value);
  DOVIStreamInfo GetVideoDoViStreamInfo();
  void SetVideoSourceDoViStreamInfo(DOVIStreamInfo value);
  DOVIStreamInfo GetVideoSourceDoViStreamInfo();
  void SetVideoDoViCodecFourCC(std::string value);
  std::string GetVideoDoViCodecFourCC();
  void SetVideoHDRStaticMetadataInfo(HDRStaticMetadataInfo value);
  HDRStaticMetadataInfo GetVideoHDRStaticMetadataInfo();
  void SetVideoLiveBitRate(double bitRate);
  double GetVideoLiveBitRate();
  void SetVideoQueueLevel(int level);
  int GetVideoQueueLevel();
  void SetVideoQueueDataLevel(int level);
  int GetVideoQueueDataLevel();

  /*!
   * @brief Set if the video is interlaced in cache.
   * @param isInterlaced Set true when the video is interlaced
   */
  void SetVideoInterlaced(bool isInterlaced);

  /*!
   * @brief Check if the video is interlaced from cache
   * @return True if interlaced, otherwise false
   */
  bool IsVideoInterlaced();

  // player audio info
  void SetAudioDecoderName(std::string name);
  std::string GetAudioDecoderName();
  void SetAudioChannels(std::string channels);
  void SetAudioChannelsSink(std::string channels);
  std::string GetAudioChannels();
  std::string GetAudioChannelsSink();
  void SetAudioSampleRate(int sampleRate);
  int GetAudioSampleRate();
  void SetAudioBitsPerSample(int bitsPerSample);
  int GetAudioBitsPerSample();

  // Speaker layout bitmasks (per-speaker booleans for skins)
  static uint64_t MakeSpeakerMask(const class CAEChannelInfo& channels);
  void SetAudioSpeakerMask(uint64_t mask);
  uint64_t GetAudioSpeakerMask();
  void SetAudioSpeakerMaskSink(uint64_t mask);
  uint64_t GetAudioSpeakerMaskSink();

  // Additional Player Process Info data (Only set in Data Core Cache)
  void SetAudioPts(double pts);
  double GetAudioPts();
  void SetAudioLiveBitRate(double bitRate);
  double GetAudioLiveBitRate();
  void SetAudioQueueLevel(int level);
  int GetAudioQueueLevel();
  void SetAudioQueueDataLevel(int level);
  int GetAudioQueueDataLevel();

  // content info

  /*!
   * @brief Set the EDL edit list to cache.
   * @param editList The vector of edits to fill.
   */
  void SetEditList(const std::vector<EDL::Edit>& editList);

  /*!
   * @brief Get the EDL edit list in cache.
   * @return The EDL edits or an empty vector if no edits exist.
   */
  const std::vector<EDL::Edit>& GetEditList() const;

  /*!
   * @brief Set the list of cut markers in cache.
   * @return The list of cuts or an empty list if no cuts exist
   */
  void SetCuts(const std::vector<int64_t>& cuts);

  /*!
   * @brief Get the list of cut markers from cache.
   * @return The list of cut markers or an empty vector if no cuts exist.
   */
  const std::vector<int64_t>& GetCuts() const;

  /*!
   * @brief Set the list of scene markers in cache.
   * @return The list of scene markers or an empty list if no scene markers exist
   */
  void SetSceneMarkers(const std::vector<int64_t>& sceneMarkers);

  /*!
   * @brief Get the list of scene markers markers from cache.
   * @return The list of scene markers or an empty vector if no scene exist.
   */
  const std::vector<int64_t>& GetSceneMarkers() const;

  void SetChapters(const std::vector<std::pair<std::string, int64_t>>& chapters);

  /*!
   * @brief Get the chapter list in cache.
   * @return The list of chapters or an empty vector if no chapters exist.
   */
  const std::vector<std::pair<std::string, int64_t>>& GetChapters() const;

  // render info
  void SetRenderClockSync(bool enabled);
  bool IsRenderClockSync();

  // player states
  /*!
   * @brief Notifies the cache core that a seek operation has finished
   * @param offset - the seek offset
  */
  void SeekFinished(int64_t offset);

  void SetStateSeeking(bool active);
  bool IsSeeking() const;

  /*!
   * @brief Checks if a seek has been performed in the last provided seconds interval
   * @param lastSecondInterval - the last elapsed second interval to check for a seek operation
   * @return true if a seek was performed in the lastSecondInterval, false otherwise
  */
  bool HasPerformedSeek(int64_t lastSecondInterval) const;

  /*!
   * @brief Gets the last seek offset
   * @return the last seek offset
  */
  int64_t GetSeekOffSet() const;

  void SetSpeed(float tempo, float speed);
  float GetSpeed() const;
  bool IsNormalPlayback();
  bool IsPausedPlayback();
  float GetTempo() const;
  void SetFrameAdvance(bool fa);
  bool IsFrameAdvance() const;
  bool IsPlayerStateChanged();
  void SetGuiRender(bool gui);
  bool GetGuiRender() const;
  void SetVideoRender(bool video);
  bool GetVideoRender() const;
  void SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max);
  void GetPlayTimes(time_t &start, int64_t &current, int64_t &min, int64_t &max) const;

  /*!
   * \brief Get the start time
   *
   * For a typical video this will be zero. For live TV, this is a reference time
   * in units of time_t (UTC) from which time elapsed starts. Ideally this would
   * be the start of the tv show but can be any other time as well.
   */
  time_t GetStartTime() const;

  /*!
   * \brief Get the current time of playback
   *
   * This is the time elapsed, in ms, since the start time.
   */
  int64_t GetPlayTime() const;

  /*!
   * \brief Get the current percentage of playback if a playback buffer is available.
   *
   *  If there is no playback buffer, percentage will be 0.
   */
  float GetPlayPercentage() const;

  /*!
   * \brief Get the minimum time
   *
   * This will be zero for a typical video. With timeshift, this is the time,
   * in ms, that the player can go back. This can be before the start time.
   */
  int64_t GetMinTime() const;

  /*!
   * \brief Get the maximum time
   *
   * This is the maximum time, in ms, that the player can skip forward. For a
   * typical video, this will be the total length. For live TV without
   * timeshift this is zero, and for live TV with timeshift this will be the
   * buffer ahead.
   */
  int64_t GetMaxTime() const;

protected:
  std::atomic_bool m_AVChange = false;
  std::atomic_bool m_AVChangeExtended = false;
  std::atomic_bool m_hasAVInfoChanges = false;

  CCriticalSection m_videoPlayerSection;
  struct SPlayerVideoInfo
  {    
    std::string decoderName;
    bool isHwDecoder;
    std::string deintMethod;
    std::string pixFormat;
    std::string stereoMode;
    int width;
    int height;
    float fps;
    float dar;
    bool m_isInterlaced;
    double pts = 0;
    int bitDepth = 0;
    StreamHdrType hdrType = StreamHdrType::HDR_TYPE_NONE;
    StreamHdrType sourceHdrType = StreamHdrType::HDR_TYPE_NONE;
    StreamHdrType sourceAdditionalHdrType = StreamHdrType::HDR_TYPE_NONE;
    AVColorSpace colorSpace = AVCOL_SPC_UNSPECIFIED;
    AVColorRange colorRange = AVCOL_RANGE_UNSPECIFIED;
    AVColorPrimaries colorPrimaries = AVCOL_PRI_UNSPECIFIED;
    AVColorTransferCharacteristic colorTransferCharacteristic = AVCOL_TRC_UNSPECIFIED;
    AgedMap<uint64_t, DOVIFrameMetadata> doviFrameMetadataMap;
    DOVIStreamMetadata doviStreamMetadata = {};
    DOVIStreamInfo doviStreamInfo = {};
    DOVIStreamInfo sourceDoViStreamInfo = {};

    std::string doviCodecFourCC = "";

    HDRStaticMetadataInfo hdrStaticMetadataInfo = {};

    double liveBitRate = 0;
    int queueLevel = 0;
    int queueDataLevel = 0;
  } m_playerVideoInfo;

  CCriticalSection m_audioPlayerSection;
  struct SPlayerAudioInfo
  {
    std::string decoderName;
    std::string channels;
    std::string channels_sink;
    int sampleRate;
    int bitsPerSample;
    uint64_t speakerMask = 0;
    uint64_t speakerMaskSink = 0;
    double pts = 0;
    double liveBitRate = 0;
    int queueLevel = 0;
    int queueDataLevel = 0;
  } m_playerAudioInfo;

  mutable CCriticalSection m_contentSection;
  struct SContentInfo
  {
  public:
    /*!
      * @brief Set the EDL edit list in cache.
      * @param editList the list of edits to store in cache
      */
    void SetEditList(const std::vector<EDL::Edit>& editList) { m_editList = editList; }

    /*!
      * @brief Get the EDL edit list in cache.
      * @return the list of edits in cache
      */
    const std::vector<EDL::Edit>& GetEditList() const { return m_editList; }

    /*!
      * @brief Save the list of cut markers in cache.
      * @param cuts the list of cut markers to store in cache
      */
    void SetCuts(const std::vector<int64_t>& cuts) { m_cuts = cuts; }

    /*!
      * @brief Get the list of cut markers in cache.
      * @return the list of cut markers in cache
      */
    const std::vector<int64_t>& GetCuts() const { return m_cuts; }

    /*!
      * @brief Save the list of scene markers in cache.
      * @param sceneMarkers the list of scene markers to store in cache
      */
    void SetSceneMarkers(const std::vector<int64_t>& sceneMarkers)
    {
      m_sceneMarkers = sceneMarkers;
    }

    /*!
      * @brief Get the list of scene markers in cache.
      * @return the list of scene markers in cache
      */
    const std::vector<int64_t>& GetSceneMarkers() const { return m_sceneMarkers; }

    /*!
      * @brief Save the chapter list in cache.
      * @param chapters the list of chapters to store in cache
      */
    void SetChapters(const std::vector<std::pair<std::string, int64_t>>& chapters)
    {
      m_chapters = chapters;
    }

    /*!
      * @brief Get the list of chapters in cache.
      * @return the list of chapters in cache
      */
    const std::vector<std::pair<std::string, int64_t>>& GetChapters() const { return m_chapters; }

    /*!
      * @brief Reset the content cache to the original values (all empty)
      */
    void Reset()
    {
      m_editList.clear();
      m_chapters.clear();
      m_cuts.clear();
      m_sceneMarkers.clear();
    }

  private:
    /*!< list of EDL edits */
    std::vector<EDL::Edit> m_editList;
    /*!< name and position for chapters */
    std::vector<std::pair<std::string, int64_t>> m_chapters;
    /*!< position for EDL cuts */
    std::vector<int64_t> m_cuts;
    /*!< position for EDL scene markers */
    std::vector<int64_t> m_sceneMarkers;
  } m_contentInfo;

  CCriticalSection m_renderSection;
  struct SRenderInfo
  {
    bool m_isClockSync;
  } m_renderInfo;

  mutable CCriticalSection m_stateSection;
  bool m_playerStateChanged = false;
  struct SStateInfo
  {
    bool m_stateSeeking{false};
    bool m_renderGuiLayer{false};
    bool m_renderVideoLayer{false};
    float m_tempo{1.0f};
    float m_speed{1.0f};
    bool m_frameAdvance{false};
    /*! Time point of the last seek operation */
    std::chrono::time_point<std::chrono::system_clock> m_lastSeekTime{
        std::chrono::time_point<std::chrono::system_clock>{}};
    /*! Last seek offset */
    int64_t m_lastSeekOffset{0};
  } m_stateInfo;

  struct STimeInfo
  {
    time_t m_startTime;
    int64_t m_time;
    int64_t m_timeMax;
    int64_t m_timeMin;
  } m_timeInfo = {};
};
