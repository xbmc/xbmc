/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
struct Bcp47Extension
{
  char name;
  std::vector<std::string> segments;
  bool operator==(const Bcp47Extension& other) const = default;
};

class CBcp47
{
public:
  //! @todo Does constructor have to be public? Construction only via ParseTag is not enough?
  CBcp47() = default;

  bool operator==(const CBcp47& other) const = default;

  /*!
     * \brief Parse a language tag into its subtags. The subtags are not altered or validated.
     * \param[in] str Text to parse 
     * \return Object initialized with the subtags of a well-formed tag.
     *         std::nullopt is returned when the text parameter is not a well-formed language tag.
     */
  static std::optional<CBcp47> ParseTag(std::string str);

  /*!
   * \brief Return the validity of the tag per RFC5646 validity rules
   * \return true for a valid tag, otherwise false.
   */
  bool IsValid() const { return m_isValid; }
  void Canonicalize();
  std::string Format() const;

  CBcp47 ToAudioLanguageTag();

  // Accessors
  std::string GetLanguage() const { return m_language; }
  std::vector<std::string> GetExtLangs() const { return m_extLangs; }
  std::string GetScript() const { return m_script; }
  std::string GetRegion() const { return m_region; }
  std::vector<std::string> GetVariants() const { return m_variants; }
  std::vector<Bcp47Extension> GetExtensions() const { return m_extensions; }
  std::vector<std::string> GetPrivateUse() const { return m_privateUse; }
  std::string GetGrandfathered() const { return m_grandfathered; }

  /*!
   * \brief Identify grandfathered tags.
   * \return true for a grandfathered tag, false otherwise.
   */
  bool IsGrandfathered() const { return !m_grandfathered.empty(); }

private:
  bool Validate();
  bool IsValidLanguage() const;
  bool HasMultipleExtLang() const;
  bool IsValidRegion() const;
  bool HasDuplicateVariants() const;
  bool HasDuplicateExtensions() const;

  bool m_isValid{false};

  std::string m_language;
  std::vector<std::string> m_extLangs;
  std::string m_script;
  std::string m_region;
  std::vector<std::string> m_variants;
  std::vector<Bcp47Extension> m_extensions;
  std::vector<std::string> m_privateUse;
  std::string m_grandfathered;
};
} // namespace KODI::UTILS::I18N
