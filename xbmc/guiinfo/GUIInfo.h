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

namespace GUIINFO
{

// class to hold multiple integer data
// for storage referenced from a single integer
class GUIInfo
{
public:
  explicit GUIInfo(int info, uint32_t data1 = 0, int data2 = 0, uint32_t flag = 0)
  : m_info(info),
    m_data1(data1),
    m_data2(data2)
  {
    if (flag)
      SetInfoFlag(flag);
  }

  GUIInfo(int info, uint32_t data1, const std::string& data3)
  : m_info(info),
    m_data1(data1),
    m_data2(0),
    m_data3(data3)
  {
  }

  GUIInfo(int info, const std::string& data3)
  : m_info(info),
    m_data1(0),
    m_data2(0),
    m_data3(data3)
  {
  }

  GUIInfo(int info, const std::string& data3, int data2)
  : m_info(info),
    m_data1(0),
    m_data2(data2),
    m_data3(data3)
  {
  }

  bool operator ==(const GUIInfo &right) const
  {
    return (m_info == right.m_info &&
            m_data1 == right.m_data1 &&
            m_data2 == right.m_data2 &&
            m_data3 == right.m_data3);
  }

  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const { return m_data2; }
  const std::string& GetData3() const { return m_data3; }

  int m_info;
private:
  void SetInfoFlag(uint32_t flag);

  uint32_t m_data1;
  int m_data2;
  std::string m_data3;
};

} // namespace GUIINFO
