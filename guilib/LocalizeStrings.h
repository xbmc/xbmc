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
  bool Load(const CStdString& strFileName);
  const string& Get(DWORD dwCode) const;
  void Clear();
protected:
  map<DWORD, string> m_vecStrings;
  typedef map<DWORD, string>::const_iterator ivecStrings;
};

/*!
 \ingroup strings
 \brief 
 */
extern CLocalizeStrings g_localizeStrings;
#endif
