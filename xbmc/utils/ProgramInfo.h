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

#include "ProgramInfoTag.h"
#include "ScraperParser.h"

class TiXmlDocument;
class CScraperUrl;
struct SScraperInfo;

namespace XFILE { class CFileCurl; }

namespace PROGRAM_GRABBER
{
class CProgramInfo
{
public:
  CProgramInfo(void);
  CProgramInfo(const CStdString& strProgramInfo, const CScraperUrl& strAlbumURL);
  CProgramInfo(const CStdString& strTitle, const CStdString strPlatform, const CStdString strYear, const CScraperUrl& strProgramURL);
  virtual ~CProgramInfo(void);
  bool Loaded() const;
  void SetLoaded(bool bOnOff);
  void SetProgram(CProgramInfoTag& program);
  const CProgramInfoTag &GetProgram() const;
  CProgramInfoTag& GetProgram();
  const CScraperUrl& GetProgramURL() const;
  bool Load(XFILE::CFileCurl& http, const SScraperInfo& info, const CStdString& strFunction="GetProgramDetails", const CScraperUrl* url=NULL);
  bool Parse(const TiXmlElement* album, bool bChained=false);
protected:
  CProgramInfoTag m_program;
  CScraperUrl m_programURL;

  bool m_bLoaded;
};
}
