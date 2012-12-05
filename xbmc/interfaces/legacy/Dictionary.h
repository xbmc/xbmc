/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <map>
#include "AddonString.h"

namespace XBMCAddon
{
  /**
   * This is a bit of a hack for dynamically typed languages. In somce
   * cases python addon api calls handle dictionaries with variable
   * value types. In this case we coerce all of these types into
   * strings and then convert them back in the api. Yes, this is messy
   * and maybe we should use the CVariant here. But for now the 
   * native api handles these calls by converting the string to the
   * appropriate types.
   */
  typedef std::map<String,String> Dictionary;
}
