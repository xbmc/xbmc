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

#include <algorithm>
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
  static constexpr int STREAM_DETAILS_VERSION = 3;
  static constexpr int STREAM_DETAILS_VERSION_FLAGS =
      3; // Version that introduced flags to audio and subtitle streams

  enum StreamType {
    VIDEO,
    AUDIO,
    SUBTITLE
  };

  // Source of the stream information
  // Order is important - higher values (towards end of list) take precedence over lower values
  // when updating stream details
  enum Source : uint8_t
  {
    UNDEFINED = 0,
    EXTERNAL = 10,
    MEDIA = 20,
    NFO = 30,
    LEGACY = 40
  };

  explicit CStreamDetail(StreamType type) : m_eType(type), m_pParent(nullptr) {}
  ~CStreamDetail() override = default;
  CStreamDetail(const CStreamDetail&) = default;
  CStreamDetail& operator=(const CStreamDetail&) = delete;
  virtual bool IsWorseThan(const CStreamDetail &that) const = 0;
  Source GetSource() const;
  void SetSource(Source source);
  int GetVersion() const { return m_version; }

  const StreamType m_eType;

protected:
  CStreamDetails *m_pParent;
  Source m_source{UNDEFINED};
  int m_version{STREAM_DETAILS_VERSION};
  friend class CStreamDetails;
  friend class CVideoDatabase;
};

// Language codes held by the classes below are ISO 639-2/B, not BCP 47 as used elsewhere in the
// player, and every writer narrows them on the way in. Archive reads and writes that form, while
// Serialize widens to BCP 47 so a JSON-RPC client sees one notation across the API - though only
// ever a bare language, since a value carrying a region lost it before it arrived.
//
// The classes are the shape of the streamdetails table, which smart playlists filter with SQL
// built from user-authored rules (see CSmartPlaylistRule::GetWhereClause). Those rules live in
// .xsp files rather than in the database, so another notation would silently stop matching and no
// database migration could repair it.

class CStreamDetailVideo final : public CStreamDetail
{
public:
  CStreamDetailVideo();
  CStreamDetailVideo(const VideoStreamInfo& info, int duration, Source source);
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
  CStreamDetailAudio(const AudioStreamInfo& info, Source source);
  CStreamDetailAudio(const CStreamDetailAudio&) = default;
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  int m_iChannels = -1;
  std::string m_strCodec;
  std::string m_strLanguage;
  StreamFlags m_flags{StreamFlags::FLAG_NONE};
};

class CStreamDetailSubtitle final : public CStreamDetail
{
public:
  CStreamDetailSubtitle();
  CStreamDetailSubtitle(const SubtitleStreamInfo& info, Source source);
  CStreamDetailSubtitle(const CStreamDetailSubtitle&) = default;
  CStreamDetailSubtitle& operator=(const CStreamDetailSubtitle &that);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  bool IsWorseThan(const CStreamDetail &that) const override;

  std::string m_strLanguage;
  StreamFlags m_flags{StreamFlags::FLAG_NONE};
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

  /*!
   * \brief A stream flag, and the name Kodi reads and writes for it in an NFO.
   */
  struct FlagName
  {
    StreamFlags flag;
    std::string_view name;
  };

  /*!
   * \brief The vocabulary of stream flag names used in NFOs.
   *
   * Written out in alphabetical order.
   */
  static constexpr auto STREAM_FLAG_NAMES = std::to_array<FlagName>({
      {StreamFlags::FLAG_COMMENT, "comment"},
      {StreamFlags::FLAG_DEFAULT, "default"},
      {StreamFlags::FLAG_DUB, "dub"},
      {StreamFlags::FLAG_FORCED, "forced"},
      {StreamFlags::FLAG_HEARING_IMPAIRED, "hearingimpaired"},
      {StreamFlags::FLAG_KARAOKE, "karaoke"},
      {StreamFlags::FLAG_LYRICS, "lyrics"},
      {StreamFlags::FLAG_ORIGINAL, "original"},
      {StreamFlags::FLAG_STILL_IMAGES, "stillimages"},
      {StreamFlags::FLAG_VISUAL_IMPAIRED, "visualimpaired"},
      {StreamFlags::FLAG_WEBVTT_DATA_PACKETS, "webvttdatapackets"},
  });

  static_assert(std::ranges::is_sorted(STREAM_FLAG_NAMES, {}, &FlagName::name),
                "STREAM_FLAG_NAMES must be in alphabetical order - StreamFlagsToNames() walks it "
                "in order and its callers rely on the names coming out sorted");

  static std::string VideoDimsToResolutionDescription(int iWidth, int iHeight);
  static std::string VideoAspectToAspectDescription(float fAspect);

  /*!
   * \brief Break a flag set out into its names, in alphabetical order.
   * \param flags The flags to name.
   * \return One entry per set bit that has a name; empty for FLAG_NONE.
   */
  static std::vector<std::string> StreamFlagsToNames(StreamFlags flags);

