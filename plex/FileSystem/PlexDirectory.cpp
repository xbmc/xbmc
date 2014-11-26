//
//  PlexDirectory.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-05.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectory.h"
#include "filesystem/FileFactory.h"
#include "File.h"
#include "XBMCTinyXML.h"
#include "utils/log.h"
#include "JobManager.h"

#include "PlexAttributeParser.h"
#include "PlexDirectoryTypeParser.h"

#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"

#include <boost/assign/list_of.hpp>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wredeclared-class-member"
#endif
#include <boost/bimap.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <boost/foreach.hpp>
#include <map>

#include "Stopwatch.h"

#include "Client/PlexServerDataLoader.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "Playlists/PlexPlayQueueManager.h"

#include "GUIViewState.h"

#include "PlexJobs.h"
#include "PlexApplication.h"

#include "DirectoryCache.h"

#include "XMLChoice.h"
#include "AdvancedSettings.h"
#include "PlexDirectoryCache.h"
#include "Client/PlexServerVersion.h"


using namespace XFILE;

#ifdef USE_RAPIDXML
using namespace rapidxml;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetXMLData(CStdString& data)
{
  bool httpSuccess;
  CStopWatch httpTimer;
  httpTimer.StartZero();

  if (m_verb == "POST" || !m_body.empty())
  {
    httpSuccess = m_file.Post(m_url.Get(), m_body, data);
  }
  else if (m_verb == "GET")
  {
    httpSuccess = m_file.Get(m_url.Get(), data);
  }
  else if (m_verb == "PUT")
  {
    httpSuccess = m_file.Put(m_url.Get(), data);
  }
  else if (m_verb == "DELETE")
  {
    httpSuccess = m_file.Delete(m_url.Get(), data);
  }
  else
  {
    CLog::Log(LOGERROR, "CPlexDirectory::GetDirectory UNKNOWN VERB %s :-(", m_verb.c_str());
    return false;
  }

  if (!httpSuccess)
  {
    CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory failed to fetch data from %s: %ld", m_url.Get().c_str(), m_file.GetLastHTTPResponseCode());
    if (m_file.GetLastHTTPResponseCode() == 500)
    {
      /* internal server error, we should handle this .. */
    }
    return false;
  }

  CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory::Timing took %f seconds to download XML document", httpTimer.GetElapsedSeconds());
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetDirectory(const CURL& url, CFileItemList& fileItems)
{
  m_url = url;

  CStopWatch timer;
  timer.StartZero();

  CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory %s", m_url.Get().c_str());

  /* Some hardcoded paths here */
  if (url.GetHostName() == "shared")
  {
    return GetSharedServerDirectory(fileItems);
  }
  else if (url.GetHostName() == "channels")
  {
    return GetChannelDirectory(fileItems);
  }
  else if (url.GetHostName() == "channeldirectory")
  {
    return GetOnlineChannelDirectory(fileItems);
  }
  else if (url.GetHostName() == "playqueue")
  {
    if (url.GetFileName() == "video")
      return GetPlayQueueDirectory(PLEX_MEDIA_TYPE_VIDEO, fileItems);
    
    if (url.GetFileName() == "audio")
      return GetPlayQueueDirectory(PLEX_MEDIA_TYPE_MUSIC, fileItems);
    
    if (url.GetFileName().IsEmpty())
    {
      CPlexPlayQueuePtr pq = g_plexApplication.playQueueManager->getPlayingPlayQueue();
      if (pq)
        return pq->get(fileItems);
    }
  }
  else if ((url.GetHostName() == "playlists") && (url.GetFileName().IsEmpty() || url.GetFileName() == "all"))
  {
    return GetPlaylistsDirectory(fileItems, url.GetOptions());
  }

  if (boost::starts_with(m_url.GetFileName(), "library/metadata") && !boost::ends_with(m_url.GetFileName(), "children") && !boost::ends_with(m_url.GetFileName(), "extras"))
    m_url.SetOption("checkFiles", "1");

  if (m_url.HasProtocolOption("containerSize"))
  {
    m_url.SetOption("X-Plex-Container-Size", m_url.GetProtocolOption("containerSize"));
    m_url.RemoveProtocolOption("containerSize");
  }
  if (m_url.HasProtocolOption("containerStart"))
  {
    m_url.SetOption("X-Plex-Container-Start", m_url.GetProtocolOption("containerStart"));
    m_url.RemoveProtocolOption("containerStart");
  }

  if (!GetXMLData(m_data))
    return false;

  {

    // now handle the cache if required
    unsigned long newHash = 0;
    std::string cacheURL = m_url.Get();

    if (m_cacheStrategy != CPlexDirectoryCache::CACHE_STARTEGY_NONE)
    {
      // first compute the hash on retrieved xml
      newHash = PlexUtils::GetFastHash(m_data);

      if (g_plexApplication.directoryCache &&
          g_plexApplication.directoryCache->GetCacheHit(cacheURL, newHash, fileItems))
      {
        float elapsed = timer.GetElapsedSeconds();
        CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory::Timing returning a directory after total %f seconds with %d items with content %s", elapsed, fileItems.Size(), fileItems.GetContent().c_str());

        // we found a hit, return it
        return true;
      }
    }

#ifdef USE_RAPIDXML

    xml_document<> doc;    // character type defaults to char
    try
    {
      if (m_data.size() > 1023)
        m_xmlData.reset(new char[m_data.size() + 1]);

      std::copy(m_data.begin(), m_data.end(), m_xmlData.get());
      m_xmlData[m_data.size()] = '\0';
      doc.parse<0>(m_xmlData.get());    // 0 means default parse flags
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory Parse with RapidXML failed");
    }

    xml_node<>* pRoot =  doc.first_node();
    if (pRoot)
    {
      if (!ReadMediaContainer(pRoot, fileItems))
      {
        CLog::Log(LOGERROR, "CPlexDirectory::GetDirectory failed to read root MediaContainer from %s", m_url.Get().c_str());
        return false;
      }
    }
    else CLog::Log(LOGERROR, "CPlexDirectory::GetDirectory Parsed root is NULL");


#else
    CXBMCTinyXML doc;

    doc.Parse(m_data.c_str());
    if (doc.Error())
    {
      CLog::Log(LOGERROR, "CPlexDirectory::GetDirectory failed to parse XML from %s\nError on %d:%d - %s\n%s", m_url.Get().c_str(), doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc(), m_data.c_str());
      return false;
    }

    if (!ReadMediaContainer(doc.RootElement(), fileItems))
    {
      CLog::Log(LOGERROR, "CPlexDirectory::GetDirectory failed to read root MediaContainer from %s", m_url.Get().c_str());
      return false;
    }
#endif

    // add evetually to the cache
    if (g_plexApplication.directoryCache)
      g_plexApplication.directoryCache->AddToCache(cacheURL, newHash, fileItems, m_cacheStrategy);
  }

  float elapsed = timer.GetElapsedSeconds();

  CLog::Log(LOGDEBUG, "CPlexDirectory::GetDirectory::Timing returning a directory after total %f seconds with %d items with content %s", elapsed, fileItems.Size(), fileItems.GetContent().c_str());

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::CancelDirectory()
{
  m_file.Cancel();
}

typedef boost::bimap<EPlexDirectoryType, CStdString> DirectoryTypeMap;
static DirectoryTypeMap g_typeMap = boost::assign::list_of<DirectoryTypeMap::relation>
                                    (PLEX_DIR_TYPE_UNKNOWN, "unknown")
                                    (PLEX_DIR_TYPE_MOVIE, "movie")
                                    (PLEX_DIR_TYPE_SHOW, "show")
                                    (PLEX_DIR_TYPE_SEASON, "season")
                                    (PLEX_DIR_TYPE_EPISODE, "episode")
                                    (PLEX_DIR_TYPE_ARTIST, "artist")
                                    (PLEX_DIR_TYPE_ALBUM, "album")
                                    // special case, we'll use song instead of track here
                                    // since it's song everywhere else
                                    (PLEX_DIR_TYPE_TRACK, "song")
                                    (PLEX_DIR_TYPE_PHOTO, "photo")
                                    (PLEX_DIR_TYPE_VIDEO, "video")
                                    (PLEX_DIR_TYPE_DIRECTORY, "directory")
                                    (PLEX_DIR_TYPE_SECTION, "section")
                                    (PLEX_DIR_TYPE_SERVER, "server")
                                    (PLEX_DIR_TYPE_DEVICE, "device")
                                    (PLEX_DIR_TYPE_SYNCITEM, "syncitem")
                                    (PLEX_DIR_TYPE_MEDIASETTINGS, "mediasettings")
                                    (PLEX_DIR_TYPE_POLICY, "policy")
                                    (PLEX_DIR_TYPE_LOCATION, "location")
                                    (PLEX_DIR_TYPE_MEDIA, "media")
                                    (PLEX_DIR_TYPE_PART, "part")
                                    (PLEX_DIR_TYPE_SYNCITEMS, "syncitems")
                                    (PLEX_DIR_TYPE_STREAM, "stream")
                                    (PLEX_DIR_TYPE_STATUS, "status")
                                    (PLEX_DIR_TYPE_TRANSCODEJOB, "transcodejob")
                                    (PLEX_DIR_TYPE_TRANSCODESESSION, "transcodesession")
                                    (PLEX_DIR_TYPE_PROVIDER, "provider")
                                    (PLEX_DIR_TYPE_CLIP, "clip")
                                    (PLEX_DIR_TYPE_PLAYLIST, "playlist")
                                    (PLEX_DIR_TYPE_CHANNEL, "channel")
                                    (PLEX_DIR_TYPE_SECONDARY, "secondary")
                                    (PLEX_DIR_TYPE_GENRE, "genre")
                                    (PLEX_DIR_TYPE_ROLE, "role")
                                    (PLEX_DIR_TYPE_WRITER, "writer")
                                    (PLEX_DIR_TYPE_PRODUCER, "producer")
                                    (PLEX_DIR_TYPE_COUNTRY, "country")
                                    (PLEX_DIR_TYPE_DIRECTOR, "director")
                                    (PLEX_DIR_TYPE_THUMB, "thumb")
                                    (PLEX_DIR_TYPE_IMAGE, "image")
                                    (PLEX_DIR_TYPE_CHANNELS, "plugin")
                                    (PLEX_DIR_TYPE_USER, "user")
                                    (PLEX_DIR_TYPE_RELEASE, "release")
                                    (PLEX_DIR_TYPE_PACKAGE, "package")
                                    (PLEX_DIR_TYPE_PHOTOALBUM, "photoalbum")
                                    ;


CPlexAttributeParserBase *g_parserInt = new CPlexAttributeParserInt;
CPlexAttributeParserBase *g_parserBool = new CPlexAttributeParserBool;
CPlexAttributeParserBase *g_parserKey = new CPlexAttributeParserKey;
CPlexAttributeParserBase *g_parserMediaUrl = new CPlexAttributeParserMediaUrl;
CPlexAttributeParserBase *g_parserType = new CPlexAttributeParserType;
CPlexAttributeParserBase *g_parserLabel = new CPlexAttributeParserLabel;
CPlexAttributeParserBase *g_parserMediaFlag = new CPlexAttributeParserMediaFlag;
CPlexAttributeParserBase *g_parserDateTime = new CPlexAttributeParserDateTime;
CPlexAttributeParserBase *g_parserTitleSort = new CPlexAttributeParserTitleSort;

typedef std::map<CStdString, CPlexAttributeParserBase*> AttributeMap;
typedef std::pair<CStdString, CPlexAttributeParserBase*> AttributePair;
static AttributeMap g_attributeMap = boost::assign::list_of<AttributePair>
                                     ("size", g_parserInt)
                                     ("channels", g_parserInt)
                                     ("createdAt", g_parserInt)
                                     ("updatedAt", g_parserInt)
                                     ("leafCount", g_parserInt)
                                     ("viewedLeafCount", g_parserInt)
                                     ("bitrate", g_parserInt)
                                     ("duration", g_parserInt)
                                     ("librarySectionID", g_parserInt)
                                     ("streamType", g_parserInt)
                                     ("index", g_parserInt)
                                     ("channels", g_parserInt)
                                     ("bitrate", g_parserInt)
                                     ("samplingRate", g_parserInt)
                                     ("dialogNorm", g_parserInt)
                                     ("viewMode", g_parserInt)
                                     ("autoRefresh", g_parserInt)
                                     ("playQueueID", g_parserInt)
                                     ("playQueueSelectedItemID", g_parserInt)
                                     ("playQueueSelectedItemOffset", g_parserInt)
                                     ("playQueueTotalCount", g_parserInt)
                                     ("playQueueVersion", g_parserInt)
                                     ("ratingKey", g_parserInt)

                                     ("filters", g_parserBool)
                                     ("refreshing", g_parserBool)
                                     ("allowSync", g_parserBool)
                                     ("secondary", g_parserBool)
                                     ("search", g_parserBool)
                                     ("selected", g_parserBool)
                                     ("indirect", g_parserBool)
                                     ("popup", g_parserBool)
                                     ("installed", g_parserBool)
                                     ("settings", g_parserBool)
                                     ("search", g_parserBool)
                                     ("live", g_parserBool)
                                     ("autoupdate", g_parserBool)
                                     ("synced", g_parserBool)

                                     ("key", g_parserKey)
                                     ("theme", g_parserKey)
                                     ("parentKey", g_parserKey)
                                     ("parentRatingKey", g_parserKey)
                                     ("grandparentKey", g_parserKey)
                                     ("composite", g_parserKey)

                                     ("thumb", g_parserMediaUrl)
                                     ("art", g_parserMediaUrl)
                                     ("poster", g_parserMediaUrl)
                                     ("banner", g_parserMediaUrl)
                                     ("parentThumb", g_parserMediaUrl)
                                     ("grandparentThumb", g_parserMediaUrl)
                                     ("sourceIcon", g_parserMediaUrl)

                                     /* Media flags */
                                     ("aspectRatio", g_parserMediaFlag)
                                     ("audioChannels", g_parserMediaFlag)
                                     ("audioCodec", g_parserMediaFlag)
                                     ("videoCodec", g_parserMediaFlag)
                                     ("videoResolution", g_parserMediaFlag)
                                     ("videoFrameRate", g_parserMediaFlag)
                                     ("contentRating", g_parserMediaFlag)
                                     ("grandparentContentRating", g_parserMediaFlag)
                                     ("studio", g_parserMediaFlag)
                                     ("grandparentStudio", g_parserMediaFlag)

                                     ("type", g_parserType)
                                     ("content", g_parserType)

                                     ("title", g_parserLabel)
                                     ("title1", g_parserLabel)
                                     ("name", g_parserLabel)

                                     ("originallyAvailableAt", g_parserDateTime)

                                     ("titleSort", g_parserTitleSort)
                                     ;

static CPlexAttributeParserBase* g_defaultAttr = new CPlexAttributeParserBase;

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::CopyAttributes(XML_ELEMENT* el, CFileItem* item, const CURL &url)
{
#ifndef USE_RAPIDXML
  XML_ATTRIBUTE *attr = el->FirstAttribute();

  while (attr)
  {
    CStdString key = attr->NameTStr();
    CStdString valStr = CStdString(attr->Value());

    if (g_attributeMap.find(key) != g_attributeMap.end())
    {
      CPlexAttributeParserBase* attr = g_attributeMap[key];
      attr->Process(url, key, valStr, item);
    }
    else
    {
      g_defaultAttr->Process(url, key, valStr, item);
    }

    attr = attr->Next();
  }
#else
  XML_ATTRIBUTE *attr = el->first_attribute();

  while (attr)
  {
    CStdString key = attr->name();
    CStdString valStr = CStdString(attr->value());

    if (g_attributeMap.find(key) != g_attributeMap.end())
    {
      CPlexAttributeParserBase* attr = g_attributeMap[key];
      attr->Process(url, key, valStr, item);
    }
    else
    {
      g_defaultAttr->Process(url, key, valStr, item);
    }

    attr = attr->next_attribute();
  }
#endif
}

CFileItemPtr CPlexDirectory::NewPlexElement(XML_ELEMENT *element, const CFileItem &parentItem, const CURL &baseUrl)
{
  CFileItemPtr newItem = CFileItemPtr(new CFileItem);

  /* Make sure this is set before running copyattributes so that
   * mediaflag urls can be calculated correctly */
  if (parentItem.HasProperties())
  {
    if (parentItem.HasProperty("mediaTagPrefix"))
      newItem->SetProperty("mediaTagPrefix", parentItem.GetProperty("mediaTagPrefix").asString());
    if (parentItem.HasProperty("mediaTagVersion"))
      newItem->SetProperty("mediaTagVersion", parentItem.GetProperty("mediaTagVersion").asString());
  }
#ifndef USE_RAPIDXML
  newItem->SetProperty("xmlElementName", element->ValueStr());
#else
  newItem->SetProperty("xmlElementName", element->name());
#endif

  CPlexDirectory::CopyAttributes(element, newItem.get(), baseUrl);

  if (newItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_UNKNOWN)
  {
    /* no type attribute, let's try to use the name of the XML element */
    CPlexAttributeParserType t;
#ifndef USE_RAPIDXML
    g_parserType->Process(baseUrl, "type", element->ValueStr(), newItem.get());
#else
    g_parserType->Process(baseUrl, "type", CStdString(element->name()), newItem.get());
#endif

  }
  else if (newItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_IMAGE)
  {
    // Images usually are things returned from /library/arts and similar endpoints
    // they just have the "key" attribute and that points to the image, so here
    // I make sure to manually pass it through the photo transcoder.
    CPlexAttributeParserMediaUrl t;
#ifndef USE_RAPIDXML
    t.Process(baseUrl, "art", element->Attribute("key"), newItem.get());
#else
    xml_attribute<> *pA = element->first_attribute("key");

    if (pA)	t.Process(baseUrl, "art", CStdString(pA->value()), newItem.get());
    else    t.Process(baseUrl, "art", "", newItem.get());
#endif

    newItem->SetProperty("key", newItem->GetArt("fanart"));
  }

  if (newItem->HasProperty("key"))
    newItem->SetPath(newItem->GetProperty("key").asString());

  newItem->SetProperty("plex", true);
  newItem->SetProperty("plexserver", baseUrl.GetHostName());

  /* set the canShare and canRecommend properties */
  newItem->SetProperty("canShare", newItem->HasProperty("url") ? "yes" : "");
  newItem->SetProperty("canRecommend", newItem->HasProperty("url") ? "yes" : "");

  if (!parentItem.GetPath().empty())
    newItem->SetProperty("containerPath", parentItem.GetPath());

#if 0
  CLog::Log(LOGDEBUG, "CPlexDirectory::NewPlexElement %s (type: %s) -> isFolder(%s)",
            newItem->GetPath().c_str(),
            GetDirectoryTypeString(newItem->GetPlexDirectoryType()).c_str(),
            newItem->m_bIsFolder ? "yes" : "no");
#endif

  return newItem;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::ReadChildren(XML_ELEMENT* root, CFileItemList& container)
{
  EPlexDirectoryType type = PLEX_DIR_TYPE_UNKNOWN;

  int itemcount = 0;
#ifdef USE_RAPIDXML
  for (XML_ELEMENT *element = root->first_node(); element; element = element->next_sibling())
#else
  for (XML_ELEMENT *element = root->FirstChildElement(); element; element = element->NextSiblingElement())
#endif
  {
    CFileItemPtr item = CPlexDirectory::NewPlexElement(element, container, m_url);

    if (boost::ends_with(item->GetPath(), "/allLeaves"))
    {
      if (g_advancedSettings.m_bVideoLibraryHideAllItems)
        continue;

      item->SetProperty("isAllItems", true);
    }

    if (type == PLEX_DIR_TYPE_UNKNOWN)
      type = item->GetPlexDirectoryType();
    else if (type != item->GetPlexDirectoryType())
      container.SetProperty("hasMixedMembers", true);

    CPlexDirectoryTypeParserBase::GetDirectoryTypeParser(item->GetPlexDirectoryType())->Process(*item, container, element);

    /* forward some mediaContainer properties */
    item->SetProperty("containerKey", container.GetProperty("unprocessed_key"));
    
    if (!item->HasProperty("identifier") && container.HasProperty("identifier"))
      item->SetProperty("identifier", container.GetProperty("identifier"));
    
    if (!item->HasArt(PLEX_ART_FANART) && container.HasArt(PLEX_ART_FANART))
      item->SetArt(PLEX_ART_FANART, container.GetArt(PLEX_ART_FANART));

    if (!item->HasArt(PLEX_ART_THUMB) && container.HasArt(PLEX_ART_THUMB))
      item->SetArt(PLEX_ART_THUMB, container.GetArt(PLEX_ART_THUMB));

    if (container.HasProperty("librarySectionUUID"))
      item->SetProperty("librarySectionUUID", container.GetProperty("librarySectionUUID"));

    if (container.HasProperty("playQueueID"))
      item->SetProperty("playQueueID", container.GetProperty("playQueueID"));

    if (container.HasProperty("playQueueVersion"))
      item->SetProperty("playQueueVersion", container.GetProperty("playQueueVersion"));

    item->SetProperty("index", container.GetProperty("offset").asInteger() + itemcount);
    
    item->m_bIsFolder = IsFolder(item, element);

    container.Add(item);

    itemcount++;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::ReadMediaContainer(XML_ELEMENT* root, CFileItemList& mediaContainer)
{
#ifndef USE_RAPIDXML
  if (root->ValueStr() != "MediaContainer" && root->ValueStr() != "ASContainer")
#else
  if (CStdString(root->name()) != "MediaContainer" && CStdString(root->name()) != "ASContainer")
#endif
  {
    CLog::Log(LOGWARNING, "CPlexDirectory::ReadMediaContainer got XML document without mediaContainer as root at %s", m_url.Get().c_str());
    //CLog::Log(LOGWARNING, "root->ValueStr() = %s", root->ValueStr().c_str());
    return false;
  }

  if (m_url.HasOption("checkFiles"))
    m_url.RemoveOption("checkFiles");

  if (m_url.HasOption("X-Plex-Container-Start"))
    m_url.RemoveOption("X-Plex-Container-Start");

  if (m_url.HasOption("X-Plex-Container-Size"))
    m_url.RemoveOption("X-Plex-Container-Size");

  mediaContainer.SetPath(m_url.Get());
  mediaContainer.SetProperty("plex", true);
  mediaContainer.SetProperty("plexserver", m_url.GetHostName());
  
  CPlexDirectory::CopyAttributes(root, &mediaContainer, m_url);
  g_parserKey->Process(m_url, "key", "/" + m_url.GetFileName(), &mediaContainer);

  /* now read all the childs to the mediaContainer */
  ReadChildren(root, mediaContainer);

  /* We just use the first item Type, it might be wrong and we should maybe have a look... */
  if (mediaContainer.GetPlexDirectoryType() == PLEX_DIR_TYPE_UNKNOWN && mediaContainer.Size() > 0)
  {
    /* When loading a season the first element can be "All Episodes" and that is just a directory
     * without a type attribute. So let's skip that. */
    std::string key = mediaContainer.Get(0)->GetProperty("unprocessed_key").asString();

    /* we need to cut everything from ? to make sure that we don't include
     * arguments when making the check */
    size_t c = key.find_last_of("?");
    if (c != std::string::npos)
      key.erase(c, key.size());

    if (boost::ends_with(key, "/allLeaves") && mediaContainer.Size() > 1)
      mediaContainer.SetPlexDirectoryType(mediaContainer.Get(1)->GetPlexDirectoryType());
    /* See https://github.com/plexinc/plex/issues/737 for a discussion around this workaround */
    else if (boost::starts_with(m_url.GetFileName(), "library/sections/") &&
             mediaContainer.Size() > 0 &&
             mediaContainer.Get(0)->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTOALBUM)
      mediaContainer.SetPlexDirectoryType(PLEX_DIR_TYPE_PHOTO);
    else if (boost::starts_with(m_url.GetFileName(), "library/metadata/") &&
             mediaContainer.Size() > 0 &&
             mediaContainer.Get(0)->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTO)
      mediaContainer.SetPlexDirectoryType(PLEX_DIR_TYPE_PHOTOALBUM);
    else
      mediaContainer.SetPlexDirectoryType(mediaContainer.Get(0)->GetPlexDirectoryType());
  }
  
  /* we need to massage channels a tiny wee bit */
  if (boost::starts_with(m_url.GetFileName(), "video") ||
      boost::starts_with(m_url.GetFileName(), "music") ||
      boost::starts_with(m_url.GetFileName(), "photos"))
  {
    // store the original value somewhere.
    mediaContainer.SetProperty("channelType", (int)mediaContainer.GetPlexDirectoryType());

    if (mediaContainer.HasProperty("message"))
      mediaContainer.SetPlexDirectoryType(PLEX_DIR_TYPE_MESSAGE);
    else
      mediaContainer.SetPlexDirectoryType(PLEX_DIR_TYPE_CHANNEL);
  }
  
  /* now we need to set content to something that XBMC expects */
  if (mediaContainer.IsEmpty())
    mediaContainer.SetContent("empty");
  else
  {
    CStdString content = CPlexDirectory::GetContentFromType(mediaContainer.GetPlexDirectoryType());
    if (!content.empty())
      mediaContainer.SetContent(content);
  }
  
  /* set the sort method to none, this means that we respect the order from the server */
  mediaContainer.AddSortMethod(SORT_METHOD_NONE, 553, LABEL_MASKS());

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
EPlexDirectoryType CPlexDirectory::GetDirectoryType(const CStdString &typeStr)
{
  DirectoryTypeMap::right_const_iterator it = g_typeMap.right.find(typeStr);
  if (it != g_typeMap.right.end())
    return it->second;
  return PLEX_DIR_TYPE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexDirectory::GetDirectoryTypeString(EPlexDirectoryType typeNr)
{
  DirectoryTypeMap::left_const_iterator it = g_typeMap.left.find(typeNr);
  if (it != g_typeMap.left.end())
    return it->second;

  return "unknown";
}

////////////////////////////////////////////////////////////////////////////////
CStdString CPlexDirectory::GetContentFromType(EPlexDirectoryType typeNr)
{
  CStdString content;

  switch(typeNr)
  {
    case PLEX_DIR_TYPE_MOVIE:
      content = "movies";
      break;
    case PLEX_DIR_TYPE_SHOW:
      content = "tvshows";
      break;
    case PLEX_DIR_TYPE_SEASON:
      content = "seasons";
      break;
    case PLEX_DIR_TYPE_EPISODE:
      content = "episodes";
      break;
    case PLEX_DIR_TYPE_ARTIST:
      content = "artists";
      break;
    case PLEX_DIR_TYPE_ALBUM:
      content = "albums";
      break;
    case PLEX_DIR_TYPE_TRACK:
      content = "songs";
      break;
    case PLEX_DIR_TYPE_SECONDARY:
      content = "secondary";
      break;
    case PLEX_DIR_TYPE_CHANNEL:
      content = "channel";
      break;
    case PLEX_DIR_TYPE_CHANNELS:
      content = "channels";
      break;
    case PLEX_DIR_TYPE_CLIP:
      content = "clips";
      break;
    case PLEX_DIR_TYPE_PHOTO:
      content = "photos";
      break;
    case PLEX_DIR_TYPE_PHOTOALBUM:
      content = "photoalbums";
      break;
    case PLEX_DIR_TYPE_PLAYLIST:
      content = "playlists";
      break;
    default:
      break;
  }

  return content;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
DIR_CACHE_TYPE CPlexDirectory::GetCacheType(const CStdString &strPath) const
{
  /* We really don't want to agressively cache stuff, so let's just start by caching remote servers */
  CURL u(strPath);
  CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(u.GetHostName());

  if (server && server->GetActiveConnection())
  {
    if (server->GetActiveConnection()->IsLocal() || !server->IsShared())
      return DIR_CACHE_NEVER;
    else
    {
      CLog::Log(LOGDEBUG, "CPlexDirectory::GetCacheType allow %s to be cached", m_url.Get().c_str());
      return DIR_CACHE_ALWAYS;
    }
  }

  return DIR_CACHE_NEVER;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::CachePath(const CStdString &path)
{
  CPlexDirectory dir;
  CFileItemList list;

  g_directoryCache.ClearDirectory(path);

  if (dir.GetDirectory(path, list))
  {
    g_directoryCache.SetDirectory(path, list, DIR_CACHE_ALWAYS);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::IsFolder(const CFileItemPtr& item, XML_ELEMENT* element)
{
#ifndef USE_RAPIDXML
  if (element->ValueStr() == "Directory")
    return true;
#else
  if (CStdString(element->name()) == "Directory")
    return true;
#endif

  switch(item->GetPlexDirectoryType())
  {
    case PLEX_DIR_TYPE_VIDEO:
    case PLEX_DIR_TYPE_EPISODE:
    case PLEX_DIR_TYPE_MOVIE:
    case PLEX_DIR_TYPE_PHOTO:
    case PLEX_DIR_TYPE_PART:
    case PLEX_DIR_TYPE_STREAM:
    case PLEX_DIR_TYPE_GENRE:
    case PLEX_DIR_TYPE_ROLE:
    case PLEX_DIR_TYPE_COUNTRY:
    case PLEX_DIR_TYPE_WRITER:
    case PLEX_DIR_TYPE_DIRECTOR:
    case PLEX_DIR_TYPE_MEDIA:
    case PLEX_DIR_TYPE_CLIP:
    case PLEX_DIR_TYPE_TRACK:
      return false;
      break;
    default:
      break;
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetSharedServerDirectory(CFileItemList &items)
{
  CFileItemListPtr sharedSections = g_plexApplication.dataLoader->GetAllSharedSections();

  for (int i = 0 ; i < sharedSections->Size(); i++)
  {
    CFileItemPtr sectionItem = sharedSections->Get(i);
    CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(sectionItem->GetProperty("serverUUID").asString());
    if (!server) continue;
    
    CFileItemPtr item(CFileItemPtr(new CFileItem()));
    
    item->m_bIsFolder = true;
    item->SetPath(sectionItem->GetPath());
    item->SetLabel(sectionItem->GetLabel());

    item->SetLabel2(server->GetOwner());
    item->SetProperty("machineIdentifier", server->GetUUID());
    item->SetProperty("serverOwner", server->GetOwner());
    item->SetProperty("serverName", server->GetName());
    item->SetPlexDirectoryType(sectionItem->GetPlexDirectoryType());

    if (sectionItem->HasProperty("composite"))
      item->SetProperty("composite", sectionItem->GetProperty("composite"));

    items.Add(item);
  }
  
  items.SetPath("plexserver://shared");

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetChannelDirectory(CFileItemList &items)
{
  CFileItemListPtr channels = g_plexApplication.dataLoader->GetAllChannels();
  for (int i = 0; i < channels->Size(); i ++)
  {
    CFileItemPtr channel = channels->Get(i);
    
    CStdString window, type;
    CURL p(channel->GetPath());
    /* figure out what type of plugin this is so we can open it correctly */
    if (boost::starts_with(p.GetFileName(), "video"))
    {
      window = "MyVideoFiles";
      type = "video";
    }
    else if (boost::starts_with(p.GetFileName(), "music"))
    {
      window = "MyMusicFiles";
      type = "music";
    }
    else if (boost::starts_with(p.GetFileName(), "photos"))
    {
      window = "MyPictures";
      type = "photos";
    }
    
    channel->SetProperty("mediaWindow", window);
    channel->SetProperty("type", type);
    channel->SetPlexDirectoryType(GetDirectoryType(type));
    channel->SetLabel2(channel->GetProperty("serverName").asString());

    CLog::Log(LOGDEBUG, "CPlexDirectory::GetChannelDirectory channel %s from server %s",
              channel->GetLabel().c_str(), channel->GetLabel2().c_str());
    
    items.Add(channel);
  }
  
  items.SetContent("channels");
  items.SetPlexDirectoryType(PLEX_DIR_TYPE_CHANNELS);
  items.SetPath("plexserver://channels");
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetOnlineChannelDirectory(CFileItemList &items)
{
  if (!g_plexApplication.serverManager->GetBestServer())
    return false;

  CURL newURL = g_plexApplication.serverManager->GetBestServer()->BuildPlexURL("/system/appstore");
  bool success = CPlexDirectory::GetDirectory(newURL.Get(), items);
  if (success)
    items.SetPath("plexserver://channeldirectory");
  
  return success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetPlaylistsDirectory(CFileItemList &items, CStdString options)
{
  items.SetPlexDirectoryType(PLEX_DIR_TYPE_PLAYLIST);
  items.SetProperty("PlexContent", PlexUtils::GetPlexContent(items));
  items.SetPath("plexserver://playlists/");
  items.AddSortMethod(SORT_METHOD_NONE, 553, LABEL_MASKS());
  
  CPlexServerVersion playlistVersion("0.9.9.12.0");
  PlexServerList list = g_plexApplication.serverManager->GetAllServers(CPlexServerManager::SERVER_OWNED, true);
  BOOST_FOREACH(CPlexServerPtr server, list)
  {
    CPlexServerVersion version(server->GetVersion());
    if (version > playlistVersion)
    {
      CURL plURL = server->BuildPlexURL("/playlists/all");
      plURL.SetOptions(options);
      plURL.SetOption("type", boost::lexical_cast<std::string>(PLEX_MEDIA_FILTER_TYPE_PLAYLISTITEM));
      plURL.SetOption("sort", "lastViewedAt");
      
      CFileItemList plList;
      
      if (GetDirectory(plURL, plList))
      {
        for (int i = 0; i < plList.Size(); i ++)
        {
          CFileItemPtr item = plList.Get(i);
          if (!item)
            continue;
          
          item->SetProperty("serverName", server->GetName());
          item->SetProperty("serverOwner", server->GetOwner());
          item->SetProperty("PlexContent", PlexUtils::GetPlexContent(*item));

          // we expect music instead of audio in the skin
          std::string type = item->GetProperty("playlistType").asString();
          if (type == "audio")
            type = "music";
          
          item->SetProperty("type", type + "playlist");
          
          items.Add(item);
        }
      }
      else
      {
        CLog::Log(LOGWARNING, "CPlexDirectory::GetPlaylistsDirectory - failed to fetch playlists from %s", server->toString().c_str());
      }
    }
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetPlayQueueDirectory(ePlexMediaType type, CFileItemList& items)
{
  g_plexApplication.playQueueManager->getPlayQueue(type, items);

  // we always want to return true here, in *worst* case we will just
  // return a empty list.
  return true;
}
