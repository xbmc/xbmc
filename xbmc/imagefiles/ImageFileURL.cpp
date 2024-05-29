/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageFileURL.h"

#include "URL.h"
#include "utils/URIUtils.h"

#include <charconv>

namespace IMAGE_FILES
{

CImageFileURL::CImageFileURL(const std::string& imageFileURL)
{
  if (!URIUtils::IsProtocol(imageFileURL, "image"))
  {
    m_filePath = imageFileURL;
    return;
  }

  CURL url{imageFileURL};
  m_filePath = url.GetHostName();
  m_specialType = url.GetUserName();

  url.GetOptions(m_options);

  auto option = m_options.find("flipped");
  if (option != m_options.end())
  {
    flipped = true;
    m_options.erase(option);
  }
}

CImageFileURL::CImageFileURL()
{
}

CImageFileURL CImageFileURL::FromFile(const std::string& filePath, std::string specialType)
{
  if (URIUtils::IsProtocol(filePath, "image"))
  {
    // this function should not be called with an image file URL
    return CImageFileURL(filePath);
  }

  CImageFileURL item;
  item.m_filePath = filePath;
  item.m_specialType = std::move(specialType);

  return item;
}

void CImageFileURL::AddOption(std::string key, std::string value)
{
  m_options[std::move(key)] = std::move(value);
}

std::string CImageFileURL::GetOption(const std::string& key) const
{
  auto value = m_options.find(key);
  if (value == m_options.end())
  {
    return {};
  }
  return value->second;
}

std::string CImageFileURL::ToString() const
{
  CURL url;
  url.SetProtocol("image");
  url.SetUserName(m_specialType);
  url.SetHostName(m_filePath);
  if (flipped)
    url.SetOption("flipped", "");
  for (const auto& option : m_options)
    url.SetOption(option.first, option.second);

  return url.Get();
}

std::string CImageFileURL::ToCacheKey() const
{
  if (!flipped && m_specialType.empty() && m_options.empty())
    return m_filePath;

  return ToString();
}

std::string URLFromFile(const std::string& filePath, std::string specialType)
{
  return CImageFileURL::FromFile(filePath, std::move(specialType)).ToString();
}

std::string ToCacheKey(const std::string& imageFileURL)
{
  return CImageFileURL(imageFileURL).ToCacheKey();
}

} // namespace IMAGE_FILES
