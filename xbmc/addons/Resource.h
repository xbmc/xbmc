#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include <memory>

#include "addons/Addon.h"

namespace ADDON
{

class CResource : public CAddon
{
public:
  virtual ~CResource() { }

  virtual AddonPtr Clone() const = 0;

  virtual bool IsAllowed(const std::string &file) const = 0;

protected:
  CResource(const AddonProps &props)
    : CAddon(props)
  { }
  CResource(const cp_extension_t *ext)
    : CAddon(ext)
  { }
  CResource(const CResource &rhs)
    : CAddon(rhs)
  { }
};

}
