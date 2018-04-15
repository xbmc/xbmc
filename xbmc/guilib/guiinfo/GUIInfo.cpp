/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIInfo.h"

#include <assert.h>

using namespace KODI::GUILIB::GUIINFO;

void CGUIInfo::SetInfoFlag(uint32_t flag)
{
  assert(flag >= (1 << 24));
  m_data1 |= flag;
}

uint32_t CGUIInfo::GetInfoFlag() const
{
  // we strip out the bottom 24 bits, where we keep data
  // and return the flag only
  return m_data1 & 0xff000000;
}

uint32_t CGUIInfo::GetData1() const
{
  // we strip out the top 8 bits, where we keep flags
  // and return the unflagged data
  return m_data1 & ((1 << 24) -1);
}
