/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _LINUX
  #include <arpa/inet.h>
#endif

#include "linux/PlatformInclude.h"
#include "AMFObject.h"
#include "utils/log.h"
#include "rtmp.h"

RTMP_LIB::AMFObjectProperty RTMP_LIB::AMFObject::m_invalidProp;

RTMP_LIB::AMFObjectProperty::AMFObjectProperty()
{
  Reset();
}

RTMP_LIB::AMFObjectProperty::AMFObjectProperty(const std::string & strName, double dValue)
{
  Reset();
}

RTMP_LIB::AMFObjectProperty::AMFObjectProperty(const std::string & strName, bool bValue)
{
  Reset();
}

RTMP_LIB::AMFObjectProperty::AMFObjectProperty(const std::string & strName, const std::string & strValue)
{
  Reset();
}

RTMP_LIB::AMFObjectProperty::AMFObjectProperty(const std::string & strName, const AMFObject & objValue)
{
  Reset();
}

RTMP_LIB::AMFObjectProperty::~ AMFObjectProperty()
{
}

const std::string &RTMP_LIB::AMFObjectProperty::GetPropName() const
{
  return m_strName;
}

RTMP_LIB::AMFDataType RTMP_LIB::AMFObjectProperty::GetType() const
{
  return m_type;
}

double RTMP_LIB::AMFObjectProperty::GetNumber() const
{
  return m_dNumVal;
}

bool RTMP_LIB::AMFObjectProperty::GetBoolean() const
{
  return (bool)m_dNumVal;
}

const std::string &RTMP_LIB::AMFObjectProperty::GetString() const
{
  return m_strVal;
}

const RTMP_LIB::AMFObject &RTMP_LIB::AMFObjectProperty::GetObject() const
{
  return m_objVal;
}

bool RTMP_LIB::AMFObjectProperty::IsValid() const
{
  return (m_type != AMF_INVALID);
}

int RTMP_LIB::AMFObjectProperty::Encode(char * pBuffer, int nSize) const
{
  int nBytes = 0;
  
  if (m_type == AMF_INVALID)
    return -1;

  if (m_type != AMF_NULL && nSize < (int)m_strName.size() + (int)sizeof(short) + 1)
    return -1;

  if (m_type != AMF_NULL && !m_strName.empty())
  {
    nBytes += EncodeName(pBuffer);
    pBuffer += nBytes;
    nSize -= nBytes;
  }

  switch (m_type)
  {
    case AMF_NUMBER:
      if (nSize < 9)
        return -1;
      nBytes += RTMP_LIB::CRTMP::EncodeNumber(pBuffer, GetNumber());
      break;

    case AMF_BOOLEAN:
      if (nSize < 2)
        return -1;
      nBytes += RTMP_LIB::CRTMP::EncodeBoolean(pBuffer, GetBoolean());
      break;

    case AMF_STRING:
      if (nSize < (int)m_strVal.size() + (int)sizeof(short))
        return -1;
      nBytes += RTMP_LIB::CRTMP::EncodeString(pBuffer, GetString());
      break;

    case AMF_NULL:
      if (nSize < 1)
        return -1;
      *pBuffer = 0x05;
      nBytes += 1;
      break;

    case AMF_OBJECT:
    {
      int nRes = m_objVal.Encode(pBuffer, nSize);
      if (nRes == -1)
        return -1;

      nBytes += nRes;
      break;
    }
    default:
      CLog::Log(LOGERROR,"%s, invalid type. %d", __FUNCTION__, m_type);
      return -1;
  };  

  return nBytes;
}

int RTMP_LIB::AMFObjectProperty::Decode(const char * pBuffer, int nSize, bool bDecodeName) 
{
  int nOriginalSize = nSize;

  if (nSize == 0 || !pBuffer)
    return -1;

  if (*pBuffer == 0x05)
  {
    m_type = AMF_NULL;
    return 1;
  }

  if (bDecodeName && nSize < 4) // at least name (length + at least 1 byte) and 1 byte of data
    return -1;

  if (bDecodeName)
  {
    short nNameSize = RTMP_LIB::CRTMP::ReadInt16(pBuffer);
    if (nNameSize > nSize - (short)sizeof(short))
      return -1;

    m_strName = RTMP_LIB::CRTMP::ReadString(pBuffer);
    nSize -= sizeof(short) + m_strName.size();
    pBuffer += sizeof(short) + m_strName.size();
  }

  if (nSize == 0)
    return -1;

  nSize--;

  switch (*pBuffer)
  {
    case 0x00: //AMF_NUMBER:
      if (nSize < (int)sizeof(double))
        return -1;
      m_dNumVal = RTMP_LIB::CRTMP::ReadNumber(pBuffer+1);
      nSize -= sizeof(double);
      m_type = AMF_NUMBER;
      break;
    case 0x01: //AMF_BOOLEAN:
      if (nSize < 1)
        return -1;
      m_dNumVal = (double)RTMP_LIB::CRTMP::ReadBool(pBuffer+1);
      nSize--;
      m_type = AMF_BOOLEAN;
      break;
    case 0x02: //AMF_STRING:
    {
      short nStringSize = RTMP_LIB::CRTMP::ReadInt16(pBuffer+1);
      if (nSize < nStringSize + (int)sizeof(short))
        return -1;
      m_strVal = RTMP_LIB::CRTMP::ReadString(pBuffer+1);
      nSize -= sizeof(short) + nStringSize;
      m_type = AMF_STRING;
      break;
    }
    case 0x03: //AMF_OBJECT:
    {
      int nRes = m_objVal.Decode(pBuffer+1, nSize, true);
      if (nRes == -1)
        return -1;
      nSize -= nRes;
      m_type = AMF_OBJECT;
      break;
    }
    default:
      CLog::Log(LOGDEBUG,"%s - unknown datatype 0x%02x", __FUNCTION__, (unsigned char)(*pBuffer));
      return -1;
  }

  return nOriginalSize - nSize;
}

