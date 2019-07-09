/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISerializable.h"
#include "utils/IArchivable.h"

#include <memory>
#include <string>
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

  explicit CStreamDetail(StreamType type) : m_eType(type), m_pParent(NULL) {};
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
  CStreamDetailSubtitle& operator=(const CStreamDetailSubtitle &that);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  std::string m_strLanguage;
};

class CStreamDetails final : public IArchivable, public ISerializable
{
public:
  CStreamDetails() { Reset(); };
  CStreamDetails(const CStreamDetails &that);
  CStreamDetails& operator=(const CStreamDetails &that);
  bool operator ==(const CStreamDetails &that) const;
  bool operator !=(const CStreamDetails &that) const;

  static std::string VideoDimsToResolutionDescription(int iWidth, int iHeight);
  static std::string VideoAspectToAspectDescription(float fAspect);

  bool HasItems(void) const { return m_vecItems.size() > 0; };
  int GetStreamCount(CStreamDetail::StreamType type) const;
  int GetVideoStreamCount(void) const;
  int GetAudioStreamCount(void) const;
  int GetSubtitleStreamCount(void) const;
  const CStreamDetail* GetNthStream(CStreamDetail::StreamType type, int idx) const;

  std::string GetVideoCodec(int idx = 0) const;
  float GetVideoAspect(int idx = 0) const;
  int GetVideoWidth(int idx = 0) const;
  int GetVideoHeight(int idx = 0) const;
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
