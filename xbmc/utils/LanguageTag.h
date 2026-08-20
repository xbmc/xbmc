/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>

namespace KODI::UTILS
{
/*!
 * \brief A language, carried as a value rather than as a bare string.
 *
 * Kodi handles language codes in several notations - ISO 639-1, ISO 639-2/B and /T, BCP 47 - and
 * different interfaces require different ones. A tag holds one canonical form and hands out the
 * others by name, so the notation a value is in is part of the value rather than a convention.
 */
class CLanguageTag
{
public:
  CLanguageTag() = default;

  /*!
   * \brief Build a tag from a language in any notation: 2-Char (ISO 639-1), 3-Char (ISO 639-2/B
   *        or /T), a BCP 47 language tag, or a full English language name.
   * \param[in] text The language.
   * \return The tag. Text that cannot be recognized is kept verbatim rather than discarded, so a
   *         value from an NFO, an addon or advancedsettings.xml survives a round trip.
   */
  static CLanguageTag Parse(const std::string& text);

  /*!
   * \brief Build a tag from text only if the text names a language.
   * \param[in] text The candidate language.
   * \return The tag, or nullopt when the text is not a language. Suitable where the answer
   *         decides something, such as whether a filename token is a language or part of a name.
   */
  static std::optional<CLanguageTag> TryParse(const std::string& text);

  /*!
   * \brief Find a language tag written inside curly braces within text.
   * \note Media carries a tag this way where its container cannot express one, so that a track
   *       can state a language the container's own field could not hold.
   * \param[in] text The text to search, such as a track title.
   * \return The tag, or nullopt when the text holds none.
   */
  static std::optional<CLanguageTag> FindInText(const std::string& text);

  /*!
   * \brief A tag for a stream that has a language, where that language is not known.
   * \note This is not the same as a stream with no language at all, which BCP 47 expresses as
   *       zxx and which media uses for instrumental audio.
   * \return The undetermined tag, und.
   */
  static CLanguageTag Undetermined();

  /*!
   * \brief Whether the tag carries no language at all.
   * \return true for a default-constructed tag or one parsed from empty text.
   */
  bool IsEmpty() const { return m_tag.empty(); }

  /*!
   * \brief Whether the language is undetermined, i.e. absent or explicitly declared unknown.
   * \note Media declares an unknown language in both ways, and a caller deciding whether it
   *       knows a stream's language treats them alike.
   * \return true for an empty tag and for und.
   */
  bool IsUndetermined() const;

  /*!
   * \brief The BCP 47 language tag.
   * \return The tag, or the unrecognized text it was built from.
   */
  const std::string& AsBcp47() const { return m_tag; }

  /*!
   * \brief The 3-Char ISO 639-2/B code.
   *
   * Narrowing: region, script and variant subtags have no ISO 639 equivalent and are dropped.
   *
   * \return The code, or the tag itself when no ISO 639-2 code exists for the language.
   */
  std::string AsIso6392B() const;

  /*!
   * \brief The 3-Char ISO 639-2/T code.
   *
   * Narrowing, as AsIso6392B is. The two forms differ for about twenty languages and are
   * spelled the same for every other.
   *
   * \return The code, or the tag itself when no ISO 639-2 code exists for the language.
   */
  std::string AsIso6392T() const;

  /*!
   * \brief The 2-Char ISO 639-1 code.
   *
   * Narrowing, as AsIso6392B is, and narrower still: ISO 639-1 covers far fewer languages.
   *
   * \return The code, or an empty string for a language that has no ISO 639-1 code.
   */
  std::string AsIso6391() const;

  /*!
   * \brief Whether this is the same tag as another.
   * \note Tags, not languages: en and eng are the same tag once parsed, but en and en-AU are
   *       not. Matches answers whether two tags name the same language.
   * \return true when the tags are identical.
   */
  bool operator==(const CLanguageTag& other) const = default;

  /*!
   * \brief Whether this tag names the same language as another.
   *
   * Unlike operator==, neither the notation the tags were built from nor the subtags qualifying
   * them affect the answer: en, eng, en-AU and en-GB all name English and so all match. Suitable
   * where a user has stated a language and means any variety of it.
   *
   * This is a question about languages, not about tags. It is not the tag matching RFC 4647
   * defines, which asks whether a tag falls within a range and so answers differently depending
   * on which tag is asked about; this is symmetric, and no tag is more specific than another.
   * Callers needing to tell varieties apart, or to rank one above another, need the subtags
   * themselves rather than this.
   *
   * \note An extlang qualifies the language its prefix names, in the same way a region does, so
   *       zh-yue names Chinese here and matches zh. RFC 5646 canonicalizes zh-yue to yue, which
   *       names a language of its own and does not match.
   *
   * \note A tag naming no language matches only another naming no language, so an empty tag does
   *       not match und even though IsUndetermined is true of both. IsUndetermined answers that
   *       question instead.
   *
   * \param[in] other The tag to compare against.
   * \return true when both name the same language.
   */
  bool Matches(const CLanguageTag& other) const;

  /*!
   * \brief The English name of the language, as shown to a user.
   * \note Text that is shaped like a tag but names no registered language is described by its own
   *       subtags, so a name is not proof that the language is a real one.
   * \return The name, or an empty string where the tag yields none. Callers decide what to
   *         display in that case.
   */
  std::string GetEnglishName() const;

  /*!
   * \brief The English name of the language alone, without what the subtags qualify it with.
   *
   * Narrowing, as AsIso6392B is: en-AU is named English rather than English (Australia).
   * Suitable where the name is given to something that knows languages but not varieties of
   * them.
   *
   * \return The name, or an empty string where the tag yields none.
   */
  std::string GetEnglishLanguageName() const;

private:
  explicit CLanguageTag(std::string tag) : m_tag(std::move(tag)) {}

  std::string m_tag;
};
} // namespace KODI::UTILS
