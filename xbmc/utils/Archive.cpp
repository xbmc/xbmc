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

#include <algorithm>
#include <cstring>
#include "Archive.h"
#include "IArchivable.h"
#include "filesystem/File.h"
#include "Variant.h"
#include "utils/log.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

using namespace XFILE;

CArchive::CArchive(CFile* pFile, int mode)
{
  m_pFile = pFile;
  m_iMode = mode;

  m_pBuffer = new uint8_t[CARCHIVE_BUFFER_MAX];
  memset(m_pBuffer, 0, CARCHIVE_BUFFER_MAX);
  if (mode == load)
  {
    m_BufferPos = m_pBuffer + CARCHIVE_BUFFER_MAX;
    m_BufferRemain = 0;
  }
  else
  {
    m_BufferPos = m_pBuffer;
    m_BufferRemain = CARCHIVE_BUFFER_MAX;
  }
}

CArchive::~CArchive()
{
  FlushBuffer();
  delete[] m_pBuffer;
}

void CArchive::Close()
{
  FlushBuffer();
}

bool CArchive::IsLoading() const
{
  return (m_iMode == load);
}

bool CArchive::IsStoring() const
{
  return (m_iMode == store);
}

CArchive& CArchive::operator<<(float f)
{
  return streamout(&f, sizeof(f));
}

CArchive& CArchive::operator<<(double d)
{
  return streamout(&d, sizeof(d));
}

CArchive& CArchive::operator<<(short int s)
{
  return streamout(&s, sizeof(s));
}

CArchive& CArchive::operator<<(unsigned short int us)
{
  return streamout(&us, sizeof(us));
}

CArchive& CArchive::operator<<(int i)
{
  return streamout(&i, sizeof(i));
}

CArchive& CArchive::operator<<(unsigned int ui)
{
  return streamout(&ui, sizeof(ui));
}

CArchive& CArchive::operator<<(long int l)
{
  return streamout(&l, sizeof(l));
}

CArchive& CArchive::operator<<(unsigned long int ul)
{
  return streamout(&ul, sizeof(ul));
}

CArchive& CArchive::operator<<(long long int ll)
{
  return streamout(&ll, sizeof(ll));
}

CArchive& CArchive::operator<<(unsigned long long int ull)
{
  return streamout(&ull, sizeof(ull));
}

CArchive& CArchive::operator<<(bool b)
{
  return streamout(&b, sizeof(b));
}

CArchive& CArchive::operator<<(char c)
{
  return streamout(&c, sizeof(c));
}

CArchive& CArchive::operator<<(const std::string& str)
{
  *this << str.size();

  return streamout(str.data(), str.size() * sizeof(char));
}

CArchive& CArchive::operator<<(const std::wstring& wstr)
{
  *this << wstr.size();

  return streamout(wstr.data(), wstr.size() * sizeof(wchar_t));
}

CArchive& CArchive::operator<<(const SYSTEMTIME& time)
{
  return streamout(&time, sizeof(SYSTEMTIME));
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
  case CVariant::VariantTypeWideString:
    *this << variant.asWideString();
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
    for (CVariant::const_iterator_map itr = variant.begin_map(); itr != variant.end_map(); ++itr)
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
  *this << strArray.size();
  for (size_t index = 0; index < strArray.size(); index++)
    *this << strArray.at(index);

  return *this;
}

CArchive& CArchive::operator<<(const std::vector<int>& iArray)
{
  *this << iArray.size();
  for (size_t index = 0; index < iArray.size(); index++)
    *this << iArray.at(index);

  return *this;
}

CArchive& CArchive::operator>>(std::string& str)
{
  size_t iLength = 0;
  *this >> iLength;

  char *s = new char[iLength];
  streamin(s, iLength * sizeof(char));
  str.assign(s, iLength);
  delete[] s;

  return *this;
}

CArchive& CArchive::operator>>(std::wstring& wstr)
{
  size_t iLength = 0;
  *this >> iLength;

  wchar_t * const p = new wchar_t[iLength];
  streamin(p, iLength * sizeof(wchar_t));
  wstr.assign(p, iLength);
  delete[] p;

  return *this;
}

CArchive& CArchive::operator>>(SYSTEMTIME& time)
{
  return streamin(&time, sizeof(SYSTEMTIME));
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
  case CVariant::VariantTypeWideString:
  {
    std::wstring value;
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
  size_t size;
  *this >> size;
  strArray.clear();
  for (size_t index = 0; index < size; index++)
  {
    std::string str;
    *this >> str;
    strArray.push_back(str);
  }

  return *this;
}

CArchive& CArchive::operator>>(std::vector<int>& iArray)
{
  size_t size;
  *this >> size;
  iArray.clear();
  for (size_t index = 0; index < size; index++)
  {
    int i;
    *this >> i;
    iArray.push_back(i);
  }

  return *this;
}

void CArchive::FlushBuffer()
{
  if (m_iMode == store && m_BufferPos != m_pBuffer)
  {
    if (m_pFile->Write(m_pBuffer, m_BufferPos - m_pBuffer) != m_BufferPos - m_pBuffer)
      CLog::Log(LOGERROR, "%s: Error flushing buffer", __FUNCTION__);
    else
    {
      m_BufferPos = m_pBuffer;
      m_BufferRemain = CARCHIVE_BUFFER_MAX;
    }
  }
}

CArchive &CArchive::streamout_bufferwrap(const uint8_t *ptr, size_t size)
{
  do
  {
    size_t chunkSize = std::min(size, m_BufferRemain);
    m_BufferPos = std::copy(ptr, ptr + chunkSize, m_BufferPos);
    ptr += chunkSize;
    size -= chunkSize;
    m_BufferRemain -= chunkSize;
    if (m_BufferRemain == 0)
      FlushBuffer();
  } while (size > 0);
  return *this;
}

void CArchive::FillBuffer()
{
  if (m_iMode == load && m_BufferRemain == 0)
  {
    ssize_t read = m_pFile->Read(m_pBuffer, CARCHIVE_BUFFER_MAX);
    if (read > 0)
    {
      m_BufferRemain = read;
      m_BufferPos = m_pBuffer;
    }
  }
}

CArchive &CArchive::streamin_bufferwrap(uint8_t *ptr, size_t size)
{
  uint8_t *orig_ptr = ptr;
  size_t orig_size = size;
  do
  {
    if (m_BufferRemain == 0)
    {
      FillBuffer();
      if (m_BufferRemain < CARCHIVE_BUFFER_MAX && m_BufferRemain < size)
      {
        CLog::Log(LOGERROR, "%s: can't stream in: requested %lu bytes, was read %lu bytes", __FUNCTION__, (unsigned long) orig_size, (unsigned long) (ptr - orig_ptr + m_BufferRemain));
        memset(orig_ptr, 0, orig_size);
        return *this;
      }
    }
    size_t chunkSize = std::min(size, m_BufferRemain);
    ptr = std::copy(m_BufferPos, m_BufferPos + chunkSize, ptr);
    m_BufferPos += chunkSize;
    m_BufferRemain -= chunkSize;
    size -= chunkSize;
  } while (size > 0);
  return *this;
}
