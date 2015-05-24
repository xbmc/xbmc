/*
 *      Copyright (C) 2015 Team Kodi
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

#include "utils/rfft.h"

#include "gtest/gtest.h"

#if defined(TARGET_WINDOWS) && !defined(_USE_MATH_DEFINES)  
#define _USE_MATH_DEFINES
#endif

#include <math.h>


TEST(TestRFFT, SimpleSignal)
{
  const int size = 32;
  const int freq1 = 5;
  const int freq2[] = {1,7};
  std::vector<float> input(2*size);
  std::vector<float> output(size);
  for (size_t i=0;i<size;++i)
  {
    input[2*i] = cos(freq1*2.0*M_PI*i/size);
    input[2*i+1] = cos(freq2[0]*2.0*M_PI*i/size)+cos(freq2[1]*2.0*M_PI*i/size);
  }
  RFFT transform(size, false);

  transform.calc(&input[0], &output[0]);

  for (size_t i=0;i<size/2;++i)
  {
    EXPECT_NEAR(output[2*i],(i==freq1?1.0:0.0), 1e-7);
    EXPECT_NEAR(output[2*i+1], ((i==freq2[0]||i==freq2[1])?1.0:0.0), 1e-7);
  }
}
