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

#include "system.h"

#if defined(HAS_FILESYSTEM_AFP)
#include "AFPDirectory.h"
#include "FileAFP.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "Application.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "PasswordManager.h"
#include "DllLibAfp.h"
  
#include "/Users/chris/Development/source/libxafp/include/libxafp.h"

using namespace XFILE;
using namespace std;

CAFPDirectory::CAFPDirectory(void)
{

}

CAFPDirectory::~CAFPDirectory(void)
{

}

bool CAFPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  
  CAFPContext ctx(CFileAFP::GetClientContext(url));  
  if (!(xafp_client_handle)ctx)
    return false;

  // TODO: 
  xafp_node_iterator iter = xafp_get_dir_iter(ctx, url.GetFileName().c_str());
  if (!iter)
  {
    if (m_allowPrompting)
    {
      RequireAuthentication(url.Get());
    }
    return false;
  }
  
  bool addSlash = (strPath.c_str()[strPath.size() - 1] != '/');
  xafp_node_info* pNode = xafp_next(iter);
  while(pNode)
  {
    CFileItemPtr pItem(new CFileItem(pNode->name));      

    if (pNode->isDirectory)
    {
      pItem->SetPath(strPath + (addSlash ? "/" : "") + pNode->name);
      pItem->m_bIsFolder = true;
      pItem->m_dateTime  = pNode->modDate;
      pItem->m_dwSize = 0;
    }
    else
    {
      pItem->SetPath(strPath + (addSlash ? "/" : "") + pNode->name);
      pItem->m_bIsFolder = false;
      pItem->m_dwSize    = pNode->fileInfo.dataForkLen;
      pItem->m_dateTime  = pNode->modDate;
    }
    
    if (pNode->attributes & xafp_node_att_hidden)
    {
      pItem->SetProperty("file:hidden", true);
    }
    
    items.Add(pItem);      
    pNode = xafp_next(iter);
  }

  return true;
}

bool CAFPDirectory::Create(const char* strPath)
{
  CURL url(strPath);
  
  xafp_client_handle ctx = CFileAFP::GetClientContext(url);
  if (!ctx)
    return false;

  return (xafp_create_dir(ctx, url.GetFileName().c_str()) == 0);
}

bool CAFPDirectory::Remove(const char *strPath)
{
  CURL url(strPath);
  
  xafp_client_handle ctx = CFileAFP::GetClientContext(url);
  if (!ctx)
    return false;
  
  return (xafp_remove(ctx, url.GetFileName().c_str()) == 0);
}

bool CAFPDirectory::Exists(const char *strPath)
{
  CURL url(strPath);
  CAFPContext ctx(CFileAFP::GetClientContext(url));  
  if (!(xafp_client_handle)ctx)
    return false;
  
  struct stat info;
  int ret = xafp_stat(ctx, url.GetFileName().c_str(), &info);
  xafp_free_context(gAFPCtxPool, ctx);
  if (ret)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
}
#endif
