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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "utils/GlobalsHandling.h"

#include <iostream>

namespace xbmcutil
{
  namespace test
  {
    class TestGlobalPattern1
    {
    public:
      static bool ctorCalled;
      static bool dtorCalled;

      int somethingToAccess;

      TestGlobalPattern1() : somethingToAccess(0) { ctorCalled = true; }
      ~TestGlobalPattern1() 
      { 
        std::cout << "Clean shutdown of TestGlobalPattern1" << std::endl << std::flush;
        dtorCalled = true; 
      }

      void beHappy() { if (somethingToAccess) throw somethingToAccess; }
    };
  }
}

XBMC_GLOBAL_REF(xbmcutil::test::TestGlobalPattern1,g_testGlobalPattern1);
#define g_testGlobalPattern1 XBMC_GLOBAL_USE(xbmcutil::test::TestGlobalPattern1)
