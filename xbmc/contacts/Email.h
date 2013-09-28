#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <vector>

#include "utils/ScraperUrl.h"

class TiXmlNode;
class CContactDatabase;

class CPhone
{
public:
  long idPhone;
  bool operator<(const CPhone& a) const
  {
    if (strPhone < a.strPhone) return true;
    if (strPhone > a.strPhone) return false;
    return false;
  }

  void Reset()
  {
    strPhone.Empty();
    idPhone = -1;
  }

  /*! \brief Load artist information from an XML file.
   See CVideoInfoTag::Load for a description of the types of elements we load.
   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
   \sa CVideoInfoTag::Load
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);

  CStdString strPhone;
  CStdString strPath;
  CScraperUrl thumbURL;
};


typedef std::vector<CPhone> VECPHONES;

