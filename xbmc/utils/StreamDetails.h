/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISerializable.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "utils/IArchivable.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class CStreamDetails;
class CVariant;
struct VideoStreamInfo;
struct AudioStreamInfo;
struct SubtitleStreamInfo;

class CStreamDetail : public IArchivable, public ISerializable
{
public:
  enum StreamType {
    VIDEO,
    AUDIO,
    SUBTITLE
  };

  explicit CStreamDetail(StreamType type) : m_eType(type), m_pParent(NULL) {}
  virtual ~CStreamDetail() = default;
  virtual bool IsWorseThan(const CStreamDetail &that) const = 0;

  const StreamType m_eType;

protected:
  CStreamDetails *m_pParent;
  friend class CStreamDetails;
};

class CStreamDetailVideo final : public CStreamDetail
{
public:
  CStreamDetailVideo();
  CStreamDetailVideo(const VideoStreamInfo &info, int duration = 0);
  CStreamDetailVideo(const CStreamDetailVideo&) = default;
  CStreamDetailVideo& operator=(const CStreamDetailVideo& that);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  int m_iWidth = 0;
  int m_iHeight = 0;
  float m_fAspect = 0.0;
  int m_iDuration = 0;
  std::string m_strCodec;
  std::string m_strStereoMode;
  std::string m_strLanguage;
  std::string m_strHdrType;
  std::string m_strHdrTypeAlt;
  std::string m_strHdrDetail;
};

class CStreamDetailAudio final : public CStreamDetail
{
public:
  CStreamDetailAudio();
  CStreamDetailAudio(const AudioStreamInfo &info);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  int m_iChannels = -1;
  std::string m_strCodec;
  std::string m_strLanguage;
};

class CStreamDetailSubtitle final : public CStreamDetail
{
public:
  CStreamDetailSubtitle();
  CStreamDetailSubtitle(const SubtitleStreamInfo &info);
  CStreamDetailSubtitle(const CStreamDetailSubtitle&) = default;
  CStreamDetailSubtitle& operator=(const CStreamDetailSubtitle &that);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  std::string m_strLanguage;
};

class CStreamDetails final : public IArchivable, public ISerializable
{
public:
  CStreamDetails() { Reset(); }
  CStreamDetails(const CStreamDetails &that);
  CStreamDetails& operator=(const CStreamDetails &that);
  bool operator ==(const CStreamDetails &that) const;
  bool operator !=(const CStreamDetails &that) const;

  /*!
   * \brief An upper bound on coded dimensions, and the label Kodi reports for content
   *        fitting within it.
   */
  struct Resolution
  {
    int maxWidth;
    int maxHeight;
    std::string_view label;
  };

  /*!
   * \brief The vocabulary of resolution labels Kodi reports, in ascending order.
   *
   * Content is described by the first entry it fits within on both axes. See
   * VideoDimsToResolutionDescription().
   *
   * \note Bounding both axes means portrait content is described by whichever entry is tall
   *       enough to hold it, so 1080x1920 is "4K". Content larger than the last entry is not
   *       described at all rather than clamped to it.
   */
  static constexpr auto COMMON_RESOLUTIONS = std::to_array<Resolution>({
      {854, 480, "480"}, // anamorphic NTSC DVD
      {960, 544, "540"}, // 960x540, sometimes 544 as a multiple of 16
      {1024, 576, "576"}, // includes 768x576, 720x576 and 1024x576
      {1280, 962, "720"},
      {1920, 1440, "1080"},
      {4096, 3072, "4K"},
      {8192, 6144, "8K"},
  });

  /*!
   * \brief A common aspect ratio, and the label Kodi reports for content closest to it.
   */
  struct AspectRatio
  {
    float ratio;
    std::string_view label;
  };

  /*!
   * \brief The vocabulary of aspect ratio labels Kodi reports, in ascending order.
   *
   * Content is classified as the closest entry in log space, which means the cutoff between
   * two adjacent entries is their geometric mean. See VideoAspectToAspectDescription().
   *
   * This is the single source of truth for the aspect labels skins display. Anything else
   * needing the same vocabulary - a selection list, a reported label, a plausibility check -
   * must use this table rather than defining a parallel list, or the two will drift.
   *
   * \note The table classifies but never rejects. Any value wider than the last entry is
   *       reported as that entry, so this cannot be used as a validity check on its own.
   */
  static constexpr auto COMMON_ASPECT_RATIOS = std::to_array<AspectRatio>({
      {1.00f, "1.00"},
      {1.19f, "1.19"},
      {1.33f, "1.33"},
      {1.37f, "1.37"},
      {1.43f, "1.43"}, // IMAX 70mm
      {1.50f, "1.50"}, // IMAX digital, VistaVision
      {1.66f, "1.66"},
      {1.78f, "1.78"},
      {1.85f, "1.85"},
      {1.90f, "1.90"}, // IMAX digital, DCI full container
      {2.00f, "2.00"},
      {2.20f, "2.20"},
      {2.35f, "2.35"},
      {2.40f, "2.40"},
      {2.55f, "2.55"},
      {2.76f, "2.76"},
  });

  static std::string VideoDimsToResolutionDescription(int iWidth, int iHeight);
  static std::string VideoAspectToAspectDescription(float fAspect);

  bool HasItems(void) const { return !m_vecItems.empty(); }
  int GetStreamCount(CStreamDetail::StreamType type) const;
  int GetVideoStreamCount(void) const;
  int GetAudioStreamCount(void) const;
  int GetSubtitleStreamCount(void) const;
  static std::string HdrTypeToString(StreamHdrType hdrType);
  const CStreamDetail* GetNthStream(CStreamDetail::StreamType type, int idx) const;

  std::string GetVideoCodec(int idx = 0) const;
  float GetVideoAspect(int idx = 0) const;
  int GetVideoWidth(int idx = 0) const;
  int GetVideoHeight(int idx = 0) const;
  std::string GetVideoHdrType(int idx = 0, bool alt = false) const;
  std::string GetVideoHdrDetail(int idx = 0) const;
  int GetVideoDuration(int idx = 0) const;
  void SetVideoDuration(int idx, const int duration);
  std::string GetStereoMode(int idx = 0) const;
  std::string GetVideoLanguage(int idx = 0) const;

  std::string GetAudioCodec(int idx = 0) const;
  std::string GetAudioLanguage(int idx = 0) const;
  int GetAudioChannels(int idx = 0) const;

  std::string GetSubtitleLanguage(int idx = 0) const;

  void AddStream(CStreamDetail *item);
  void Reset(void);
  void DetermineBestStreams(void);

  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;

  bool SetStreams(const VideoStreamInfo& videoInfo, int videoDuration, const AudioStreamInfo& audioInfo, const SubtitleStreamInfo& subtitleInfo);
private:
  CStreamDetail *NewStream(CStreamDetail::StreamType type);
  std::vector<std::unique_ptr<CStreamDetail>> m_vecItems;
  const CStreamDetailVideo *m_pBestVideo;
  const CStreamDetailAudio *m_pBestAudio;
  const CStreamDetailSubtitle *m_pBestSubtitle;
};
