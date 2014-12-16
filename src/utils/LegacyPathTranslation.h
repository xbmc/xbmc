#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
