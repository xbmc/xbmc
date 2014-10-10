/*
 *      Copyright (C) 2013 Team XBMC
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

#include "IListProvider.h"
#include "utils/XBMCTinyXML.h"
#include "StaticProvider.h"
#include "DirectoryProvider.h"

IListProvider *IListProvider::Create(const TiXmlNode *node, int parentID)
{
  const TiXmlElement *root = node->FirstChildElement("content");
  if (root)
  {
    const TiXmlElement *item = root->FirstChildElement("item");
    if (item)
      return new CStaticListProvider(root, parentID);

    if (!root->NoChildren())
      return new CDirectoryProvider(root, parentID);
  }
  return NULL;
}
