#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DVDSUBTITLES_DVDSUBTITLEPARSER_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLEPARSER_H_INCLUDED
#include "DVDSubtitleParser.h"
#endif

#ifndef DVDSUBTITLES_DVDSUBTITLELINECOLLECTION_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLELINECOLLECTION_H_INCLUDED
#include "DVDSubtitleLineCollection.h"
#endif


class CDVDSubtitleParserMPL2 : public CDVDSubtitleParserText
{
public:
  CDVDSubtitleParserMPL2(CDVDSubtitleStream* stream, const std::string& strFile);
  virtual ~CDVDSubtitleParserMPL2();

  virtual bool Open(CDVDStreamInfo &hints);
private:
  double m_framerate;
};
