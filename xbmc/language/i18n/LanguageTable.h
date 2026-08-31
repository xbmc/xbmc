/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::LANGUAGE::I18N
{
/*!
 * \brief Every language Kodi can name, and the codes each answers to.
 *
 * Built once from the ISO 639-1 and ISO 639-2 data, then extended at startup by the languages
 * declared in advancedsettings.xml. A declaration replaces whatever a code named before it, so
 * once the table is built nothing can tell the two sources apart - which is the point, as no
 * lookup then has to consult the user's declarations separately.
 */
class CLanguageTable
{
public:
  static CLanguageTable& GetInstance();

  /*!
   * \brief Take the languages declared in advancedsettings.xml into the table.
   * \param[in] languages The declarations, as language code to English name.
   */
  void Declare(const std::map<std::string, std::string>& languages);

  /*!
   * \brief Take the names installed language addons give languages into the table.
   *
   * Names only. A language an addon names is not thereby a language a user picks from a list -
   * List() answers that, and the addons contribute to it separately.
   *
   * \note A name already held is kept, so the ISO 639 tables and a user's <languagecodes> both
   *       outrank an addon. That is the order this had when naming was a live lookup the addons
   *       only answered after the table missed, and it is the order upstream settled on.
   * \param[in] languages The names, as language code to English name.
   */
  void DeclareNames(const std::map<std::string, std::string>& languages);

  /*! \brief Discard the declared languages, leaving the ISO 639 data alone. */
  void Reset();

  /*!
   * \brief The English name of a language.
   * \param[in] code The language code, in whichever notation the table holds it. Case and
   *            surrounding whitespace are not significant.
   * \return The name, or nullopt when no language answers to the code.
   */
  std::optional<std::string> NameOf(std::string_view code) const;

  /*!
   * \brief The code of a language given by its English name.
   * \param[in] name The English name, or any of the alternative names ISO 639-2 records for it.
   *            Case and surrounding whitespace are not significant.
   * \return The ISO 639-1 code where that standard records the name, the ISO 639-2/T code where
   *         only that one does, or nullopt when no language goes by the name.
   */
  std::optional<std::string> CodeOf(std::string_view name) const;

  /*!
   * \brief Every language the table holds, to build a list to choose from.
   * \note Keyed by the ISO 639-1 codes, as that is the code space a language a user picks from a
   *       list is stored in. A declared language is listed whatever notation it was written in.
   * \param[in,out] languages Map to assign code to English name pairs into.
   */
  void List(std::map<std::string, std::string>& languages) const;

private:
  CLanguageTable();

  void Seed();

  //! Language code, lowercased, to English name
  std::map<std::string, std::string> m_names;
  //! English name, lowercased, to language code
  std::map<std::string, std::string> m_codes;
  //! The declarations alone, so a listing can offer them alongside the ISO 639 languages
  std::map<std::string, std::string> m_declared;
};
} // namespace KODI::LANGUAGE::I18N
