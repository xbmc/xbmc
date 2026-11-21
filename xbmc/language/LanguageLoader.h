/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

struct StringSettingOption;

namespace ADDON
{
struct AddonEvent;
}

namespace KODI::LANGUAGE
{
/*!
 * \brief Puts a language pack in use, and everything that has to happen for the interface to
 *        come back up in it.
 */
class CLanguageLoader : public ISettingCallback
{
public:
  static CLanguageLoader& GetInstance();

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   * \brief Put a language pack in use.
   * \param[in] language The pack, named either by its addon id or by the locale it is for.
   *            Empty asks for the one the setting names.
   * \param[in] reloadServices Whether the services that draw text are told to draw it again.
   * \return True when the language is in use.
   */
  bool Load(std::string language = "", bool reloadServices = true);

  /*!
   * \brief Take in the language a language addon names when one is installed, updated or
   *        enabled after the language loaded, as the language settings already offer its name.
   * \param[in] event The addon manager's event.
   */
  void OnAddonEvent(const ADDON::AddonEvent& event);

  //! \brief Where the installed language packs live.
  static std::string GetLanguagePath() { return "resource://"; }

  /*!
   * \brief Where one language pack's resources live.
   * \param[in] language The pack, by addon id or by the locale it is for.
   * \return The path, empty when no language was named.
   */
  static std::string GetLanguagePath(const std::string& language);

  /*!
   * \brief Where one language pack states its region profiles.
   * \param[in] language The pack, by addon id or by the locale it is for.
   * \return The path to its langinfo.xml, empty when no language was named.
   */
  static std::string GetLanguageInfoPath(const std::string& language);

  /*!
   * \brief The languages the installed language packs are for.
   * \param[out] languages The languages, each as the code it states and the name it goes by.
   */
  static void GetAddonsLanguageCodes(std::map<std::string, std::string>& languages);

  static void SettingOptionsAudioStreamLanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);
  static void SettingOptionsSubtitleStreamLanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);
  static void SettingOptionsSubtitleDownloadlanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);

private:
  CLanguageLoader() = default;
  ~CLanguageLoader() override = default;
};
} // namespace KODI::LANGUAGE
