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

// RTVDirectory.cpp: implementation of the CRTVDirectory class.
//
//////////////////////////////////////////////////////////////////////

#include "RTVDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "utils/XBMCTinyXML.h"
#include "FileItem.h"

using namespace XFILE;

extern "C"
{
#include "lib/libRTV/interface.h"
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRTVDirectory::CRTVDirectory(void)
{
}

CRTVDirectory::~CRTVDirectory(void)
{
}

//*********************************************************************************************
bool CRTVDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);

  CStdString strRoot = strPath;
  URIUtils::AddSlashAtEnd(strRoot);

  // Host name is "*" so we try to discover all ReplayTVs.  This requires some trickery but works.
  if (url.GetHostName() == "*")
  {
    // Check to see whether the URL's path is blank or "Video"
    if (url.GetFileName() == "" || url.GetFileName() == "Video")
    {
      int iOldSize=items.Size();
      struct RTV * rtv = NULL;
      int numRTV;

      // Request that all ReplayTVs on the LAN identify themselves.  Each ReplayTV
      // is given 3000ms to respond to the request.  rtv_discovery returns an array
      // of structs containing the IP and friendly name of all ReplayTVs found on the LAN.
      // For some reason, DVArchive doesn't respond to this request (probably only responds
      // to requests from an IP address of a real ReplayTV).
      numRTV = rtv_discovery(&rtv, 3000);

      // Run through the array and add the ReplayTVs found as folders in XBMC.
      // We must add the IP of each ReplayTV as if it is a file name at the end of a the
      // auto-discover URL--e.g. rtv://*/192.168.1.100--because XBMC does not permit
      // dyamically added shares and will not play from them.  This little trickery is
      // the best workaround I could come up with.
      for (int i = 0; i < numRTV; i++)
      {
        CFileItemPtr pItem(new CFileItem(rtv[i].friendlyName));
        // This will keep the /Video or / and allow one to set up an auto ReplayTV
        // share of either type--simple file listing or ReplayGuide listing.
        pItem->SetPath(strRoot + rtv[i].hostname);
        pItem->m_bIsFolder = true;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
      }
      free(rtv);
      return (items.Size()>iOldSize);
      // Else the URL's path should be an IP address of the ReplayTV
    }
    else
    {
      CStdString strURL, strRTV;
      int pos;

      // Isolate the IP from the URL and replace the "*" with the real IP
      // of the ReplayTV.  E.g., rtv://*/Video/192.168.1.100/ becomes
      // rtv://192.168.1.100/Video/ .  This trickery makes things work.
      strURL = strRoot.TrimRight('/');
      pos = strURL.ReverseFind('/');
      strRTV = strURL.Left(pos + 1);
      strRTV.Replace("*", strURL.Mid(pos + 1));
      CURL tmpURL(strRTV);

      // Force the newly constructed share into the right variables to
      // be further processed by the remainder of GetDirectory.
      url = tmpURL;
      strRoot = strRTV;
    }
  }

  // Allow for ReplayTVs on ports other than 80
  CStdString strHostAndPort;
  strHostAndPort = url.GetHostName();
  if (url.HasPort())
  {
    char buffer[10];
    sprintf(buffer,"%i",url.GetPort());
    strHostAndPort += ':';
    strHostAndPort += buffer;
  }

  // No path given, list shows from ReplayGuide
  if (url.GetFileName() == "")
  {
    unsigned char * data = NULL;

    // Get the RTV guide data in XML format
    rtv_get_guide_xml(&data, strHostAndPort.c_str());

    // Begin parsing the XML data
    CXBMCTinyXML xmlDoc;
    xmlDoc.Parse( (const char *) data );
    if ( xmlDoc.Error() )
    {
      free(data);
      return false;
    }
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement)
    {
      free(data);
      return false;
    }

    const TiXmlNode *pChild = pRootElement->FirstChild();
    while (pChild > 0)
    {
      CStdString strTagName = pChild->Value();

      if ( !strcmpi(strTagName.c_str(), "ITEM") )
      {
        const TiXmlNode *nameNode = pChild->FirstChild("DISPLAYNAME");
//        const TiXmlNode *qualityNode = pChild->FirstChild("QUALITY");
        const TiXmlNode *recordedNode = pChild->FirstChild("RECORDED");
        const TiXmlNode *pathNode = pChild->FirstChild("PATH");
//        const TiXmlNode *durationNode = pChild->FirstChild("DURATION");
        const TiXmlNode *sizeNode = pChild->FirstChild("SIZE");
        const TiXmlNode *atrbNode = pChild->FirstChild("ATTRIB");

        SYSTEMTIME dtDateTime;
        DWORD dwFileSize = 0;
        memset(&dtDateTime, 0, sizeof(dtDateTime));

        // DISPLAYNAME
        const char* szName = NULL;
        if (nameNode)
        {
          szName = nameNode->FirstChild()->Value() ;
        }
        else
        {
          // Something went wrong, the recording has no name
          free(data);
          return false;
        }

        // QUALITY
//        const char* szQuality = NULL;
//        if (qualityNode)
//        {
//          szQuality = qualityNode->FirstChild()->Value() ;
//        }

        // RECORDED
        if (recordedNode)
        {
          CStdString strRecorded = recordedNode->FirstChild()->Value();
          int iYear, iMonth, iDay;

          iYear = atoi(strRecorded.Left(4).c_str());
          iMonth = atoi(strRecorded.Mid(5, 2).c_str());
          iDay = atoi(strRecorded.Mid(8, 2).c_str());
          dtDateTime.wYear = iYear;
          dtDateTime.wMonth = iMonth;
          dtDateTime.wDay = iDay;

          int iHour, iMin, iSec;
          iHour = atoi(strRecorded.Mid(11, 2).c_str());
          iMin = atoi(strRecorded.Mid(14, 2).c_str());
          iSec = atoi(strRecorded.Mid(17, 2).c_str());
          dtDateTime.wHour = iHour;
          dtDateTime.wMinute = iMin;
          dtDateTime.wSecond = iSec;
        }

        // PATH
        const char* szPath = NULL;
        if (pathNode)
        {
          szPath = pathNode->FirstChild()->Value() ;
        }
        else
        {
          // Something went wrong, the recording has no filename
          free(data);
          return false;
        }

        // DURATION
//        const char* szDuration = NULL;
//        if (durationNode)
//        {
//          szDuration = durationNode->FirstChild()->Value() ;
//        }

        // SIZE
        // NOTE: Size here is actually just duration in minutes because
        // filesize is not reported by the stripped down GuideParser I use
        if (sizeNode)
        {
          dwFileSize = atol( sizeNode->FirstChild()->Value() );
        }

        // ATTRIB
        // NOTE: Not currently reported in the XML guide data, nor is it particularly
        // needed unless someone wants to add the ability to sub-divide the recordings
        // into categories, as on a real RTV.
        int attrib = 0;
        if (atrbNode)
        {
          attrib = atoi( atrbNode->FirstChild()->Value() );
        }

        bool bIsFolder(false);
        if (attrib & FILE_ATTRIBUTE_DIRECTORY)
          bIsFolder = true;

        CFileItemPtr pItem(new CFileItem(szName));
        pItem->m_dateTime=dtDateTime;
        pItem->SetPath(strRoot + szPath);
        // Hack to show duration of show in minutes as KB in XMBC because
        // it doesn't currently permit showing duration in minutes.
        // E.g., a 30 minute show will show as 29.3 KB in XBMC.
        pItem->m_dwSize = dwFileSize * 1000;
        pItem->m_bIsFolder = bIsFolder;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
      }

      pChild = pChild->NextSibling();
    }

    free(data);

    // Path given (usually Video), list filenames only
  }
  else
  {

    unsigned char * data;
    char * p, * q;
    unsigned long status;

    // Return a listing of all files in the given path
    status = rtv_list_files(&data, strHostAndPort.c_str(), url.GetFileName().c_str());
    if (status == 0)
    {
      return false;
    }

    // Loop through the file list using pointers p and q, where p will point to the current
    // filename and q will point to the next filename
    p = (char *) data;
    while (p)
    {
      // Look for the end of the current line of the file listing
      q = strchr(p, '\n');
      // If found, replace the newline character with the NULL terminator
      if (q)
      {
        *q = '\0';
        // Increment q so that it points to the next filename
        q++;
        // *p should be the current null-terminated filename in the list
        if (*p)
        {
          // Only display MPEG files in XBMC (but not circular.mpg, as that is the RTV
          // video buffer and XBMC may cause problems if it tries to play it)
          if (strstr(p, ".mpg") && !strstr(p, "circular"))
          {
            CFileItemPtr pItem(new CFileItem(p));
            pItem->SetPath(strRoot + p);
            pItem->m_bIsFolder = false;
            // The list returned by the RTV doesn't include file sizes, unfortunately
            //pItem->m_dwSize = atol(szSize);
            pItem->SetLabelPreformated(true);
            items.Add(pItem);
          }
        }
      }
      // Point p to the next filename in the list and loop
      p = q;
    }

    free(data);
  }

  return true;
}
