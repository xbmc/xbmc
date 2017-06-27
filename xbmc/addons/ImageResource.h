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

#include <string>

#include "addons/Resource.h"

class CURL;

namespace ADDON
{

//! \brief A collection of images. The collection can have a type.
class CImageResource : public CResource
{
public:
  static std::unique_ptr<CImageResource> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

  explicit CImageResource(CAddonInfo addonInfo) : CResource(std::move(addonInfo)) {};
  CImageResource(CAddonInfo addonInfo, std::string type);

  void OnPreUnInstall() override;

  bool IsAllowed(const std::string &file) const override;
  std::string GetFullPath(const std::string &filePath) const override;

  //! \brief Returns type of image collection
  const std::string& GetType() const { return m_type; }

private:
  bool HasXbt(CURL& xbtUrl) const;

  std::string m_type; //!< Type of images
};

} /* namespace ADDON */
