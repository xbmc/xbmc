/*
 *  Copyright (C) 2018 Tyler Szabo
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListXSPF.h"

#include "FileItem.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

namespace
{

constexpr char const* LOCATION_TAGNAME = "location";
constexpr char const* PLAYLIST_TAGNAME = "playlist";
constexpr char const* TITLE_TAGNAME = "title";
constexpr char const* TRACK_TAGNAME = "track";
constexpr char const* TRACKLIST_TAGNAME = "trackList";

std::string GetXMLText(const tinyxml2::XMLElement* pXmlElement)
{
  std::string result;
  if (pXmlElement)
  {
    const char* const innerText = pXmlElement->GetText();
    if (innerText)
      result = innerText;
  }
  return result;
}

}

namespace KODI::PLAYLIST
{

CPlayListXSPF::CPlayListXSPF(void) = default;

CPlayListXSPF::~CPlayListXSPF(void) = default;

bool CPlayListXSPF::Load(const std::string& strFileName)
{
  CXBMCTinyXML2 xmlDoc;

  if (!xmlDoc.LoadFile(strFileName))
  {
    CLog::Log(LOGERROR, "Error parsing XML file {} ({}): {}", strFileName, xmlDoc.ErrorLineNum(),
              xmlDoc.ErrorStr());
    return false;
  }

  auto* pPlaylist = xmlDoc.FirstChildElement(PLAYLIST_TAGNAME);
  if (!pPlaylist)
  {
    CLog::Log(LOGERROR, "Error parsing XML file {}: missing root element {}", strFileName,
              PLAYLIST_TAGNAME);
    return false;
  }

  auto* pTracklist = pPlaylist->FirstChildElement(TRACKLIST_TAGNAME);
  if (!pTracklist)
  {
    CLog::Log(LOGERROR, "Error parsing XML file {}: missing element {}", strFileName,
              TRACKLIST_TAGNAME);
    return false;
  }

  Clear();
  URIUtils::GetParentPath(strFileName, m_strBasePath);

  m_strPlayListName = GetXMLText(pPlaylist->FirstChildElement(TITLE_TAGNAME));

  auto* pCurTrack = pTracklist->FirstChildElement(TRACK_TAGNAME);
  while (pCurTrack)
  {
    std::string location = GetXMLText(pCurTrack->FirstChildElement(LOCATION_TAGNAME));
    if (!location.empty())
    {
      std::string label = GetXMLText(pCurTrack->FirstChildElement(TITLE_TAGNAME));

      CFileItemPtr newItem(new CFileItem(label));

      CURL uri(location);

      // at the time of writing CURL doesn't handle file:// URI scheme the way
      // it's presented in this format, parse to local path instead
      std::string localpath;
      if (StringUtils::StartsWith(location, "file:///"))
      {
#ifndef TARGET_WINDOWS
        // Linux absolute path must start with root
        localpath = "/";
#endif
        // Path starts after "file:///"
        localpath += CURL::Decode(location.substr(8));
      }
      else if (uri.GetProtocol().empty())
      {
        localpath = URIUtils::AppendSlash(m_strBasePath) + CURL::Decode(location);
      }

      if (!localpath.empty())
      {
        // We don't use URIUtils::CanonicalizePath because m_strBasePath may be a
        // protocol e.g. smb
#ifdef TARGET_WINDOWS
        StringUtils::Replace(localpath, "/", "\\");
#endif
        localpath = URIUtils::GetRealPath(localpath);

        newItem->SetPath(localpath);
      }
      else
      {
        newItem->SetURL(uri);
      }

      Add(newItem);
    }

    pCurTrack = pCurTrack->NextSiblingElement(TRACK_TAGNAME);
  }

  return true;
}

} // namespace KODI::PLAYLIST
