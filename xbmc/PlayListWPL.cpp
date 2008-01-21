/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "PlayListWPL.h"
#include "Util.h"

using namespace XFILE;
using namespace PLAYLIST;

/* ------------------------ example wpl playlist file ---------------------------------
  <?wpl version="1.0"?>
  <smil>
      <head>
          <meta name="Generator" content="Microsoft Windows Media Player -- 10.0.0.3646"/>
          <author/>
          <title>Playlist</title>
      </head>
      <body>
          <seq>
              <media src="E:\MP3\Track1.mp3"/>
              <media src="E:\MP3\Track2.mp3"/>
              <media src="E:\MP3\track3.mp3"/>
          </seq>
      </body>
  </smil>
------------------------ end of example wpl playlist file ---------------------------------*/
//Note: File is utf-8 encoded by default

CPlayListWPL::CPlayListWPL(void)
{}

CPlayListWPL::~CPlayListWPL(void)
{}


bool CPlayListWPL::LoadData(std::istream& stream)
{
  TiXmlDocument xmlDoc;

  stream >> xmlDoc;
  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "Unable to parse B4S info Error: %s", xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement ) return false;

  TiXmlElement* pHeadElement = pRootElement->FirstChildElement("head");
  if (pHeadElement )
  {
    TiXmlElement* pTitelElement = pHeadElement->FirstChildElement("title");
    if (pTitelElement )
      m_strPlayListName = pTitelElement->Value();
  }

  TiXmlElement* pBodyElement = pRootElement->FirstChildElement("body");
  if (!pBodyElement ) return false;

  TiXmlElement* pSeqElement = pBodyElement->FirstChildElement("seq");
  if (!pSeqElement ) return false;

  TiXmlElement* pMediaElement = pSeqElement->FirstChildElement("media");

  if (!pMediaElement) return false;
  while (pMediaElement)
  {
    CStdString strFileName = pMediaElement->Attribute("src");
    if (strFileName.size())
    {
      if (CUtil::IsRemote(m_strBasePath) && g_advancedSettings.m_pathSubstitutions.size() > 0)
        strFileName = CUtil::SubstitutePath(strFileName);
      CUtil::GetQualifiedFilename(m_strBasePath, strFileName);
      CStdString strDescription = CUtil::GetFileName(strFileName);
      CPlayListItem newItem(strDescription, strFileName);
      Add(newItem);
    }
    pMediaElement = pMediaElement->NextSiblingElement();
  }
  return true;
}

void CPlayListWPL::Save(const CStdString& strFileName) const
{
  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = strFileName;
  // force HD saved playlists into fatx compliance
  if (CUtil::IsHD(strPlaylist))
    CUtil::GetFatXQualifiedPath(strPlaylist);
  FILE *fd = fopen(strPlaylist.c_str(), "w+");
  if (!fd)
  {
    CLog::Log(LOGERROR, "Could not save WPL playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  fprintf(fd, "<?wpl version=%c1.0%c>\n", 34, 34);
  fprintf(fd, "<smil>\n");
  fprintf(fd, "    <head>\n");
  fprintf(fd, "        <meta name=%cGenerator%c content=%cMicrosoft Windows Media Player -- 10.0.0.3646%c/>\n", 34, 34, 34, 34);
  fprintf(fd, "        <author/>\n");
  fprintf(fd, "        <title>%s</title>\n", m_strPlayListName.c_str());
  fprintf(fd, "    </head>\n");
  fprintf(fd, "    <body>\n");
  fprintf(fd, "        <seq>\n");
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    const CPlayListItem& item = m_vecItems[i];
    fprintf(fd, "            <media src=%c%s%c/>", 34, item.GetFileName().c_str(), 34);
  }
  fprintf(fd, "        </seq>\n");
  fprintf(fd, "    </body>\n");
  fprintf(fd, "</smil>\n");
  fclose(fd);
}
