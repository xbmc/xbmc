/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RSSDirectory.h"

#include "CurlFile.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/HTMLUtil.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <climits>
#include <mutex>
#include <utility>

using namespace XFILE;
using namespace MUSIC_INFO;

namespace {

  struct SResource
  {
    std::string tag;
    std::string path;
    std::string mime;
    std::string lang;
    int        width = 0;
    int        height = 0;
    int        bitrate = 0;
    int        duration = 0;
    int64_t    size = 0;
  };

  typedef std::vector<SResource> SResources;

}

std::map<std::string,CDateTime> CRSSDirectory::m_cache;
CCriticalSection CRSSDirectory::m_section;

CRSSDirectory::CRSSDirectory() = default;

CRSSDirectory::~CRSSDirectory() = default;

bool CRSSDirectory::ContainsFiles(const CURL& url)
{
  CFileItemList items;
  if(!GetDirectory(url, items))
    return false;

  return items.Size() > 0;
}

static bool IsPathToMedia(const std::string& strPath )
{
  return URIUtils::HasExtension(strPath,
                                CServiceBroker::GetFileExtensionProvider().GetVideoExtensions() + '|' +
                                CServiceBroker::GetFileExtensionProvider().GetMusicExtensions() + '|' +
                                CServiceBroker::GetFileExtensionProvider().GetPictureExtensions());
}

static bool IsPathToThumbnail(const std::string& strPath )
{
  // Currently just check if this is an image, maybe we will add some
  // other checks later
  return URIUtils::HasExtension(strPath,
                                CServiceBroker::GetFileExtensionProvider().GetPictureExtensions());
}

static time_t ParseDate(const std::string & strDate)
{
  struct tm pubDate = {};
  //! @todo Handle time zone
  strptime(strDate.c_str(), "%a, %d %b %Y %H:%M:%S", &pubDate);
  // Check the difference between the time of last check and time of the item
  return mktime(&pubDate);
}
static void ParseItem(CFileItem* item, SResources& resources, TiXmlElement* root, const std::string& path);

static std::string GetValue(TiXmlElement *element)
{
  if (element && !element->NoChildren())
    return element->FirstChild()->ValueStr();
  return "";
}

