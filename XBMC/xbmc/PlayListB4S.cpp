
#include "stdafx.h"
#include "playlistb4s.h"
#include "util.h"


/* ------------------------ example b4s playlist file ---------------------------------
 <?xml version="1.0" encoding='UTF-8' standalone="yes"?>
 <WinampXML>
 <!-- Generated by: Nullsoft Winamp3 version 3.0d -->
  <playlist num_entries="2" label="Playlist 001">
   <entry Playstring="file:E:\Program Files\Winamp3\demo.mp3">
    <Name>demo</Name>
    <Length>5982</Length>
   </entry>
   <entry Playstring="file:E:\Program Files\Winamp3\demo.mp3">
    <Name>demo</Name>
    <Length>5982</Length>
   </entry>
  </playlist>
 </WinampXML>
------------------------ end of example b4s playlist file ---------------------------------*/
CPlayListB4S::CPlayListB4S(void)
{}

CPlayListB4S::~CPlayListB4S(void)
{}


bool CPlayListB4S::Load(const CStdString& strFileName)
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

  TiXmlElement* pPlayListElement = pRootElement->FirstChildElement("playlist");
  if (!pPlayListElement ) return false;
  m_strPlayListName = pPlayListElement->Attribute("label");

  TiXmlElement* pEntryElement = pPlayListElement->FirstChildElement("entry");

  if (!pEntryElement) return false;
  while (pEntryElement)
  {
    CStdString strFileName = pEntryElement->Attribute("Playstring");
    int iColon = strFileName.Find(":");
    if (iColon > 0)
    {
      iColon++;
      strFileName = strFileName.Right((int)strFileName.size() - iColon);
    }
    if (strFileName.size())
    {
      TiXmlNode* pNodeInfo = pEntryElement->FirstChild("Name");
      TiXmlNode* pNodeLength = pEntryElement->FirstChild("Length");
      long lDuration = 0;
      if (pNodeLength)
      {
        lDuration = atol(pNodeLength->FirstChild()->Value());
        lDuration;
      }
      if (pNodeInfo)
      {
        CStdString strInfo = pNodeInfo->FirstChild()->Value();
        if (CUtil::IsRemote(strBasePath) && g_settings.m_vecPathSubstitutions.size() > 0)
          strFileName = CUtil::SubstitutePath(strFileName);
        CUtil::GetQualifiedFilename(strBasePath, strFileName);
        CPlayListItem newItem(strInfo, strFileName, lDuration);
        Add(newItem);
      }
    }
    pEntryElement = pEntryElement->NextSiblingElement();
  }
  return true;
}

void CPlayListB4S::Save(const CStdString& strFileName) const
{
  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = strFileName;
  // force HD saved playlists into fatx compliance
  if (CUtil::IsHD(strPlaylist))
    CUtil::GetFatXQualifiedPath(strPlaylist);
  FILE *fd = fopen(strPlaylist.c_str(), "w+");
  if (!fd)
  {
    CLog::Log(LOGERROR, "Could not save B4S playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  fprintf(fd, "<?xml version=%c1.0%c encoding='UTF-8' standalone=%cyes%c?>\n", 34, 34, 34, 34);
  fprintf(fd, "<WinampXML>\n");
  fprintf(fd, "  <playlist num_entries=%c%i%c label=%c%s%c>\n", 34, m_vecItems.size(), 34, 34, m_strPlayListName.c_str(), 34);
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    const CPlayListItem& item = m_vecItems[i];
    fprintf(fd, "    <entry Playstring=%cfile:%s%c>\n", 34, item.GetFileName().c_str(), 34 );
    fprintf(fd, "      <Name>%s</Name>\n", item.GetDescription().c_str());
    fprintf(fd, "      <Length>%i</Length>\n", item.GetDuration());
  }
  fprintf(fd, "  </playlist>\n");
  fprintf(fd, "</WinampXML>\n");
  fclose(fd);
}
