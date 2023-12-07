/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoActionProcessorHelper
{
public:
  CVideoActionProcessorHelper(const std::shared_ptr<CFileItem>& item,
                              const std::shared_ptr<const CFileItem>& videoVersion)
    : m_item{item}, m_videoVersion{videoVersion}
  {
  }
  virtual ~CVideoActionProcessorHelper();

  std::shared_ptr<CFileItem> ChooseVideoVersion();

private:
  CVideoActionProcessorHelper() = delete;
  void SetDefaultVideoVersion();
  void RestoreDefaultVideoVersion();

  std::shared_ptr<CFileItem> m_item;
  std::shared_ptr<const CFileItem> m_videoVersion;
  int m_defaultVideoVersionFileId{-1};
  std::string m_defaultVideoVersionDynPath;
  bool m_restoreFolderFlag{false};
};
} // namespace GUILIB
} // namespace VIDEO
