#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "utils/XMLUtils.h"
#include "utils/RegExp.h"
#include "FileItem.h"
#include "StreamsManager.h"

class TiXmlElement;

class CShadersSelectionRule
{
public:
  CShadersSelectionRule(TiXmlElement* rule);
  virtual ~CShadersSelectionRule();

  void GetShaders(const CFileItem& item, std::vector<uint32_t> &shaders, bool dxva = false);

private:
  int GetTristate(const char* szValue) const;
  bool CompileRegExp(const CStdString& str, CRegExp& regExp) const;
  bool MatchesRegExp(const CStdString& str, CRegExp& regExp) const;
  void Initialize(TiXmlElement* pRule);

  CStdString m_name;

  CStdString m_mimeTypes;
  CStdString m_fileName;

  CStdString m_audioCodec;
  CStdString m_audioChannels;
  CStdString m_videoCodec;
  CStdString m_videoResolution;
  CStdString m_videoAspect;
  CStdString m_videoFourcc;

  int32_t m_shaderId;
  bool m_bStreamDetails;

  std::vector<CShadersSelectionRule *> vecSubRules;
  int m_dxva;
};
