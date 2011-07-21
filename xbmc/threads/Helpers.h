/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <assert.h>

namespace XbmcThreads
{
  /**
   * Any class that inherits from NonCopyable will ... not be copyable (Duh!)
   */
  class NonCopyable
  {
    inline NonCopyable(const NonCopyable& ) {}
    inline NonCopyable& operator=(const NonCopyable& ) { return *this; }
  public:
    inline NonCopyable() {}
  };

}

#ifdef NDEBUG
#define XBMC_ASSERT_TRUE(expression) assert( expression )
#define XBMC_ASSERT_NOTEQUALS(compareTo,expression) assert( compareTo != expression )
#define XBMC_ASSERT_EQUALS(compareTo,expression) assert( compareTo == expression )
#define XBMC_ASSERT_ZERO(expression) assert( 0 == expression )
#else
#define XBMC_ASSERT_TRUE(expression) expression
#define XBMC_ASSERT_NOTEQUALS(compareTo,expression) expression
#define XBMC_ASSERT_EQUALS(compareTo,expression) expression
#define XBMC_ASSERT_ZERO(expression) expression
#endif
