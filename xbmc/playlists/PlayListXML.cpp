/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListXML.h"

#include "FileItem.h"
#include "Util.h"
#include "filesystem/File.h"
#include "media/MediaLockState.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

using namespace PLAYLIST;
using namespace XFILE;

/*
 Playlist example (must be stored with .pxml extension):

<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<streams>
  <!-- Stream definition. To have multiple streams, just add another <stream>...</stream> set.  !-->
  <stream>
    <!-- Stream URL !-->
    <url>mms://stream02.rambler.ru/eurosport</url>
    <!-- Stream name - used for display !-->
    <name>Евроспорт</name>
    <!-- Stream category - currently only LIVETV is supported !-->
    <category>LIVETV</category>
    <!-- Stream language code !-->
    <lang>RU</lang>
    <!-- Stream channel number - will be used to select stream by channel number !-->
    <channel>1</channel>
    <!-- Stream is password-protected !-->
    <lockpassword>123</lockpassword>
  </stream>

  <stream>
    <url>mms://video.rfn.ru/vesti_24</url>
    <name>Вести 24</name>
    <category>LIVETV</category>
    <lang>RU</lang>
    <channel>2</channel>
  </stream>

</streams>
*/


CPlayListXML::CPlayListXML(void) = default;

CPlayListXML::~CPlayListXML(void) = default;


static inline std::string GetString( const TiXmlElement* pRootElement, const char *tagName )
{
  std::string strValue;
  if ( XMLUtils::GetString(pRootElement, tagName, strValue) )
    return strValue;

  return "";
}

bool CPlayListXML::Load( const std::string& strFileName )
{
  CXBMCTinyXML xmlDoc;

  m_strPlayListName = URIUtils::GetFileName(strFileName);
  URIUtils::GetParentPath(strFileName, m_strBasePath);

  Clear();

  // Try to load the file as XML. If it does not load, return an error.
  if ( !xmlDoc.LoadFile( strFileName ) )
  {
    CLog::Log(LOGERROR, "Playlist {} has invalid format/malformed xml", strFileName);
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();

  // If the stream does not contain "streams", still ok. Not an error.
  if (!pRootElement || StringUtils::CompareNoCase(pRootElement->Value(), "streams"))
  {
    CLog::Log(LOGERROR, "Playlist {} has no <streams> root", strFileName);
    return false;
  }

  TiXmlElement* pSet = pRootElement->FirstChildElement("stream");

  while ( pSet )
  {
    // Get parameters
    std::string url = GetString( pSet, "url" );
    std::string name = GetString( pSet, "name" );
    std::string category = GetString( pSet, "category" );
    std::string lang = GetString( pSet, "lang" );
    std::string channel = GetString( pSet, "channel" );
    std::string lockpass = GetString( pSet, "lockpassword" );

    // If url is empty, it doesn't make any sense
    if ( !url.empty() )
    {
       // If the name is empty, use url
       if ( name.empty() )
         name = url;

       // Append language to the name, and also set as metadata
       if ( !lang.empty() )
         name += " [" + lang + "]";

       std::string info = name;
       CFileItemPtr newItem( new CFileItem(info) );
       newItem->SetPath(url);

       // Set language as metadata
       if ( !lang.empty() )
         newItem->SetProperty("language", lang.c_str() );

       // Set category as metadata
       if ( !category.empty() )
         newItem->SetProperty("category", category.c_str() );

       // Set channel as extra info and as metadata
       if ( !channel.empty() )
       {
         newItem->SetProperty("remotechannel", channel.c_str() );
         newItem->SetExtraInfo( "Channel: " + channel );
       }

       if ( !lockpass.empty() )
       {
         newItem->m_strLockCode = lockpass;
         newItem->m_iHasLock = LOCK_STATE_LOCKED;
         newItem->m_iLockMode = LOCK_MODE_NUMERIC;
       }

       Add(newItem);
    }
    else
      CLog::Log(LOGERROR, "Playlist entry {} in file {} has missing <url> tag", name, strFileName);

    pSet = pSet->NextSiblingElement("stream");
  }

  return true;
}


void CPlayListXML::Save(const std::string& strFileName) const
{
  if (!m_vecItems.size()) return ;
  std::string strPlaylist = CUtil::MakeLegalPath(strFileName);
  CFile file;
  if (!file.OpenForWrite(strPlaylist, true))
  {
    CLog::Log(LOGERROR, "Could not save WPL playlist: [{}]", strPlaylist);
    return ;
  }
  std::string write;
  write += StringUtils::Format("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  write += StringUtils::Format("<streams>\n");
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    write += StringUtils::Format("  <stream>\n" );
    write += StringUtils::Format("    <url>{}</url>", item->GetPath().c_str());
    write += StringUtils::Format("    <name>{}</name>", item->GetLabel());

    if ( !item->GetProperty("language").empty() )
      write += StringUtils::Format("    <lang>{}</lang>", item->GetProperty("language").asString());

    if ( !item->GetProperty("category").empty() )
      write += StringUtils::Format("    <category>{}</category>",
                                   item->GetProperty("category").asString());

    if ( !item->GetProperty("remotechannel").empty() )
      write += StringUtils::Format("    <channel>{}</channel>",
                                   item->GetProperty("remotechannel").asString());

    if (item->m_iHasLock > LOCK_STATE_NO_LOCK)
      write += StringUtils::Format("    <lockpassword>{}<lockpassword>", item->m_strLockCode);

    write += StringUtils::Format("  </stream>\n\n" );
  }

  write += StringUtils::Format("</streams>\n");
  file.Write(write.c_str(), write.size());
  file.Close();
}
