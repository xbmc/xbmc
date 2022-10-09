/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

#include <memory>

class CApplicationPlayer;
class CVideoInfoTag;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CVideoGUIInfo : public CGUIInfoProvider
{
public:
  CVideoGUIInfo();
  ~CVideoGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const override;
  bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const CGUIInfo& info,
                        std::string* fallback) override;
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;

private:
  int GetPercentPlayed(const CVideoInfoTag* tag) const;
  bool GetPlaylistInfo(std::string& value, const CGUIInfo& info) const;

  const std::shared_ptr<CApplicationPlayer> m_appPlayer;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
