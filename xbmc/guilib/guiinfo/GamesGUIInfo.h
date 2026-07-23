/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
namespace KODI::GAME
{
class CGameSettings;
} // namespace KODI::GAME

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfo;

class CGamesGUIInfo : public CGUIInfoProvider
{
public:
  CGamesGUIInfo() = default;
  explicit CGamesGUIInfo(const KODI::GAME::CGameSettings& gs) : m_gameSettings(&gs) {}
  ~CGamesGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem* item) override;
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const CGUIInfo& info,
                std::string* fallback) const override;
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const CGUIInfo& info) const override;
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const CGUIInfo& info) const override;

private:
  const KODI::GAME::CGameSettings& GameSettings() const;

  const KODI::GAME::CGameSettings* m_gameSettings{nullptr};
};

} // namespace KODI::GUILIB::GUIINFO
