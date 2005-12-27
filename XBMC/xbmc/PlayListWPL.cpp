
#include "stdafx.h"
#include "playlistwpl.h"
#include "util.h"


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
CPlayListWPL::CPlayListWPL(void)
{}

CPlayListWPL::~CPlayListWPL(void)
{}


bool CPlayListWPL::Load(const CStdString& strFileName)
{
  CStdString strBasePath;
  CUtil::GetParentPath(strFileName, strBasePath);

  Clear();

  CFile file;
  if (!file.Open(strFileName)) return false;
  int iLenght = (int)file.GetLength();
  auto_aptr<char> xmlData(new char[iLenght]);
  file.Read(xmlData.get(), iLenght);

  TiXmlDocument xmlDoc;
  if (xmlDoc.Parse(xmlData.get()) == NULL) return false;

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
      if (CUtil::IsRemote(strBasePath) && g_settings.m_vecPathSubstitutions.size() > 0)
        strFileName = CUtil::SubstitutePath(strFileName);
      CUtil::GetQualifiedFilename(strBasePath, strFileName);
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
