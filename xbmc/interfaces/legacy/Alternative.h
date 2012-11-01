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

#include "Exception.h"

namespace XBMCAddon
{
  enum WhichAlternative { none, first, second };

  template<typename T1, typename T2> class Alternative
  {
  public:
  private:
    WhichAlternative pos;
    unsigned char data[sizeof(T1) > sizeof(T2) ? sizeof(T1) : sizeof(T2)];

  public:
    Alternative() : pos(none) {}
    Alternative(const Alternative& o)
    {
      pos = o.pos;
      if (pos == first)
        new(&data) T1(o.former());
      else if (pos == second)
        new(&data) T2(o.later());
    }

    inline WhichAlternative which() { return pos; }

    inline T1& former() throw (WrongTypeException)
    {
      if (pos == second)// first and none is ok
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      if (pos == none)
        new(&data) T1();
      pos = first;
      return *((T1*)data);
    }

    inline const T1& former() const throw (WrongTypeException)
    {
      if (pos != first)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      return *((T1*)data);
    }

    inline T2& later() throw (WrongTypeException)
    {
      if (pos == first)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      if (pos == none)
        new(&data) T2();
      pos = second;
      return *((T2*)data);
    }

    inline const T2& later() const throw (WrongTypeException)
    {
      if (pos != second)
        throw WrongTypeException("Access of XBMCAddon::Alternative as incorrect type");
      return *((T2*)data);
    }

    inline operator T1& () throw (WrongTypeException) { return former(); }
    inline operator const T1& () const throw (WrongTypeException) { return former(); }
    inline operator T2& () throw (WrongTypeException) { return later(); }
    inline operator const T2& () const throw (WrongTypeException) { return later(); }
  };
}

