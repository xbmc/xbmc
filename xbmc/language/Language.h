/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/LanguageTag.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace ADDON
{
class CLanguageResource;
}

namespace KODI::LANGUAGE
{
class CLangInfo;

using LanguageResourcePtr = std::shared_ptr<ADDON::CLanguageResource>;

// The setting values that name no language
constexpr std::string_view languageSettingDefault = "default";
constexpr std::string_view languageSettingOriginal = "original";
constexpr std::string_view audioLanguageSettingMediaDefault = "mediadefault";
constexpr std::string_view subtitleLanguageSettingNone = "none";
constexpr std::string_view subtitleLanguageSettingForcedOnly = "forced_only";

/*!
 * \brief What the user asked for when choosing an audio or subtitle language.
 *
 * A setting states either a language or a way of choosing one, and the two are not
 * interchangeable: asking for the original-language track is not the same question as asking for
 * a track in a named language, and neither is answerable by the other.
 */
class CLanguagePreference
{
public:
  enum class Kind
  {
    Language, //!< the language the preference names
    FollowUI, //!< whichever language the interface is in
    MediaDefault, //!< whatever the media itself presents first - audio only
    Original, //!< the track carrying the work's original language
    Off, //!< nothing at all - subtitles only; not None, which X11 defines as a macro
    ForcedOnly, //!< only tracks marked forced - subtitles only
  };

  CLanguagePreference() = default;

  /*!
   * \brief Read an audio language setting.
   * \param[in] setting The setting value. Empty is no preference.
   * \return The preference, or nothing where the text names neither a language nor a choice.
   */
  static std::optional<CLanguagePreference> ForAudio(const std::string& setting);

  /*!
   * \brief Read a subtitle language setting.
   * \param[in] setting The setting value.
   * \return The preference, as ForAudio describes.
   */
  static std::optional<CLanguagePreference> ForSubtitles(const std::string& setting);

  Kind GetKind() const { return m_kind; }
  bool Is(Kind kind) const { return m_kind == kind; }

  /*!
   * \brief The language the preference names.
   * \return The language, or an empty tag for every kind that names none.
   */
  const CLanguageTag& GetLanguage() const { return m_language; }

  bool operator==(const CLanguagePreference& other) const = default;

private:
  CLanguagePreference(Kind kind, CLanguageTag language)
    : m_kind(kind),
      m_language(std::move(language))
  {
  }

  static std::optional<CLanguagePreference> Parse(const std::string& setting);

  Kind m_kind{Kind::FollowUI};
  CLanguageTag m_language;
};

/*!
 * \brief What language Kodi is running in and what languages the viewer asked to hear and read.
 */
class CLanguage
{
public:
  //! The leading words a sort steps over, each held with the separator that follows it
  using Tokens = std::set<std::string, std::less<>>;

  //! What may follow a sort token where its declaration states no separators
  static constexpr std::string_view DEFAULT_SORT_TOKEN_SEPARATORS{" ._"};

  /*!
   * \brief Add a sort word to a set of tokens, once per separator that may follow it.
   * \param[in,out] tokens The set to add to.
   * \param[in] word The word.
   * \param[in] separators The characters that may follow the word, or empty for the word alone.
   */
  static void AddSortToken(Tokens& tokens, const std::string& word, std::string_view separators);

  static CLanguage& GetInstance();

  CLanguage() = default;

  /*!
   * \brief The language the interface is in.
   * \note This is the language of the pack the user chose, so it carries whatever region that
   *       pack states - en-GB and en-US are different answers, and anything ranking a
   *       translation by territory needs them to be.
   */
  const CLanguageTag& UI() const { return m_ui; }

  /*!
   * \brief The language an audio track should be matched against.
   *
   * The language the audio preference names. Any other choice - the interface language, the
   * media's own first track, the original-language track - is answered by the interface
   * language, so that a caller with nothing better to match against still has a language.
   */
  CLanguageTag Audio() const;

  /*!
   * \brief The language a subtitle track should be matched against.
   *
   * The subtitle preference, falling back to the audio one: a viewer who has stated no subtitle
   * language is answered by the one they asked to hear. Where neither states a language, the
   * caller that knows what is playing answers it with that instead, as it is not a setting.
   *
   * \param[in] fallbackToUI Whether a preference naming no language is answered by the interface
   *            language.
   * \return The language, or an empty tag where neither preference names one and there is no
   *         fallback.
   */
  CLanguageTag Subtitle(bool fallbackToUI = true) const;

  /*!
   * \brief What the user asked for, rather than the language it resolves to.
   * \note For the caller that has to act on the choice itself - preferring the media's own
   *       first track, or the one flagged original, neither of which is a language.
   */
  const CLanguagePreference& AudioPreference() const { return m_audio; }

  //! \brief What the user asked for, as AudioPreference describes.
  const CLanguagePreference& SubtitlePreference() const { return m_subtitle; }

  /*!
   * \brief The name the active language pack states for itself, in English.
   * \return The name, or empty when no pack has been loaded.
   */
  std::string PackName() const;

  /*!
   * \brief The character set the interface's text is in.
   * \return The set the user chose, or the one the pack states where the user chose none.
   */
  std::string GuiCharset() const;

  //! \brief The character set subtitles are read as, as GuiCharset describes.
  std::string SubtitleCharset() const;

  /*!
   * \brief The leading words a sort steps over - "the", "le", "der".
   * \return The pack's words, plus the declared ones.
   */
  const Tokens& SortTokens() const { return m_sortTokens; }

  /*!
   * \brief Run the interface in the language pack the user chose.
   * \param[in] pack The pack, or nullptr to fall back to English.
   */
  void SetPack(const LanguageResourcePtr& pack);

  /*!
   * \brief The words advancedsettings.xml asks a sort to step over, alongside the pack's.
   * \param[in] tokens The words, replacing any declared before.
   */
  void DeclareSortTokens(Tokens tokens);

  //! \brief The interface language without loading a pack. For tests only.
  void SetUI(const CLanguageTag& language) { m_ui = language; }
  /*!
   * \brief Apply the audio language setting.
   * \return false where the value names nothing, which then follows the interface language.
   */
  bool SetAudio(const std::string& setting);
  //! \brief Apply the subtitle language setting, as SetAudio describes.
  bool SetSubtitle(const std::string& setting);

private:
  void MergeSortTokens();

  //! English until a pack states otherwise
  CLanguageTag m_ui{CLanguageTag::English()};
  CLanguagePreference m_audio;
  CLanguagePreference m_subtitle;
  LanguageResourcePtr m_pack;
  Tokens m_declaredTokens;
  //! The pack's words and the declared ones, merged once as either changes
  Tokens m_sortTokens;
};

/*!
 * \brief Name what Kodi is running in, for an interface that states how it wants it named.
 *
 * The add-on and Python interfaces let a caller choose the notation, and whether the place is
 * named alongside the language. The language is the interface's and the place is the one the
 * selected region profile is for, so the two are separate choices and either can answer with
 * nothing.
 *
 * \param[in] notation How the language is to be named.
 * \param[in] language The interface language and the pack it came from.
 * \param[in] region The region profile the place is taken from.
 * \param[in] withRegion Whether the place is named after the language, separated by "-".
 * \return The name, empty where the language has none in that notation. A place is never named
 *         on its own, so a language that cannot be named answers with nothing at all.
 */
std::string DescribeLanguage(CLanguageTag::Notation notation,
                             const CLanguage& language,
                             const CLangInfo& region,
                             bool withRegion);
} // namespace KODI::LANGUAGE
