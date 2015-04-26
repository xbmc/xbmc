#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include "addons/Resource.h"

namespace ADDON
{

class CUISoundsResource : public CResource
{
public:
  CUISoundsResource(const AddonProps &props) : CResource(props) {}
  CUISoundsResource(const cp_extension_t *ext) : CResource(ext) {}
  virtual ~CUISoundsResource() {}

  virtual AddonPtr Clone() const;
  virtual bool IsAllowed(const std::string &file) const;

  virtual bool IsInUse() const;
  virtual void OnPostInstall(bool update, bool modal);

private:
  CUISoundsResource(const CUISoundsResource &rhs) : CResource(rhs) {}
};

}