  /*!
   * \brief Look a single flag up by name.
   * \param name The name to look up. Leading and trailing space is ignored, as is case.
   * \return The matching flag, or FLAG_NONE if the name isn't known.
   */
  static StreamFlags StreamFlagFromName(std::string_view name);

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

  /*!
   * \name Audio stream properties
   *
   * An idx of 0 asks for the technically best stream, as judged by StreamUtils::CompareAudioQuality()
   * (based on codec and channel count only).
   *
   * Any other idx is a 1-based ordinal into the streams in the order the source presented them.
   *
   * @{
   */
  std::string GetAudioCodec(int idx = 0) const;
  std::string GetAudioLanguage(int idx = 0) const;
  int GetAudioChannels(int idx = 0) const;
  StreamFlags GetAudioFlags(int idx = 0) const;
  /*! @} */

  std::string GetSubtitleLanguage(int idx = 0) const;
  StreamFlags GetSubtitleFlags(int idx = 0) const;

  /*!
   * \brief Get the index of the best audio stream in the given language.
   *
   * \param language The preferred audio language as an ISO 639 code, empty for no preference
   * \return The 1-based index of the best stream in that language, or 0 (meaning the technically
   *         best stream, see GetAudioCodec()) when no preference was given or no stream matches
   */
  int GetPreferredAudioStreamIndex(const std::string& language) const;

  /*!
   * \brief Get the language of the first audio stream in the order the source lists them.
   *
   * This is neither the technically best stream (GetAudioLanguage()) nor the one the user's
   * preferences ask for (GetPreferredAudioStreamIndex()) - it is simply the stream listed first
   * by whatever produced these details, which is one of:
   *
   * - a bluray playlist, whose streams are stored in stream number order, so the first is audio
   *   stream number 1 - the stream the disc expects a player to start with. This is the only
   *   case where the order is specified rather than conventional.
   * - any other container, in the order its demuxer reports the streams.
   * - an NFO, in the order its elements appear.
   *
   * Note this does not say which stream playback will actually start on. That also depends on
   * stream flags, a choice remembered from a previous watch and the audio layout at play time,
   * none of which the library carries.
   *
   * Note also that "first" is the order the details were stored, which is the order the source
   * presented for freshly scanned or disc-browsed content. It is not currently guaranteed to
   * survive a round trip through the video database, which stores no stream ordinal and reads
   * the rows back without an ORDER BY.
   *
   * \todo Persist a stream ordinal and order by it, so this holds unconditionally.
   *
   * \return The language of the first audio stream, or an empty string if there is none
   */
  std::string GetFirstAudioLanguage() const;

  /*!
   * \brief Get the codec of the first audio stream in the order the source lists them.
   *
   * The counterpart of GetFirstAudioLanguage() for the codec.
   *
   * \return The codec of the first audio stream, or an empty string if there is none
   */
  std::string GetFirstAudioCodec() const;

  /*!
   * \brief Get the channel count of the first audio stream in the order the source lists them.
   *
   * The counterpart of GetFirstAudioLanguage() for the channel count.
   *
   * \return The channel count of the first audio stream, or -1 if there is none
   */
  int GetFirstAudioChannels() const;

  /*!
   * \brief Get the language of the first subtitle stream in the order the source lists them.
   *
   * The counterpart of GetFirstAudioLanguage() for subtitles, ie. presentation graphic stream
   * number 1 of a bluray playlist. Note that this says nothing about whether subtitles are
   * displayed to begin with, which the locale.subtitlelanguage setting decides.
   *
   * \return The language of the first subtitle stream, or an empty string if there is none
   */
  std::string GetFirstSubtitleLanguage() const;

  void AddStream(CStreamDetail *item);
  void Reset(void);
  void DetermineBestStreams(void);

  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;

  bool SetStreams(const VideoStreamInfo& videoInfo,
                  int videoDuration,
                  const AudioStreamInfo& audioInfo,
                  const SubtitleStreamInfo& subtitleInfo,
                  CStreamDetail::Source source);

  CStreamDetail::Source GetSource(CStreamDetail::StreamType type, int idx) const;
  CStreamDetail::Source GetSources() const;
  void SetSources(CStreamDetail::Source source);
  bool ShouldUpdateWithNewDetails(const CStreamDetails& newInfo) const;

  int GetVersion(CStreamDetail::StreamType type, int idx) const;

private:
  CStreamDetail *NewStream(CStreamDetail::StreamType type);
  std::vector<std::unique_ptr<CStreamDetail>> m_vecItems;
  const CStreamDetailVideo *m_pBestVideo;
  const CStreamDetailAudio *m_pBestAudio;
  const CStreamDetailSubtitle *m_pBestSubtitle;
};