static void ParseItemMRSS(CFileItem* item, SResources& resources, TiXmlElement* item_child, const std::string& name, const std::string& xmlns, const std::string& path)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  std::string text = GetValue(item_child);

  if(name == "content")
  {
    SResource res;
    res.tag = "media:content";
    res.mime    = XMLUtils::GetAttribute(item_child, "type");
    res.path    = XMLUtils::GetAttribute(item_child, "url");
    item_child->Attribute("width", &res.width);
    item_child->Attribute("height", &res.height);
    item_child->Attribute("bitrate", &res.bitrate);
    item_child->Attribute("duration", &res.duration);
    if(item_child->Attribute("fileSize"))
      res.size = std::atoll(item_child->Attribute("fileSize"));

    resources.push_back(res);
    ParseItem(item, resources, item_child, path);
  }
  else if(name == "group")
  {
    ParseItem(item, resources, item_child, path);
  }
  else if(name == "thumbnail")
  {
    if(!item_child->NoChildren() && IsPathToThumbnail(item_child->FirstChild()->ValueStr()))
      item->SetArt("thumb", item_child->FirstChild()->ValueStr());
    else
    {
      const char * url = item_child->Attribute("url");
      if(url && IsPathToThumbnail(url))
        item->SetArt("thumb", url);
    }
  }
  else if (name == "title")
  {
    if(text.empty())
      return;

    if(text.length() > item->m_strTitle.length())
      item->m_strTitle = text;
  }
  else if(name == "description")
  {
    if(text.empty())
      return;

    std::string description = text;
    if(XMLUtils::GetAttribute(item_child, "type") == "html")
      HTML::CHTMLUtil::RemoveTags(description);
    item->SetProperty("description", description);
  }
  else if(name == "category")
  {
    if(text.empty())
      return;

    std::string scheme = XMLUtils::GetAttribute(item_child, "scheme");

    /* okey this is silly, boxee what did you think?? */
    if     (scheme == "urn:boxee:genre")
      vtag->m_genre.push_back(text);
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
      vtag->m_strShowTitle = text;
    else if(scheme == "urn:boxee:view-count")
      vtag->SetPlayCount(atoi(text.c_str()));
    else if(scheme == "urn:boxee:source")
      item->SetProperty("boxee:provider_source", text);
    else
      vtag->m_genre = StringUtils::Split(text, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
  else if(name == "rating")
  {
    std::string scheme = XMLUtils::GetAttribute(item_child, "scheme");
    if(scheme == "urn:user")
      vtag->SetRating((float)atof(text.c_str()));
    else
      vtag->m_strMPAARating = text;
  }
  else if(name == "credit")
  {
    std::string role = XMLUtils::GetAttribute(item_child, "role");
    if     (role == "director")
      vtag->m_director.push_back(text);
    else if(role == "author"
         || role == "writer")
      vtag->m_writingCredits.push_back(text);
    else if(role == "actor")
    {
      SActorInfo actor;
      actor.strName = text;
      vtag->m_cast.push_back(actor);
    }
  }
  else if(name == "copyright")
    vtag->m_studio = StringUtils::Split(text, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  else if(name == "keywords")
    item->SetProperty("keywords", text);

}

static void ParseItemItunes(CFileItem* item, SResources& resources, TiXmlElement* item_child, const std::string& name, const std::string& xmlns, const std::string& path)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  std::string text = GetValue(item_child);

  if(name == "image")
  {
    const char * url = item_child->Attribute("href");
    if(url)
      item->SetArt("thumb", url);
    else
      item->SetArt("thumb", text);
  }
  else if(name == "summary")
    vtag->m_strPlot = text;
  else if(name == "subtitle")
    vtag->m_strPlotOutline = text;
  else if(name == "author")
    vtag->m_writingCredits.push_back(text);
  else if(name == "duration")
    vtag->SetDuration(StringUtils::TimeStringToSeconds(text));
  else if(name == "keywords")
    item->SetProperty("keywords", text);
}

static void ParseItemRSS(CFileItem* item, SResources& resources, TiXmlElement* item_child, const std::string& name, const std::string& xmlns, const std::string& path)
{
  std::string text = GetValue(item_child);
  if (name == "title")
  {
    if(text.length() > item->m_strTitle.length())
      item->m_strTitle = text;
  }
  else if (name == "pubDate")
  {
    CDateTime pubDate(ParseDate(text));
    item->m_dateTime = pubDate;
  }
  else if (name == "link")
  {
    SResource res;
    res.tag  = "rss:link";
    res.path = text;
    resources.push_back(res);
  }
  else if(name == "enclosure")
  {
    const char * len  = item_child->Attribute("length");

    SResource res;
    res.tag = "rss:enclosure";
    res.path = XMLUtils::GetAttribute(item_child, "url");
    res.mime = XMLUtils::GetAttribute(item_child, "type");
    if(len)
      res.size = std::atoll(len);

    resources.push_back(res);
  }
  else if(name == "description")
  {
    std::string description = text;
    HTML::CHTMLUtil::RemoveTags(description);
    item->SetProperty("description", description);
  }
  else if(name == "guid")
  {
    if(IsPathToMedia(text))
    {
      SResource res;
      res.tag  = "rss:guid";
      res.path = text;
      resources.push_back(res);
    }
  }
}

static void ParseItemVoddler(CFileItem* item, SResources& resources, TiXmlElement* element, const std::string& name, const std::string& xmlns, const std::string& path)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  std::string text = GetValue(element);

  if(name == "trailer")
  {
    vtag->m_strTrailer = text;

    SResource res;
    res.tag  = "voddler:trailer";
    res.mime = XMLUtils::GetAttribute(element, "type");
    res.path = text;
    resources.push_back(res);
  }
  else if(name == "year")
    vtag->SetYear(atoi(text.c_str()));
  else if(name == "rating")
    vtag->SetRating((float)atof(text.c_str()));
  else if(name == "tagline")
    vtag->m_strTagLine = text;
  else if(name == "posterwall")
  {
    const char* url = element->Attribute("url");
    if(url)
      item->SetArt("fanart", url);
    else if(IsPathToThumbnail(text))
      item->SetArt("fanart", text);
  }
}

static void ParseItemBoxee(CFileItem* item, SResources& resources, TiXmlElement* element, const std::string& name, const std::string& xmlns, const std::string& path)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  std::string text = GetValue(element);

  if     (name == "image")
    item->SetArt("thumb", text);
  else if(name == "user_agent")
    item->SetProperty("boxee:user_agent", text);
  else if(name == "content_type")
    item->SetMimeType(text);
  else if(name == "runtime")
    vtag->SetDuration(StringUtils::TimeStringToSeconds(text));
  else if(name == "episode")
    vtag->m_iEpisode = atoi(text.c_str());
  else if(name == "season")
    vtag->m_iSeason = atoi(text.c_str());
  else if(name == "view-count")
    vtag->SetPlayCount(atoi(text.c_str()));
  else if(name == "tv-show-title")
    vtag->m_strShowTitle = text;
  else if(name == "release-date")
    item->SetProperty("boxee:releasedate", text);
}

