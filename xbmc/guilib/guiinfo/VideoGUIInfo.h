/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
#include "threads/CriticalSection.h"
#include "video/geometry/EffectiveGeometry.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CApplicationPlayer;
class CVideoInfoTag;

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfo;

class CVideoGUIInfo : public CGUIInfoProvider
{
public:
  CVideoGUIInfo();
  ~CVideoGUIInfo() override = default;

  //! \brief Expire the content geometry resolved during the info refresh that is ending, from
  //! CGUIInfoManager::ResetCache(). One resolution answers every member of a refresh.
  void ResetContentGeometry();

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem* item) override;
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const CGUIInfo& info,
                std::string* fallback) const override;
  bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const CGUIInfo& info,
                        std::string* fallback) override;
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const CGUIInfo& info) const override;
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const CGUIInfo& info) const override;

private:
  int GetPercentPlayed(const CVideoInfoTag* tag) const;
  bool GetPlaylistInfo(std::string& value, const CGUIInfo& info) const;

  /*!
   * \brief Answer one content geometry member, for the item or for what is playing.
   *
   * \param item the list item to answer for, or nullptr for the player
   * \param index which of the ratios the title contains, for the members that index
   * \return false when the member is not one of the geometry ones, so the caller falls through
   */
  bool GetContentAspectLabel(std::string& value, const CFileItem* item, int id, int index) const;

  //! \brief Whether the title's geometry changes partway through. \p item nullptr for the player.
  bool GetContentAspectVaries(const CFileItem* item) const;

  //! \brief The ratios in force, resolved once for the refresh and held under m_geometrySection.
  const KODI::VIDEO::GEOMETRY::ContentAspectSet& ContentAspects(const CFileItem* item) const;

  mutable CCriticalSection m_geometrySection;
  mutable bool m_playerAspectsValid{false};
  mutable KODI::VIDEO::GEOMETRY::ContentAspectSet m_playerAspects;

  //! \brief Keyed by item path: a list draws many items in the refresh that one snapshot has
  //! to cover, and they are not the same title.
  mutable std::vector<std::pair<std::string, KODI::VIDEO::GEOMETRY::ContentAspectSet>>
      m_itemAspects;

  const std::shared_ptr<CApplicationPlayer> m_appPlayer;
};

} // namespace KODI::GUILIB::GUIINFO
