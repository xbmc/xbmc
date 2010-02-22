/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "RSSDirectory.h"
#include "FileItem.h"
#include "Settings.h"
#include "Util.h"
#include "tinyXML/tinyxml.h"
#include "HTMLUtil.h"
#include "StringUtils.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "utils/log.h"
#include "URL.h"

using namespace XFILE;
using namespace std;
using namespace MUSIC_INFO;

CRSSDirectory::CRSSDirectory()
{
  SetCacheDirectory(DIR_CACHE_ONCE);
}

CRSSDirectory::~CRSSDirectory()
{
}

bool CRSSDirectory::ContainsFiles(const CStdString& strPath)
{
  CFileItemList items;
  if(!GetDirectory(strPath, items))
    return false;

  return items.Size() > 0;
}

static bool IsPathToMedia(const CStdString& strPath )
{
  CStdString extension;
  CUtil::GetExtension(strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  if (g_settings.m_videoExtensions.Find(extension) != -1)
    return true;

  if (g_settings.m_musicExtensions.Find(extension) != -1)
    return true;

  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  return false;
}

static bool IsPathToThumbnail(const CStdString& strPath )
{
  // Currently just check if this is an image, maybe we will add some
  // other checks later
  CStdString extension;
  CUtil::GetExtension(strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  return false;
}

static time_t ParseDate(const CStdString & strDate)
{
  struct tm pubDate = {0};
  // TODO: Handle time zone
  strptime(strDate.c_str(), "%a, %d %b %Y %H:%M:%S", &pubDate);
  // Check the difference between the time of last check and time of the item
  return mktime(&pubDate);
}
static void ParseItem(CFileItem* item, TiXmlElement* root);

static void ParseItemMRSS(CFileItem* item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  CStdString text = item_child->GetText();

  if(name == "content")
  {
    const char* url  = item_child->Attribute("url");
    const char* type = item_child->Attribute("type");
    const char* dur  = item_child->Attribute("duration");

    if (url && item->m_strPath == "" && IsPathToMedia(url))
      item->m_strPath = url;

    if (type)
    {
      CStdString strType(type);
      item->SetContentType(strType);
      if (url && item->m_strPath.IsEmpty() &&
        (strType.Left(6).Equals("video/") ||
         strType.Left(6).Equals("audio/")
        ))
        item->m_strPath = url;
    }

    if(dur)
      StringUtils::SecondsToTimeString(atoi(dur), vtag->m_strRuntime);

    ParseItem(item, item_child);
  }
  else if(name == "thumbnail")
  {
    if(item_child->GetText() && IsPathToThumbnail(item_child->GetText()))
      item->SetThumbnailImage(item_child->GetText());
    else
    {
      const char * url = item_child->Attribute("url");
      if(url && IsPathToThumbnail(url))
        item->SetThumbnailImage(url);
    }
  }
  else if (name == "title")
  {
    if(text.IsEmpty())
      return;

    vtag->m_strTitle = text;
  }
  else if(name == "description")
  {
    if(text.IsEmpty())
      return;

    CStdString description = text;
    if(CStdString(item_child->Attribute("type")) == "html")
      HTML::CHTMLUtil::RemoveTags(description);
    item->SetProperty("description", description);
  }
  else if(name == "category")
  {
    if(text.IsEmpty())
      return;

    CStdString scheme = item_child->Attribute("scheme");

    /* okey this is silly, boxee what did you think?? */
    if     (scheme == "urn:boxee:genre")
      vtag->m_strGenre = text;
    else if(scheme == "urn:boxee:title-type")
    {
      if     (text == "tv")
        item->SetProperty("boxee:istvshow", true);
      else if(text == "movie")
        item->SetProperty("boxee:ismovie", true);
    }
    else if(scheme == "urn:boxee:episode")
      vtag->m_iEpisode = atoi(text.c_str());
    else if(scheme == "urn:boxee:season")
      vtag->m_iSeason  = atoi(text.c_str());
    else if(scheme == "urn:boxee:show-title")
      vtag->m_strShowTitle = text.c_str();
    else if(scheme == "urn:boxee:view-count")
      vtag->m_playCount = atoi(text.c_str());
    else if(scheme == "urn:boxee:source")
      item->SetProperty("boxee:provider_source", text);
    else
      vtag->m_strGenre = text;
  }
  else if(name == "rating")
  {
    CStdString scheme = item_child->Attribute("scheme");
    if(scheme == "urn:user")
      vtag->m_fRating = (float)atof(text.c_str());
    else
      vtag->m_strMPAARating = text;
  }
  else if(name == "credit")
  {
    CStdString role = item_child->Attribute("role");
    if     (role == "director")
      vtag->m_strDirector += ", " + text;
    else if(role == "author"
         || role == "writer")
      vtag->m_strWritingCredits += ", " + text;
    else if(role == "actor")
    {
      SActorInfo actor;
      actor.strName = text;
      vtag->m_cast.push_back(actor);
    }
  }
  else if(name == "copyright")
    vtag->m_strStudio = text;
  else if(name == "keywords")
    item->SetProperty("keywords", text);

}

static void ParseItemItunes(CFileItem* item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  CStdString text = item_child->GetText();

  if(name == "image")
  {
    const char * url = item_child->Attribute("href");
    if(url)
      item->SetThumbnailImage(url);
    else
      item->SetThumbnailImage(text);
  }
  else if(name == "summary")
    vtag->m_strPlot = text;
  else if(name == "subtitle")
    vtag->m_strPlotOutline = text;
  else if(name == "author")
    vtag->m_strWritingCredits += ", " + text;
  else if(name == "duration")
    vtag->m_strRuntime = text;
  else if(name == "keywords")
    item->SetProperty("keywords", text);
}

static void ParseItemRSS(CFileItem* item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
{
  CStdString text = item_child->GetText();
  if (name == "title")
    item->m_strTitle = text;
  else if (name == "pubDate")
  {
    CDateTime pubDate(ParseDate(text));
    item->m_dateTime = pubDate;
  }
  else if (name == "link")
  {
    string strPrefix = text.substr(0, text.find_first_of(":"));
    if (strPrefix == "rss")
    {
      // If this is an rss item, we treat it as another level in the directory
      item->m_bIsFolder = true;
      item->m_strPath = text;
    }
    else if (item->m_strPath == "" && IsPathToMedia(text))
      item->m_strPath = text;
  }
  else if(name == "enclosure")
  {
    const char * url = item_child->Attribute("url");
    if (url && item->m_strPath.IsEmpty() && IsPathToMedia(url))
      item->m_strPath = url;
    const char * content_type = item_child->Attribute("type");
    if (content_type)
    {
      item->SetContentType(content_type);
      CStdString strContentType(content_type);
      if (url && item->m_strPath.IsEmpty() &&
        (strContentType.Left(6).Equals("video/") ||
         strContentType.Left(6).Equals("audio/")
        ))
        item->m_strPath = url;
    }
    const char * len = item_child->Attribute("length");
    if (len)
      item->m_dwSize = _atoi64(len);
  }
  else if(name == "description")
  {
    CStdString description = text;
    HTML::CHTMLUtil::RemoveTags(description);
    item->SetProperty("description", description);
  }
  else if(name == "guid")
  {
    if (item->m_strPath.IsEmpty() && IsPathToMedia(text))
      item->m_strPath = text;
  }
}

static void ParseItemVoddler(CFileItem* item, TiXmlElement* element, const CStdString& name, const CStdString& xmlns)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  CStdString text = element->GetText();

  if(name == "trailer")
  {
    vtag->m_strTrailer = text;

    CStdString type = element->Attribute("type");
    if(item->m_strPath.IsEmpty())
    {
      item->m_strPath = text;
      item->SetContentType(type);
    }
  }
  else if(name == "year")
    vtag->m_iYear = atoi(text);
  else if(name == "rating")
    vtag->m_fRating = (float)atof(text);
  else if(name == "tagline")
    vtag->m_strTagLine = text;
  else if(name == "posterwall")
  {
    const char* url = element->Attribute("url");
    if(url)
      item->SetProperty("fanart_image", url);
    else if(IsPathToThumbnail(text))
      item->SetProperty("fanart_image", text);
  }
}

static void ParseItemBoxee(CFileItem* item, TiXmlElement* element, const CStdString& name, const CStdString& xmlns)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  CStdString text = element->GetText();

  if     (name == "image")
    item->SetThumbnailImage(text);
  else if(name == "user_agent")
    item->SetProperty("boxee:user_agent", text);
  else if(name == "content_type")
    item->SetContentType(text);
  else if(name == "runtime")
    vtag->m_strRuntime = text;
  else if(name == "episode")
    vtag->m_iEpisode = atoi(text);
  else if(name == "season")
    vtag->m_iSeason = atoi(text);
  else if(name == "view-count")
    vtag->m_playCount = atoi(text);
  else if(name == "tv-show-title")
    vtag->m_strShowTitle = text;
  else if(name == "release-date")
    item->SetProperty("boxee:releasedate", text);
}

static void ParseItemZink(CFileItem* item, TiXmlElement* element, const CStdString& name, const CStdString& xmlns)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  CStdString text = element->GetText();
  if     (name == "episode")
    vtag->m_iEpisode = atoi(text);
  else if(name == "season")
    vtag->m_iSeason = atoi(text);
  else if(name == "views")
    vtag->m_playCount = atoi(text);
  else if(name == "airdate")
    vtag->m_strFirstAired = text;
  else if(name == "userrating")
    vtag->m_fRating = (float)atof(text.c_str());
  else if(name == "duration")
    StringUtils::SecondsToTimeString(atoi(text), vtag->m_strRuntime);
  else if(name == "durationstr")
    vtag->m_strRuntime = text;
}