static void ParseItemZink(CFileItem* item, SResources& resources, TiXmlElement* element, const std::string& name, const std::string& xmlns, const std::string& path)
{
  CVideoInfoTag* vtag = item->GetVideoInfoTag();
  std::string text = GetValue(element);
  if     (name == "episode")
    vtag->m_iEpisode = atoi(text.c_str());
  else if(name == "season")
    vtag->m_iSeason = atoi(text.c_str());
  else if(name == "views")
    vtag->SetPlayCount(atoi(text.c_str()));
  else if(name == "airdate")
    vtag->m_firstAired.SetFromDateString(text);
  else if(name == "userrating")
    vtag->SetRating((float)atof(text.c_str()));
  else if(name == "duration")
    vtag->SetDuration(atoi(text.c_str()));
  else if(name == "durationstr")
    vtag->SetDuration(StringUtils::TimeStringToSeconds(text));
}

static void ParseItemSVT(CFileItem* item, SResources& resources, TiXmlElement* element, const std::string& name, const std::string& xmlns, const std::string& path)
{
  std::string text = GetValue(element);
  if     (name == "xmllink")
  {
    SResource res;
    res.tag  = "svtplay:xmllink";
    res.path = text;
    res.mime = "application/rss+xml";
    resources.push_back(res);
  }
  else if (name == "broadcasts")
  {
    CURL url(path);
    if(StringUtils::StartsWith(url.GetFileName(), "v1/"))
    {
      SResource res;
      res.tag  = "svtplay:broadcasts";
      res.path = url.GetWithoutFilename() + "v1/video/list/" + text;
      res.mime = "application/rss+xml";
      resources.push_back(res);
    }
  }
}

static void ParseItem(CFileItem* item, SResources& resources, TiXmlElement* root, const std::string& path)
{
  for (TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    std::string name = child->Value();
    std::string xmlns;
    size_t pos = name.find(':');
    if(pos != std::string::npos)
    {
      xmlns = name.substr(0, pos);
      name.erase(0, pos+1);
    }

    if      (xmlns == "media")
      ParseItemMRSS   (item, resources, child, name, xmlns, path);
    else if (xmlns == "itunes")
      ParseItemItunes (item, resources, child, name, xmlns, path);
    else if (xmlns == "voddler")
      ParseItemVoddler(item, resources, child, name, xmlns, path);
    else if (xmlns == "boxee")
      ParseItemBoxee  (item, resources, child, name, xmlns, path);
    else if (xmlns == "zn")
      ParseItemZink   (item, resources, child, name, xmlns, path);
    else if (xmlns == "svtplay")
      ParseItemSVT    (item, resources, child, name, xmlns, path);
    else
      ParseItemRSS    (item, resources, child, name, xmlns, path);
  }
}

static bool FindMime(const SResources& resources, const std::string& mime)
{
  for (const auto& it : resources)
  {
    if (StringUtils::StartsWithNoCase(it.mime, mime))
      return true;
  }
  return false;
}

