/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    WhichAlternative pos = none;
    T1 d1;
    T2 d2;

  public:
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

