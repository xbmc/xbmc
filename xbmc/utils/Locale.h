/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>
#include <unordered_map>

/*!
 \brief Class representing a full locale of the form `[language[_territory][.codeset][@modifier]]`.
 */
class CLocale
{
public:
  CLocale();
  explicit CLocale(const std::string& language);
  CLocale(const std::string& language, const std::string& territory);
  CLocale(const std::string& language, const std::string& territory, const std::string& codeset);
  CLocale(const std::string& language, const std::string& territory, const std::string& codeset, const std::string& modifier);
  ~CLocale();

  /*!
   \brief Empty (and invalid) CLocale instance.
   */
  static const CLocale Empty;

  /*!
   \brief Parses the given string representation and turns it into a locale.

   \param locale String representation of a locale
   */
  static CLocale FromString(const std::string& locale);

  bool operator==(const CLocale& other) const;
  inline bool operator!=(const CLocale& other) const { return !(*this == other); }

  /*!
   \brief Whether the locale is valid or not.

   \details A locale is considered valid if at least the language code is set.
   */
  bool IsValid() const { return m_valid; }

  /*!
   \brief Returns the (lower-case) ISO 639-1 language code of the locale.
   */
  const std::string& GetLanguageCode() const { return m_language; }
  /*!
   \brief Returns the (upper-case) ISO 3166-1 Alpha-2 territory code of the locale.
   */
  const std::string& GetTerritoryCode() const { return m_territory; }
  /*!
   \brief Returns the codeset of the locale.
   */
  const std::string& GetCodeset() const { return m_codeset; }
  /*!
   \brief Returns the modifier of the locale.
   */
  const std::string& GetModifier() const { return m_modifier; }

  /*!
   \brief Returns the full string representation of the locale.

   \details The format of the string representation is
   `[language[_territory][.codeset][@modifier]]` where the language is
   represented as a (lower-case) two character ISO 639-1 code and the territory
   is represented as a (upper-case) two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToString() const;
  /*!
   \brief Returns the full string representation of the locale in lowercase.

   \details The format of the string representation is
   `language[_territory][.codeset][@modifier]]` where the language is
   represented as a two character ISO 639-1 code and the territory is
   represented as a two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToStringLC() const;
  /*!
   \brief Returns the short string representation of the locale.

   \details The format of the short string representation is
   `[language[_territory]` where the language is represented as a (lower-case)
   two character ISO 639-1 code and the territory is represented as a
   (upper-case) two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToShortString() const;
  /*!
   \brief Returns the short string representation of the locale in lowercase.

   \details The format of the short string representation is
   `[language[_territory]` where the language is represented as a two character
   ISO 639-1 code and the territory is represented as a two character
   ISO 3166-1 Alpha-2 code.
   */
  std::string ToShortStringLC() const;

  /*!
   \brief Checks if the given string representation of a locale exactly matches
          the locale.

   \param locale String representation of a locale
   \return True if the string representation matches the locale, false otherwise.
   */
  bool Equals(const std::string& locale) const;

  /*!
   \brief Checks if the given string representation of a locale partly matches
          the locale.

   \details Partial matching means that every available locale part needs to
   match the same locale part of the other locale if present.

   \param locale String representation of a locale
   \return True if the string representation matches the locale, false otherwise.
  */
  bool Matches(const std::string& locale) const;

  /*!
  \brief Tries to find the locale in the given list that matches this locale
         best.

  \param locales List of string representations of locales
  \return Best matching locale from the given list or empty string.
  */
  std::string FindBestMatch(const std::set<std::string>& locales) const;

  /*!
  \brief Tries to find the locale in the given list that matches this locale
         best.

  \param locales Map list of string representations of locales with first as
                 locale identifier
  \return Best matching locale from the given list or empty string.

  \remark Used from \ref CAddonInfo::GetTranslatedText to prevent copy from map
          to set.
  */
  std::string FindBestMatch(const std::unordered_map<std::string, std::string>& locales) const;

private:
  static bool CheckValidity(const std::string& language, const std::string& territory, const std::string& codeset, const std::string& modifier);
  static bool ParseLocale(const std::string &locale, std::string &language, std::string &territory, std::string &codeset, std::string &modifier);

  void Initialize();

  int GetMatchRank(const std::string& locale) const;

  bool m_valid = false;
  std::string m_language;
  std::string m_territory;
  std::string m_codeset;
  std::string m_modifier;
};

