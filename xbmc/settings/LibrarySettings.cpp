/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "LibrarySettings.h"
#include "LangInfo.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

CLibrarySettings::CLibrarySettings()
{
  Initialise();
}

CLibrarySettings::CLibrarySettings(TiXmlElement *pRootElement)
{
  Initialise();
  TiXmlElement *pElement = pRootElement->FirstChildElement("musiclibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", m_bMusicLibraryHideAllItems);
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iMusicLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "prioritiseapetags", m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "albumssortbyartistthenyear", m_bMusicLibraryAlbumsSortByArtistThenYear);
    XMLUtils::GetString(pElement, "albumformat", m_strMusicLibraryAlbumFormat);
    XMLUtils::GetString(pElement, "albumformatright", m_strMusicLibraryAlbumFormatRight);
    XMLUtils::GetString(pElement, "itemseparator", m_musicItemSeparator);
  }

  pElement = pRootElement->FirstChildElement("videolibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", m_bVideoLibraryHideAllItems);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iVideoLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "hiderecentlyaddeditems", m_bVideoLibraryHideRecentlyAddedItems);
    XMLUtils::GetBoolean(pElement, "hideemptyseries", m_bVideoLibraryHideEmptySeries);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", m_bVideoLibraryCleanOnUpdate);
    XMLUtils::GetString(pElement, "itemseparator", m_videoItemSeparator);
    XMLUtils::GetBoolean(pElement, "exportautothumbs", m_bVideoLibraryExportAutoThumbs);
    XMLUtils::GetBoolean(pElement, "importwatchedstate", m_bVideoLibraryImportWatchedState);
  }

  pElement = pRootElement->FirstChildElement("videoscanner");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "ignoreerrors", m_bVideoScannerIgnoreErrors);
  }

  XMLUtils::GetInt(pRootElement, "playlistretries", m_playlistRetries, -1, 5000);
  XMLUtils::GetInt(pRootElement, "playlisttimeout", m_playlistTimeout, 0, 5000);
  XMLUtils::GetInt(pRootElement, "thumbsize", m_thumbSize, 0, 1024);
  XMLUtils::GetInt(pRootElement, "fanartheight", m_fanartHeight, 0, 1080);
  XMLUtils::GetBoolean(pRootElement, "useddsfanart", m_useDDSFanart);
  XMLUtils::GetBoolean(pRootElement, "playlistasfolders", m_playlistAsFolders);
  XMLUtils::GetBoolean(pRootElement, "detectasudf", m_detectAsUdf);

  // music extensions
  TiXmlElement* pExts = pRootElement->FirstChildElement("musicextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_settings.m_musicExtensions);

  // video extensions
  pExts = pRootElement->FirstChildElement("videoextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_settings.m_videoExtensions);

  // stub extensions
  pExts = pRootElement->FirstChildElement("discstubextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_settings.m_discStubExtensions);

  m_vecTokens.clear();
  CLangInfo::LoadTokens(pRootElement->FirstChild("sorttokens"),m_vecTokens);

  // stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("moviestacking");
  if (pVideoStacking)
    GetCustomRegexps(pVideoStacking, m_videoStackRegExps);

  //tv stacking regexps
  TiXmlElement* pTVStacking = pRootElement->FirstChildElement("tvshowmatching");
  if (pTVStacking)
    GetCustomTVRegexps(pTVStacking, m_tvshowEnumRegExps);

  //tv multipart enumeration regexp
  XMLUtils::GetString(pRootElement, "tvmultipartmatching", m_tvshowMultiPartEnumRegExp);

  // music thumbs
  TiXmlElement* pThumbs = pRootElement->FirstChildElement("musicthumbs");
  if (pThumbs)
    GetCustomExtensions(pThumbs,m_musicThumbs);

  // dvd thumbs
  pThumbs = pRootElement->FirstChildElement("dvdthumbs");
  if (pThumbs)
    GetCustomExtensions(pThumbs,m_dvdThumbs);

  // movie fanarts
  TiXmlElement* pFanart = pRootElement->FirstChildElement("fanart");
  if (pFanart)
    GetCustomExtensions(pFanart,m_fanartImages);

  // music filename->tag filters
  TiXmlElement* filters = pRootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    TiXmlNode* filter = filters->FirstChild("filter");
    while (filter)
    {
      if (filter->FirstChild())
        m_musicTagsFromFileFilters.push_back(filter->FirstChild()->ValueStr());
      filter = filter->NextSibling("filter");
    }
  }

  TiXmlElement* pDatabase = pRootElement->FirstChildElement("videodatabase");
  if (pDatabase)
  {
    CLog::Log(LOGWARNING, "VIDEO database configuration is experimental.");
    XMLUtils::GetString(pDatabase, "type", m_databaseVideo.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseVideo.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseVideo.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseVideo.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseVideo.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseVideo.name);
  }

  pDatabase = pRootElement->FirstChildElement("musicdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseMusic.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseMusic.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseMusic.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseMusic.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseMusic.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseMusic.name);
  }

  XMLUtils::GetInt(pRootElement, "songinfoduration", m_songInfoDuration, 0, INT_MAX);
  XMLUtils::GetString(pRootElement, "cddbaddress", m_cddbAddress);
}

void CLibrarySettings::Clear()
{
  m_videoStackRegExps.clear();
}

bool CLibrarySettings::ShowPlaylistAsFolders()
{
  return m_playlistAsFolders;
}

bool CLibrarySettings::UseDDSFanArt()
{
  return m_useDDSFanart;
}

bool CLibrarySettings::MusicLibraryAllItemsOnBottom()
{
  return m_bMusicLibraryAllItemsOnBottom;
}

bool CLibrarySettings::MusicLibraryAlbumsSortByArtistThenYear()
{
  return m_bMusicLibraryAlbumsSortByArtistThenYear;
}

bool CLibrarySettings::MusicLibraryHideAllItems()
{
  return m_bMusicLibraryHideAllItems;
}

bool CLibrarySettings::VideoLibraryCleanOnUpdate()
{
  return m_bVideoLibraryCleanOnUpdate;
}

bool CLibrarySettings::VideoScannerIgnoreErrors()
{
  return m_bVideoScannerIgnoreErrors;
}

bool CLibrarySettings::PrioritiseAPEv2tags()
{
  return m_prioritiseAPEv2tags;
}

bool CLibrarySettings::VideoLibraryHideEmptySeries()
{
  return m_bVideoLibraryHideEmptySeries;
}

bool CLibrarySettings::VideoLibraryExportAutoThumbs()
{
  return m_bVideoLibraryExportAutoThumbs;
}

bool CLibrarySettings::VideoLibraryImportWatchedState()
{
  return m_bVideoLibraryImportWatchedState;
}

bool CLibrarySettings::VideoLibraryHideAllItems()
{
  return m_bVideoLibraryHideAllItems;
}

bool CLibrarySettings::VideoLibraryAllItemsOnBottom()
{
  return m_bVideoLibraryAllItemsOnBottom;
}

bool CLibrarySettings::VideoLibraryHideRecentlyAddedItems()
{
  return m_bVideoLibraryHideRecentlyAddedItems;
}

int CLibrarySettings::ThumbSize()
{
  return m_thumbSize;
}

int CLibrarySettings::FanartHeight()
{
  return m_fanartHeight;
}

int CLibrarySettings::PlaylistTimeout()
{
  return m_playlistTimeout;
}

int CLibrarySettings::PlaylistRetries()
{
  return m_playlistRetries;
}

int CLibrarySettings::VideoLibraryRecentlyAddedItems()
{
  return m_iVideoLibraryRecentlyAddedItems;
}

int CLibrarySettings::MusicLibraryRecentlyAddedItems()
{
  return m_iMusicLibraryRecentlyAddedItems;
}

int CLibrarySettings::SongInfoDuration()
{
  return m_songInfoDuration;
}

CStdString CLibrarySettings::MusicItemSeparator()
{
  return m_musicItemSeparator;
}

CStdString CLibrarySettings::VideoItemSeparator()
{
  return m_videoItemSeparator;
}

CStdString CLibrarySettings::FanartImages()
{
  return m_fanartImages;
}

CStdString CLibrarySettings::DVDThumbs()
{
  return m_dvdThumbs;
}

CStdString CLibrarySettings::MusicThumbs()
{
  return m_musicThumbs;
}

CStdString CLibrarySettings::MusicLibraryAlbumFormat()
{
  return m_strMusicLibraryAlbumFormat;
}

CStdString CLibrarySettings::MusicLibraryAlbumFormatRight()
{
  return m_strMusicLibraryAlbumFormatRight;
}

CStdString CLibrarySettings::TVShowMultiPartEnumRegExp()
{
  return m_tvshowMultiPartEnumRegExp;
}

CStdString CLibrarySettings::CDDBAddress()
{
  return m_cddbAddress;
}

CStdStringArray CLibrarySettings::VideoStackRegExps()
{
  return m_videoStackRegExps;
}

std::vector<CStdString> CLibrarySettings::VecTokens()
{
  return m_vecTokens;
}

std::vector<CStdString> CLibrarySettings::MusicTagsFromFileFilters()
{
  return m_musicTagsFromFileFilters;
}

SETTINGS_TVSHOWLIST CLibrarySettings::TVShowEnumRegExps()
{
  return m_tvshowEnumRegExps;
}

void CLibrarySettings::Initialise()
{
  m_bMusicLibraryHideAllItems = false;
  m_bMusicLibraryAllItemsOnBottom = false;
  m_bMusicLibraryAlbumsSortByArtistThenYear = false;
  m_iMusicLibraryRecentlyAddedItems = 25;
  m_strMusicLibraryAlbumFormat = "";
  m_strMusicLibraryAlbumFormatRight = "";
  m_prioritiseAPEv2tags = false;
  m_musicItemSeparator = " / ";
  m_videoItemSeparator = " / ";

  m_bVideoLibraryHideAllItems = false;
  m_bVideoLibraryAllItemsOnBottom = false;
  m_iVideoLibraryRecentlyAddedItems = 25;
  m_bVideoLibraryHideRecentlyAddedItems = false;
  m_bVideoLibraryHideEmptySeries = false;
  m_bVideoLibraryCleanOnUpdate = false;
  m_bVideoLibraryExportAutoThumbs = false;
  m_bVideoLibraryImportWatchedState = false;
  m_bVideoScannerIgnoreErrors = false;
  m_playlistAsFolders = true;
  m_detectAsUdf = false;
  m_playlistRetries = 100;
  m_playlistTimeout = 20; // 20 seconds timeout
  m_thumbSize = DEFAULT_THUMB_SIZE;
  m_fanartHeight = DEFAULT_FANART_HEIGHT;
  m_useDDSFanart = false;

  m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg|thumb.jpg|Thumb.jpg|thumb.JPG|Thumb.JPG";
  m_dvdThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG";
  m_fanartImages = "fanart.jpg|fanart.png";

  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck]|d)[ _.-]*[0-9]+)(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck]|d)[ _.-]*[a-d])(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ ._-]*[a-d])(.*?)(\\.[^.]+)$");

  // foo.s01.e01, foo.s01_e01, S01E02 foo, S01 - E02
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[Ss]([0-9]+)[][ ._-]*[Ee]([0-9]+)([^\\\\/]*)$"));
  // foo.ep01, foo.EP_01
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\._ -]()[Ee][Pp]_?([0-9]+)([^\\\\/]*)$"));
  // foo.yyyy.mm.dd.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true,"([0-9]{4})[\\.-]([0-9]{2})[\\.-]([0-9]{2})"));
  // foo.mm.dd.yyyy.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true,"([0-9]{2})[\\.-]([0-9]{2})[\\.-]([0-9]{4})"));
  // foo.1x09* or just /1x09*
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\\\/\\._ \\[\\(-]([0-9]+)x([0-9]+)([^\\\\/]*)$"));
  // foo.103*, 103 foo
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\\\/\\._ -]([0-9]+)([0-9][0-9])([\\._ -][^\\\\/]*)$"));
  // Part I, Pt.VI
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\/._ -]p(?:ar)?t[_. -]()([ivx]+)([._ -][^\\/]*)$"));

  m_tvshowMultiPartEnumRegExp = "^[-_EeXx]+([0-9]+)";
  m_cddbAddress = "freedb.freedb.org";
  m_songInfoDuration = 10;
}

