/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "LibraryDirectory.h"
#include "Directory.h"
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
  CStdString libNode = GetNode(strPath);
  if (libNode.IsEmpty())
    return false;

  if (URIUtils::GetExtension(libNode).Equals(".xml"))
  { // a filter node
    TiXmlElement *node = LoadXML(libNode);
    if (node)
    {
      CStdString type = node->Attribute("type");
      if (type == "filter")
      {
        CSmartPlaylist playlist;
        CStdString type, label;
        XMLUtils::GetString(node, "content", type);
        if (type.IsEmpty())
        {
          CLog::Log(LOGERROR, "<content> tag must not be empty for type=\"filter\" node '%s'", libNode.c_str());
          return false;
        }
        if (XMLUtils::GetString(node, "label", label))
          label = CGUIControlFactory::FilterLabel(label);
        playlist.SetType(type);
        playlist.SetName(label);
        if (playlist.LoadFromXML(node) &&
            CSmartPlaylistDirectory::GetDirectory(playlist, items))
        {
          items.SetProperty("library.filter", "true");
          return true;
        }
      }
    }
    return false;
  }

  // just a plain node - read the folder for XML nodes and other folders
  CFileItemList nodes;
  if (!CDirectory::GetDirectory(libNode, nodes, ".xml", DIR_FLAG_NO_FILE_DIRS))
    return false;

  // iterate over our nodes
  for (int i = 0; i < nodes.Size(); i++)
  {
    const TiXmlElement *node = NULL;
    CStdString xml = nodes[i]->GetPath();
    if (nodes[i]->m_bIsFolder)
      node = LoadXML(URIUtils::AddFileToFolder(xml, "index.xml"));
    else
    {
      node = LoadXML(xml);
      if (node && URIUtils::GetFileName(xml).Equals("index.xml"))
      { // set the label on our items
        CStdString label;
        if (XMLUtils::GetString(node, "label", label))
          label = CGUIControlFactory::FilterLabel(label);
        items.SetLabel(label);
        continue;
      }
    }
    if (node)
    {
      CStdString label, icon;
      if (XMLUtils::GetString(node, "label", label))
        label = CGUIControlFactory::FilterLabel(label);
      XMLUtils::GetString(node, "icon", icon);
      CStdString type = node->Attribute("type");
      int order = 0;
      node->Attribute("order", &order);
      CFileItemPtr item;
      if (type == "folder")
      { // folder type - grab our path
        CStdString path;
        XMLUtils::GetPath(node, "path", path);
        if (path.IsEmpty())
        {
          CLog::Log(LOGERROR, "<path> tag must be not be empty for type=\"folder\" node '%s'", xml.c_str());
          continue;
        }
        item.reset(new CFileItem(path, true));
      }
      else
      { // virtual folder or filter
        URIUtils::RemoveSlashAtEnd(xml);
        CStdString folder = URIUtils::GetFileName(xml);
        item.reset(new CFileItem(URIUtils::AddFileToFolder(strPath, folder), true));
      }
      item->SetLabel(label);
      if (!icon.IsEmpty() && g_TextureManager.HasTexture(icon))
        item->SetIconImage(icon);
      item->m_iprogramCount = order;
      items.Add(item);
    }
  }
  items.Sort(SORT_METHOD_PLAYLIST_ORDER, SortOrderAscending);
  return true;
}

TiXmlElement *CLibraryDirectory::LoadXML(const CStdString &xmlFile)
{
  if (!CFile::Exists(xmlFile))
    return NULL;

  if (!m_doc.LoadFile(xmlFile))
    return NULL;

  TiXmlElement *xml = m_doc.RootElement();
  if (!xml || xml->ValueStr() != "node")
    return NULL;

  // check the condition
  CStdString condition = xml->Attribute("visible");
  if (condition.IsEmpty() || g_infoManager.EvaluateBool(condition))
    return xml;

  return NULL;
}

bool CLibraryDirectory::Exists(const char* strPath)
{
  return !GetNode(strPath).IsEmpty();
}

CStdString CLibraryDirectory::GetNode(const CStdString &path)
{
  CURL url(path);
  CStdString libDir = URIUtils::AddFileToFolder(g_settings.GetDatabaseFolder(), url.GetHostName() + "/");
  if (!CDirectory::Exists(libDir))
    libDir = URIUtils::AddFileToFolder("special://xbmc/system/library/", url.GetHostName() + "/");

  libDir = URIUtils::AddFileToFolder(libDir, url.GetFileName());

  // is this a virtual node (aka actual folder on disk?)
  if (CDirectory::Exists(libDir))
    return libDir;

  // maybe it's an XML node?
  CStdString xmlNode = libDir;
  URIUtils::RemoveSlashAtEnd(xmlNode);

  if (CFile::Exists(xmlNode))
    return xmlNode;

  return "";
}
