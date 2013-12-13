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
#include "PlatformDefs.h" // for SYSTEMTIME

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

  /* CArchive support storing and loading of all C basic integer types
   * C basic types was chosen instead of fixed size ints (int16_t - int64_t) to support all integer typedefs
   * For example size_t can be typedef of unsigned int, long or long long depending on platform 
   * while int32_t and int64_t are usually unsigned short, int or long long, but not long 
   * and even if int and long can have same binary representation they are different types for compiler 
   * According to section 5.2.4.2.1 of C99 http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf
   * minimal size of short int is 16 bits
   * minimal size of int is 16 bits (usually 32 or 64 bits, larger or equal to short int)
   * minimal size of long int is 32 bits (larger or equal to int)
   * minimal size of long long int is 64 bits (larger or equal to long int) */
  // storing
  CArchive& operator<<(float f);
  CArchive& operator<<(double d);
  CArchive& operator<<(short int s);
  CArchive& operator<<(unsigned short int us);
  CArchive& operator<<(int i);
  CArchive& operator<<(unsigned int ui);
  CArchive& operator<<(long int l);
  CArchive& operator<<(unsigned long int ul);
  CArchive& operator<<(long long int ll);
  CArchive& operator<<(unsigned long long int ull);
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
  CArchive& operator>>(short int& s);
  CArchive& operator>>(unsigned short int& us);
  CArchive& operator>>(int& i);
  CArchive& operator>>(unsigned int& ui);
  CArchive& operator>>(long int& l);
  CArchive& operator>>(unsigned long int& ul);
  CArchive& operator>>(long long int& ll);
  CArchive& operator>>(unsigned long long int& ull);
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
  CArchive& streamout(const void* dataPtr, size_t size);
  CArchive& streamin(void* dataPtr, const size_t size);
  void FlushBuffer();
  XFILE::CFile* m_pFile;
  int m_iMode;
  uint8_t *m_pBuffer;
  int m_BufferPos;
};

