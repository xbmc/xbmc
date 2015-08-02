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
  CImageResource(const AddonProps &props)
    : CResource(props)
  { }
  CImageResource(const cp_extension_t *ext);
  virtual ~CImageResource() { }

  virtual AddonPtr Clone() const;
  virtual void OnPreUnInstall();

  virtual bool IsAllowed(const std::string &file) const;
  virtual std::string GetFullPath(const std::string &filePath) const;

  //! \brief Returns type of image collection
  const std::string& GetType() const { return m_type; }

private:
  CImageResource(const CImageResource &rhs);

  bool HasXbt(CURL& xbtUrl) const;

  std::string m_type; //!< Type of images
};

} /* namespace ADDON */
