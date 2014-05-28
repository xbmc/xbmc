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

#ifndef UTILS_STDSTRING_H_INCLUDED
#define UTILS_STDSTRING_H_INCLUDED
#include "StdString.h"
#endif


class Crc32
{
public:
  Crc32();
  void Reset();
  void Compute(const char* buffer, size_t count);
  void Compute(const CStdString& strValue);
  void ComputeFromLowerCase(const CStdString& strValue);

  operator uint32_t () const
  {
    return m_crc;
  }

private:
  uint32_t m_crc;
};

