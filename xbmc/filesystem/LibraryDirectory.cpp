/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryDirectory.h"

#include "Directory.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "SmartPlaylistDirectory.h"
#include "URL.h"
#include "guilib/GUIControlFactory.h" // for label parsing
#include "guilib/TextureManager.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfileManager.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

using namespace XFILE;

CLibraryDirectory::CLibraryDirectory(void) = default;

CLibraryDirectory::~CLibraryDirectory(void) = default;

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
          CLog::Log(LOGERROR, "<content> tag must not be empty for type=\"filter\" node '{}'",
                    libNode);
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
        std::string label;
        if (XMLUtils::GetString(node, "label", label))
          label = CGUIControlFactory::FilterLabel(label);
        items.SetLabel(label);
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
      if (!icon.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(icon))
        item->SetArt("icon", icon);
      item->m_iprogramCount = order;
      items.Add(item);
    }
  }
  items.Sort(SortByPlaylistOrder, SortOrderAscending);
  return true;
}

TiXmlElement *CLibraryDirectory::LoadXML(const std::string &xmlFile)
{
  if (!CFileUtils::Exists(xmlFile))
    return nullptr;

  if (!m_doc.LoadFile(xmlFile))
    return nullptr;

  TiXmlElement *xml = m_doc.RootElement();
  if (!xml || xml->ValueStr() != "node")
    return nullptr;

  // check the condition
  std::string condition = XMLUtils::GetAttribute(xml, "visible");
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (condition.empty() ||
      (gui && gui->GetInfoManager().EvaluateBool(condition, INFO::DEFAULT_CONTEXT)))
    return xml;

  return nullptr;
}

bool CLibraryDirectory::Exists(const CURL& url)
{
  return !GetNode(url).empty();
}

std::string CLibraryDirectory::GetNode(const CURL& url)
{
  std::string libDir = URIUtils::AddFileToFolder(m_profileManager->GetLibraryFolder(), url.GetHostName() + "/");
  if (!CDirectory::Exists(libDir))
    libDir = URIUtils::AddFileToFolder("special://xbmc/system/library/", url.GetHostName() + "/");

  libDir = URIUtils::AddFileToFolder(libDir, url.GetFileName());

  // is this a virtual node (aka actual folder on disk?)
  if (CDirectory::Exists(libDir))
    return libDir;

  // maybe it's an XML node?
  std::string xmlNode = libDir;
  URIUtils::RemoveSlashAtEnd(xmlNode);

  if (CFileUtils::Exists(xmlNode))
    return xmlNode;

  return "";
}