void CLibrarySettings::GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && stricmp(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (stricmp(szAction, "append") == 0)
        iAction = 1; // append
      else if (stricmp(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        CStdString regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, regExp);
        else
          settings.push_back(regExp);
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}

void CLibrarySettings::GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions)
{
  CStdString extraExtensions;
  CSettings::GetString(pRootElement,"add",extraExtensions,"");
  if (extraExtensions != "")
    extensions += "|" + extraExtensions;
  CSettings::GetString(pRootElement,"remove",extraExtensions,"");
  if (extraExtensions != "")
  {
    CStdStringArray exts;
    StringUtils::SplitString(extraExtensions,"|",exts);
    for (unsigned int i=0;i<exts.size();++i)
    {
      int iPos = extensions.Find(exts[i]);
      if (iPos == -1)
        continue;
      extensions.erase(iPos,exts[i].size()+1);
    }
  }
}

void CLibrarySettings::GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings)
{
  int iAction = 0; // overwrite
  // for backward compatibility
  const char* szAppend = pRootElement->Attribute("append");
  if ((szAppend && stricmp(szAppend, "yes") == 0))
    iAction = 1;
  // action takes precedence if both attributes exist
  const char* szAction = pRootElement->Attribute("action");
  if (szAction)
  {
    iAction = 0; // overwrite
    if (stricmp(szAction, "append") == 0)
      iAction = 1; // append
    else if (stricmp(szAction, "prepend") == 0)
      iAction = 2; // prepend
  }
  if (iAction == 0)
    settings.clear();
  TiXmlNode* pRegExp = pRootElement->FirstChild("regexp");
  int i = 0;
  while (pRegExp)
  {
    if (pRegExp->FirstChild())
    {
      bool bByDate = false;
      if (pRegExp->ToElement())
      {
        CStdString byDate = pRegExp->ToElement()->Attribute("bydate");
        if(byDate && stricmp(byDate, "true") == 0)
        {
          bByDate = true;
        }
      }
      CStdString regExp = pRegExp->FirstChild()->Value();
      regExp.MakeLower();
      if (iAction == 2)
        settings.insert(settings.begin() + i++, 1, TVShowRegexp(bByDate,regExp));
      else
        settings.push_back(TVShowRegexp(bByDate,regExp));
    }
    pRegExp = pRegExp->NextSibling("regexp");
  }
}
