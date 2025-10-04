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

namespace KODI::GUILIB::GUIINFO
{

// class to hold multiple integer and string data
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
      m_data2(data2)
  {
    if (flag)
      SetInfoFlag(flag);
  }

  CGUIInfo(int info, uint32_t data1, int data2, const std::string& data3)
    : m_info(info),
      m_data1(data1),
      m_data2(data2),
      m_data3(data3)
  {
  }

  CGUIInfo(int info, uint32_t data1, const std::string& data3)
    : m_info(info),
      m_data1(data1),
      m_data3(data3)
  {
  }

  CGUIInfo(int info, const std::string& data3) : m_info(info), m_data3(data3) {}

  CGUIInfo(int info, const std::string& data3, int data2)
    : m_info(info),
      m_data2(data2),
      m_data3(data3)
  {
  }

  CGUIInfo(int info, const std::string& data3, const std::string& data5)
    : m_info(info),
      m_data3(data3),
      m_data5(data5)
  {
  }

  bool operator==(const CGUIInfo& right) const = default;

  int GetInfo() const { return m_info; }

  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const { return m_data2; }
  const std::string& GetData3() const { return m_data3; }
  int GetData4() const { return m_data4; }
  const std::string& GetData5() const { return m_data5; }

private:
  void SetInfoFlag(uint32_t flag);

  int m_info{0};
  uint32_t m_data1{0};
  int m_data2{0};
  std::string m_data3;
  int m_data4{0};
  std::string m_data5;
};

} // namespace KODI::GUILIB::GUIINFO
