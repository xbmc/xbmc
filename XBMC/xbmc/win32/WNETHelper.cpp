/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "WNETHelper.h"
#include "WIN32Util.h"

using namespace std;


CWNETHelper::CWNETHelper(void)
{
}

CWNETHelper::~CWNETHelper(void)
{
}

bool CWNETHelper::GetShares(CFileItemList &items)
{
  bool ret;
  LPNETRESOURCE lpnr = NULL;
  ret = EnumerateFunc(lpnr, items);
  return ret;
}


bool CWNETHelper::EnumerateFunc(LPNETRESOURCE lpnr, CFileItemList &items)
{
  DWORD dwResult, dwResultEnum;
  HANDLE hEnum;
  DWORD cbBuffer = 16384;     // 16K is a good size
  DWORD cEntries = -1;        // enumerate all possible entries
  LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
  DWORD i;
  //
  // Call the WNetOpenEnum function to begin the enumeration.
  //
  dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
                          RESOURCETYPE_ANY,   // all resources
                          0,  // enumerate all resources
                          lpnr,       // NULL first time the function is called
                          &hEnum);    // handle to the resource

  if (dwResult != NO_ERROR) 
  {
    CLog::Log(LOGERROR,"WnetOpenEnum failed with error %d\n", dwResult);
    return false;
  }
  //
  // Call the GlobalAlloc function to allocate resources.
  //
  lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
  if (lpnrLocal == NULL) 
  {
    CLog::Log(LOGERROR,"WnetOpenEnum failed with error %d\n", dwResult);
    return false;
  }

  do 
  {
    //
    // Initialize the buffer.
    //
    ZeroMemory(lpnrLocal, cbBuffer);
    //
    // Call the WNetEnumResource function to continue
    //  the enumeration.
    //
    dwResultEnum = WNetEnumResource(hEnum,  // resource handle
                                    &cEntries,      // defined locally as -1
                                    lpnrLocal,      // LPNETRESOURCE
                                    &cbBuffer);     // buffer size
    //
    // If the call succeeds, loop through the structures.
    //
    if (dwResultEnum == NO_ERROR) 
    {
      for (i = 0; i < cEntries; i++) 
      {
        if(lpnrLocal[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
        {
          CStdString strurl = "smb:";
          CStdString strName = lpnrLocal[i].lpComment;
          if(strName.empty())
          {
            strName = lpnrLocal[i].lpRemoteName;
            strName.Replace("\\","");
          }
          CFileItemPtr pItem(new CFileItem(strName));     
          strurl.append(lpnrLocal[i].lpRemoteName);
          strurl.Replace("\\","/");
          CURL rooturl(strurl);
          rooturl.SetFileName("");
          pItem->m_strPath = CWIN32Util::URLEncode(rooturl);
          pItem->m_bIsFolder = true;
          items.Add(pItem);
        }

        // If the NETRESOURCE structure represents a container resource, 
        //  call the EnumerateFunc function recursively.
        if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER))
          EnumerateFunc(&lpnrLocal[i], items);
      }
    }
    // Process errors.
    //
    else if (dwResultEnum != ERROR_NO_MORE_ITEMS) 
    {
      CLog::Log(LOGERROR,"WNetEnumResource failed with error %d\n", dwResultEnum);
      break;
    }
  }
  //
  // End do.
  //
  while (dwResultEnum != ERROR_NO_MORE_ITEMS);
  //
  // Call the GlobalFree function to free the memory.
  //
  GlobalFree((HGLOBAL) lpnrLocal);
  //
  // Call WNetCloseEnum to end the enumeration.
  //
  dwResult = WNetCloseEnum(hEnum);

  if (dwResult != NO_ERROR) 
  {
      //
      // Process errors.
      //
      CLog::Log(LOGERROR,"WNetCloseEnum failed with error %d\n", dwResult);
      return false;
  }

  return true;
}