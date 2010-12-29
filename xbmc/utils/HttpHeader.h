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

#include <map>
#include <vector>
#include "StdString.h"

#define HTTPHEADER_CONTENT_TYPE "Content-Type"

typedef std::map<CStdString, CStdString> HeaderParams;
typedef std::map<CStdString, CStdString>::iterator HeaderParamsIter;

class CHttpHeader
{
public:
  CHttpHeader();
  ~CHttpHeader();

  void Parse(CStdString strData);
  CStdString GetValue(CStdString strParam) const;

  void GetHeader(CStdString& strHeader) const;

  CStdString GetMimeType() { return GetValue(HTTPHEADER_CONTENT_TYPE); }
  CStdString GetProtoLine() { return m_protoLine; }

  void Clear();

protected:
  HeaderParams m_params;
  CStdString   m_protoLine;
};

