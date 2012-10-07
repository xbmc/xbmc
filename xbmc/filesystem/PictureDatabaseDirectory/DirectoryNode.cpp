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

#include "DirectoryNode.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeFolder.h"
#include "DirectoryNodeYear.h"
#include "DirectoryNodeCamera.h"
#include "DirectoryNodeTags.h"
#include "DirectoryNodePictures.h"
#include "QueryParams.h"
#include "guilib/LocalizeStrings.h"
#include "pictures/PictureDatabase.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;


typedef struct
{
  NODE_TYPE   node;
  const char *vanity;
} NodeMap;

// Provides a URL of picturedb://folder instead of picturedb://1
static const NodeMap vanityLabel[] =
{
  { NODE_TYPE_FOLDER,     "folder"   },
  { NODE_TYPE_YEAR,       "year"     },
  { NODE_TYPE_CAMERA,     "camera"   },
  { NODE_TYPE_TAGS,       "tag"      },
};

//  Constructor is protected, use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  m_Type = Type;
  m_strName = strName;
  m_pParent = pParent;
}

CDirectoryNode::~CDirectoryNode()
{
  delete m_pParent;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode *CDirectoryNode::ParseURL(const CStdString& strPath)
{
  CURL url(strPath);

  // Ignore the hostname, consider it part of the filename
  CStdString strDirectory = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectory);

  CStdStringArray Path;
  StringUtils::SplitString(strDirectory, "/", Path);
  if (!strDirectory.IsEmpty())
    Path.insert(Path.begin(), "");

  CDirectoryNode* pNode = NULL;
  CDirectoryNode* pParent = NULL;
  NODE_TYPE NodeType = NODE_TYPE_ROOT;

  for (unsigned int i = 0; i < Path.size(); i++)
  {
    pNode = CreateNode(NodeType, Path[i], pParent);
    NodeType = pNode ? pNode->GetChildType() : NODE_TYPE_NONE;
    pParent = pNode;
  }

  // Add all the additional URL options to the last node
  if (pNode)
    pNode->AddOptions(url.GetOptions());

  return pNode;
}

//  returns the database IDs of the path,
void CDirectoryNode::GetDatabaseInfo(const CStdString& strPath, CQueryParams& params)
{
  auto_ptr<CDirectoryNode> pNode(ParseURL(strPath));

  if (!pNode.get())
    return;

  pNode->CollectQueryParams(params);
}

//  Create a node object
CDirectoryNode* CDirectoryNode::CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  switch (Type)
  {
  case NODE_TYPE_ROOT:
    return new CDirectoryNodeRoot(strName, pParent);
  case NODE_TYPE_OVERVIEW:
    return new CDirectoryNodeOverview(strName, pParent);
  case NODE_TYPE_FOLDER:
    return new CDirectoryNodeFolder(strName, pParent);
  case NODE_TYPE_YEAR:
    return new CDirectoryNodeYear(strName, pParent);
  case NODE_TYPE_CAMERA:
    return new CDirectoryNodeCamera(strName, pParent);
  case NODE_TYPE_TAGS:
    return new CDirectoryNodeTags(strName, pParent);
  case NODE_TYPE_PICTURES:
    return new CDirectoryNodePictures(strName, pParent);
  default:
    break;
  }
  return NULL;
}

//  Creates a pituredb:// URL
CStdString CDirectoryNode::BuildPath() const
{
  CStdStringArray strArray;

  if (!m_strName.IsEmpty())
    strArray.insert(strArray.begin(), m_strName);

  CDirectoryNode* pParent = m_pParent;
  while (pParent)
  {
    const CStdString& strNodeName = pParent->GetName();
    if (!strNodeName.IsEmpty())
      strArray.insert(strArray.begin(), strNodeName);
    pParent = pParent->GetParent();
  }

  CStdString strPath = "picturedb://";
  for (unsigned int i = 0; i < strArray.size(); ++i)
    strPath += strArray[i] + "/";

  string options = m_options.GetOptionsString();
  if (!options.empty())
    strPath += "?" + options;

  return strPath;
}

void CDirectoryNode::AddOptions(const CStdString &options)
{
  if (!options.empty())
    m_options.AddOptions(options);
}

//  Collects Query params from this and all parent nodes. If a NODE_TYPE can
//  be used as a database parameter, it will be added to the
//  params object.
void CDirectoryNode::CollectQueryParams(CQueryParams& params) const
{
  params.SetQueryParam(m_Type, m_strName);

  CDirectoryNode* pParent = m_pParent;
  while (pParent)
  {
    params.SetQueryParam(pParent->GetType(), pParent->GetName());
    pParent = pParent->GetParent();
  }
}

//  Get the child fileitems of this node
bool CDirectoryNode::GetChilds(CFileItemList& items)
{
  if (CanCache() && items.Load())
    return true;

  auto_ptr<CDirectoryNode> pNode(CreateNode(GetChildType(), "", this));

  bool bSuccess = false;
  if (pNode.get())
  {
    pNode->m_options = m_options;
    bSuccess = pNode->GetContent(items);
    if (bSuccess)
    {
      //AddQueuingFolder(items);
      if (CanCache())
        items.SetCacheToDisc(CFileItemList::CACHE_ALWAYS);
    }
    else
      items.Clear();

    pNode->RemoveParent();
  }

  return bSuccess;
}

CStdString CDirectoryNode::GetLocalizedName() const
{
  CPictureDatabase db;
  if (db.Open())
  {
    // Variants are used because item can be an int (Year) or text (Folder)
    CVariant item;
    if (db.GetItemByID(GetVanity(m_Type), atoi(GetName()), item))
      return item.asString();
  }
  return "";
}

bool CDirectoryNode::GetContent(CFileItemList& items) const
{
  bool bSuccess = false;
  CPictureDatabase pictureDatabase;
  if (pictureDatabase.Open())
  {
    CQueryParams params;
    map<string, long> predicates;
    CollectQueryParams(params);
    params.Mapify(predicates);

    if (m_Type == NODE_TYPE_PICTURES)
      bSuccess = pictureDatabase.GetObjectsNav(items, predicates);
    else
      bSuccess = pictureDatabase.GetItemNav(GetVanity(m_Type), items, BuildPath(), predicates);

    pictureDatabase.Close();
  }
  return bSuccess;
}

const char* CDirectoryNode::GetVanity(NODE_TYPE node)
{
  for (unsigned int i = 0; i < sizeof(vanityLabel) / sizeof(NodeMap); i++)
    if (vanityLabel[i].node == node)
      return vanityLabel[i].vanity;
  return "";
}
