#ifndef GUILIB_LOCALIZESTRINGS_H
#define GUILIB_LOCALIZESTRINGS_H
#pragma once
#include "gui3d.h"

#include "stdstring.h"
#include <map>
using namespace std;

class CLocalizeStrings
{
public:
  CLocalizeStrings(void);
  virtual ~CLocalizeStrings(void);
  bool            Load(const CStdString& strFileName);
  const wstring&  Get(DWORD dwCode);
protected:
  map<DWORD,wstring> m_vecStrings;
  typedef map<DWORD,wstring>::iterator ivecStrings;
};

extern CLocalizeStrings g_localizeStrings;
#endif