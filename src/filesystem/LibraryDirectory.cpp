/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "LibraryDirectory.h"
#include "Directory.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfilesManager.h"
#include "SmartPlaylistDirectory.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "guilib/GUIControlFactory.h" // for label parsing
#include "guilib/TextureManager.h"
#include "FileItem.h"
#include "File.h"
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

bool CLibraryDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string libNode = GetNode(url);
  if (libNode.empty())
    return false;

  if (URIUtils::HasExtension(libNode, ".xml"))
  { // a filter or folder node
    TiXmlElement *node = LoadXML(libNode);
    if (node)
    {
      std::string type = XMLUtils::GetAttribute(node, "type");
      if (type == "filter")
      {
        CSmartPlaylist playlist;
        std::string type, label;
        XMLUtils::GetString(node, "content", type);
        if (type.empty())
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
          items.SetPath(items.GetProperty("path.db").asString());
          return true;
        }
      }
      else if (type == "folder")
      {
        std::string path;
        XMLUtils::GetPath(node, "path", path);
        if (!path.empty())
        {
          URIUtils::AddSlashAtEnd(path);
          return CDirectory::GetDirectory(path, items, m_strFileMask, m_flags);
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
  std::string basePath = url.Get();
  for (int i = 0; i < nodes.Size(); i++)
  {
    const TiXmlElement *node = NULL;
    std::string xml = nodes[i]->GetPath();
    if (nodes[i]->m_bIsFolder)
      node = LoadXML(URIUtils::AddFileToFolder(xml, "index.xml"));
    else
    {
      node = LoadXML(xml);
      if (node && URIUtils::GetFileName(xml) == "index.xml")
      { // set the label on our items
        std::string label;
        if (XMLUtils::GetString(node, "label", label))
          label = CGUIControlFactory::FilterLabel(label);
        items.SetLabel(label);
        continue;
      }
    }
    if (node)
    {
      std::string label, icon;
      if (XMLUtils::GetString(node, "label", label))
        label = CGUIControlFactory::FilterLabel(label);
      XMLUtils::GetString(node, "icon", icon);
      int order = 0;
      node->Attribute("order", &order);

      // create item
      URIUtils::RemoveSlashAtEnd(xml);
      std::string folder = URIUtils::GetFileName(xml);
      CFileItemPtr item(new CFileItem(URIUtils::AddFileToFolder(basePath, folder), true));

      item->SetLabel(label);
      if (!icon.empty() && g_TextureManager.HasTexture(icon))
        item->SetIconImage(icon);
      item->m_iprogramCount = order;
      items.Add(item);
    }
  }
  items.Sort(SortByPlaylistOrder, SortOrderAscending);
  return true;
}

TiXmlElement *CLibraryDirectory::LoadXML(const std::string &xmlFile)
{
  if (!CFile::Exists(xmlFile))
    return NULL;

  if (!m_doc.LoadFile(xmlFile))
    return NULL;

  TiXmlElement *xml = m_doc.RootElement();
  if (!xml || xml->ValueStr() != "node")
    return NULL;

  // check the condition
  std::string condition = XMLUtils::GetAttribute(xml, "visible");
  if (condition.empty() || g_infoManager.EvaluateBool(condition))
    return xml;

  return NULL;
}

bool CLibraryDirectory::Exists(const CURL& url)
{
  return !GetNode(url).empty();
}

std::string CLibraryDirectory::GetNode(const CURL& url)
{
  std::string libDir = URIUtils::AddFileToFolder(CProfilesManager::Get().GetLibraryFolder(), url.GetHostName() + "/");
  if (!CDirectory::Exists(libDir))
    libDir = URIUtils::AddFileToFolder("special://xbmc/system/library/", url.GetHostName() + "/");

  libDir = URIUtils::AddFileToFolder(libDir, url.GetFileName());

  // is this a virtual node (aka actual folder on disk?)
  if (CDirectory::Exists(libDir))
    return libDir;

  // maybe it's an XML node?
  std::string xmlNode = libDir;
  URIUtils::RemoveSlashAtEnd(xmlNode);

  if (CFile::Exists(xmlNode))
    return xmlNode;

  return "";
}