static void ParseItem(CFileItem* item, TiXmlElement* root)
{
  for (TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    CStdString name = child->Value();
    CStdString xmlns;
    int pos = name.Find(':');
    if(pos >= 0)
    {
      xmlns = name.Left(pos);
      name.Delete(0, pos+1);
    }

    if      (xmlns == "media")
      ParseItemMRSS   (item, child, name, xmlns);
    else if (xmlns == "itunes")
      ParseItemItunes (item, child, name, xmlns);
    else if (xmlns == "voddler")
      ParseItemVoddler(item, child, name, xmlns);
    else if (xmlns == "boxee")
      ParseItemBoxee  (item, child, name, xmlns);
    else if (xmlns == "zn")
      ParseItemZink   (item, child, name, xmlns);
    else
      ParseItemRSS    (item, child, name, xmlns);
  }

  if(!item->m_strTitle.IsEmpty())
    item->SetLabel(item->m_strTitle);

  if(item->HasVideoInfoTag())
  {
    CVideoInfoTag* vtag = item->GetVideoInfoTag();
    // clean up ", " added during build
    vtag->m_strDirector.Delete(0, 2);
    vtag->m_strWritingCredits.Delete(0, 2);

    if(item->HasProperty("duration")    && vtag->m_strRuntime.IsEmpty())
      vtag->m_strRuntime = item->GetProperty("duration");

    if(item->HasProperty("description") && vtag->m_strPlot.IsEmpty())
      vtag->m_strPlot = item->GetProperty("description");

    if(vtag->m_strPlotOutline.IsEmpty() && !vtag->m_strPlot.IsEmpty())
    {
      int pos = vtag->m_strPlot.Find('\n');
      if(pos >= 0)
        vtag->m_strPlotOutline = vtag->m_strPlot.Left(pos);
      else
        vtag->m_strPlotOutline = vtag->m_strPlot;
    }

    if(!vtag->m_strRuntime.IsEmpty())
      item->SetLabel2(vtag->m_strRuntime);
  }
}

