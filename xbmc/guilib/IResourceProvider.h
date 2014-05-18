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

#include <stdint.h>
#include <boost/shared_ptr.hpp>

class CGUIFont;
typedef uint32_t color_t;

/*! \brief a resource container class for resolving UI resources
 */
class IGUIResourceProvider
{
public:
  virtual ~IGUIResourceProvider() {};

  /*! \brief Get a font from the resource provider.
   \param the name of the font to obtain.
   \return a pointer to the font object, else NULL if the font is unavailable.
   */
  virtual CGUIFont *GetFont(const std::string &fontName) const=0;

  /*! \brief Get a color from the resource provider.
   \param the name of the color to obtain.
   \return the color, or white (0xffffffff) if unavailable.
   */
  virtual color_t GetColor(const std::string &fontName) const=0;
};

typedef boost::shared_ptr<IGUIResourceProvider> GUIResourceProviderPtr;
