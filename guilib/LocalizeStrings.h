/*!
	\file LocalizeStrings.h
	\brief 
	*/

#ifndef GUILIB_LOCALIZESTRINGS_H
#define GUILIB_LOCALIZESTRINGS_H

#pragma once

/*!
	\ingroup strings
	\brief 
	*/
class CLocalizeStrings
{
public:
  CLocalizeStrings(void);
  virtual ~CLocalizeStrings(void);
  bool            Load(const CStdString& strFileName);
  const wstring&  Get(DWORD dwCode) const;
protected:
  map<DWORD,wstring> m_vecStrings;
  typedef map<DWORD,wstring>::const_iterator ivecStrings;
};

/*!
	\ingroup strings
	\brief 
	*/
extern CLocalizeStrings g_localizeStrings;
#endif
