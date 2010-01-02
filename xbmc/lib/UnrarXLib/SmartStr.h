#pragma once
#ifndef SMARTSTR_H_
#define SMARTSTR_H_

class CSmartStr
{
public:
  CSmartStr(unsigned int uBufferSize) : m_szPtr(0), m_uBufferSize(uBufferSize)
  {
    if (m_uBufferSize)
    {
      m_szPtr = new char[m_uBufferSize];
    }
  }

  ~CSmartStr()
  {
    if (m_szPtr)
    {
      delete[] m_szPtr;
    }

    m_uBufferSize = 0;
  }

  operator char *()
  {
    return m_szPtr;
  }

  unsigned int BufferSize()
  {
    return m_uBufferSize;
  }

private:
  char *m_szPtr;
  unsigned int m_uBufferSize;
};

class CSmartStrW
{
public:
  CSmartStrW(unsigned int uBufferSize) : m_szPtr(0), m_uBufferSize(uBufferSize)
  {
    if (m_uBufferSize)
    {
      m_szPtr = new wchar[m_uBufferSize];
    }
  }

  ~CSmartStrW()
  {
    if (m_szPtr)
    {
      delete[] m_szPtr;
    }
  }

  operator wchar *()
  {
    return m_szPtr;
  }

  unsigned int BufferSize()
  {
    return m_uBufferSize;
  }

private:
  wchar * m_szPtr;
  unsigned int m_uBufferSize;
};

#endif /* SMARTSTR_H_ */

