/*
 *      Copyright (C) 2016 Team XBMC
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

#include "Os.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

/************************************************************************/
/************************************************************************/
void CJNIOsVibrator::cancel() const
{
  call_method<void>(m_object,
    "cancel", "()V");
}

bool CJNIOsVibrator::hasVibrator() const
{
  return call_method<jboolean>(m_object,
    "hasVibrator", "()Z");
}

void CJNIOsVibrator::vibrate(std::vector<int64_t> pattern, int repeat) const
{
  /* TODO
  call_method<void>(m_object,
    "vibrate", "([JI)F",
    jcast<jlongArray>(pattern), repeat);
  */
}

void	CJNIOsVibrator::vibrate(int64_t milliseconds) const
{
  call_method<void>(m_object,
    "vibrate", "(J)V",
    milliseconds);
}
