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
class CAchievementRuntime;
} // namespace KODI::GAME

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfo;

class CGamesGUIInfo : public CGUIInfoProvider
{
public:
  CGamesGUIInfo() = default;
  explicit CGamesGUIInfo(const KODI::GAME::CAchievementRuntime& achievementRuntime)
    : m_achievementRuntime(&achievementRuntime)
  {
  }
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
  const KODI::GAME::CAchievementRuntime& AchievementRuntime() const;

  //! \brief Whether the player wants indicators drawn over the game
  static bool ShowIndicators();

  const KODI::GAME::CAchievementRuntime* m_achievementRuntime{nullptr};
};

} // namespace KODI::GUILIB::GUIINFO
