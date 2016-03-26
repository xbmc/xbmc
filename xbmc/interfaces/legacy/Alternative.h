/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#pragma once

#include "Exception.h"

namespace XBMCAddon
{
  enum WhichAlternative { none, first, second };

  template<typename T1, typename T2> class Alternative
  {
  public:
  private:
    WhichAlternative pos;
    T1 d1;
    T2 d2;

  public:
    Alternative() : pos(none) {}

    inline WhichAlternative which() const { return pos; }

    inline T1& former()
    {
      if (pos == second)// first and none is ok
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      if (pos == none)
        d1 = T1();
      pos = first;
      return d1;
    }

    inline const T1& former() const
    {
      if (pos != first)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      return d1;
    }

    inline T2& later()
    {
      if (pos == first)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      if (pos == none)
        d2 = T2();
      pos = second;
      return d2;
    }

    inline const T2& later() const
    {
      if (pos != second)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      return d2;
    }

    inline operator T1& () { return former(); }
    inline operator const T1& () const { return former(); }
    inline operator T2& () { return later(); }
    inline operator const T2& () const { return later(); }
  };
}

