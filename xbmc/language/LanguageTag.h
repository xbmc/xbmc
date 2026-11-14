/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/Territory.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::LANGUAGE
{
//! Past this a name composed from a tag's subtags is cut short and the tag appended, so that a
//! list of languages stays readable
inline constexpr std::size_t MAX_COMPOSED_NAME_LENGTH{30};

/*!
 * \brief A language, carried as a value rather than as a bare string.
 *
 * Kodi handles language codes in several notations - ISO 639-1, ISO 639-2/B and /T, BCP 47 - and
 * different interfaces require different ones. A tag holds one canonical form and hands out the
 * others by name, so the notation a value is in is part of the value rather than a convention.
 *
 * The form a tag holds is a BCP 47 language tag, spelled the way RFC 5646 recommends - en, en-GB,
 * zh-Hant-HK. Every other notation is derived from it on demand, and none of them is stored.
 *
 * A tag is taken apart where it is built and keeps what that read - the language it names and the
 * place it names - so asking it about either is a read rather than a second look at the text.
 *
 * The members say what they answer with:
 *
 * - ToString is the canonical form itself, which is what the tag is rather than a view of it
 * - As* states the language to a named standard, narrowing where that standard says less
 * - To* expresses the language some other way, such as a name composed for a reader
 * - Get* answers for one part of the tag rather than for the language
 * - In is whichever of the As* and To* members a Notation names
 */
class CLanguageTag
{
public:
  /*!
   * \brief The notations a language can be handed out in, naming one projection each.
   * \note The values are part of the add-on and Python interfaces - kodi::GetLanguage takes them
   *       as LANG_FMT_*, and xbmc.getLanguage as xbmc.ISO_639_1 and its siblings - so both their
   *       order and their spelling are fixed.
   */
  enum Notation
  {
    ISO_639_1,
    ISO_639_2,
    ENGLISH_NAME,
    ISO_NAME
  };

  CLanguageTag() = default;

  /*!
   * \brief Build a tag from a language in any notation: 2-Char (ISO 639-1), 3-Char (ISO 639-2/B
   *        or /T), a BCP 47 language tag, a POSIX locale name (en_GB), or a full English
   *        language name.
   *
   * Text that names no language becomes und, which is what BCP 47 says for a language that is
   * present but not known: such text cannot be chosen, matched, named or narrowed as a language.
   * A language no standard assigns is declared in advancedsettings.xml <languagecodes>, and then
   * parses like any other.
   *
   * \note Empty text gives an empty tag, so a source that declared no language is still
   *       distinguishable from one whose declaration could not be read.
   * \param[in] text The language.
   * \return The tag, und where the text names no language, or an empty tag for empty text.
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
   * \brief A tag for a stream that has a language, where that language is not known.
   * \note This is not the same as a stream with no language at all, which BCP 47 expresses as
   *       zxx and which media uses for instrumental audio.
   * \return The undetermined tag, und.
   */
  static CLanguageTag Undetermined();

  /*!
   * \brief English, the language Kodi falls back to when it has no other answer.
   * \note Named rather than parsed so that it costs nothing and is available at any point in
   *       the process, including before the paths a parse reads its data through are set up.
   * \return The English tag, en.
   */
  static CLanguageTag English();

  /*!
   * \brief Whether the tag holds a language, und included.
   * \note An empty projection is a different question: AsIso6391 is empty for a valid language
   *       that has no ISO 639-1 code.
   * \return true for every tag but an empty one.
   */
  bool IsValid() const { return !m_tag.empty(); }

  /*!
   * \brief Whether the tag names English, in any of its varieties.
   * \return true for en and for anything qualifying it, such as en-GB.
   */
  bool IsEnglish() const;

  /*!
   * \brief Whether the tag names a given language, whatever qualifies either: en-GB is English,
   *        enm - Middle English - is not.
   * \note The subtag as canonical BCP 47 spells it, which is the shortest code: en, not eng.
   * \param[in] subtag The primary language subtag to test against.
   * \return true when the tag names that language.
   */
  bool IsLanguage(std::string_view subtag) const;

  /*!
   * \brief Whether the language is undetermined, i.e. absent or explicitly declared unknown.
   * \note Media declares an unknown language in both ways, and a caller deciding whether it
   *       knows a stream's language treats them alike.
   * \return true for an empty tag and for und.
   */
  bool IsUndetermined() const;

  /*!
   * \brief Whether text in the language is written right to left.
   * \note A script subtag decides it where the tag states one, as it names the writing system
   *       outright: ku-Arab is written right to left, ku is not. Otherwise the script the
   *       language is usually written in decides it.
   * \return true for Hebrew, Arabic, Persian, Urdu and the other languages written in a
   *         right-to-left script. false for a tag naming no language.
   */
  bool IsRightToLeft() const;

  /*!
   * \brief The canonical form, which is a BCP 47 language tag.
   * \return The tag, or an empty string for an empty tag.
   */
  const std::string& ToString() const { return m_tag; }

  /*!
   * \brief The 3-Char ISO 639-2/B code.
   *
   * Narrowing: region, script and variant subtags have no ISO 639 equivalent and are dropped.
   *
   * \return The code, or the primary language subtag when ISO 639-2 assigns the language
   *         none.
   */
  std::string AsIso6392B() const;

  /*!
   * \brief The 3-Char ISO 639-2/T code.
   *
   * Narrowing, as AsIso6392B is. The two forms differ for about twenty languages and are
   * spelled the same for every other.
   *
   * \return The code, or the primary language subtag when ISO 639-2 assigns the language
   *         none.
   */
  std::string AsIso6392T() const;

  /*!
   * \brief The territory the tag names, from its region subtag.
   * \note Not every territory is a country - es-419 places Spanish in Latin America, which
   *       ISO 3166-1 assigns no code to. CTerritory answers that; a caller needing a country
   *       specifically does not have to know which cases to fear.
   * \return The territory, naming no place where the tag states no region, which is true of
   *         most tags.
   */
  CTerritory GetTerritory() const;

  /*!
   * \brief The 2-Char ISO 639-1 code.
   *
   * Narrowing, as AsIso6392B is, and narrower still: ISO 639-1 covers far fewer languages.
   *
   * \return The code, or an empty string for a language that has no ISO 639-1 code.
   */
  std::string AsIso6391() const;

  //! The primary language subtag, as canonical BCP 47 spells it: the ISO 639-1 code where one
  //! exists
  std::string_view Language() const { return {m_tag.data(), m_languageLength}; }

  /*!
   * \brief Whether this is the same tag as another.
   * \note Tags, not languages: en and eng are the same tag once parsed, but en and en-AU are
   *       not. Matches answers whether two tags name the same language.
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
   * \note An empty tag matches only another empty one, so it does not match und even though
   *       IsUndetermined is true of both. IsUndetermined answers that question instead.
   *
   * \param[in] other The tag to compare against.
   * \return true when both name the same language.
   */
  bool Matches(const CLanguageTag& other) const;

  /*!
   * \brief Whether the language of this tag is one the language of \p macrolanguage groups, as
   *        Norwegian groups Norwegian Bokmal: nb is within no.
   * \param[in] macrolanguage The tag naming the macrolanguage.
   * \return true when the subtag registry gives the language of \p macrolanguage as this one's.
   */
  bool IsWithin(const CLanguageTag& macrolanguage) const;

  /*!
   * \brief The English name of the language, as shown to a user.
   * \note Text that is shaped like a tag but names no registered language is described by its own
   *       subtags, so a name is not proof that the language is a real one.
   * \return The name, or an empty string where the tag yields none. Callers decide what to
   *         display in that case.
   */
  std::string ToEnglishName() const;

  /*!
   * \brief The English name of the language alone, without what the subtags qualify it with.
   *
   * Narrowing, as AsIso6392B is: en-AU is named English rather than English (Australia).
   * Suitable where the name is given to something that knows languages but not varieties of
   * them.
   *
   * \return The name, or an empty string where the tag yields none.
   */
  std::string ToEnglishLanguageName() const;

  /*!
   * \brief The language in a notation chosen at run time.
   * \param[in] notation The notation.
   * \return What the member the notation names returns: AsIso6391, AsIso6392B, ToEnglishName or
   *         ToEnglishLanguageName.
   */
  std::string In(Notation notation) const;

private:
  /*!
   * \brief A tag, with where the parse found its parts.
   * \param[in] tag The canonical form.
   * \param[in] languageLength The length of the primary language subtag, which the tag starts
   *            with.
   * \param[in] scriptOffset Where the script subtag starts, or 0 where there is none.
   * \param[in] scriptLength The length of the script subtag, or 0 where there is none.
   * \param[in] regionOffset Where the region subtag starts, or 0 where there is none.
   * \param[in] regionLength The length of the region subtag, or 0 where there is none.
   */
  CLanguageTag(std::string tag,
               std::size_t languageLength,
               std::size_t scriptOffset,
               std::size_t scriptLength,
               std::size_t regionOffset,
               std::size_t regionLength)
    : m_tag(std::move(tag)),
      m_languageLength(languageLength),
      m_scriptOffset(scriptOffset),
      m_scriptLength(scriptLength),
      m_regionOffset(regionOffset),
      m_regionLength(regionLength)
  {
  }

  //! The script subtag, as canonical BCP 47 spells it, or empty where the tag states none
  std::string_view Script() const { return {m_tag.data() + m_scriptOffset, m_scriptLength}; }

  //! The region subtag, as canonical BCP 47 spells it, or empty where the tag states none
  std::string_view Region() const { return {m_tag.data() + m_regionOffset, m_regionLength}; }

  std::string m_tag;
  //! Where the parse found the subtags, so that nothing has to take the tag apart again
  std::size_t m_languageLength{0};
  std::size_t m_scriptOffset{0};
  std::size_t m_scriptLength{0};
  std::size_t m_regionOffset{0};
  std::size_t m_regionLength{0};
};
} // namespace KODI::LANGUAGE
