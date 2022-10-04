/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#define CARCHIVE_BUFFER_MAX 4096

namespace XFILE
{
  class CFile;
}
class CVariant;
class IArchivable;
namespace KODI::TIME
{
struct SystemTime;
}

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
  CArchive& operator<<(const KODI::TIME::SystemTime& time);
  CArchive& operator<<(IArchivable& obj);
  CArchive& operator<<(const CVariant& variant);
  CArchive& operator<<(const std::vector<std::string>& strArray);
  CArchive& operator<<(const std::vector<int>& iArray);

  // loading
  inline CArchive& operator>>(float& f)
  {
    return streamin(&f, sizeof(f));
  }

  inline CArchive& operator>>(double& d)
  {
    return streamin(&d, sizeof(d));
  }

  inline CArchive& operator>>(short int& s)
  {
    return streamin(&s, sizeof(s));
  }

  inline CArchive& operator>>(unsigned short int& us)
  {
    return streamin(&us, sizeof(us));
  }

  inline CArchive& operator>>(int& i)
  {
    return streamin(&i, sizeof(i));
  }

  inline CArchive& operator>>(unsigned int& ui)
  {
    return streamin(&ui, sizeof(ui));
  }

  inline CArchive& operator>>(long int& l)
  {
    return streamin(&l, sizeof(l));
  }

  inline CArchive& operator>>(unsigned long int& ul)
  {
    return streamin(&ul, sizeof(ul));
  }

  inline CArchive& operator>>(long long int& ll)
  {
    return streamin(&ll, sizeof(ll));
  }

  inline CArchive& operator>>(unsigned long long int& ull)
  {
    return streamin(&ull, sizeof(ull));
  }

  inline CArchive& operator>>(bool& b)
  {
    return streamin(&b, sizeof(b));
  }

  inline CArchive& operator>>(char& c)
  {
    return streamin(&c, sizeof(c));
  }

  CArchive& operator>>(std::string &str);
  CArchive& operator>>(std::wstring& wstr);
  CArchive& operator>>(KODI::TIME::SystemTime& time);
  CArchive& operator>>(IArchivable& obj);
  CArchive& operator>>(CVariant& variant);
  CArchive& operator>>(std::vector<std::string>& strArray);
  CArchive& operator>>(std::vector<int>& iArray);

  bool IsLoading() const;
  bool IsStoring() const;

  void Close();

  enum Mode {load = 0, store};

protected:
  inline CArchive &streamout(const void *dataPtr, size_t size)
  {
    auto ptr = static_cast<const uint8_t *>(dataPtr);
    /* Note, the buffer is flushed as soon as it is full (m_BufferRemain == size) rather
     * than waiting until we attempt to put more data into an already full buffer */
    if (m_BufferRemain > size)
    {
      memcpy(m_BufferPos, ptr, size);
      m_BufferPos += size;
      m_BufferRemain -= size;
      return *this;
    }

    return streamout_bufferwrap(ptr, size);
  }

  inline CArchive &streamin(void *dataPtr, size_t size)
  {
    auto ptr = static_cast<uint8_t *>(dataPtr);
    /* Note, refilling the buffer is deferred until we know we need to read more from it */
    if (m_BufferRemain >= size)
    {
      memcpy(ptr, m_BufferPos, size);
      m_BufferPos += size;
      m_BufferRemain -= size;
      return *this;
    }

    return streamin_bufferwrap(ptr, size);
  }

  XFILE::CFile* m_pFile; //non-owning
  int m_iMode;
  std::unique_ptr<uint8_t[]> m_pBuffer;
  uint8_t *m_BufferPos;
  size_t m_BufferRemain;

private:
  void FlushBuffer();
  CArchive &streamout_bufferwrap(const uint8_t *ptr, size_t size);
  void FillBuffer();
  CArchive &streamin_bufferwrap(uint8_t *ptr, size_t size);
};
