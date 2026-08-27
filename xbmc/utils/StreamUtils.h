/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "utils/LanguageTag.h"

#include <cstdint>
#include <string>
#include <string_view>

static constexpr int MP4_BOX_HEADER_SIZE = 8;

class StreamUtils
{
public:
  /*!
   * \brief The settings that decide which audio stream playback starts with.
   *
   * Read once and reused for every comparison, so that a setting changed midway through
   * cannot make a sort order inconsistent, and so a sort does not pay for a settings lookup
   * per comparison.
   */
  struct AudioPreferences
  {
    //! \brief The language wanted. Empty when the preference is not a language.
    KODI::UTILS::CLanguageTag language;

    //! \brief The preference is "original language": prefer whichever stream is flagged original.
    bool preferOriginal{false};

    //! \brief The preference is "media default"
    bool mediaDefault{false};

    bool preferHearingImpaired{false};
    bool preferVisualImpaired{false};
    bool preferDefaultFlag{false};

    //! \brief Prefer a stereo stream. This follows from the audio output layout rather than a
    //!        setting, so only the player can fill it in; the library leaves it false.
    bool preferStereo{false};

    /*!
     * \brief Read the settings as they currently stand.
     * \note preferStereo is not a setting and is left false. The player sets it afterwards.
     */
    static AudioPreferences Current();
  };

  /*!
   * \brief One audio stream
   *
   * Used by player's SelectionStream and library's CStreamDetailAudio
   */
  struct AudioCandidate
  {
    KODI::UTILS::CLanguageTag language;
    std::string_view codec;
    int channels{0};
    StreamFlags flags{StreamFlags::FLAG_NONE};
  };

  /*!
   * \brief Compare two audio streams the way playback picks the stream it starts on.
   *
   * The tiers, in order: the wanted language (or the original flag when that is the
   * preference), the hearing and visual impaired flags, the default flag when the user asked
   * for it, a stereo layout, technical quality, and finally the default flag as a tiebreak.
   *
   * \param lh The first stream
   * \param rh The second stream
   * \param preferences The settings in force, see AudioPreferences::Current()
   * \return A positive value when the first stream is the better match, a negative value when
   *         the second is, and zero when these properties cannot separate the two
   *
   * \note This is a strict weak ordering, as CSelectionStreams::Get() hands it to
   *       std::stable_sort. Every tier is a property each stream decides for itself, never one
   *       that depends on the pair.
   */
  static int CompareAudioPreference(const AudioCandidate& lh,
                                    const AudioCandidate& rh,
                                    const AudioPreferences& preferences);

  static int GetCodecPriority(const std::string& codec);

  /*!
   * \brief Compare two audio streams on the technical quality of what they carry.
   *
   * \param codecA The codec name of the first stream, as GetCodecPriority() expects it
   * \param channelsA The channel count of the first stream, zero or negative when unknown
   * \param codecB The codec name of the second stream
   * \param channelsB The channel count of the second stream
   * \return A positive value when the first stream is better, a negative value when the second
   *         is, and zero when the two are equally good (which is not the same as interchangeable,
   *         as equally ranked codecs are different presentations)
   */
  static int CompareAudioQuality(const std::string& codecA,
                                 int channelsA,
                                 const std::string& codecB,
                                 int channelsB);

  /*!
   * \brief Make a FourCC code as unsigned integer value
   * \param c1 The first FourCC char
   * \param c2 The second FourCC char
   * \param c3 The third FourCC char
   * \param c4 The fourth FourCC char
   * \return The FourCC as unsigned integer value
   */
  static constexpr uint32_t MakeFourCC(char c1, char c2, char c3, char c4)
  {
    return ((static_cast<uint32_t>(c1) << 24) | (static_cast<uint32_t>(c2) << 16) |
            (static_cast<uint32_t>(c3) << 8) | (static_cast<uint32_t>(c4)));
  }

  /*!
   * \brief Get the codec name translated from ffmpeg codec id and profile
   * \param codecId The ffmpeg codec id
   * \param profile The ffmpeg codec profile
   * \return The codec name
   */
  static std::string GetCodecName(int codecId, int profile);

  /*!
   * \brief Normalise an externally supplied (eg. NFO) audio codec name to the name Kodi uses
   *
   * \param codec The audio codec name, lowercased
   * \return The equivalent Kodi codec name, or the codec name unchanged if nothing maps
   */
  static std::string NormalizeAudioCodecName(const std::string& codec);

  /*!
   * \brief Return a default channel layout in x.y.z form for a channel count.
   * \param[in] channels the count of channels
   * \return the default layout
   */
  static std::string GetDefaultLayout(unsigned int channels);

  /*!
   * \brief Return a default channel layout for a channel count or localized count of channels
   * when no default exists.
   * \param[in] channels the count of channels
   * \return the layout
   */
  static std::string GetLayout(unsigned int channels);

  /*!
   * \brief Determines if a codec support forced overlays (on image type subtitles).
   * \param codecId The ffmpeg codec id
   * \return True when support forced overlay, otherwise false
   */
  static bool IsCodecSupportForcedOverlay(int codecId);
};
