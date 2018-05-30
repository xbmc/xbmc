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

#pragma once

#include <stdint.h>
#include <string>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

// class to hold multiple integer data
// for storage referenced from a single integer
class CGUIInfo
{
public:
  CGUIInfo(int info, uint32_t data1, int data2, uint32_t flag, const std::string& data3, int data4)
  : m_info(info),
    m_data1(data1),
    m_data2(data2),
    m_data3(data3),
    m_data4(data4)
  {
    if (flag)
      SetInfoFlag(flag);
  }

  explicit CGUIInfo(int info, uint32_t data1 = 0, int data2 = 0, uint32_t flag = 0)
  : m_info(info),
    m_data1(data1),
    m_data2(data2),
    m_data4(0)
  {
    if (flag)
      SetInfoFlag(flag);
  }

  CGUIInfo(int info, uint32_t data1, const std::string& data3)
  : m_info(info),
    m_data1(data1),
    m_data2(0),
    m_data3(data3),
    m_data4(0)
  {
  }

  CGUIInfo(int info, const std::string& data3)
  : m_info(info),
    m_data1(0),
    m_data2(0),
    m_data3(data3),
    m_data4(0)
  {
  }

  CGUIInfo(int info, const std::string& data3, int data2)
  : m_info(info),
    m_data1(0),
    m_data2(data2),
    m_data3(data3),
    m_data4(0)
  {
  }

  bool operator ==(const CGUIInfo &right) const
  {
    return (m_info == right.m_info &&
            m_data1 == right.m_data1 &&
            m_data2 == right.m_data2 &&
            m_data3 == right.m_data3 &&
            m_data4 == right.m_data4);
  }

  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const { return m_data2; }
  const std::string& GetData3() const { return m_data3; }
  int GetData4() const { return m_data4; }

  int m_info;
private:
  void SetInfoFlag(uint32_t flag);

  uint32_t m_data1;
  int m_data2;
  std::string m_data3;
  int m_data4;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
