/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CharArrayParser.h"

#include "utils/log.h"

void CCharArrayParser::Reset()
{
  m_limit = 0;
  m_position = 0;
}

void CCharArrayParser::Reset(const char* data, int limit)
{
  m_data = data;
  m_limit = limit;
  m_position = 0;
}

int CCharArrayParser::CharsLeft()
{
  return m_limit - m_position;
}

int CCharArrayParser::GetPosition()
{
  return m_position;
}

bool CCharArrayParser::SetPosition(int position)
{
  if (position >= 0 && position <= m_limit)
    m_position = position;
  else
  {
    CLog::Log(LOGERROR, "{} - Position out of range", __FUNCTION__);
    return false;
  }
  return true;
}

bool CCharArrayParser::SkipChars(int nChars)
{
  return SetPosition(m_position + nChars);
}

uint8_t CCharArrayParser::ReadNextUnsignedChar()
{
  m_position++;
  if (m_position > m_limit)
    CLog::Log(LOGERROR, "{} - Position out of range", __FUNCTION__);
  return static_cast<uint8_t>(m_data[m_position - 1]) & 0xFF;
}

uint16_t CCharArrayParser::ReadNextUnsignedShort()
{
  m_position += 2;
  if (m_position > m_limit)
    CLog::Log(LOGERROR, "{} - Position out of range", __FUNCTION__);
  return (static_cast<uint16_t>(m_data[m_position - 2]) & 0xFF) << 8 |
         (static_cast<uint16_t>(m_data[m_position - 1]) & 0xFF);
}

uint32_t CCharArrayParser::ReadNextUnsignedInt()
{
  m_position += 4;
  if (m_position > m_limit)
    CLog::Log(LOGERROR, "{} - Position out of range", __FUNCTION__);
  return (static_cast<uint32_t>(m_data[m_position - 4]) & 0xFF) << 24 |
         (static_cast<uint32_t>(m_data[m_position - 3]) & 0xFF) << 16 |
         (static_cast<uint32_t>(m_data[m_position - 2]) & 0xFF) << 8 |
         (static_cast<uint32_t>(m_data[m_position - 1]) & 0xFF);
}

std::string CCharArrayParser::ReadNextString(int length)
{
  std::string str(m_data + m_position, length);
  m_position += length;
  if (m_position > m_limit)
    CLog::Log(LOGERROR, "{} - Position out of range", __FUNCTION__);
  return str;
}
