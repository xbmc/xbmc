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
#include "utils/log.h"
#include "URL.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace std;

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

  if (g_stSettings.m_videoExtensions.Find(extension) != -1)
    return true;

  if (g_stSettings.m_musicExtensions.Find(extension) != -1)
    return true;

  if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
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

  if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
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

static void ParseItemMRSS(CFileItemPtr& item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
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

    // Go over all child nodes of the media content and get the thumbnail
    TiXmlElement* media_content_child = item_child->FirstChildElement("media:thumbnail");
    if (media_content_child && media_content_child->Value() && strcmp(media_content_child->Value(), "media:thumbnail") == 0)
    {
      const char * url = media_content_child->Attribute("url");
      if (url && IsPathToThumbnail(url))
        item->SetThumbnailImage(url);
    }
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

    item->SetLabel(text);
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
    item->SetLabel2(description);

    vtag->m_strPlotOutline = description;
    vtag->m_strPlot        = description;
  }
  else if(name == "category")
  {
    if(text.IsEmpty())
      return;

    vtag->m_strGenre = text;
  }
  else if(name == "rating")
  {
    if(text.IsEmpty())
      return;
    if(atof(text.c_str()) > 0.0f && atof(text.c_str()) <= 10.0f)
      vtag->m_fRating = (float)atof(text.c_str());
    else
      vtag->m_strMPAARating = text;
  }
  else if(name == "credit")
  {
    CStdString role = item_child->Attribute("role");
    if(role == "director")
      vtag->m_strDirector += ", " + text;
    else if(role == "author")
      vtag->m_strWritingCredits += ", " + text;
    else if(role == "actor")
    {
      SActorInfo actor;
      actor.strName = text;
      vtag->m_cast.push_back(actor);
    }
  }

}

static void ParseItemItunes(CFileItemPtr& item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
{
  if(name == "image")
  {
    if(item_child->GetText() && IsPathToThumbnail(item_child->GetText()))
      item->SetThumbnailImage(item_child->GetText());
    else
    {
      const char * url = item_child->Attribute("href");
      if(url && IsPathToThumbnail(url))
        item->SetThumbnailImage(url);
    }
  }
  else if(name == "summary")
  {
    if(item_child->GetText())
    {
      CStdString description = item_child->GetText();
      item->SetProperty("description", description);
      item->SetLabel2(description);
    }
  }
  else if(name == "subtitle")
  {
    if(item_child->GetText())
    {
      CStdString description = item_child->GetText();
      item->SetProperty("description", description);
      item->SetLabel2(description);
    }
  }
  else if(name == "author")
  {
    if(item_child->GetText())
      item->SetProperty("author", item_child->GetText());
  }
  else if(name == "duration")
  {
    if(item_child->GetText())
      item->SetProperty("duration", item_child->GetText());
  }
  else if(name == "keywords")
  {
    if(item_child->GetText())
      item->SetProperty("keywords", item_child->GetText());
  }
}

static void ParseItemRSS(CFileItemPtr& item, TiXmlElement* item_child, const CStdString& name, const CStdString& xmlns)
{
  if (name == "title")
  {
    if (item_child->GetText())
      item->SetLabel(item_child->GetText());
  }
  else if (name == "pubDate")
  {
    CDateTime pubDate(ParseDate(item_child->GetText()));
    item->m_dateTime = pubDate;
  }
  else if (name == "link")
  {
    if (item_child->GetText())
    {
      string strLink = item_child->GetText();

      string strPrefix = strLink.substr(0, strLink.find_first_of(":"));
      if (strPrefix == "rss")
      {
        // If this is an rss item, we treat it as another level in the directory
        item->m_bIsFolder = true;
        item->m_strPath = strLink;
      }
      else if (item->m_strPath == "" && IsPathToMedia(strLink))
        item->m_strPath = strLink;
    }
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
    CStdString description = item_child->GetText();
    HTML::CHTMLUtil::RemoveTags(description);
    item->SetProperty("description", description);
    item->SetLabel2(description);
  }
  else if(name == "guid")
  {
    if (item->m_strPath.IsEmpty() && IsPathToMedia(item_child->Value()))
    {
      if(item_child->GetText())
        item->m_strPath = item_child->GetText();
    }
  }
}

static void ParseItemVoddler(CFileItemPtr& item, TiXmlElement* element, const CStdString& name, const CStdString& xmlns)
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


bool CRSSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
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

  CStdString strMediaThumbnail ;

  TiXmlHandle docHandle( &xmlDoc );
  TiXmlElement* channelXmlNode = docHandle.FirstChild( "rss" ).FirstChild( "channel" ).Element();
  if (channelXmlNode)
  {
    TiXmlElement* aNode = channelXmlNode->FirstChildElement("title");
    if (aNode && !aNode->NoChildren())
      items.SetProperty("rss:title", aNode->FirstChild()->Value());

    aNode = channelXmlNode->FirstChildElement("itunes:summary");
    if (aNode && !aNode->NoChildren())
      items.SetProperty("rss:description", aNode->FirstChild()->Value());

    if (!items.HasProperty("rss:description"))
    {
      aNode = channelXmlNode->FirstChildElement("description");
      if (aNode && !aNode->NoChildren())
        items.SetProperty("rss:description", aNode->FirstChild()->Value());
    }

    // Get channel thumbnail
    TiXmlHandle chanHandle( channelXmlNode );
    aNode = chanHandle.FirstChild("image").FirstChild("url").Element();
    if (aNode && !aNode->NoChildren())
      items.SetProperty("rss:image", aNode->FirstChild()->Value());

    if (!items.HasProperty("rss:image"))
    {
      aNode = chanHandle.FirstChild("itunes:image").Element();
      if (aNode && !aNode->NoChildren())
        items.SetProperty("rss:image", aNode->FirstChild()->Value());
    }
  }
  else
    return false;

  TiXmlElement* child = NULL;
  TiXmlElement* item_child = NULL;
  for (child = channelXmlNode->FirstChildElement("item"); child; child = child->NextSiblingElement())
  {
    // Create new item,
    CFileItemPtr item(new CFileItem());

    for (item_child = child->FirstChildElement(); item_child; item_child = item_child->NextSiblingElement())
    {
      CStdString name = item_child->Value();
      CStdString xmlns;
      int pos = name.Find(':');
      if(pos >= 0)
      {
        xmlns = name.Left(pos);
        name.Delete(0, pos+1);
      }

      if(xmlns == "media")
        ParseItemMRSS(item, item_child, name, xmlns);
      else if (xmlns == "itunes")
        ParseItemItunes(item, item_child, name, xmlns);
      else if (xmlns == "voddler")
        ParseItemVoddler(item, item_child, name, xmlns);
      else
        ParseItemRSS(item, item_child, name, xmlns);

    } // for item

    // clean up ", " added during build
    if(item->HasVideoInfoTag())
    {
      item->GetVideoInfoTag()->m_strDirector.Delete(0, 2);
      item->GetVideoInfoTag()->m_strWritingCredits.Delete(0, 2);
    }

    item->SetProperty("isrss", "1");
    item->SetProperty("chanthumb",strMediaThumbnail);

    if (!item->m_strPath.IsEmpty())
      items.Add(item);
  }

  m_items = items;
  m_path  = strPath;

  return true;
}
