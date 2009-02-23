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

#include "IDirectory.h"

class CURL;
class TiXmlElement;

#define SHOUTCAST_MASTER_LINK "shout://classic.shoutcast.com/sbin/newxml.phtml"

namespace DIRECTORY
{
class CShoutcastDirectory :
      public IDirectory
{
public:
  CShoutcastDirectory(void);
  virtual ~CShoutcastDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const { return DIR_CACHE_ALWAYS; };
  bool ParseGenres(TiXmlElement *root, CFileItemList &items, CURL &url);
  bool ParseStations(TiXmlElement *root, CFileItemList &items, CURL &url);
protected:
};

}
