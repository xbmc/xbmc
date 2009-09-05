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

#include "utils/Archive.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include <map>
#include <vector>

class CProgramInfoTag : public ISerializable
{
  public:
    enum {TYPE_PROGRAM, TYPE_GAME};

  CProgramInfoTag() { m_idProgram = 0; };
  void Reset()
  {
    m_idProgram = -1;
    m_iType = TYPE_PROGRAM;
    m_strTitle.Empty();
    m_strPlatform.Empty();
    m_strDescription.Empty();
    m_strGenre.Empty();
    m_strStyle.Empty();
    m_strPublisher.Empty();
    m_strDateOfRelease.Empty();
    m_thumbURL.Clear();
    m_fanart.m_xml = "";
    m_strYear.Empty();
  }

  bool Load(const TiXmlElement *node, bool chained=false);
  bool Save(TiXmlNode *node, const CStdString &tag);
  virtual void Serialize(CArchive& ar);
  
  long m_idProgram;
  int m_iType;
  CStdString m_strTitle;
  CStdString m_strPlatform;
  CStdString m_strDescription;
  CStdString m_strGenre;
  CStdString m_strStyle;
  CStdString m_strPublisher;
  CStdString m_strDateOfRelease;
  CScraperUrl m_thumbURL;
  CFanart     m_fanart;
  CStdString m_strYear;
};
typedef std::vector<CProgramInfoTag> VECPROGRAMS;
