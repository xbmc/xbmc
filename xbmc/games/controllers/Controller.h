/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "ControllerLayout.h"
#include "ControllerTypes.h"
#include "addons/Addon.h"

#include <atomic>
#include <string>

namespace GAME
{

class CController : public ADDON::CAddon
{
public:
  static std::unique_ptr<CController> FromExtension(ADDON::AddonProps props, const cp_extension_t* ext);

  CController(ADDON::AddonProps addonprops);

  virtual ~CController(void) { }

  static const ControllerPtr EmptyPtr;

  // implementation of IAddon via CAddon
  virtual ADDON::AddonPtr GetRunningInstance() const override;
  virtual void OnPostInstall(bool update, bool modal) override;

  std::string Label(void);
  std::string ImagePath(void) const;

  bool LoadLayout(void);

  const CControllerLayout& Layout(void);

private:
  CControllerLayout m_layout;
  std::atomic<bool> m_bLoaded;
};

}
