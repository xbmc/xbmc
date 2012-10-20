/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Archive.h"
#include "filesystem/File.h"
#include "Variant.h"

using namespace XFILE;

#define BUFFER_MAX 4096

CArchive::CArchive(CFile* pFile, int mode)
{
  m_pFile = pFile;
  m_iMode = mode;

  m_pBuffer = new BYTE[BUFFER_MAX];
  memset(m_pBuffer, 0, BUFFER_MAX);

  m_BufferPos = 0;
}

CArchive::~CArchive()
{
  FlushBuffer();
  delete[] m_pBuffer;
  m_BufferPos = 0;
}

void CArchive::Close()
{
  FlushBuffer();
}

bool CArchive::IsLoading()
{
  return (m_iMode == load);
}

bool CArchive::IsStoring()
{
  return (m_iMode == store);
}

CArchive& CArchive::operator<<(float f)
{
  int size = sizeof(float);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &f, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(double d)
{
  int size = sizeof(double);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &d, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(int i)
{
  int size = sizeof(int);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &i, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(unsigned int i)
{
  int size = sizeof(unsigned int);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &i, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(int64_t i64)
{
  int size = sizeof(int64_t);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &i64, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(uint64_t ui64)
{
  int size = sizeof(uint64_t);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &ui64, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(bool b)
{
  int size = sizeof(bool);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &b, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(char c)
{
  int size = sizeof(char);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &c, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(const std::string& str)
{
  *this << (int)str.size();

  int size = str.size();
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  int iBufferMaxParts=size/BUFFER_MAX;
  for (int i=0; i<iBufferMaxParts; i++)
  {
    memcpy(&m_pBuffer[m_BufferPos], str.c_str()+(i*BUFFER_MAX), BUFFER_MAX);
    m_BufferPos+=BUFFER_MAX;
    FlushBuffer();
  }

  int iPos=iBufferMaxParts*BUFFER_MAX;
  int iSizeLeft=size-iPos;
  memcpy(&m_pBuffer[m_BufferPos], str.c_str()+iPos, iSizeLeft);
  m_BufferPos+=iSizeLeft;

  return *this;
}

CArchive& CArchive::operator<<(const CStdString& str)
{
  *this << str.GetLength();

  int size = str.GetLength();
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  int iBufferMaxParts=size/BUFFER_MAX;
  for (int i=0; i<iBufferMaxParts; i++)
  {
    memcpy(&m_pBuffer[m_BufferPos], str.c_str()+(i*BUFFER_MAX), BUFFER_MAX);
    m_BufferPos+=BUFFER_MAX;
    FlushBuffer();
  }

  int iPos=iBufferMaxParts*BUFFER_MAX;
  int iSizeLeft=size-iPos;
  memcpy(&m_pBuffer[m_BufferPos], str.c_str()+iPos, iSizeLeft);
  m_BufferPos+=iSizeLeft;

  return *this;
}

CArchive& CArchive::operator<<(const CStdStringW& str)
{
  *this << str.GetLength();

  int size = str.GetLength() * sizeof(wchar_t);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  int iBufferMaxParts=size/BUFFER_MAX;
  for (int i=0; i<iBufferMaxParts; ++i)
  {
    memcpy(&m_pBuffer[m_BufferPos], str.c_str()+(i*BUFFER_MAX), BUFFER_MAX);
    m_BufferPos+=BUFFER_MAX;
    FlushBuffer();
  }

  int iPos=iBufferMaxParts*BUFFER_MAX;
  int iSizeLeft=size-iPos;
  memcpy(&m_pBuffer[m_BufferPos], str.c_str()+iPos, iSizeLeft);
  m_BufferPos+=iSizeLeft;

  return *this;
}

CArchive& CArchive::operator<<(const SYSTEMTIME& time)
{
  int size = sizeof(SYSTEMTIME);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &time, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(IArchivable& obj)
{
  obj.Archive(*this);

  return *this;
}

CArchive& CArchive::operator<<(const CVariant& variant)
{
  *this << (int)variant.type();
  switch (variant.type())
  {
  case CVariant::VariantTypeInteger:
    *this << variant.asInteger();
    break;
  case CVariant::VariantTypeUnsignedInteger:
    *this << variant.asUnsignedInteger();
    break;
  case CVariant::VariantTypeBoolean:
    *this << variant.asBoolean();
    break;
  case CVariant::VariantTypeString:
    *this << variant.asString();
    break;
  case CVariant::VariantTypeDouble:
    *this << variant.asDouble();
    break;
  case CVariant::VariantTypeArray:
    *this << variant.size();
    for (unsigned int index = 0; index < variant.size(); index++)
      *this << variant[index];
    break;
  case CVariant::VariantTypeObject:
    *this << variant.size();
    for (CVariant::const_iterator_map itr = variant.begin_map(); itr != variant.end_map(); itr++)
    {
      *this << itr->first;
      *this << itr->second;
    }
    break;
  case CVariant::VariantTypeNull:
  case CVariant::VariantTypeConstNull:
  default:
    break;
  }

  return *this;
}

CArchive& CArchive::operator<<(const std::vector<std::string>& strArray)
{
  *this << (unsigned int)strArray.size();
  for (unsigned int index = 0; index < strArray.size(); index++)
    *this << strArray.at(index);

  return *this;
}

CArchive& CArchive::operator<<(const std::vector<int>& iArray)
{
  *this << (unsigned int)iArray.size();
  for (unsigned int index = 0; index < iArray.size(); index++)
    *this << iArray.at(index);

  return *this;
}

CArchive& CArchive::operator>>(float& f)
{
  m_pFile->Read((void*)&f, sizeof(float));

  return *this;
}

CArchive& CArchive::operator>>(double& d)
{
  m_pFile->Read((void*)&d, sizeof(double));

  return *this;
}

CArchive& CArchive::operator>>(int& i)
{
  m_pFile->Read((void*)&i, sizeof(int));

  return *this;
}

CArchive& CArchive::operator>>(unsigned int& i)
{
  m_pFile->Read((void*)&i, sizeof(unsigned int));

  return *this;
}

CArchive& CArchive::operator>>(int64_t& i64)
{
  m_pFile->Read((void*)&i64, sizeof(int64_t));

  return *this;
}

CArchive& CArchive::operator>>(uint64_t& ui64)
{
  m_pFile->Read((void*)&ui64, sizeof(uint64_t));

  return *this;
}

CArchive& CArchive::operator>>(bool& b)
{
  m_pFile->Read((void*)&b, sizeof(bool));

  return *this;
}

CArchive& CArchive::operator>>(char& c)
{
  m_pFile->Read((void*)&c, sizeof(char));

  return *this;
}

CArchive& CArchive::operator>>(std::string& str)
{
  int iLength = 0;
  *this >> iLength;

  char *s = new char[iLength];
  m_pFile->Read(s, iLength);
  str.assign(s, iLength);
  delete[] s;

  return *this;
}

CArchive& CArchive::operator>>(CStdString& str)
{
  int iLength = 0;
  *this >> iLength;

  m_pFile->Read((void*)str.GetBufferSetLength(iLength), iLength);
  str.ReleaseBuffer();


  return *this;
}

CArchive& CArchive::operator>>(CStdStringW& str)
{
  int iLength = 0;
  *this >> iLength;

  m_pFile->Read((void*)str.GetBufferSetLength(iLength), iLength * sizeof(wchar_t));
  str.ReleaseBuffer();


  return *this;
}

CArchive& CArchive::operator>>(SYSTEMTIME& time)
{
  m_pFile->Read((void*)&time, sizeof(SYSTEMTIME));

  return *this;
}

CArchive& CArchive::operator>>(IArchivable& obj)
{
  obj.Archive(*this);

  return *this;
}

CArchive& CArchive::operator>>(CVariant& variant)
{
  int type;
  *this >> type;
  variant = CVariant((CVariant::VariantType)type);

  switch (variant.type())
  {
  case CVariant::VariantTypeInteger:
  {
    int64_t value;
    *this >> value;
    variant = value;
    break;
  }
  case CVariant::VariantTypeUnsignedInteger:
  {
    uint64_t value;
    *this >> value;
    variant = value;
    break;
  }
  case CVariant::VariantTypeBoolean:
  {
    bool value;
    *this >> value;
    variant = value;
    break;
  }
  case CVariant::VariantTypeString:
  {
    std::string value;
    *this >> value;
    variant = value;
    break;
  }
  case CVariant::VariantTypeDouble:
  {
    double value;
    *this >> value;
    variant = value;
    break;
  }
  case CVariant::VariantTypeArray:
  {
    unsigned int size;
    *this >> size;
    for (; size > 0; size--)
    {
      CVariant value;
      *this >> value;
      variant.append(value);
    }
    break;
  }
  case CVariant::VariantTypeObject:
  {
    unsigned int size;
    *this >> size;
    for (; size > 0; size--)
    {
      std::string name;
      CVariant value;
      *this >> name;
      *this >> value;
      variant[name] = value;
    }
    break;
  }
  case CVariant::VariantTypeNull:
  case CVariant::VariantTypeConstNull:
  default:
    break;
  }

  return *this;
}

CArchive& CArchive::operator>>(std::vector<std::string>& strArray)
{
  int size;
  *this >> size;
  strArray.clear();
  for (int index = 0; index < size; index++)
  {
    std::string str;
    *this >> str;
    strArray.push_back(str);
  }

  return *this;
}

CArchive& CArchive::operator>>(std::vector<int>& iArray)
{
  int size;
  *this >> size;
  iArray.clear();
  for (int index = 0; index < size; index++)
  {
    int i;
    *this >> i;
    iArray.push_back(i);
  }

  return *this;
}

void CArchive::FlushBuffer()
{
  if (m_BufferPos > 0)
  {
    m_pFile->Write(m_pBuffer, m_BufferPos);
    m_BufferPos = 0;
  }
}
