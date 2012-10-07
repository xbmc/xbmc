/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "QueryParams.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace XFILE::PICTUREDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idFolder = -1;
  m_idYear = -1;
  m_idCamera = -1;
  m_idTag = -1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName)
{
  switch (NodeType)
  {
  case NODE_TYPE_FOLDER:
    m_idFolder = atol(strNodeName.c_str());
    break;
  case NODE_TYPE_YEAR:
    m_idYear = atol(strNodeName.c_str());
    break;
  case NODE_TYPE_CAMERA:
    m_idCamera = atol(strNodeName.c_str());
    break;
  case NODE_TYPE_TAGS:
    m_idTag = atol(strNodeName.c_str());
    break;
  default:
    break;
  }
}

void CQueryParams::Mapify(map<string, long> &params)
{
  CStdString dbId;
  if (m_idFolder > 0)
    params["folder"] = m_idFolder;
  if (m_idYear > 0)
    params["year"] = m_idYear;
  if (m_idCamera > 0)
    params["camera"] = m_idCamera;
  if (m_idTag > 0)
    params["tag"] = m_idTag;
}
