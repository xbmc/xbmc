/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  CGUIInfo(int info, uint32_t data1, int data2, const std::string& data3)
    : m_info(info), m_data1(data1), m_data2(data2), m_data3(data3), m_data4(0)
  {
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

  CGUIInfo(int info, const std::string& data3, const std::string& data5)
    : m_info(info), m_data1(0), m_data3(data3), m_data4(0), m_data5(data5)
  {
  }

  bool operator ==(const CGUIInfo &right) const
  {
    return (m_info == right.m_info && m_data1 == right.m_data1 && m_data2 == right.m_data2 &&
            m_data3 == right.m_data3 && m_data4 == right.m_data4 && m_data5 == right.m_data5);
  }

  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const { return m_data2; }
  const std::string& GetData3() const { return m_data3; }
  int GetData4() const { return m_data4; }
  const std::string& GetData5() const { return m_data5; }

  int m_info;
private:
  void SetInfoFlag(uint32_t flag);

  uint32_t m_data1;
  int m_data2;
  std::string m_data3;
  int m_data4;
  std::string m_data5;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
