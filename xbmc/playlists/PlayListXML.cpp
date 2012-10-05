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

#include "PlayListXML.h"
#include "filesystem/File.h"
#include "Util.h"
#include "utils/RegExp.h"
#include "utils/log.h"
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


static inline CStdString GetString( const TiXmlElement* pRootElement, const char *tagName )
{
  CStdString strValue;
  if ( XMLUtils::GetString(pRootElement, tagName, strValue) )
    return strValue;

  return "";
}

bool CPlayListXML::Load( const CStdString& strFileName )
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
    CStdString url = GetString( pSet, "url" );
    CStdString name = GetString( pSet, "name" );
    CStdString category = GetString( pSet, "category" );
    CStdString lang = GetString( pSet, "lang" );
    CStdString channel = GetString( pSet, "channel" );
    CStdString lockpass = GetString( pSet, "lockpassword" );

    // If url is empty, it doesn't make any sense
    if ( !url.IsEmpty() )
    {
       // If the name is empty, use url
       if ( name.IsEmpty() )
         name = url;

       // Append language to the name, and also set as metadata
       if ( !lang.IsEmpty() )
         name += " [" + lang + "]";

       CStdString info = name;
       CFileItemPtr newItem( new CFileItem(info) );
       newItem->SetPath(url);

       // Set language as metadata
       if ( !lang.IsEmpty() )
         newItem->SetProperty("language", lang.c_str() );

       // Set category as metadata
       if ( !category.IsEmpty() )
         newItem->SetProperty("category", category.c_str() );

       // Set channel as extra info and as metadata
       if ( !channel.IsEmpty() )
       {
         newItem->SetProperty("remotechannel", channel.c_str() );
         newItem->SetExtraInfo( "Channel: " + channel );
       }

       if ( !lockpass.IsEmpty() )
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


void CPlayListXML::Save(const CStdString& strFileName) const
{
  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = CUtil::MakeLegalPath(strFileName);
  CFile file;
  if (!file.OpenForWrite(strPlaylist, true))
  {
    CLog::Log(LOGERROR, "Could not save WPL playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  CStdString write;
  write.AppendFormat("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  write.AppendFormat("<streams>\n");
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    write.AppendFormat("  <stream>\n" );
    write.AppendFormat("    <url>%s</url>", item->GetPath().c_str() );
    write.AppendFormat("    <name>%s</name>", item->GetLabel().c_str() );

    if ( !item->GetProperty("language").empty() )
      write.AppendFormat("    <lang>%s</lang>", item->GetProperty("language").c_str() );

    if ( !item->GetProperty("category").empty() )
      write.AppendFormat("    <category>%s</category>", item->GetProperty("category").c_str() );

    if ( !item->GetProperty("remotechannel").empty() )
      write.AppendFormat("    <channel>%s</channel>", item->GetProperty("remotechannel").c_str() );

    if ( item->m_iHasLock > 0 )
      write.AppendFormat("    <lockpassword>%s<lockpassword>", item->m_strLockCode.c_str() );

    write.AppendFormat("  </stream>\n\n" );
  }

  write.AppendFormat("</streams>\n");
  file.Write(write.c_str(), write.size());
  file.Close();
}
