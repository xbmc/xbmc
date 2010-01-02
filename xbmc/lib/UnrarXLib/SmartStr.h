#pragma once
#ifndef SMARTSTR_H_
#define SMARTSTR_H_

class CSmartStr
{
public:
  CSmartStr(int iCount) : m_szPtr(NULL)
  {
    if (iCount)
    {
      m_szPtr = new char[iCount];
    }
  }

  ~CSmartStr()
  {
    if (m_szPtr)
    {
      delete[] m_szPtr;
    }
  }

  operator char *()
  {
    return m_szPtr;
  }

private:
  char *m_szPtr;
};

class CSmartStrW
{
public:
  CSmartStrW(int iCount) : m_szPtr(NULL)
  {
    if (iCount)
    {
      m_szPtr = new wchar[iCount];
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

private:
  wchar * m_szPtr;
};

#endif /* SMARTSTR_H_ */

