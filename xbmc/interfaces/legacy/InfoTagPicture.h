/*
*      Copyright (C) 2005-2016 Team Kodi
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once
#include "pictures/PictureInfoTag.h"
#include "AddonClass.h"

#include <memory>

namespace XBMCAddon
{
  namespace xbmc
  {

    ///
    /// \defgroup python_InfoTagPicture InfoTagPicture
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's picture info tag class.**
    ///
    /// \python_class{ InfoTagPicture() }
    ///
    /// To get picture info tag data of currently played source or listitem.
    ///
    ///
    ///
    ///-------------------------------------------------------------------------
      /// @python_v18 New class added.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = listitem.getPictureInfoTag()
    ///
    /// gps_lat = tag.getInfo("latitude")
    /// exposure_time = tag.getInfo("exposuretime")
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class InfoTagPicture : public AddonClass
    {
    private:
      std::unique_ptr<CPictureInfoTag> infoTag;

    public:
#ifndef SWIG
      InfoTagPicture(const CPictureInfoTag& tag);
#endif
      InfoTagPicture() = default;
      virtual ~InfoTagPicture() = default;


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagPicture
      /// @brief \python_func{ getInfo(info) }
      ///-----------------------------------------------------------------------
      /// Get picture information
      /// For a list of all available info tags, see xbmc/pictures/PictureInfoTag.cpp in code
      ///
      /// @return [string] picture info.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      ///
      getInfo(...);
#else
      String getInfo(const String& info);
#endif
    };
  }
}