void RTMP_LIB::AMFObjectProperty::Dump() const
{
  if (m_type == AMF_INVALID)
  {
    CLog::Log(LOGDEBUG,"Property: INVALID");
    return;
  }

  if (m_type == AMF_NULL)
  {
    CLog::Log(LOGDEBUG,"Property: NULL");
    return;
  }

  if (m_type == AMF_OBJECT)
  {
    CLog::Log(LOGDEBUG,"Property: OBJECT ====>");
    m_objVal.Dump();
    return;
  }

  CStdString strRes = "no-name. ";
  if (!m_strName.empty())
    strRes = "Name: " + m_strName + ",  ";

  CStdString strVal;

  switch(m_type)
  {
    case AMF_NUMBER:
      strVal.Format("NUMBER: %.2f", m_dNumVal);
      break;
    case AMF_BOOLEAN:
      strVal.Format("BOOLEAN: %s", m_dNumVal == 1.?"TRUE":"FALSE");
      break;
    case AMF_STRING:
      strVal.Format("STRING: %s", m_strVal.c_str());
      break;
    default:
      strVal.Format("INVALID TYPE 0x%02x", (unsigned char)m_type);
  }

  strRes += strVal;
  CLog::Log(LOGDEBUG,"Property: <%s>", strRes.c_str());
}

void RTMP_LIB::AMFObjectProperty::Reset()
{
  m_dNumVal = 0.;
  m_strVal.clear();
  m_objVal.Reset();
  m_type = AMF_INVALID;
}

int RTMP_LIB::AMFObjectProperty::EncodeName(char *pBuffer) const
{
  short length = htons(m_strName.size());
  memcpy(pBuffer, &length, sizeof(short));
  pBuffer += sizeof(short);

  memcpy(pBuffer, m_strName.c_str(), m_strName.size());
  return m_strName.size() + sizeof(short);
}


// AMFObject

RTMP_LIB::AMFObject::AMFObject()
{
  Reset();
}

RTMP_LIB::AMFObject::~ AMFObject()
{
  Reset();
}

int RTMP_LIB::AMFObject::Encode(char * pBuffer, int nSize) const
{
  if (nSize < 4)
    return -1;

  *pBuffer = 0x03; // object

  int nOriginalSize = nSize;
  for (size_t i=0; i<m_properties.size(); i++)
  {
    int nRes = m_properties[i].Encode(pBuffer, nSize);
    if (nRes == -1)
    {
      CLog::Log(LOGERROR,"AMFObject::Encode - failed to encode property in index %d", i);
    }
    else
    {
      nSize -= nRes;
      pBuffer += nRes;
    }
  }

  if (nSize < 3)
    return -1; // no room for the end marker

  RTMP_LIB::CRTMP::EncodeInt24(pBuffer, 0x000009);
  nSize -= 3;

  return nOriginalSize - nSize;
}

int RTMP_LIB::AMFObject::Decode(const char * pBuffer, int nSize, bool bDecodeName)
{
  int nOriginalSize = nSize;
  bool bError = false; // if there is an error while decoding - try to at least find the end mark 0x000009

  while (nSize > 3)
  {
    if (RTMP_LIB::CRTMP::ReadInt24(pBuffer) == 0x00000009)
    {
      nSize -= 3;
      bError = false;
      break;
    }

    if (bError)
    {
      nSize--;
      pBuffer++;
      continue;
    }

    RTMP_LIB::AMFObjectProperty prop;
    int nRes = prop.Decode(pBuffer, nSize, bDecodeName);
    if (nRes == -1)
      bError = true;
    else
    {
      nSize -= nRes;
      pBuffer += nRes;
      m_properties.push_back(prop);
    }
  }

  if (bError)
    return -1;

  return nOriginalSize - nSize;
}

void RTMP_LIB::AMFObject::AddProperty(const AMFObjectProperty & prop)
{
  m_properties.push_back(prop);
}

int RTMP_LIB::AMFObject::GetPropertyCount() const
{
  return m_properties.size();
}

const RTMP_LIB::AMFObjectProperty & RTMP_LIB::AMFObject::GetProperty(const std::string & strName) const
{
  for (size_t n=0; n<m_properties.size(); n++)
  {
    if (m_properties[n].GetPropName() == strName)
      return m_properties[n];
  }

  return m_invalidProp;
}

const RTMP_LIB::AMFObjectProperty & RTMP_LIB::AMFObject::GetProperty(size_t nIndex) const
{
  if (nIndex >= m_properties.size())
    return m_invalidProp;

  return m_properties[nIndex];
}

void RTMP_LIB::AMFObject::Dump() const
{
  CLog::Log(LOGDEBUG,"START AMF Object Dump:");
  
  for (size_t n=0; n<m_properties.size(); n++)
    m_properties[n].Dump();

  CLog::Log(LOGDEBUG,"END AMF Object Dump:");
}

void RTMP_LIB::AMFObject::Reset()
{
  m_properties.clear();
}

