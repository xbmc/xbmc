#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_DialogOK
  {
    void Dialogs_OK_ShowAndGetInputSingleText(
          const std::string&      heading,
          const std::string&      text);
    void Dialogs_OK_ShowAndGetInputLineText(
          const std::string&      heading,
          const std::string&      line0,
          const std::string&      line1,
          const std::string&      line2);
  };

}; /* extern "C" */