static void ParseItem(CFileItem* item, TiXmlElement* root, const std::string& path)
{
  SResources resources;
  ParseItem(item, resources, root, path);

  const char* prio[] = { "media:content", "voddler:trailer", "rss:enclosure", "svtplay:broadcasts", "svtplay:xmllink", "rss:link", "rss:guid", NULL };

  std::string mime;
  if     (FindMime(resources, "video/"))
    mime = "video/";
  else if(FindMime(resources, "audio/"))
    mime = "audio/";
  else if(FindMime(resources, "application/rss"))
    mime = "application/rss";
  else if(FindMime(resources, "image/"))
    mime = "image/";

  int maxrate = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_NETWORK_BANDWIDTH);
  if(maxrate == 0)
    maxrate = INT_MAX;

  SResources::iterator best = resources.end();
  for(const char** type = prio; *type && best == resources.end(); type++)
  {
    for (SResources::iterator it = resources.begin(); it != resources.end(); ++it)
    {
      if(!StringUtils::StartsWith(it->mime, mime))
        continue;

      if(it->tag == *type)
      {
        if(best == resources.end())
        {
          best = it;
          continue;
        }

        if(it->bitrate == best->bitrate)
        {
          if(it->width*it->height > best->width*best->height)
            best = it;
          continue;
        }

        if(it->bitrate > maxrate)
        {
          if(it->bitrate < best->bitrate)
            best = it;
          continue;
        }

        if(it->bitrate > best->bitrate)
        {
          best = it;
          continue;
        }
      }
    }
  }

  if(best != resources.end())
  {
    item->SetMimeType(best->mime);
    item->SetPath(best->path);
    item->m_dwSize  = best->size;

    if(best->duration)
      item->SetProperty("duration", StringUtils::SecondsToTimeString(best->duration));

    /* handling of mimetypes fo directories are sub optimal at best */
    if(best->mime == "application/rss+xml" && StringUtils::StartsWithNoCase(item->GetPath(), "http://"))
      item->SetPath("rss://" + item->GetPath().substr(7));

    if(best->mime == "application/rss+xml" && StringUtils::StartsWithNoCase(item->GetPath(), "https://"))
      item->SetPath("rsss://" + item->GetPath().substr(8));

    if(StringUtils::StartsWithNoCase(item->GetPath(), "rss://")
      || StringUtils::StartsWithNoCase(item->GetPath(), "rsss://"))
      item->m_bIsFolder = true;
    else
      item->m_bIsFolder = false;
  }

  if(!item->m_strTitle.empty())
    item->SetLabel(item->m_strTitle);

  if(item->HasVideoInfoTag())
  {
    CVideoInfoTag* vtag = item->GetVideoInfoTag();

    if(item->HasProperty("duration")    && !vtag->GetDuration())
      vtag->SetDuration(StringUtils::TimeStringToSeconds(item->GetProperty("duration").asString()));

    if(item->HasProperty("description") && vtag->m_strPlot.empty())
      vtag->m_strPlot = item->GetProperty("description").asString();

    if(vtag->m_strPlotOutline.empty() && !vtag->m_strPlot.empty())
    {
      size_t pos = vtag->m_strPlot.find('\n');
      if(pos != std::string::npos)
        vtag->m_strPlotOutline = vtag->m_strPlot.substr(0, pos);
      else
        vtag->m_strPlotOutline = vtag->m_strPlot;
    }

    if(!vtag->GetDuration())
      item->SetLabel2(StringUtils::SecondsToTimeString(vtag->GetDuration()));
  }
}

bool CRSSDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const std::string pathToUrl(url.Get());
  std::string strPath(pathToUrl);
  URIUtils::RemoveSlashAtEnd(strPath);
  std::map<std::string,CDateTime>::iterator it;
  items.SetPath(strPath);
  std::unique_lock<CCriticalSection> lock(m_section);
  if ((it=m_cache.find(strPath)) != m_cache.end())
  {
    if (it->second > CDateTime::GetCurrentDateTime() &&
        items.Load())
      return true;
    m_cache.erase(it);
  }
  lock.unlock();

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "failed to load xml from <{}>. error: <{}>", strPath, xmlDoc.ErrorId());
    return false;
  }
  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "error parsing xml doc from <{}>. error: <{}>", strPath, xmlDoc.ErrorId());
    return false;
  }

  TiXmlElement* rssXmlNode = xmlDoc.RootElement();

  if (!rssXmlNode)
    return false;

  TiXmlHandle docHandle( &xmlDoc );
  TiXmlElement* channelXmlNode = docHandle.FirstChild( "rss" ).FirstChild( "channel" ).Element();
  if (channelXmlNode)
    ParseItem(&items, channelXmlNode, pathToUrl);
  else
    return false;

  TiXmlElement* child = NULL;
  for (child = channelXmlNode->FirstChildElement("item"); child; child = child->NextSiblingElement())
  {
    // Create new item,
    CFileItemPtr item(new CFileItem());
    ParseItem(item.get(), child, pathToUrl);

    item->SetProperty("isrss", "1");
    // Use channel image if item doesn't have one
    if (!item->HasArt("thumb") && items.HasArt("thumb"))
      item->SetArt("thumb", items.GetArt("thumb"));

    if (!item->GetPath().empty())
      items.Add(item);
  }

  items.AddSortMethod(SortByNone   , 231, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortByLabel  , 551, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize   , 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
  items.AddSortMethod(SortByDate   , 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date

  CDateTime time = CDateTime::GetCurrentDateTime();
  int mins = 60;
  TiXmlElement* ttl = docHandle.FirstChild("rss").FirstChild("ttl").Element();
  if (ttl)
    mins = strtol(ttl->FirstChild()->Value(),NULL,10);
  time += CDateTimeSpan(0,0,mins,0);
  items.SetPath(strPath);
  items.Save();
  std::unique_lock<CCriticalSection> lock2(m_section);
  m_cache.insert(make_pair(strPath,time));

  return true;
}

bool CRSSDirectory::Exists(const CURL& url)
{
  CCurlFile rss;
  return rss.Exists(url);
}
