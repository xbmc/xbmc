#pragma once

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

#include <string>
#include <vector>
#include "system.h" // for SYSTEMTIME

namespace XFILE
{
  class CFile;
}
class CVariant;

class CArchive;

class IArchivable
{
public:
  virtual void Archive(CArchive& ar) = 0;
  virtual ~IArchivable() {}
};

class CArchive
{
public:
  CArchive(XFILE::CFile* pFile, int mode);
  ~CArchive();
  // storing
  CArchive& operator<<(float f);
  CArchive& operator<<(double d);
  CArchive& operator<<(int16_t i16);
  CArchive& operator<<(uint16_t ui16);
  CArchive& operator<<(int32_t i32);
  CArchive& operator<<(uint32_t ui32);
  CArchive& operator<<(int64_t i64);
  CArchive& operator<<(uint64_t ui64);
  CArchive& operator<<(bool b);
  CArchive& operator<<(char c);
  CArchive& operator<<(const std::string &str);
  CArchive& operator<<(const std::wstring& wstr);
  CArchive& operator<<(const SYSTEMTIME& time);
  CArchive& operator<<(IArchivable& obj);
  CArchive& operator<<(const CVariant& variant);
  CArchive& operator<<(const std::vector<std::string>& strArray);
  CArchive& operator<<(const std::vector<int>& iArray);

  // loading
  CArchive& operator>>(float& f);
  CArchive& operator>>(double& d);
  CArchive& operator>>(int16_t& i16);
  CArchive& operator>>(uint16_t& ui16);
  CArchive& operator>>(int32_t& i32);
  CArchive& operator>>(uint32_t& ui32);
  CArchive& operator>>(int64_t& i64);
  CArchive& operator>>(uint64_t& ui64);
  CArchive& operator>>(bool& b);
  CArchive& operator>>(char& c);
  CArchive& operator>>(std::string &str);
  CArchive& operator>>(std::wstring& wstr);
  CArchive& operator>>(SYSTEMTIME& time);
  CArchive& operator>>(IArchivable& obj);
  CArchive& operator>>(CVariant& variant);
  CArchive& operator>>(std::vector<std::string>& strArray);
  CArchive& operator>>(std::vector<int>& iArray);

  bool IsLoading();
  bool IsStoring();

  void Close();

  enum Mode {load = 0, store};

protected:
  void FlushBuffer();
  XFILE::CFile* m_pFile;
  int m_iMode;
  uint8_t *m_pBuffer;
  int m_BufferPos;
};

