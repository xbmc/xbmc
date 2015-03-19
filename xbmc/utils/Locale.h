#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <set>
#include <string>

/*!
 \brief Class representing a full locale of the form [language[_territory][.codeset][@modifier]].
 */
class CLocale
{
public:
  CLocale();
  CLocale(const std::string& language);
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
   [language[_territory][.codeset][@modifier]] where the language is
   represented as a (lower-case) two character ISO 639-1 code and the territory
   is represented as a (upper-case) two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToString() const;
  /*!
   \brief Returns the full string representation of the locale in lowercase.

   \details The format of the string representation is
   [language[_territory][.codeset][@modifier]] where the language is
   represented as a two character ISO 639-1 code and the territory is
   represented as a two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToStringLC() const;
  /*!
   \brief Returns the short string representation of the locale.

   \details The format of the short string representation is
   [language[_territory] where the language is represented as a (lower-case)
   two character ISO 639-1 code and the territory is represented as a
   (upper-case) two character ISO 3166-1 Alpha-2 code.
   */
  std::string ToShortString() const;
  /*!
   \brief Returns the short string representation of the locale in lowercase.

   \details The format of the short string representation is
   [language[_territory] where the language is represented as a two character
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

private:
  static bool CheckValidity(const std::string& language, const std::string& territory, const std::string& codeset, const std::string& modifier);
  static bool ParseLocale(const std::string &locale, std::string &language, std::string &territory, std::string &codeset, std::string &modifier);

  void Initialize();

  int GetMatchRank(const std::string& locale) const;

  bool m_valid;
  std::string m_language;
  std::string m_territory;
  std::string m_codeset;
  std::string m_modifier;
};

