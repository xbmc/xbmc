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
  bool Load(const CStdString& strFileName, const CStdString& strFallbackFileName="Q:\\language\\english\\strings.xml");
  bool LoadSkinStrings(const CStdString& path, const CStdString& fallbackPath);
  void ClearSkinStrings();
  const CStdString& Get(DWORD dwCode) const;
  void Clear();
protected:
  bool LoadXML(const CStdString &filename, CStdString &encoding, CStdString &error);
  CStdString ToUTF8(const CStdString &encoding, const CStdString &str);
  std::map<DWORD, CStdString> m_strings;
  typedef std::map<DWORD, CStdString>::const_iterator ciStrings;
  typedef std::map<DWORD, CStdString>::iterator       iStrings;
};

/*!
 \ingroup strings
 \brief 
 */
extern CLocalizeStrings g_localizeStrings;
extern CLocalizeStrings g_localizeStringsTemp;
#endif