bool CRSSDirectory::GetDirectory(const CStdString& path, CFileItemList &items)
{
  CStdString strPath(path);
  CUtil::RemoveSlashAtEnd(strPath);

  /* check cache */
  if(m_path == strPath)
  {
    items = m_items;
    return true;
  }

  /* clear cache */
  m_items.Clear();
  m_path == "";

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR,"failed to load xml from <%s>. error: <%d>", strPath.c_str(), xmlDoc.ErrorId());
    return false;
  }
  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR,"error parsing xml doc from <%s>. error: <%d>", strPath.c_str(), xmlDoc.ErrorId());
    return false;
  }

  TiXmlElement* rssXmlNode = xmlDoc.RootElement();

  if (!rssXmlNode)
    return false;

  TiXmlHandle docHandle( &xmlDoc );
  TiXmlElement* channelXmlNode = docHandle.FirstChild( "rss" ).FirstChild( "channel" ).Element();
  if (channelXmlNode)
    ParseItem(&items, channelXmlNode);
  else
    return false;

  TiXmlElement* child = NULL;
  for (child = channelXmlNode->FirstChildElement("item"); child; child = child->NextSiblingElement())
  {
    // Create new item,
    CFileItemPtr item(new CFileItem());
    ParseItem(item.get(), child);

    item->SetProperty("isrss", "1");

    if (!item->m_strPath.IsEmpty())
      items.Add(item);
  }

  m_items = items;
  m_path  = strPath;

  return true;
}
