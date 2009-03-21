#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef REGEXP_H
#define REGEXP_H

#ifdef _XBOX
#include <xtl.h>
#endif

#include <stdio.h>
#include <string>

namespace PCRE {
#ifdef _WIN32
#define PCRE_STATIC
#include "lib/libpcre/pcre.h"
#elif defined (__APPLE__)
#include "lib/libpcre/pcre.h"
#else
#include <pcre.h>
#endif
}

// maximum of 20 backreferences
// OVEVCOUNT must be a multiple of 3
const int OVECCOUNT=(20+1)*3;

class CRegExp
{
public:
  CRegExp(bool caseless = false);
  ~CRegExp();

  CRegExp *RegComp( const char *re);
  int RegFind(const char *str, int startoffset = 0);
  char* GetReplaceString( const char* sReplaceExp );
  int GetFindLen()
  {
    if (!m_re || !m_bMatched)
      return 0;

    return (m_iOvector[1] - m_iOvector[0]);
  };
  int GetSubCount() { return m_iMatchCount - 1; } // PCRE returns the number of sub-patterns + 1
  int GetSubStart(int iSub) { return m_iOvector[iSub*2] - m_iOvector[0]; } // normalized to match old engine
  int GetSubLength(int iSub) { return (m_iOvector[(iSub*2)+1] - m_iOvector[(iSub*2)]); } // correct spelling
  CStdString GetMatch(int iSub = 0);
  bool GetNamedSubPattern(const char* strName, CStdString& strMatch);
  void DumpOvector(int iLog = LOGDEBUG);

private:
  void Cleanup() { if (m_re) { PCRE::pcre_free(m_re); m_re = NULL; } }

private:
  PCRE::pcre* m_re;
  int         m_iOvector[OVECCOUNT];
  int         m_iMatchCount;
  int         m_iOptions;
  bool        m_bMatched;
  std::string m_subject;
};

#endif

