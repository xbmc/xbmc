/*
 *      Copyright (C) 2011 Team XBMC
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

#include "LibraryDirectory.h"
#include "playlists/SmartPlayList.h"
#include "SmartPlaylistDirectory.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "guilib/GUIControlFactory.h" // for label parsing
#include "guilib/TextureManager.h"
#include "FileItem.h"
#include "File.h"
#include "settings/Settings.h"
#include "URL.h"
#include "GUIInfoManager.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;

CLibraryDirectory::CLibraryDirectory(void)
{
}

CLibraryDirectory::~CLibraryDirectory(void)
{
}

bool CLibraryDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  TiXmlElement *node = GetNode(strPath);
  if (!node)
    return false;

  CStdString label;
  if (XMLUtils::GetString(node, "label", label))
    label = CGUIControlFactory::FilterLabel(label);

  // check if our node is a filter node
  CStdString type = node->Attribute("type");
  if (type == "filter")
  {
    CSmartPlaylist playlist;
    CStdString type;
    XMLUtils::GetString(node, "content", type);
    playlist.SetType(type);
    playlist.SetName(label);
    if (playlist.LoadFromXML(node) &&
        CSmartPlaylistDirectory::GetDirectory(playlist, items))
    {
      items.SetProperty("library.filter", "true");
      return true;
    }
    return false;
  }

  // now that we have our node, return all subnodes of this one, and set the label and thumb etc.
  TiXmlElement *child = node->FirstChildElement("node");
  while (child)
  {
    CStdString label, icon;
    if (XMLUtils::GetString(child, "label", label))
      label = CGUIControlFactory::FilterLabel(label);
    XMLUtils::GetString(child, "icon", icon);
    CStdString condition = child->Attribute("visible");
    CFileItemPtr item;
    if (condition.IsEmpty() || g_infoManager.EvaluateBool(condition))
    {
      CStdString type = child->Attribute("type");
      if (type == "folder")
      { // folder type - grab our path
        CStdString path;
        XMLUtils::GetPath(child, "path", path);
        item.reset(new CFileItem(path, true));
      }
      else if (type.IsEmpty() || type == "filter")
      { // virtual folder or filter
        CStdString id = child->Attribute("id");
        if (!id.IsEmpty())
          item.reset(new CFileItem(URIUtils::AddFileToFolder(strPath, id), true));
        else // illegal
          CLog::Log(LOGERROR, "virtual folder or filter <node>'s must have a valid id");
      }
    }
    if (item)
    {
      item->SetLabel(label);
      if (!icon.IsEmpty() && g_TextureManager.HasTexture(icon))
        item->SetIconImage(icon);
      items.Add(item);
    }
    child = child->NextSiblingElement("node");
  }
  items.SetLabel(label);
  return true;
}

bool CLibraryDirectory::Exists(const char* strPath)
{
  return GetNode(strPath) != NULL;
}

TiXmlElement *CLibraryDirectory::GetNode(const CStdString &path)
{
  // step 1: load our xml
  CURL url(path);
  CStdString xmlFile = URIUtils::AddFileToFolder(g_settings.GetDatabaseFolder(), url.GetHostName() + ".xml");
  if (!CFile::Exists(xmlFile))
    xmlFile = URIUtils::AddFileToFolder("special://xbmc/system/library/", url.GetHostName() + ".xml");
  if (!m_doc.LoadFile(xmlFile))
    return NULL;

  // step 2: sanitize the xml
  TiXmlElement *node = m_doc.RootElement();
  if (!node || node->ValueStr() != "library")
    return NULL;

  // step 3: parse the xml to find which node this path corresponds to
  vector<CStdString> nodes;
  StringUtils::SplitString(url.GetFileName(), "/", nodes);
  for (vector<CStdString>::iterator i = nodes.begin(); i != nodes.end(); ++i)
  {
    if (i->IsEmpty())
      continue;
    TiXmlElement *subNode = node->FirstChildElement("node");
    while (subNode)
    {
      if (*i == subNode->Attribute("id"))
      {
        node = subNode;
        break;
      }
      subNode = subNode->NextSiblingElement("node");
    }
    if (!subNode)
      return NULL; // didn't find requested node
  }
  return node;
}
