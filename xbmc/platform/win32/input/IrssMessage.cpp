/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IrssMessage.h"

CIrssMessage::CIrssMessage()
{
  m_type = IRSSMT_Unknown;
  m_flags = 0;
  m_data = NULL;
  m_dataSize = 0;
}

CIrssMessage::CIrssMessage(IRSS_MessageType type, uint32_t flags)
{
  m_type     = type;
  m_flags    = flags;
  m_data     = NULL;
  m_dataSize = 0;
}

CIrssMessage::CIrssMessage(IRSS_MessageType type, uint32_t flags, char* data, int size)
{
  m_type = type;
  m_flags = flags;
  SetDataAsBytes(data, size);
}

CIrssMessage::CIrssMessage(IRSS_MessageType type, uint32_t flags, const std::string& data)
{
  m_type = type;
  m_flags = flags;
  SetDataAsString(data);
}

CIrssMessage::~CIrssMessage()
{
  FreeData();
}

void CIrssMessage::SetDataAsBytes(char* data, int size)
{
  if (!data)
  {
    FreeData();
  }
  else
  {
    m_data = (char*)malloc(size * sizeof(char));
    memcpy(m_data, data, size);
    m_dataSize = size;
  }
}

void CIrssMessage::SetDataAsString(const std::string& data)
{
  if (data.empty())
  {
    FreeData();
  }
  else
  {
    m_data = strdup(data.c_str());
    m_dataSize = strlen(data.c_str());
  }
}

void CIrssMessage::FreeData()
{
  free(m_data);
  m_data = NULL;
  m_dataSize = 0;
}

char* CIrssMessage::ToBytes(int& size)
{
  int dataLength = 0;
  if (m_data)
  {
    dataLength = m_dataSize;
  }

  size = 8 + dataLength;
  char* byteArray = new char[size];

  memcpy(&byteArray[0], &m_type, 4);
  memcpy(&byteArray[4], &m_flags, 4);

  if (m_data)
  {
    memcpy(&byteArray[8], &m_data, m_dataSize);
  }

  return byteArray;
}

bool CIrssMessage::FromBytes(char* from, int size, CIrssMessage& message)
{
  if (!from)
    return false;

  if (size < 8)
    return false;

  //IRSS_MessageType type    = (MessageType)BitConverter.ToInt32(from, 0);
  //IRSS_MessageFlags flags  = (MessageFlags)BitConverter.ToInt32(from, 4);
  uint32_t type;
  memcpy(&type, from, 4);
  uint32_t flags;
  memcpy(&flags, from + 4, 4);

  message.SetType((IRSS_MessageType)type);
  message.SetFlags(flags);
  if (size > 8)
  {
    message.SetDataAsBytes(from + 8, size - 8);
  }
  return true;
}

void CIrssMessage::SetType(IRSS_MessageType type)
{
  m_type = type;
}
void CIrssMessage::SetFlags(uint32_t flags)
{
  m_flags = flags;
}
