/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfDirectory.h"

#include "Directory.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "network/ZeroconfBrowser.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cassert>
#include <stdexcept>

using namespace XFILE;

CZeroconfDirectory::CZeroconfDirectory()
{
  CZeroconfBrowser::GetInstance()->Start();
}

CZeroconfDirectory::~CZeroconfDirectory() = default;

namespace
{
  std::string GetHumanReadableProtocol(std::string const& fcr_service_type)
  {
    if(fcr_service_type == "_smb._tcp.")
      return "SAMBA";
    else if(fcr_service_type == "_ftp._tcp.")
      return "FTP";
    else if(fcr_service_type == "_webdav._tcp.")
      return "WebDAV";
    else if(fcr_service_type == "_nfs._tcp.")
      return "NFS";
    else if(fcr_service_type == "_sftp-ssh._tcp.")
      return "SFTP";
    //fallback, just return the received type
    return fcr_service_type;
  }
  bool GetXBMCProtocol(std::string const& fcr_service_type, std::string& fr_protocol)
  {
    if(fcr_service_type == "_smb._tcp.")
      fr_protocol = "smb";
    else if(fcr_service_type == "_ftp._tcp.")
      fr_protocol = "ftp";
    else if(fcr_service_type == "_webdav._tcp.")
      fr_protocol = "dav";
    else if(fcr_service_type == "_nfs._tcp.")
      fr_protocol = "nfs";
    else if(fcr_service_type == "_sftp-ssh._tcp.")
      fr_protocol = "sftp";
    else
      return false;
    return true;
  }
}

bool GetDirectoryFromTxtRecords(const CZeroconfBrowser::ZeroconfService& zeroconf_service, CURL& url, CFileItemList &items)
{
  bool ret = false;

  //get the txt-records from this service
  CZeroconfBrowser::ZeroconfService::tTxtRecordMap txtRecords=zeroconf_service.GetTxtRecords();

  //if we have some records
  if(!txtRecords.empty())
  {
    std::string path;
    std::string username;
    std::string password;

    //search for a path key entry
    CZeroconfBrowser::ZeroconfService::tTxtRecordMap::iterator it = txtRecords.find(TXT_RECORD_PATH_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      //from now on we treat the value as a path - everything else would mean
      //a misconfigured zeroconf server.
      path=it->second;
    }

    //search for a username key entry
    it = txtRecords.find(TXT_RECORD_USERNAME_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      username=it->second;
      url.SetUserName(username);
    }

    //search for a password key entry
    it = txtRecords.find(TXT_RECORD_PASSWORD_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      password=it->second;
      url.SetPassword(password);
    }

    //if we got a path - add a item - else at least we maybe have set username and password to theurl
    if( !path.empty())
    {
      CFileItemPtr item(new CFileItem("", true));
      std::string urlStr(url.Get());
      //if path has a leading slash (sure it should have one)
      if( path.at(0) == '/' )
      {
        URIUtils::RemoveSlashAtEnd(urlStr);//we don't need the slash at and of url then
      }
      else//path doesn't start with slash -
      {//this is some kind of misconfiguration - we fix it by adding a slash to the url
        URIUtils::AddSlashAtEnd(urlStr);
      }

      //add slash at end of path since it has to be a folder
      URIUtils::AddSlashAtEnd(path);
      //this is the full path including remote stuff (e.x. nfs://ip/path
      item->SetPath(urlStr + path);
      //remove the slash at the end of the path or GetFileName will not give the last dir
      URIUtils::RemoveSlashAtEnd(path);
      //set the label to the last directory in path
      if( !URIUtils::GetFileName(path).empty() )
        item->SetLabel(URIUtils::GetFileName(path));
      else
        item->SetLabel("/");

      item->SetLabelPreformatted(true);
      //just set the default folder icon
      item->FillInDefaultIcon();
      item->m_bIsShareOrDrive=true;
      items.Add(item);
      ret = true;
    }
  }
  return ret;
}

bool CZeroconfDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  assert(url.IsProtocol("zeroconf"));
  std::string strPath = url.Get();
  std::string path = strPath.substr(11, strPath.length());
  URIUtils::RemoveSlashAtEnd(path);
  if(path.empty())
  {
    std::vector<CZeroconfBrowser::ZeroconfService> found_services = CZeroconfBrowser::GetInstance()->GetFoundServices();
    for (auto& it : found_services)
    {
      //only use discovered services we can connect to through directory
      std::string tmp;
      if (GetXBMCProtocol(it.GetType(), tmp))
      {
        CFileItemPtr item(new CFileItem("", true));
        CURL url;
        url.SetProtocol("zeroconf");
        std::string service_path(CURL::Encode(CZeroconfBrowser::ZeroconfService::toPath(it)));
        url.SetFileName(service_path);
        item->SetPath(url.Get());

        //now do the formatting
        std::string protocol = GetHumanReadableProtocol(it.GetType());
        item->SetLabel(it.GetName() + " (" + protocol + ")");
        item->SetLabelPreformatted(true);
        //just set the default folder icon
        item->FillInDefaultIcon();
        items.Add(item);
      }
    }
    return true;
  }
  else
  {
    //decode the path first
    std::string decoded(CURL::Decode(path));
    try
    {
      CZeroconfBrowser::ZeroconfService zeroconf_service = CZeroconfBrowser::ZeroconfService::fromPath(decoded);

      if(!CZeroconfBrowser::GetInstance()->ResolveService(zeroconf_service))
      {
        CLog::Log(LOGINFO,
                  "CZeroconfDirectory::GetDirectory service ( {} ) could not be resolved in time",
                  zeroconf_service.GetName());
        return false;
      }
      else
      {
        assert(!zeroconf_service.GetIP().empty());
        CURL service;
        service.SetPort(zeroconf_service.GetPort());
        service.SetHostName(zeroconf_service.GetIP());
        //do protocol conversion (_smb._tcp -> smb)
        //! @todo try automatic conversion -> remove leading '_' and '._tcp'?
        std::string protocol;
        if(!GetXBMCProtocol(zeroconf_service.GetType(), protocol))
        {
          CLog::Log(LOGERROR,
                    "CZeroconfDirectory::GetDirectory Unknown service type ({}), skipping; ",
                    zeroconf_service.GetType());
          return false;
        }

        service.SetProtocol(protocol);

        //first try to show the txt-record defined path if any
        if(GetDirectoryFromTxtRecords(zeroconf_service, service, items))
        {
          return true;
        }
        else//no txt record path - so let the CDirectory handler show the folders
        {
          return CDirectory::GetDirectory(service.Get(), items, "", DIR_FLAG_ALLOW_PROMPT);
        }
      }
    } catch (std::runtime_error& e) {
      CLog::Log(LOGERROR,
                "CZeroconfDirectory::GetDirectory failed getting directory: '{}'. Error: '{}'",
                decoded, e.what());
      return false;
    }
  }
}
