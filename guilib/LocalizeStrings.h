/*!
\file LocalizeStrings.h
\brief 
*/

#ifndef GUILIB_LOCALIZESTRINGS_H
#define GUILIB_LOCALIZESTRINGS_H

#pragma once

#include <map>

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
  const CStdString& Get(DWORD dwCode) const;
  void Clear();
protected:
  std::map<DWORD, CStdString> m_vecStrings;
  typedef std::map<DWORD, CStdString>::const_iterator ivecStrings;
};

/*!
 \ingroup strings
 \brief 
 */
extern CLocalizeStrings g_localizeStrings;
#endif
