/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

typedef struct Translator Translator;

class CURL;

/*!
 \brief Translates old internal paths into new ones

 Translates old videodb:// and musicdb:// paths which used numbers
 to indicate a specific category to new paths using more descriptive
 strings to indicate categories.
 */
class CLegacyPathTranslation
{
public:
  /*!
   \brief Translates old videodb:// paths to new ones

   \param legacyPath Path in the old videodb:// format using numbers
   \return Path in the new videodb:// format using descriptive strings
   */
  static std::string TranslateVideoDbPath(const CURL &legacyPath);
  static std::string TranslateVideoDbPath(const std::string &legacyPath);

  /*!
   \brief Translates old musicdb:// paths to new ones

   \param legacyPath Path in the old musicdb:// format using numbers
   \return Path in the new musicdb:// format using descriptive strings
   */
  static std::string TranslateMusicDbPath(const CURL &legacyPath);
  static std::string TranslateMusicDbPath(const std::string &legacyPath);

private:
  static std::string TranslatePath(const std::string &legacyPath, Translator *translationMap, size_t translationMapSize);
};
