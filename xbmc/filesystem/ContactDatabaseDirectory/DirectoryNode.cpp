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

#include "DirectoryNode.h"
#include "utils/URIUtils.h"
#include "QueryParams.h"
#include "DirectoryNodeRoot.h"

#include "DirectoryNodeOverview.h"
#include "DirectoryNodeFace.h"
#include "DirectoryNodeContact.h"
#include "DirectoryNodeContactRecentlyAdded.h"


#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "contacts/ContactDbUrl.h"

using namespace std;
using namespace XFILE::CONTACTDATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
    m_Type=Type;
    m_strName=strName;
    m_pParent=pParent;
}

CDirectoryNode::~CDirectoryNode()
{
    delete m_pParent;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode* CDirectoryNode::ParseURL(const CStdString& strPath)
{
    CURL url(strPath);
    
    CStdString strDirectory=url.GetFileName();
    URIUtils::RemoveSlashAtEnd(strDirectory);
    
    CStdStringArray Path;
    StringUtils::SplitString(strDirectory, "/", Path);
    if (!strDirectory.IsEmpty())
        Path.insert(Path.begin(), "");
    
    CDirectoryNode* pNode=NULL;
    CDirectoryNode* pParent=NULL;
    NODE_TYPE NodeType=NODE_TYPE_ROOT;
    
    for (int i=0; i<(int)Path.size(); ++i)
    {
        pNode=CDirectoryNode::CreateNode(NodeType, Path[i], pParent);
        NodeType= pNode ? pNode->GetChildType() : NODE_TYPE_NONE;
        pParent=pNode;
    }
    
    // Add all the additional URL options to the last node
    if (pNode)
        pNode->AddOptions(url.GetOptions());
    
    return pNode;
}

//  returns the database ids of the path,
void CDirectoryNode::GetDatabaseInfo(const CStdString& strPath, CQueryParams& params)
{
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
    
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
        case NODE_TYPE_FACE:
            return new CDirectoryNodeFace(strName, pParent);
        case NODE_TYPE_CONTACT:
            return new CDirectoryNodeContact(strName, pParent);
        case NODE_TYPE_CONTACT_RECENTLY_ADDED:
            return new CDirectoryNodeContactRecentlyAdded(strName, pParent);
        default:
            break;
    }
    return NULL;
}

//  Current node name
const CStdString& CDirectoryNode::GetName() const
{
    return m_strName;
}

int CDirectoryNode::GetID() const
{
    return atoi(m_strName.c_str());
}

CStdString CDirectoryNode::GetLocalizedName() const
{
    return "";
}

//  Current node type
NODE_TYPE CDirectoryNode::GetType() const
{
    return m_Type;
}

//  Return the parent directory node or NULL, if there is no
CDirectoryNode* CDirectoryNode::GetParent() const
{
    return m_pParent;
}

void CDirectoryNode::RemoveParent()
{
    m_pParent=NULL;
}

//  should be overloaded by a derived class
//  to get the content of a node. Will be called
//  by GetChilds() of a parent node
bool CDirectoryNode::GetContent(CFileItemList& items) const
{
    return false;
}

//  Creates a contactdb url
CStdString CDirectoryNode::BuildPath() const
{
    CStdStringArray array;
    
    if (!m_strName.IsEmpty())
        array.insert(array.begin(), m_strName);
    
    CDirectoryNode* pParent=m_pParent;
    while (pParent!=NULL)
    {
        const CStdString& strNodeName=pParent->GetName();
        if (!strNodeName.IsEmpty())
            array.insert(array.begin(), strNodeName);
        
        pParent=pParent->GetParent();
    }
    
    CStdString strPath="contactdb://";
    for (int i=0; i<(int)array.size(); ++i)
        strPath+=array[i]+"/";
    
    string options = m_options.GetOptionsString();
    if (!options.empty())
        strPath += "?" + options;
    
    return strPath;
}

void CDirectoryNode::AddOptions(const CStdString &options)
{
    if (options.empty())
        return;
    
    m_options.AddOptions(options);
}

//  Collects Query params from this and all parent nodes. If a NODE_TYPE can
//  be used as a database parameter, it will be added to the
//  params object.
void CDirectoryNode::CollectQueryParams(CQueryParams& params) const
{
    params.SetQueryParam(m_Type, m_strName);
    
    CDirectoryNode* pParent=m_pParent;
    while (pParent!=NULL)
    {
        params.SetQueryParam(pParent->GetType(), pParent->GetName());
        pParent=pParent->GetParent();
    }
}

//  Should be overloaded by a derived class.
//  Returns the NODE_TYPE of the child nodes.
NODE_TYPE CDirectoryNode::GetChildType() const
{
    return NODE_TYPE_NONE;
}

//  Get the child fileitems of this node
bool CDirectoryNode::GetChilds(CFileItemList& items)
{
    if (CanCache() && items.Load())
        return true;
    
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));
    
    bool bSuccess=false;
    if (pNode.get())
    {
        pNode->m_options = m_options;
        bSuccess=pNode->GetContent(items);
        if (bSuccess)
        {
            AddQueuingFolder(items);
            if (CanCache())
                items.SetCacheToDisc(CFileItemList::CACHE_ALWAYS);
        }
        else
            items.Clear();
        
        pNode->RemoveParent();
    }
    
    return bSuccess;
}

//  Add an "* All ..." folder to the CFileItemList
//  depending on the child node
void CDirectoryNode::AddQueuingFolder(CFileItemList& items) const
{
    CFileItemPtr pItem;
    
    CContactDbUrl contactUrl;
    if (!contactUrl.FromString(BuildPath()))
        return;
    
    // always hide "all" items
//    if (g_advancedSettings.m_bContactLibraryHideAllItems)
//        return;
    
    // no need for "all" item when only one item
    if (items.GetObjectCount() <= 1)
        return;
    
    switch (GetChildType())
    {
            //  Have no queuing folder
        case NODE_TYPE_ROOT:
        case NODE_TYPE_OVERVIEW:
        case NODE_TYPE_TOP100:
            break;
            
            /* no need for all genres
             case NODE_TYPE_LOCATION:
             pItem.reset(new CFileItem(g_localizeStrings.Get(15105)));  // "All Genres"
             contactUrl.AppendPath("-1/");
             pItem->SetPath(contactUrl.ToString());
             break;
             */
            
        case NODE_TYPE_FACE:
            if (GetType() == NODE_TYPE_OVERVIEW) return;
            pItem.reset(new CFileItem(g_localizeStrings.Get(15103)));  // "All Faces"
            contactUrl.AppendPath("-1/");
            pItem->SetPath(contactUrl.ToString());
            break;
            
            //  All album related nodes
        case NODE_TYPE_CONTACT:
            if (GetType() == NODE_TYPE_OVERVIEW) return;
        default:
            break;
    }
  /*
    
    if (pItem)
    {
        pItem->m_bIsFolder = true;
        pItem->SetSpecialSort(g_advancedSettings.m_bContactLibraryAllItemsOnBottom ? SortSpecialOnBottom : SortSpecialOnTop);
        pItem->SetCanQueue(false);
        pItem->SetLabelPreformated(true);
        if (g_advancedSettings.m_bContactLibraryAllItemsOnBottom)
            items.Add(pItem);
        else
            items.AddFront(pItem, (items.Size() > 0 && items[0]->IsParentFolder()) ? 1 : 0);
    }
   */
}

bool CDirectoryNode::CanCache() const
{
    // JM: No need to cache these views, as caching is added in the mediawindow baseclass for anything that takes
    //     longer than a second
    return false;
}
