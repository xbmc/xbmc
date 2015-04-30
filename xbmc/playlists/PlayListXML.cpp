/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "PlayListXML.h"
#include "filesystem/File.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"

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


CPlayListXML::CPlayListXML(void)
{}

CPlayListXML::~CPlayListXML(void)
{}


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
    CLog::Log(LOGERROR, "Playlist %s has invalid format/malformed xml", strFileName.c_str());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();

  // If the stream does not contain "streams", still ok. Not an error.
  if ( !pRootElement || stricmp( pRootElement->Value(), "streams" ) )
  {
    CLog::Log(LOGERROR, "Playlist %s has no <streams> root", strFileName.c_str());
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
         newItem->m_iHasLock = 2;
         newItem->m_iLockMode = LOCK_MODE_NUMERIC;
       }

       Add(newItem);
    }
    else
       CLog::Log(LOGERROR, "Playlist entry %s in file %s has missing <url> tag", name.c_str(), strFileName.c_str());

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
    CLog::Log(LOGERROR, "Could not save WPL playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  std::string write;
  write += StringUtils::Format("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  write += StringUtils::Format("<streams>\n");
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    write += StringUtils::Format("  <stream>\n" );
    write += StringUtils::Format("    <url>%s</url>", item->GetPath().c_str() );
    write += StringUtils::Format("    <name>%s</name>", item->GetLabel().c_str() );

    if ( !item->GetProperty("language").empty() )
      write += StringUtils::Format("    <lang>%s</lang>", item->GetProperty("language").c_str() );

    if ( !item->GetProperty("category").empty() )
      write += StringUtils::Format("    <category>%s</category>", item->GetProperty("category").c_str() );

    if ( !item->GetProperty("remotechannel").empty() )
      write += StringUtils::Format("    <channel>%s</channel>", item->GetProperty("remotechannel").c_str() );

    if ( item->m_iHasLock > 0 )
      write += StringUtils::Format("    <lockpassword>%s<lockpassword>", item->m_strLockCode.c_str() );

    write += StringUtils::Format("  </stream>\n\n" );
  }

  write += StringUtils::Format("</streams>\n");
  file.Write(write.c_str(), write.size());
  file.Close();
}
