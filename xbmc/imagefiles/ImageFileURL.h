/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

class CURL;

namespace IMAGE_FILES
{
/*!
 * \brief A mostly-typed representation of a URL to any image, whether a simple path to an image
 *        file, embedded in another file, or generated in some way.
 *
 * A URL string is good to pass between most classes, store, and transmit, but use this if you need
 * to _create_ or _modify_ a special image path, or are working on the details of such image files.
 *
 * URL string format is `image://[type@]<url_encoded_path>?options`
*/
class CImageFileURL
{
public:
  /*!
   * \brief Create an ImageFileURL from a string representation.
  */
  CImageFileURL(const std::string& imageFileURL);

  /*!
   * \brief Create an ImageFileURL pointing to a specific file.
   *
   * Provide a 'specialType' parameter to specify a special image loader.
   *
   * \param specialType set a special image type - an implementation of \ref ISpecialImageFileLoader
  */
  static CImageFileURL FromFile(const std::string& filePath, std::string specialType = "");

  void AddOption(std::string key, std::string value);

  std::string GetOption(const std::string& key) const;

  /*!
   * \brief Build a complete string representation of this ImageFileURL.
  */
  std::string ToString() const;

  /*!
   * \brief Build a cache key for this ImageFileURL.
   *
   * Return base filePath if not special and multiple options are in a stable order,
   * otherwise return a complete string representation.
  */
  std::string ToCacheKey() const;

  const std::string& GetTargetFile() const { return m_filePath; }

  bool IsSpecialImage() const { return !m_specialType.empty(); }
  const std::string& GetSpecialType() const { return m_specialType; }

  bool flipped{false};

private:
  CImageFileURL();
  std::string m_filePath;
  std::string m_specialType;

  std::map<std::string, std::string> m_options{};
};

/*!
 * \brief Build an ImageFileURL string from a file path.
 *
 * Provide a 'specialType' parameter to specify a special image loader.
 *
 * \param specialType set a special image type - an implementation of \ref ISpecialImageFileLoader
*/
std::string URLFromFile(const std::string& filePath, std::string specialType = "");

/*!
 * \brief Build a cache key for an ImageFileURL string.
*/
std::string ToCacheKey(const std::string& imageFileURL);

} // namespace IMAGE_FILES
