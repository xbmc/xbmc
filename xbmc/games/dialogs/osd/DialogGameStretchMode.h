/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "DialogGameVideoSelect.h"
#include "cores/GameSettings.h"

#include <vector>

namespace KODI
{
namespace GAME
{
  class CDialogGameStretchMode : public CDialogGameVideoSelect
  {
  public:
    CDialogGameStretchMode();
    ~CDialogGameStretchMode() override = default;

  protected:
    // implementation of CDialogGameVideoSelect
    std::string GetHeading() override;
    void PreInit() override;
    void GetItems(CFileItemList &items) override;
    void OnItemFocus(unsigned int index) override;
    unsigned int GetFocusedItem() const override;
    void PostExit() override;

  private:
    struct ViewModeProperties
    {
      int stringIndex;
      RETRO::VIEWMODE viewMode;
    };

    std::vector<ViewModeProperties> m_viewModes;

    /*!
     * \brief The list of all the view modes along with their properties
     */
    static const std::vector<ViewModeProperties> m_allViewModes;
  };
}
}
