/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "ZeroconfDirectory.h"
#include <stdexcept>

#include "URL.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "network/ZeroconfBrowser.h"
#include "Directory.h"
#include "utils/log.h"

using namespace XFILE;

CZeroconfDirectory::CZeroconfDirectory()
{
  CZeroconfBrowser::GetInstance()->Start();
}

CZeroconfDirectory::~CZeroconfDirectory()
{
}

namespace
{
  CStdString GetHumanReadableProtocol(std::string const& fcr_service_type)
  {
    if(fcr_service_type == "_smb._tcp.")
      return "SAMBA";
    else if(fcr_service_type == "_ftp._tcp.")
      return "FTP";
    else if(fcr_service_type == "_htsp._tcp.")
      return "HTS";
    else if(fcr_service_type == "_daap._tcp.")
      return "iTunes Music Sharing";
    //fallback, just return the received type
    return fcr_service_type;
  }
  bool GetXBMCProtocol(std::string const& fcr_service_type, CStdString& fr_protocol)
  {
    if(fcr_service_type == "_smb._tcp.")
      fr_protocol = "smb";
    else if(fcr_service_type == "_ftp._tcp.")
      fr_protocol = "ftp";
    else if(fcr_service_type == "_htsp._tcp.")
      fr_protocol = "htsp";
    else if(fcr_service_type == "_daap._tcp.")
      fr_protocol = "daap";
    else
      return false;
    return true;
  }
}

bool CZeroconfDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  assert(strPath.substr(0, 11) == "zeroconf://");
  CStdString path = strPath.substr(11, strPath.length());
  URIUtils::RemoveSlashAtEnd(path);
  if(path.empty())
  {
    std::vector<CZeroconfBrowser::ZeroconfService> found_services = CZeroconfBrowser::GetInstance()->GetFoundServices();
    for(std::vector<CZeroconfBrowser::ZeroconfService>::iterator it = found_services.begin(); it != found_services.end(); ++it)
    {
      //only use discovered services we can connect to through directory
      CStdString tmp;
      if(GetXBMCProtocol(it->GetType(), tmp))
      {
        CFileItemPtr item(new CFileItem("", true));
        CURL url;
        url.SetProtocol("zeroconf");
        CStdString service_path = CZeroconfBrowser::ZeroconfService::toPath(*it);
        CURL::Encode(service_path);
        url.SetFileName(service_path);
        item->m_strPath = url.Get();

        //now do the formatting
        CStdString protocol = GetHumanReadableProtocol(it->GetType());
        item->SetLabel(it->GetName() + " (" + protocol  + ")");
        item->SetLabelPreformated(true);
        //just set the default folder icon
        item->FillInDefaultIcon();
        items.Add(item);
      }
    }
    return true;
  } else
  {
    //decode the path first
    CStdString decoded = path;
    CURL::Decode(decoded);
    try
    {
      CZeroconfBrowser::ZeroconfService zeroconf_service = CZeroconfBrowser::ZeroconfService::fromPath(decoded);

      if(!CZeroconfBrowser::GetInstance()->ResolveService(zeroconf_service))
      {
        CLog::Log(LOGINFO, "CZeroconfDirectory::GetDirectory service ( %s ) could not be resolved in time", zeroconf_service.GetName().c_str());
        return false;
      }
      else
      {
        assert(!zeroconf_service.GetIP().empty());
        CURL service;
        service.SetPort(zeroconf_service.GetPort());
        service.SetHostName(zeroconf_service.GetIP());
        //do protocol conversion (_smb._tcp -> smb)
        //ToDo: try automatic conversion -> remove leading '_' and '._tcp'?
        CStdString protocol;
        if(!GetXBMCProtocol(zeroconf_service.GetType(), protocol))
        {
          CLog::Log(LOGERROR, "CZeroconfDirectory::GetDirectory Unknown service type (%s), skipping; ", zeroconf_service.GetType().c_str());
          return false;
        }
        service.SetProtocol(protocol);
        return CDirectory::GetDirectory(service.Get(), items, "", true, true);
      }
    } catch (std::runtime_error& e) {
      CLog::Log(LOGERROR, "CZeroconfDirectory::GetDirectory failed getting directory: '%s'. Error: '%s'", decoded.c_str(), e.what());
      return false;
    }
  }
}
