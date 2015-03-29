#pragma once
/*
 *      Copyright (C) 2013-2015 Team XBMC
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

#include <list>
#include <memory>
#include "Addon.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace INFO
{
  class InfoBool;
  typedef std::shared_ptr<InfoBool> InfoPtr;
}

namespace ADDON
{
  class CContextItemAddon : public CAddon
  {
  public:
    CContextItemAddon(const cp_extension_t *ext);
    CContextItemAddon(const AddonProps &props);
    virtual ~CContextItemAddon();

    std::string GetLabel();

    /*!
     * \brief Get the parent category of this context item.
     *
     * \details Returns empty string if at root level or
     * CONTEXT_MENU_GROUP_MANAGE when it should be in the 'manage' submenu.
     */
    const std::string& GetParent() const { return m_parent; }

    /*!
     * \brief Returns true if this contex menu should be visible for given item.
     */
    bool IsVisible(const CFileItemPtr& item) const;

  private:
    std::string m_label;
    std::string m_parent;
    INFO::InfoPtr m_visCondition;
  };

  typedef std::shared_ptr<CContextItemAddon> ContextItemAddonPtr;
}
