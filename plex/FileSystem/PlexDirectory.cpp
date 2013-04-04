/*
 * PlexDirectory.cpp
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/foreach.hpp"
#include "utils/XBMCTinyXML.h"

#include "PlexMediaPart.h"

#include "log.h"
#include "TimeUtils.h"
#include "Album.h"
#include "AdvancedSettings.h"
#include "File.h"
#include "PlexDirectory.h"
#include "DirectoryCache.h"
#include "Util.h"
#include "CurlFile.h"
#include "utils/HttpHeader.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "GUISettings.h"
#include "URL.h"
#include "FileItem.h"
#include "GUIViewState.h"
#include "GUIDialogOK.h"
#include "Picture.h"
#include "PlexLibrarySectionManager.h"
#include "PlexServerManager.h"
#include "BackgroundInfoLoader.h"
#include "PlexTypes.h"
#include "URIUtils.h"
#include "PlexUtils.h"
#include "TextureCache.h"
#include "StringUtils.h"
#include "LocalizeStrings.h"

#include "GUIInfoManager.h"

#include "LangInfo.h"

using namespace std;
using namespace XFILE;

#define MASTER_PLEX_MEDIA_SERVER "http://localhost:32400"
#define MAX_THUMBNAIL_AGE (3600*24*2)
#define MAX_FANART_AGE    (3600*24*7)

CFileItemListPtr CPlexDirectory::g_filterList;
std::vector<CStdString> g_homeVideoMap;
boost::mutex g_homeVideoMapMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectory::CPlexDirectory(bool parseResults, bool displayDialog)
: m_bStop(false)
, m_bSuccess(true)
, m_bParseResults(parseResults)
, m_bReplaceLocalhost(true)
, m_dirCacheType(DIR_CACHE_ALWAYS)
{
  m_timeout = 300;
  
  if (displayDialog)
    m_flags |= DIR_FLAG_ALLOW_PROMPT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectory::CPlexDirectory(bool parseResults, bool displayDialog, bool replaceLocalhost, int timeout)
  : m_bStop(false)
  , m_bSuccess(true)
  , m_bParseResults(parseResults)
  , m_bReplaceLocalhost(replaceLocalhost)
  , m_dirCacheType(DIR_CACHE_ALWAYS)
{
  m_timeout = timeout;

  if (displayDialog)
    m_flags |= DIR_FLAG_ALLOW_PROMPT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetDirectory(const CStdString& path, CFileItemList &items)
{
  CStdString strPath = path;
  
  // Hackish, but a few special directories.
  if (strPath == "plex://shared")
  {
    vector<CFileItemPtr> list;
    PlexLibrarySectionManager::Get().getSharedSections(list);
    
    BOOST_FOREACH(CFileItemPtr item, list)
      items.Add(item);
    
    return true;
  }
  
  // Additionally, if we're going to 127.0.0.1, this is actually an alias for "selected server".
  // which historically was always 127.0.0.1 but now of course could be anywhere.
  //
  CURL url(strPath);
  if (m_bReplaceLocalhost == true && url.GetHostName() == "127.0.0.1")
  {
    PlexServerPtr server = PlexServerManager::Get().bestServer();
    if (server)
    {
      url.SetHostName(server->address);
      url.SetPort(server->port);
      
      dprintf("Plex Server Manager: Using selected best server %s to make URL %s", server->name.c_str(), strPath.c_str());
    }
  }
  
  // Add accessibility flag if we are in the library
  if (path.find("library/metadata") != string::npos)
  {
    url.SetOption("checkFiles", "1");
  }
  
  strPath = url.Get();
  
  // Get the directory.
  bool ret = CPlexDirectory::ReallyGetDirectory(strPath, items);
  
  // See if it's an intermediate filter directory.
  if (false && items.GetContent() == "secondary")
  {
    printf("Found a filter directory on %s\n", strPath.c_str());
    
    // And request the first item in the list (for now).
    CFileItemPtr firstItem = items.Get(0);
    if (firstItem)
    {
      // We'll save the filter.
      g_filterList = CFileItemListPtr(new CFileItemList());
      g_filterList->Assign(items);
      items.Clear();
      
      // Get the filtered directory.
      ret = CPlexDirectory::ReallyGetDirectory(firstItem->GetPath(), items);
    }      
  }
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::ReallyGetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (URIUtils::HasSlashAtEnd(strRoot) && strRoot != "plex://")
    strRoot.Delete(strRoot.size() - 1);

  strRoot.Replace(" ", "%20");

  // Start the download thread running.
  CLog::Log(LOGNOTICE, "PlexDirectory::GetDirectory(%s)", strRoot.c_str());
  m_url = strRoot;

  Process();

  if (m_bStop)
    return false;

  // See if we suceeded.
  if (m_bSuccess == false)
  {
    if (!m_data.IsEmpty())
    {
      TiXmlDocument doc;
      doc.Parse(m_data.c_str());
      TiXmlElement* eRoot = doc.RootElement();
      if (eRoot != 0)
      {
        const char *code = eRoot->Attribute("code");
        const char *status = eRoot->Attribute("status");
        if (code && status)
        {
          int errCode = atoi(code);
          /* We got an error from the plugin */
          CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(50000 + errCode), "", "");
        }
      }
    }
    return false;
  }

  // See if we're supposed to parse the results or not.
  if (m_bParseResults == false)
    return true;

  // Parse returned xml.
  TiXmlDocument doc;
  doc.Parse(m_data.c_str());

  TiXmlElement* root = doc.RootElement();
  if (root == 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to parse XML\n%s", __FUNCTION__, m_data.c_str());
    return false;
  }

  // Is the server local?
  CURL url(m_url);
  bool localServer = NetworkInterface::IsLocalAddress(url.GetHostName());
  
  // Save some properties.
  if (root->Attribute("updatedAt"))
    items.SetProperty("updatedAt", root->Attribute("updatedAt"));
  
  if (root->Attribute("friendlyName"))
    items.SetProperty("friendlyName", root->Attribute("friendlyName"));

  if (root->Attribute("machineIdentifier"))
    items.SetProperty("machineIdentifier", root->Attribute("machineIdentifier"));

  // Get the fanart.
  const char* fanart = root->Attribute("art");
  string strFanart;
  if (fanart && strlen(fanart) > 0)
    strFanart = ProcessMediaElement(strPath, fanart, MAX_FANART_AGE, localServer);

  // Get the thumb.
  const char* thumb = root->Attribute("thumb");
  string strThumb;
  if (thumb && strlen(thumb) > 0)
    strThumb = ProcessMediaElement(strPath, thumb, MAX_THUMBNAIL_AGE, localServer);

  // Walk the parsed tree.
  string strFileLabel = "%N - %T";
  string strSecondFileLabel = "%D";
  string strDirLabel = "%B";
  string strSecondDirLabel = "%Y";

  Parse(m_url, root, items, strFileLabel, strSecondFileLabel, strDirLabel, strSecondDirLabel, localServer);

  // Set the window titles
  const char* title1 = root->Attribute("title1");
  const char* title2 = root->Attribute("title2");

  if (title1 && strlen(title1) > 0)
    items.SetFirstTitle(title1);
  if (title2 && strlen(title2) > 0)
    items.SetSecondTitle(title2);

  // Get container summary.
  const char* summary = root->Attribute("summary");
  if (summary)
    items.SetProperty("description", summary);

  // Get color values
  const char* communityRatingColor = root->Attribute("ratingColor");

  const char* httpCookies = root->Attribute("httpCookies");
  const char* userAgent = root->Attribute("userAgent");

  const char* pluginIdentifier = root->Attribute("identifier");
  if (pluginIdentifier)
    items.SetProperty("identifier", pluginIdentifier);

  // Set fanart on items if they don't have their own, or if individual item fanart
  // is disabled. Also set HTTP & rating info
  //
  int firstProvider = -1;
  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];

    // Save the container URL.
    pItem->SetProperty("containerKey", strPath);
    
    // See if this is a provider.
    if (pItem->GetProperty("provider") == "1")
    {
      items.AddProvider(pItem);
      
      if (firstProvider == -1)
        firstProvider = i;
    }
    
    // Fall back to directory fanart?
    if (strFanart.size() > 0 && !pItem->HasArt(PLEX_ART_FANART))
    {
      pItem->SetArt(PLEX_ART_FANART, strFanart);

      if (strFanart.find("/:/resources") != string::npos)
        pItem->SetProperty("fanart_fallback", "1");
    }

    // Fall back to directory thumb?
    if (strThumb.size() > 0 && !pItem->HasArt(PLEX_ART_THUMB))
      pItem->SetArt(PLEX_ART_THUMB, strThumb);

    // Make sure sort label is lower case.
    CStdStringW sortLabel = pItem->GetSortLabel();
    if (sortLabel.empty() == false)
    {
      boost::to_lower(sortLabel);
      pItem->SetSortLabel(sortLabel);
    }

    // See if there's a cookie property to set.
    if (httpCookies)
      pItem->SetProperty("httpCookies", httpCookies);

    if (userAgent)
      pItem->SetProperty("userAgent", userAgent);

    if (communityRatingColor)
      pItem->SetProperty("communityRatingColor", communityRatingColor);

    if (pluginIdentifier)
    {
      pItem->SetProperty("pluginIdentifier", pluginIdentifier);
      
      CStdString identifier(pluginIdentifier);
      if (identifier == "com.plexapp.plugins.library")
        pItem->SetProperty("HasWatchedState", true);
    }
  }

  // If we had providers, whack them.
  if (firstProvider != -1)
  {
    for (int i=items.Size()-1; i>=firstProvider; i--)
      items.Remove(i);
  }
  
  // Set fanart on directory.
  if (strFanart.size() > 0)
    items.SetArt(PLEX_ART_FANART, strFanart);

  // Set the view mode.
  bool hasViewMode = false;
  const char* viewMode = root->Attribute("viewmode");
  if (viewMode && strlen(viewMode) > 0)
  {
    hasViewMode = true;
  }
  else
  {
    viewMode = root->Attribute("viewMode");
    if (viewMode && strlen(viewMode) > 0)
      hasViewMode = true;
  }

  if (hasViewMode)
  {
    items.SetDefaultViewMode(boost::lexical_cast<int>(viewMode));
    CGUIViewState* viewState = CGUIViewState::GetViewState(0, items);
    viewState->SaveViewAsControl(boost::lexical_cast<int>(viewMode));
    delete viewState;
  }

  // The view group.
  const char* viewGroup = root->Attribute("viewGroup");
  if (viewGroup)
    items.SetProperty("viewGroup", viewGroup);

  // See if we're in plug-in land.
  CURL theURL(m_url);
  if (theURL.GetFileName().Find("library/") == -1)
    items.SetProperty("insidePlugins", true);

  // Override labels.
  const char* fileLabel = root->Attribute("filelabel");
  if (fileLabel && strlen(fileLabel) > 0)
    strFileLabel = fileLabel;

  const char* dirLabel = root->Attribute("dirlabel");
  if (dirLabel && strlen(dirLabel) > 0)
    strDirLabel = dirLabel;

  // Add the sort method.
  items.AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS(strFileLabel, strSecondFileLabel, strDirLabel, strSecondDirLabel));

  // See if it's mixed parent content.
  if (root->Attribute("mixedParents") && strcmp(root->Attribute("mixedParents"), "1") == 0)
    items.SetProperty("mixedParents", "1");

  // Theme music.
  if (root->Attribute("theme"))
  {
    string strTheme = ProcessUrl(m_url, root->Attribute("theme"), false);
    items.SetProperty("theme", strTheme);
  }

  // Check for dialog message attributes
  CStdString strMessage = "";
  const char* header = root->Attribute("header");
  if (header && strlen(header) > 0)
  {
    const char* message = root->Attribute("message");
    if (message && strlen(message) > 0)
      strMessage = message;

    items.m_displayMessage = true;
    items.m_displayMessageTitle = header;
    items.m_displayMessageContents = strMessage;

    // Don't cache these.
    m_dirCacheType = DIR_CACHE_NEVER;
  }

  // See if this directory replaces the parent.
  const char* replace = root->Attribute("replaceParent");
  if (replace && strcmp(replace, "1") == 0)
    items.SetReplaceListing(true);

  // See if we're saving this into the history or not.
  const char* noHistory = root->Attribute("noHistory");
  if (noHistory && strcmp(noHistory, "1") == 0)
      items.SetSaveInHistory(false);

  // See if we're not supposed to cache this directory.
  const char* noCache = root->Attribute("nocache");
  const char* noCache2 = root->Attribute("noCache");
  
  if ((noCache && strcmp(noCache, "1") == 0) ||
      (noCache2 && strcmp(noCache2, "1") == 0))
    m_dirCacheType = DIR_CACHE_NEVER;

  // See if the directory should be automatically refreshed
  const char* autoRefresh = root->Attribute("autoRefresh");
  if (autoRefresh && strlen(autoRefresh) > 0)
  {
    items.SetAutoRefresh(boost::lexical_cast<int>(autoRefresh));

    // Don't cache the directory if it's going to be autorefreshed.
    m_dirCacheType = DIR_CACHE_NEVER;
  }

  const char *pVal = root->Attribute("size");
  if (pVal && *pVal != 0)
    items.SetProperty("size", atoi(pVal));

  pVal = root->Attribute("totalSize");
  if (pVal && *pVal != 0)
    items.SetProperty("totalSize", atoi(pVal));

  pVal = root->Attribute("offset");
  if (pVal && *pVal != 0)
    items.SetProperty("offset", atoi(pVal));

  if (IsHomeVideoSection(strPath))
    items.SetProperty("HomeVideoSection", true);

#if 0
  CLog::Log(LOGDEBUG, "**** PROPERTY DUMP OF %s ***", strPath.c_str());
  PropertyMap pMap = items.GetPropertyDict();
  BOOST_FOREACH(PropertyMap::value_type &val, pMap)
  {
    CLog::Log(LOGDEBUG, " * %s = %s", val.first.c_str(), val.second.c_str());
  }
  for (int i = 0; i < items.Size(); i ++)
  {
    CFileItemPtr item = items.Get(i);

    CLog::Log(LOGDEBUG, "   > %s", item->GetPath().c_str());

    PropertyMap pMap = item->GetPropertyDict();
    BOOST_FOREACH(PropertyMap::value_type &val, pMap)
    {
      CLog::Log(LOGDEBUG, "     * %s = %s", val.first.c_str(), val.second.c_str());
    }
  }
#endif

  return true;
}

string CPlexDirectory::BuildImageURL(const string& parentURL, const string& imageURL, bool local)
{
  // This is a poster, fanart, or banner. Let's send it through the transcoder
  // so that the media server downloads the original. The cache for these don't
  // expire because if they change, the URL will change.
  //
  CStdString encodedUrl = imageURL;
  CURL mediaUrl(encodedUrl);
  CURL url(parentURL);

  CStdString token;
  map<CStdString, CStdString> options;
  url.GetOptions(options);
  if (options.find("X-Plex-Token") != options.end())
  {
    token = "&X-Plex-Token=" + options["X-Plex-Token"];
    /* Don't include the token in the cache key */
    mediaUrl.RemoveOption("X-Plex-Token");
  }

  // If the image is on the same host as our transcoder, rewrite the URL to be local.
  if (mediaUrl.GetHostName() == url.GetHostName())
  {
    mediaUrl.SetHostName("127.0.0.1");
    mediaUrl.SetPort(32400);
  }

  if (mediaUrl.GetProtocol() == "https")
    mediaUrl.SetProtocol("http");

  encodedUrl = mediaUrl.Get();
  CURL::Encode(encodedUrl);

  // Pick the sizes.
  CStdString width = "1280";
  CStdString height = "720";

  if (strstr(imageURL.c_str(), "poster") || strstr(imageURL.c_str(), "thumb"))
  {
    width = boost::lexical_cast<string>(g_advancedSettings.GetThumbSize());
    height = width;
  }
  else if (strstr(imageURL.c_str(), "banner"))
  {
    width = "800";
    height = "200";
  }
  else if (strstr(imageURL.c_str(), "system"))
  {
    width = "0";
    height = "0";
  }
  
  if (url.GetProtocol() == "plex")
  {
    url.SetProtocol("http");
    url.SetPort(32400);
  }
  
  if (url.GetHostName() == "my.plexapp.com")
  {
    PlexServerPtr server = PlexServerManager::Get().bestServer();
    if (server)
    {
      url.SetHostName(server->address);
      url.SetPort(server->port);
      /* FIXME */
      url.SetProtocol("http");
    }
  }
  
  url.SetOptions("");
  url.SetFileName("photo/:/transcode?width=" + width + "&height=" + height + "&url=" + encodedUrl + token);
  return url.Get();
}

class PlexMediaNode
{
 public:
   static PlexMediaNode* Create(TiXmlElement* element);

   virtual ~PlexMediaNode() {}

   virtual bool needKey() { return true; }
  
   CFileItemPtr BuildFileItem(const CURL& url, TiXmlElement& el, bool localServer)
   {
     CStdString parentPath = url.Get();

     CFileItemPtr pItem(new CFileItem());
     pItem->m_bIsFolder = true;

     // Compute the new path.
     const char* key = el.Attribute("key");
     if (key == 0)
       key = "";
     
     pItem->SetPath(CPlexDirectory::ProcessUrl(parentPath, key, true));

     // Let subclass finish.
     DoBuildFileItem(pItem, string(parentPath), el);

     // If we don't a key *or* media items, get out.
     if (needKey() && (strlen(key)== 0 && pItem->m_mediaItems.empty() == true))
       return CFileItemPtr();
     
     // Parent path.
     if (el.Attribute("parentKey"))
       pItem->SetProperty("parentPath", CPlexDirectory::ProcessUrl(parentPath, el.Attribute("parentKey"), true));
     
     // Sort label.
     if (el.Attribute("titleSort"))
       pItem->SetSortLabel(CStdStringW(el.Attribute("titleSort")));
     else
       pItem->SetSortLabel(CStdStringW(pItem->GetLabel()));

     // Set the key.
     pItem->SetProperty("unprocessedKey", key);
     pItem->SetProperty("key", CPlexDirectory::ProcessUrl(parentPath, key, false));

     // Make sure it's set on all "media item" children.
     BOOST_FOREACH(CFileItemPtr mediaItem, pItem->m_mediaItems)
     {
       mediaItem->SetProperty("unprocessedKey", pItem->GetProperty("unprocessedKey"));
       mediaItem->SetProperty("key", pItem->GetProperty("key"));
     }
     
     // Source title and owned.
     SetProperty(pItem, el, "sourceTitle");
     SetProperty(pItem, el, "owned");
     SetProperty(pItem, el, "recommender");

     // Date.
     SetProperty(pItem, el, "subtitle");

     // Ancestry.
     SetProperty(pItem, el, "parentTitle");
     SetProperty(pItem, el, "grandparentTitle");

     // Bitrate.
     const char* bitrate = el.Attribute("bitrate");
     if (bitrate && strlen(bitrate) > 0)
       pItem->SetBitrate(boost::lexical_cast<int>(bitrate));

     // View offset.
     if (pItem->IsRemoteSharedPlexMediaServerLibrary() == false)
       SetProperty(pItem, el, "viewOffset");

     const char* label2;
     label2 = el.Attribute("infolabel");
     if (label2 && strlen(label2) > 0)
     {
       pItem->SetLabel2(label2);
       pItem->SetLabelPreformated(true);
     }

     // The type of the media.
     if (el.Attribute("type"))
     {
       pItem->SetProperty("mediaType::" + string(el.Attribute("type")), "1");
       pItem->SetProperty("type", el.Attribute("type"));
       pItem->SetProperty("typeNumber", TypeStringToNumber(el.Attribute("type")));
     }

     try
     {
       // Thumb.
       string strThumb = CPlexDirectory::ProcessMediaElement(parentPath, el.Attribute("thumb"), MAX_THUMBNAIL_AGE, localServer);
       if (strThumb.size() > 0)
         pItem->SetArt(PLEX_ART_THUMB, strThumb);

       // Multiple thumbs?
       int i=0;
       for (TiXmlElement* thumbEl = el.FirstChildElement("Thumb"); thumbEl; thumbEl=thumbEl->NextSiblingElement("Thumb"))
         pItem->SetArt(PLEX_ART_THUMB, i++, thumbEl->Attribute("key"));

       // Grandparent thumb.
       string strGrandparentThumb = CPlexDirectory::ProcessMediaElement(parentPath, el.Attribute("grandparentThumb"), MAX_THUMBNAIL_AGE, localServer);
       if (strGrandparentThumb.size() > 0)
         pItem->SetArt(PLEX_ART_TVSHOW_THUMB, strGrandparentThumb);
       
       // Fanart.
       string strArt = CPlexDirectory::ProcessMediaElement(parentPath, el.Attribute("art"), MAX_FANART_AGE, localServer);
       if (strArt.size() > 0)
         pItem->SetArt(PLEX_ART_FANART, strArt);

       // Banner.
       string strBanner = CPlexDirectory::ProcessMediaElement(parentPath, el.Attribute("banner"), MAX_FANART_AGE, localServer);
       if (strBanner.size() > 0)
         pItem->SetArt(PLEX_ART_BANNER, strBanner);

       // Theme music.
       string strTheme = CPlexDirectory::ProcessMediaElement(parentPath, el.Attribute("theme"), MAX_FANART_AGE, localServer);
       if (strTheme.size() > 0)
         pItem->SetProperty("theme", strTheme);
     }
     catch (...)
     {
       printf("ERROR: Exception setting directory thumbnail.\n");
     }

     // Make sure we have the trailing slash.
     if (pItem->m_bIsFolder == true && pItem->GetPath()[pItem->GetPath().size()-1] != '/' && pItem->GetPath().find("?") == string::npos)
       pItem->SetPath(pItem->GetPath() + "/");

     // Set up the context menu
     for (TiXmlElement* element = el.FirstChildElement(); element; element=element->NextSiblingElement())
     {
       string name = element->Value();
       if (name == "ContextMenu")
       {
         const char* includeStandardItems = element->Attribute("includeStandardItems");
         if (includeStandardItems && strcmp(includeStandardItems, "0") == 0)
           pItem->SetIncludeStandardContextItems(false);

         PlexMediaNode* contextNode = 0;
         for (TiXmlElement* contextElement = element->FirstChildElement(); contextElement; contextElement=contextElement->NextSiblingElement())
         {
           contextNode = PlexMediaNode::Create(contextElement);
           if (contextNode != 0)
           {
             CFileItemPtr contextItem = contextNode->BuildFileItem(url, *contextElement, false);
             if (contextItem)
             {
               pItem->m_contextItems.push_back(contextItem);
             }
             
             delete contextNode;
             contextNode = 0;
           }
         }
       }
     }

     // Ratings.
     SetProperty(pItem, el, "userRating");

     const char* ratingKey = el.Attribute("ratingKey");
     if (ratingKey && strlen(ratingKey) > 0)
     {
       pItem->SetProperty("ratingKey", ratingKey);

       // Build the root URL.
       CURL theURL(parentPath);
       theURL.SetFileName("library/metadata/" + string(ratingKey));
       pItem->SetProperty("rootKey", theURL.Get());
     }

     const char* rating = el.Attribute("rating");
     if (rating && strlen(rating) > 0)
       pItem->SetProperty("rating", atof(rating));

     // Content rating.
     SetProperty(pItem, el, "contentRating");

     // Dates.
     const char* originallyAvailableAt = el.Attribute("originallyAvailableAt");
     if (originallyAvailableAt)
     {
       char date[128];
       tm t;
       memset(&t, 0, sizeof(t));

       string strDate = originallyAvailableAt;
       size_t space = strDate.find(" ");
       if (space != string::npos)
         strDate = strDate.substr(0, space-1);         
       
       vector<string> parts;
       boost::split(parts, strDate, boost::is_any_of("-"));
       if (parts.size() == 3)
       {
         t.tm_year = boost::lexical_cast<int>(parts[0]) - 1900;
         t.tm_mon  = boost::lexical_cast<int>(parts[1]) - 1;
         t.tm_mday = boost::lexical_cast<int>(parts[2]);

         strftime(date, 128, "%b %d, %Y", &t);
         pItem->SetProperty("originallyAvailableAt", date);
       }
     }

     // Extra attributes for prefixes.
     const char* share = el.Attribute("share");
     if (share && strcmp(share, "1") == 0)
       pItem->SetProperty("share", true);

     const char* hasPrefs = el.Attribute("hasPrefs");
     if (hasPrefs && strcmp(hasPrefs, "1") == 0)
       pItem->SetProperty("hasPrefs", true);

     const char* hasStoreServices = el.Attribute("hasStoreServices");
     if (hasStoreServices && strcmp(hasStoreServices, "1") > 0)
       pItem->SetProperty("hasStoreServices", true);

     const char* pluginIdentifier = el.Attribute("identifier");
     if (pluginIdentifier && strlen(pluginIdentifier) > 0)
       pItem->SetProperty("pluginIdentifier", pluginIdentifier);

     // Build the URLs for the flags.
     TiXmlElement* parent = (TiXmlElement* )el.Parent();
     const char* pRoot = parent->Attribute("mediaTagPrefix");
     const char* pVersion = parent->Attribute("mediaTagVersion");
     if (pRoot && pVersion)
     {
       string url = CPlexDirectory::ProcessUrl(parentPath, pRoot, false);
       CacheMediaThumb(pItem, &el, url, "contentRating", pVersion);
       CacheMediaThumb(pItem, &el, url, "studio", pVersion);
     }


     return pItem;
   }

   void CacheMediaThumb(const CFileItemPtr& mediaItem, TiXmlElement* el, const string& baseURL, const string& resource, const string& version, const string& attrName="")
   {
     const char* val = el->Attribute(resource.c_str());
     if (val && strlen(val) > 0)
     {
       CStdString attr = resource;
       if (attrName.size() > 0)
         attr = attrName;

       CStdString encodedValue = val;
       CURL::Encode(encodedValue);

       // Complete the URL.
       if (baseURL.empty() == false && version.empty() == false)
       {
         CURL theURL(baseURL);
         if (boost::ends_with(theURL.GetFileName(), "/") == false)
           theURL.SetFileName(theURL.GetFileName() + "/");
           
         theURL.SetFileName(theURL.GetFileName() + attr + "/" + encodedValue);
         
         if (theURL.GetOptions().empty())
           theURL.SetOptions("?t=" + version);
         else
           theURL.SetOptions("?t=" + version + "&" + theURL.GetOptions().substr(1));
       
         string url = CPlexDirectory::BuildImageURL(baseURL, theURL.Get(), true);
         
         /* FIXME: attr here might be wrong? */
         mediaItem->SetArt("mediaTag::" + attr, url);
         
         /* Cache this in the bg */
         if(resource != "studio")
         {
           if (!CTextureCache::Get().HasCachedImage(url))
             CTextureCache::Get().BackgroundCacheImage(url);
         }
       }

       string value = val;

       if (attr == "contentRating")
       {
         // Set the value, taking off any ".../" subdirectory.
         string::size_type slash = value.find("/");
         if (slash != string::npos)
           value = value.substr(slash+1);
       }

       mediaItem->SetProperty("mediaTag-" + attr, value);
     }
   }

   virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el) = 0;
   virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strSecondFileLabel, string& strDirLabel, string& strSecondDirLabel)
   {
     strFileLabel = "%N - %T";
     strDirLabel = "%B";
     strSecondDirLabel = "%Y";

     if (strPath.find("/Albums") != string::npos)
       strDirLabel = "%B - %A";
     else if (strPath.find("/Recently%20Played/By%20Artist") != string::npos || strPath.find("/Most%20Played/By%20Artist") != string::npos)
       strDirLabel = "%B";
     else if (strPath.find("/Decades/") != string::npos || strPath.find("/Recently%20Added") != string::npos || strPath.find("/Most%20Played") != string::npos || strPath.find("/Recently%20Played/") != string::npos || strPath.find("/Genre/") != string::npos)
       strDirLabel = "%A - %B";
   }

   string GetLabel(const TiXmlElement& el)
   {
     // FIXME: We weren't consistent, so no we need to accept multiple ones until
     // we release this and update the framework.
     //
     if (el.Attribute("title"))
       return el.Attribute("title");
     else if (el.Attribute("label"))
       return el.Attribute("label");
     else if (el.Attribute("name"))
       return el.Attribute("name");
     else if (el.Attribute("track"))
       return el.Attribute("track");

     return "";
   }

   void SetPropertyForceInt(const CFileItemPtr& item, const TiXmlElement& el, const string& attrName)
   {
     const char* pVal = el.Attribute(attrName.c_str());
     if (pVal && *pVal != 0)
       item->SetProperty(attrName, atoi(pVal));
   }

   void SetProperty(const CFileItemPtr& item, const TiXmlElement& el, const string& attrName, const string& propertyName="")
   {
     string propName = attrName;
     if (propertyName.size() > 0)
       propName = propertyName;

     const char* pVal = el.Attribute(attrName.c_str());
     if (pVal && *pVal != 0)
       item->SetProperty(propName, pVal);
   }

   void SetValue(const TiXmlElement& el, CStdString& value, const char* name)
   {
     const char* pVal = el.Attribute(name);
     if (pVal && *pVal != 0)
       value = pVal;
   }

   void SetValue(const TiXmlElement& el, int& value, const char* name)
   {
     const char* pVal = el.Attribute(name);
     if (pVal && *pVal != 0)
       value = boost::lexical_cast<int>(pVal);
   }

   void SetValue(const TiXmlElement& el, const TiXmlElement& parentEl, CStdString& value, const char* name)
   {
     const char* val = el.Attribute(name);
     const char* valParent = parentEl.Attribute(name);

     if (val && *val != 0)
       value = val;
     else if (valParent && *valParent)
       value = valParent;
   }

   void SetValue(const TiXmlElement& el, const TiXmlElement& parentEl, int& value, const char* name)
   {
     const char* val = el.Attribute(name);
     const char* valParent = parentEl.Attribute(name);

     if (val && *val != 0)
       value = boost::lexical_cast<int>(val);
     else if (valParent && *valParent)
       value = boost::lexical_cast<int>(valParent);
   }

   void SetPropertyValue(const TiXmlElement& el, CFileItemPtr& item, const string& propName, const char* attrName)
   {
     const char* pVal = el.Attribute(attrName);
     if (pVal && *pVal != 0)
       item->SetProperty(propName, pVal);
   }

  int TypeStringToNumber(const CStdString& type)
  {
    if (type == "show")
      return PLEX_METADATA_SHOW;
    else if (type == "episode")
      return PLEX_METADATA_EPISODE;
    else if (type == "movie")
      return PLEX_METADATA_MOVIE;
    else if (type == "artist")
      return PLEX_METADATA_ARTIST;
    else if (type == "album")
      return PLEX_METADATA_ALBUM;
    else if (type == "track")
      return PLEX_METADATA_TRACK;
    else if (type == "clip")
      return PLEX_METADATA_CLIP;
    else if (type == "person")
      return PLEX_METADATA_PERSON;
    else if (type == "mixed")
      return PLEX_METADATA_MIXED;
    
    return -1;
  }
};

class PlexMediaNodeLibrary : public PlexMediaNode
{
 public:

  string ComputeMediaUrl(const CFileItemPtr& pItem, const string& parentPath, TiXmlElement* media, string& localPath, vector<PlexMediaPartPtr>& mediaParts)
  {
    string ret;

    vector<string> urls;
    vector<string> localPaths;

    // Collect the URLs.
    int partIndex = 0;
    for (TiXmlElement* part = media->FirstChildElement(); part; part=part->NextSiblingElement(), partIndex++)
    {
      if (part->Attribute("key"))
      {
        string url = CPlexDirectory::ProcessUrl(parentPath, part->Attribute("key"), false);
        urls.push_back(url);

        CStdString path = part->Attribute("file");
        CURL::Decode(path);
        localPaths.push_back(path);
      }

      if (part->Attribute("postURL"))
        pItem->SetProperty("postURL", part->Attribute("postURL"));
      
      if (part->Attribute("postHeaders"))
        pItem->SetProperty("postHeaders", part->Attribute("postHeaders"));
      
      // Create a media part.
      int partID = 0;
      if (part->Attribute("id"))
        partID = boost::lexical_cast<int>(part->Attribute("id"));

      int duration = 0;
      if (part->Attribute("duration") && strlen(part->Attribute("duration")) > 0)
      {
        duration = boost::lexical_cast<int>(part->Attribute("duration"));
        pItem->SetProperty("part::duration::" + boost::lexical_cast<string>(partIndex), duration);
      }
      
      if (part->Attribute("accessible"))
        pItem->SetProperty("accessible", part->Attribute("accessible"));

      if (part->Attribute("exists"))
        pItem->SetProperty("exists", part->Attribute("exists"));

      if (part->Attribute("size"))
      {
        uint64_t fileSize = boost::lexical_cast<uint64_t>(part->Attribute("size"));
        pItem->SetProperty("fileSize", (uint64_t)fileSize);
      }
      
      PlexMediaPartPtr mediaPart(new PlexMediaPart(partID, part->Attribute("key"), duration));
      mediaParts.push_back(mediaPart);

      // Parse the streams.
      for (TiXmlElement* stream = part->FirstChildElement(); stream; stream=stream->NextSiblingElement())
      {
        int id = boost::lexical_cast<int>(stream->Attribute("id"));
        int streamType = boost::lexical_cast<int>(stream->Attribute("streamType"));
        int index = -1;
        int subIndex = -1;
        bool selected = false;

        if (stream->Attribute("index"))
          index = boost::lexical_cast<int>(stream->Attribute("index"));

        if (stream->Attribute("subIndex"))
          subIndex = boost::lexical_cast<int>(stream->Attribute("subIndex"));
        
        if (stream->Attribute("selected") && strcmp(stream->Attribute("selected"), "1") == 0)
          selected = true;

        string language;
        if (stream->Attribute("language"))
          language = stream->Attribute("language");

        string key;
        if (stream->Attribute("key"))
          key = CPlexDirectory::ProcessUrl(parentPath, stream->Attribute("key"), false);

        string codec;
        if (stream->Attribute("codec"))
          codec = stream->Attribute("codec");

        PlexMediaStreamPtr mediaStream(new PlexMediaStream(id, key, streamType, codec, index, subIndex, selected, language));
        mediaPart->mediaStreams.push_back(mediaStream);
      }
    }

    if (urls.size() > 0)
    {
      // See if we need a stack or not.
      ret = urls[0];
      localPath = localPaths[0];

      if (urls.size() > 1 && boost::iequals(URIUtils::GetExtension(urls[0]), ".ifo") == false)
      {
        ret = localPath = "stack://";

        for (unsigned int i=0; i<urls.size(); i++)
        {
          ret += urls[i];
          localPath += localPaths[i];

          if (i < urls.size()-1)
          {
            ret += " , ";
            localPath += " , ";
          }
        }
      }
    }

    return ret;
  }

  void DoBuildTrackItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    TiXmlElement* parent = (TiXmlElement* )el.Parent();
    CSong song;

    // Artist, album, year.
    CStdString artist;
    SetValue(el, *parent, artist, "grandparentTitle");
    song.artist.push_back(artist);

    SetValue(el, *parent, song.strAlbum, "parentTitle");
    SetValue(el, *parent, song.iYear, "parentYear");

    song.strTitle = GetLabel(el);

    // Duration.
    SetValue(el, song.iDuration, "duration");
    song.iDuration /= 1000;

    // Track number.
    SetValue(el, song.iTrack, "index");

    // Media tags.
    const char* pRoot = parent->Attribute("mediaTagPrefix");
    const char* pVersion = parent->Attribute("mediaTagVersion");

    vector<CFileItemPtr> mediaItems;
    for (TiXmlElement* media = el.FirstChildElement(); media; media=media->NextSiblingElement())
    {
      if (media->ValueStr() == "Media")
      {
        // Replace the item.
        CFileItemPtr newItem(new CFileItem(song));
        newItem->SetPath(pItem->GetPath());
        pItem = newItem;

        // Check for indirect.
        if (media->Attribute("indirect") && strcmp(media->Attribute("indirect"), "1") == 0)
          pItem->SetProperty("indirect", 1);
        
        const char* bitrate = el.Attribute("bitrate");
        if (bitrate && strlen(bitrate) > 0)
        {
          pItem->m_dwSize = boost::lexical_cast<int>(bitrate);
          pItem->GetMusicInfoTag()->SetAlbum(bitrate);
        }

        // Summary.
        SetPropertyValue(*parent, pItem, "description", "summary");

        // Path to the track.
        string localPath;
        vector<PlexMediaPartPtr> mediaParts;
        string url = ComputeMediaUrl(pItem, parentPath, media, localPath, mediaParts);
        pItem->SetPath(url);
        song.strFileName = pItem->GetPath();

        if (pRoot && pVersion)
        {
          string url = CPlexDirectory::ProcessUrl(parentPath, pRoot, false);
          CacheMediaThumb(pItem, media, url, "audioChannels", pVersion);
          CacheMediaThumb(pItem, media, url, "audioCodec", pVersion);
          CacheMediaThumb(pItem, &el, url, "contentRating", pVersion);
          CacheMediaThumb(pItem, &el, url, "studio", pVersion);
        }

        // We're done.
        break;
      }
    }

    pItem->m_bIsFolder = false;

    // Summary.
    SetPropertyValue(el, pItem, "description", "summary");
  }

  virtual void DoBuildVideoFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    string type = el.Attribute("type");

    // Top level data.
    CVideoInfoTag videoInfo;
    SetValue(el, videoInfo.m_strTitle, "title");
    SetValue(el, videoInfo.m_strOriginalTitle, "originalTitle");
    videoInfo.m_strPlot = videoInfo.m_strPlotOutline = el.Attribute("summary");
    SetValue(el, videoInfo.m_iYear, "year");

    // Show, season.
    SetValue(el, *(TiXmlElement* )el.Parent(), videoInfo.m_strShowTitle, "grandparentTitle");
    
    // Indexes.
    if (type == "episode")
    {
      SetValue(el, videoInfo.m_iEpisode, "index");
      SetValue(el, *((TiXmlElement* )el.Parent()), videoInfo.m_iSeason, "parentIndex");
    }
    else if (type == "season")
    {
      SetValue(el, *(TiXmlElement* )el.Parent(), videoInfo.m_strShowTitle, "parentTitle");
      SetValue(el, *((TiXmlElement* )el.Parent()), videoInfo.m_iSeason, "index");
    }
    else if (type == "show")
    {
      videoInfo.m_strShowTitle = videoInfo.m_strTitle;
    }

    // Tagline.
    SetValue(el, videoInfo.m_strTagLine, "tagline");

    // Views.
    int viewCount = 0;

    if (pItem->IsRemoteSharedPlexMediaServerLibrary() == false)
      SetValue(el, viewCount, "viewCount");

    vector<CFileItemPtr> mediaItems;
    for (TiXmlElement* media = el.FirstChildElement(); media; media=media->NextSiblingElement())
    {
      if (media->ValueStr() == "Media")
      {
        // Create a new file item.
        CVideoInfoTag theVideoInfo = videoInfo;

        // Compute the URL.
        string localPath;
        vector<PlexMediaPartPtr> mediaParts;
        string url = ComputeMediaUrl(pItem, parentPath, media, localPath, mediaParts);
        videoInfo.m_strFile = url;
        videoInfo.m_strFileNameAndPath = url;

        // Duration.
        const char* pDuration = el.Attribute("duration");
        if (pDuration && strlen(pDuration) > 0)
          theVideoInfo.m_duration = atoi(pDuration) / 1000;

        // Viewed.
        theVideoInfo.m_playCount = viewCount;

        // Create the file item.
        CFileItemPtr theMediaItem(new CFileItem(theVideoInfo));
        theMediaItem->m_mediaParts = mediaParts;

        if (pDuration)
          theMediaItem->SetProperty("duration", pDuration);
        
        // Check for indirect.
        if (media->Attribute("indirect") && strcmp(media->Attribute("indirect"), "1") == 0)
        {
          pItem->SetProperty("indirect", 1);
          theMediaItem->SetProperty("indirect", 1);
          
          if (pItem->HasProperty("postURL"))
            theMediaItem->SetProperty("postURL", pItem->GetProperty("postURL"));
          if (pItem->HasProperty("postHeaders"))
            theMediaItem->SetProperty("postHeaders", pItem->GetProperty("postHeaders"));
        }
        
        if (pItem->HasProperty("accessible"))
          theMediaItem->SetProperty("accessible", pItem->GetProperty("accessible"));
        
        if (pItem->HasProperty("exists"))
          theMediaItem->SetProperty("exists", pItem->GetProperty("exists"));

        if (pItem->HasProperty("fileSize"))
          theMediaItem->SetProperty("fileSize", pItem->GetProperty("fileSize"));

        // If it's not an STRM file then save the local path.
        if (URIUtils::GetExtension(localPath) != ".strm")
          theMediaItem->SetProperty("localPath", localPath);

        theMediaItem->m_bIsFolder = false;
        theMediaItem->SetPath(url);

        // See if the item has been played or is in progress. Don't do this for remote PMS libraries.
        if (theMediaItem->IsRemoteSharedPlexMediaServerLibrary() == false)
        {
          if (el.Attribute("viewOffset") != 0 && strlen(el.Attribute("viewOffset")) > 0)
            theMediaItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);
          else
            theMediaItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, viewCount > 0);
        }
        
        // Bitrate.
        int br;
        SetValue(*media, br, "bitrate");
        theMediaItem->SetBitrate(br);

        // Build the URLs for the flags.
        TiXmlElement* parent = (TiXmlElement* )el.Parent();
        const char* pRoot = parent->Attribute("mediaTagPrefix");
        const char* pVersion = parent->Attribute("mediaTagVersion");
        
        {
          string url = pRoot ? CPlexDirectory::ProcessUrl(parentPath, pRoot, false) : "";
          string version = pVersion ? pVersion : ""; 
          
          CacheMediaThumb(theMediaItem, media, url, "aspectRatio", version);
          CacheMediaThumb(theMediaItem, media, url, "audioChannels", version);
          CacheMediaThumb(theMediaItem, media, url, "audioCodec", version);
          CacheMediaThumb(theMediaItem, media, url, "videoCodec", version);
          CacheMediaThumb(theMediaItem, media, url, "videoResolution", version);
          CacheMediaThumb(theMediaItem, media, url, "videoFrameRate", version);
          
          // From the metadata item.
          if (el.Attribute("contentRating"))
            CacheMediaThumb(theMediaItem, &el, url, "contentRating", version);
          else
            CacheMediaThumb(theMediaItem, parent, url, "grandparentContentRating", version, "contentRating");
          
          if (el.Attribute("studio"))
            CacheMediaThumb(theMediaItem, &el, url, "studio", version);
          else
            CacheMediaThumb(theMediaItem, parent, url, "grandparentStudio", version, "studio");

        }
          
        // But we add each one to the list.
        mediaItems.push_back(theMediaItem);
      }
    }

    pItem = mediaItems[0];
    pItem->m_mediaItems = mediaItems;
  }

  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    string type = el.Attribute("type");

    // Check for track.
    if (type == "track")
      DoBuildTrackItem(pItem, parentPath, el);
    else
      DoBuildVideoFileItem(pItem, parentPath, el);
  }

};

class PlexMediaDirectory : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    string type;
    if (el.Attribute("type"))
    {
      type = el.Attribute("type");
      pItem->SetProperty("type", type);
    }

    pItem->SetLabel(GetLabel(el));

    // Video.
    CVideoInfoTag tag;
    tag.m_strTitle = pItem->GetLabel();
    tag.m_strShowTitle = tag.m_strTitle;

    // Summary.
    const char* summary = el.Attribute("summary");
    if (summary)
    {
      pItem->SetProperty("description", summary);
      tag.m_strPlot = tag.m_strPlotOutline = summary;
    }

    // Studio.
    if (el.Attribute("studio"))
      tag.m_studio.push_back(el.Attribute("studio"));

    // Year.
    if (el.Attribute("year"))
      tag.m_iYear = boost::lexical_cast<int>(el.Attribute("year"));

    // Duration.
    if (el.Attribute("duration"))
      tag.m_duration = atoi(el.Attribute("duration")) / 1000;

    CFileItemPtr newItem(new CFileItem(tag));
    newItem->m_bIsFolder = true;
    newItem->SetPath(pItem->GetPath());
    newItem->SetProperty("description", pItem->GetProperty("description"));
    pItem = newItem;

    // Watch/unwatched.
    if (el.Attribute("leafCount") && el.Attribute("viewedLeafCount"))
    {
      int count = boost::lexical_cast<int>(el.Attribute("leafCount"));
      int watchedCount = boost::lexical_cast<int>(el.Attribute("viewedLeafCount"));

      // If we're browsing through a shared library, it's all unwatched.
      if (pItem->IsRemoteSharedPlexMediaServerLibrary() == true)
        watchedCount = 0;
      
      pItem->SetEpisodeData(count, watchedCount);
      pItem->GetVideoInfoTag()->m_iEpisode = count;
      pItem->GetVideoInfoTag()->m_playCount = (count == watchedCount) ? 1 : 0;
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pItem->GetVideoInfoTag()->m_playCount > 0);
    }

    // Music.
    if (type == "artist")
    {
      pItem->SetProperty("artist", pItem->GetLabel());
    }
    else if (type == "album")
    {
      pItem->SetProperty("album", pItem->GetLabel());
      if (el.Attribute("year"))
        pItem->SetLabel2(el.Attribute("year"));
      
      CStdString artist;
      SetValue(el, *(TiXmlElement* )el.Parent(), artist, "parentTitle");
      pItem->SetProperty("artist", artist);
    }
    else if (type == "track")
    {
      if (el.Attribute("index"))
        pItem->SetProperty("index", el.Attribute("index"));
    }

    // Check for special directories.
    const char* search = el.Attribute("search");
    const char* prompt = el.Attribute("prompt");
    const char* settings = el.Attribute("settings");

    // Check for search directory.
    if (search && strlen(search) > 0 && prompt)
    {
      string strSearch = search;
      if (strSearch == "1")
      {
        pItem->SetIsSearchDir(true);
        pItem->SetSearchPrompt(prompt);
      }
    }

    // Check for popup menus.
    const char* popup = el.Attribute("popup");
    if (popup && strlen(popup) > 0)
    {
      string strPopup = popup;
      if (strPopup == "1")
        pItem->SetIsPopupMenuItem(true);
    }

    // Check for preferences.
    if (settings && strlen(settings) > 0)
    {
      string strSettings = settings;
      if (strSettings == "1")
        pItem->SetIsSettingsDir(true);
    }
    
    // Sections.
    SetProperty(pItem, el, "machineIdentifier");
    SetProperty(pItem, el, "owned");
    SetProperty(pItem, el, "accessToken");
    SetProperty(pItem, el, "serverName");
    SetProperty(pItem, el, "sourceTitle");
    SetProperty(pItem, el, "address");
    SetProperty(pItem, el, "port");

    /* Filter stuff */
    SetProperty(pItem, el, "filterType");
    SetProperty(pItem, el, "filter");

    /* We need to track these sections, ugly as it might be */
    SetProperty(pItem, el, "agent");
    if (pItem->HasProperty("agent") &&
        pItem->GetProperty("agent").asString() == "com.plexapp.agents.none")
    {
      CPlexDirectory::AddHomeVideoSection(CPlexDirectory::ProcessUrl(parentPath, el.Attribute("key"), true));
    }

  }
};

class PlexMediaObject : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el) {}
};

class PlexMediaArtist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("artist"));
  }
};

class PlexMediaAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    CAlbum album;

    album.strLabel = GetLabel(el);
    //album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("album");
    album.artist.push_back(el.Attribute("artist"));
    album.genre.push_back(el.Attribute("genre"));
    album.iYear = boost::lexical_cast<int>(el.Attribute("year"));

    CFileItemPtr newItem(new CFileItem(pItem->GetPath(), album));
    pItem = newItem;
  }
};

class PlexMediaPodcast : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    CAlbum album;

    album.strLabel = el.Attribute("title");
    album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("title");

    if (strlen(el.Attribute("year")) > 0)
      album.iYear = boost::lexical_cast<int>(el.Attribute("year"));

    CFileItemPtr newItem(new CFileItem(pItem->GetPath(), album));
    pItem = newItem;
  }
};

class PlexMediaPlaylist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("title"));
  }
};

class PlexMediaGenre : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("genre"));
  }
};

class PlexMediaProvider : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetPath(CPlexDirectory::ProcessUrl(parentPath, el.Attribute("key"), false));
    pItem->SetLabel(el.Attribute("title"));
    pItem->SetProperty("type", el.Attribute("type"));
    pItem->SetProperty("provider", "1");
  }
};

class PlexMediaVideo : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;

    CURL parentURL(parentPath);
    CVideoInfoTag videoInfo;
    videoInfo.m_strTitle = el.Attribute("title");
    videoInfo.m_strPlot = videoInfo.m_strPlotOutline = el.Attribute("summary");

    const char* year = el.Attribute("year");
    if (year && strlen(year) > 0)
      videoInfo.m_iYear = boost::lexical_cast<int>(year);

    const char* pDuration = el.Attribute("duration");
    if (pDuration && strlen(pDuration) > 0)
      videoInfo.m_duration = atoi(pDuration) / 1000;

    // Path to the track itself.
    CURL url2(pItem->GetPath());
    if (url2.GetProtocol() == "plex" && url2.GetFileName().Find("video/:/") == -1)
    {
      url2.SetProtocol("http");
      url2.SetPort(32400);
      pItem->SetPath(url2.Get());
    }

    videoInfo.m_strFile = pItem->GetPath();
    videoInfo.m_strFileNameAndPath = pItem->GetPath();

    CFileItemPtr newItem(new CFileItem(videoInfo));
    newItem->m_bIsFolder = false;
    newItem->SetPath(pItem->GetPath());
    pItem = newItem;

    // Support for specifying RTMP play paths.
    const char* pPlayPath = el.Attribute("rtmpPlayPath");
    if (pPlayPath && strlen(pPlayPath) > 0)
      pItem->SetProperty("PlayPath", pPlayPath);
  }

  virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strSecondFileLabel, string& strDirLabel, string& strSecondDirLabel)
  {
    strDirLabel = "%B";
    strFileLabel = "%K";
    strSecondDirLabel = "%D";
  }
};

class PlexMediaTrack : public PlexMediaNode
{
 public:
  PlexMediaTrack()
    : m_hasBitrate(false)
    , m_hasDuration(false) {}

 private:
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;

    CSong song;
    song.strTitle = GetLabel(el);
    song.artist.push_back(el.Attribute("artist"));
    song.strAlbum = el.Attribute("album");
    song.strFileName = pItem->GetPath();

    // Old-school.
    const char* totalTime = el.Attribute("totalTime");
    if (totalTime && strlen(totalTime) > 0)
    {
      song.iDuration = boost::lexical_cast<int>(totalTime)/1000;
      m_hasDuration = true;
    }

    // Standard.
    totalTime = el.Attribute("duration");
    if (totalTime && strlen(totalTime) > 0)
      song.iDuration = boost::lexical_cast<int>(totalTime)/1000;

    const char* trackNumber = el.Attribute("index");
    if (trackNumber && strlen(trackNumber) > 0)
      song.iTrack = boost::lexical_cast<int>(trackNumber);

    // Replace the item.
    CFileItemPtr newItem(new CFileItem(song));
    newItem->SetPath(pItem->GetPath());
    pItem = newItem;

    const char* bitrate = el.Attribute("bitrate");
    if (bitrate && strlen(bitrate) > 0)
    {
      pItem->m_dwSize = boost::lexical_cast<int>(bitrate);
      pItem->GetMusicInfoTag()->SetAlbum(bitrate);
      m_hasBitrate = true;
    }

    // Path to the track.
    pItem->SetPath(CPlexDirectory::ProcessUrl(parentPath, el.Attribute("key"), false));

    // Summary.
    const char* summary = el.Attribute("summary");
    if  (summary)
      pItem->SetProperty("description", summary);
  }

  virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strSecondFileLabel, string& strDirLabel, string& strSecondDirLabel)
  {
    strDirLabel = "%B";

    // Use bitrate if it's available and duration is not.
    if (m_hasBitrate == true && m_hasDuration == false)
      strSecondFileLabel = "%B kbps";

    if (strPath.find("/Artists/") != string::npos || strPath.find("/Genre/") != string::npos || strPath.find("/Albums/") != string::npos ||
        strPath.find("/Recently%20Played/By%20Album/") != string::npos || strPath.find("Recently%20Played/By%20Artist") != string::npos ||
        strPath.find("/Recently%20Added/") != string::npos)
      strFileLabel = "%N - %T";
    else if (strPath.find("/Compilations/") != string::npos)
      strFileLabel = "%N - %A - %T";
    else if (strPath.find("/Tracks/") != string::npos)
      strFileLabel = "%T - %A";
    else if (strPath.find("/Podcasts/") != string::npos)
      strFileLabel = "%T";
    else
      strFileLabel = "%A - %T";
  }

 private:

  bool m_hasBitrate;
  bool m_hasDuration;
};

class PlexMediaRoll : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(GetLabel(el));
  }
};

class PlexMediaPhotoAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(GetLabel(el));
  }
};

class PlexMediaPhotoKeyword : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(GetLabel(el));
  }
};

class PlexMediaPhoto : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
    pItem->SetLabel(GetLabel(el));

    // Summary.
    const char* summary = el.Attribute("summary");
    if  (summary)
      pItem->SetProperty("description", summary);

    // Selected.
    const char* selected = el.Attribute("selected");
    if (selected)
      pItem->SetProperty("selected", selected);

    // Path to the photo.
    pItem->SetPath(CPlexDirectory::ProcessUrl(parentPath, el.Attribute("key"), false));
  }
};

class PlexServerNode : public PlexMediaNode
{

  virtual bool needKey() { return false; }

  virtual void DoBuildFileItem(CFileItemPtr &pItem, const string &parentPath, TiXmlElement &el)
  {
    pItem->m_bIsFolder = false;
    pItem->SetLabel(GetLabel(el));

    dprintf("Parsing server node %s", pItem->GetLabel().c_str());

    // Token
    SetProperty(pItem, el, "accessToken");
    SetProperty(pItem, el, "address");
    SetProperty(pItem, el, "version");
    SetProperty(pItem, el, "localAddresses");
    SetProperty(pItem, el, "updatedAt");
    SetProperty(pItem, el, "sourceTitle");

    const char* port = el.Attribute("port");
    if (port)
      pItem->SetProperty("port", atoi(port));

    const char* owned = el.Attribute("owned");
    if (owned)
      pItem->SetProperty("owned", bool(atoi(owned)));

    const char* uuid = el.Attribute("machineIdentifier");
    if (uuid)
      pItem->SetProperty("uuid", uuid);

  }

};

///////////////////////////////////////////////////////////////////////////////////////////////////
PlexMediaNode* PlexMediaNode::Create(TiXmlElement* element)
{
  string name = element->Value();

  // FIXME: Move to using factory pattern.
  if (name == "Directory")
    return new PlexMediaDirectory();
  else if (element->FirstChild("Media") != 0)
    return new PlexMediaNodeLibrary();
  else if (name == "Artist")
    return new PlexMediaArtist();
  else if (name == "Album")
    return new PlexMediaAlbum();
  else if (name == "Playlist")
    return new PlexMediaPlaylist();
  else if (name == "Podcast")
    return new PlexMediaPodcast();
  else if (name == "Track")
    return new PlexMediaTrack();
  else if (name == "Genre")
    return new PlexMediaGenre();
  else if (name == "Roll")
    return new PlexMediaRoll();
  else if (name == "Photo")
    return new PlexMediaPhoto();
  else if (name == "PhotoAlbum")
    return new PlexMediaPhotoAlbum();
  else if (name == "PhotoKeyword")
    return new PlexMediaPhotoKeyword();
  else if (name == "Video")
    return new PlexMediaVideo();
  else if (name == "Provider")
    return new PlexMediaProvider();
  else if (name == "Server")
    return new PlexServerNode();
  else
    printf("ERROR: Unknown class [%s]\n", name.c_str());

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, string& strFileLabel, string& strSecondFileLabel, string& strDirLabel, string& strSecondDirLabel, bool isLocal)
{
  PlexMediaNode* mediaNode = 0;

  bool gotType = false;
  const char* content = root->Attribute("content");
  if (content)
    items.SetContent(CStdString(content));

  for (TiXmlElement* element = root->FirstChildElement(); element; element=element->NextSiblingElement())
  {
    bool hasMediaNodes = element->FirstChild("Media") != 0;
    mediaNode = PlexMediaNode::Create(element);
    if (mediaNode != 0)
    {
      // Get the type.
      const char* pType = element->Attribute("type");
      string type;

      if (pType)
      {
        type = pType;

        if (type == "show")
          type = "tvshows";
        else if (type == "season")
          type = "seasons";
        else if (type == "episode")
          type = "episodes";
        else if (type == "movie")
          type = "movies";
        else if (type == "artist")
          type = "artists";
        else if (type == "album")
          type = "albums";
        else if (type == "track")
          type = "songs";

        // Set the content type for the collection.
        if (gotType == false)
        {
          items.SetContent(type);
          gotType = true;
          items.SetProperty("typeNumber", mediaNode->TypeStringToNumber(pType));
        }
      }

      CFileItemPtr item = mediaNode->BuildFileItem(url, *element, isLocal);
      if (item)
      {
        // Set the content type for the item.
        if (!type.empty())
        {
          item->SetProperty("mediaType", type);
          item->SetProperty("IsSynthesized", !hasMediaNodes);
        }

        // Tags.
        ParseTags(element, item, "Genre");
        ParseTags(element, item, "Writer");
        ParseTags(element, item, "Director");
        ParseTags(element, item, "Role");
        ParseTags(element, item, "Country");

        items.Add(item);
      }
    }
    
    if (element->NextSiblingElement())
      delete mediaNode;
  }

  if (mediaNode != 0)
  {
    CStdString strURL = url.Get();
    mediaNode->ComputeLabels(strURL, strFileLabel, strSecondFileLabel, strDirLabel, strSecondDirLabel);
    delete mediaNode;
  }  
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::ParseTags(TiXmlElement* element, const CFileItemPtr& item, const string& name)
{
  // Tags.
  vector<string> tagList;
  string tagString;

  for (TiXmlElement* child = element->FirstChildElement(); child; child=child->NextSiblingElement())
  {
    if (child->ValueStr() == name)
    {
      string tag = child->Attribute("tag");
      tagList.push_back(tag);
      tagString += tag + ", ";
    }
  }

  if (tagList.size() > 0)
  {
    string tagName = name;
    boost::to_lower(tagName);

    tagString = tagString.substr(0, tagString.size()-2);
    item->SetProperty(tagName, tagString);
    item->SetProperty("first" + name, tagList[0]);
  }
}

#ifndef __APPLE__
extern string Cocoa_GetLanguage();
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Process()
{
  CURL url(m_url);
  CStdString protocol = url.GetProtocol();
  
  if (url.GetProtocol() == "plex")
  {
    url.SetProtocol("http");
    url.SetPort(32400);
  }

  if (m_bStop)
    return;

  // Set request headers.
  m_http.SetRequestHeader("X-Plex-Version", g_infoManager.GetVersion());
  m_http.SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));

  // Only send headers if we're NOT going to the node.
  if (url.Get().find("http://node.plexapp.com") == -1 && url.Get().find("http://nodedev.plexapp.com") == -1)
    m_http.SetRequestHeader("X-Plex-Language", g_langInfo.GetLanguageLocale());

#ifdef __APPLE__
  m_http.SetRequestHeader("X-Plex-Client-Platform", "MacOSX");
#elif defined (_WIN32)
  m_http.SetRequestHeader("X-Plex-Client-Platform", "Windows");
#endif  
  
  // Build a description of what we support.
  CStdString protocols = "protocols=shoutcast,webkit,http-video;videoDecoders=h264{profile:high&resolution:1080&level:51};audioDecoders=mp3,aac";
  
  if (AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")))
  {
    if (g_guiSettings.GetBool("audiooutput.dtspassthrough"))
      protocols += ",dts{bitrate:800000&channels:8}";
    
    if (g_guiSettings.GetBool("audiooutput.ac3passthrough"))
      protocols += ",ac3{bitrate:800000&channels:8}";
  }
  
  m_http.SetRequestHeader("X-Plex-Client-Capabilities", protocols);
  m_http.UseOldHttpVersion(true);
  m_http.SetTimeout(m_timeout);

  if (m_bStop) return;
  
  if (m_body.empty() == false)
    m_bSuccess = m_http.Post(url.Get(), m_body, m_data);
  else
    m_bSuccess = m_http.Get(url.Get(), m_data);
  
  m_downloadEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::CancelDirectory()
{
  m_http.Cancel();
  m_bStop = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
string CPlexDirectory::ProcessMediaElement(const string& parentPath, const char* media, int maxAge, bool local)
{
  if (media && strlen(media) > 0)
  {
    CStdString strMedia = CPlexDirectory::ProcessUrl(parentPath, media, false);

    if (strstr(media, "/library/metadata/"))
    {
      // Build a special URL to get the media from the media server.
      strMedia = CPlexDirectory::BuildImageURL(parentPath, strMedia, local);
    }
    return strMedia;
  }

  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
string CheckAuthToken(map<CStdString, CStdString>& options, string finalURL, const string& parameterName)
{
  if (options.find(parameterName) != options.end())
  {
    if (finalURL.find("?") == string::npos)
      finalURL += "?";
    else
      finalURL += "&";
    
    finalURL += parameterName + "=" + string(options[parameterName]);
  }
  
  return finalURL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
string CPlexDirectory::ProcessUrl(const string& parent, const string& url, bool isDirectory)
{
  string parentPath(parent);

  // If the parent has ? we need to lop off whatever comes after.
  CURL theFullURL(parentPath);
  unsigned int questionMark = parentPath.find('?');
  if (questionMark != string::npos)
    parentPath = parentPath.substr(0, questionMark);

  CURL theURL(parentPath);

  // Files use plain HTTP.
  if (isDirectory == false)
  {
    if (theURL.GetProtocol() == "plex")
    {
      theURL.SetProtocol("http");
      theURL.SetPort(32400);
    }
  }

  if (url.find("://") != string::npos)
  {
    // It's got its own protocol, so leave it alone.
    return url;
  }
  else if (url.find("/") == 0)
  {
    // Absolute off parent.
    theURL.SetFileName(url.substr(1));
  }
  else
  {
    // Relative off parent.
    string path = theURL.GetFileName();
    if (path[path.size() -1] != '/')
      path += '/';

    theURL.SetFileName(path + url);
  }
  
  string finalURL = theURL.Get();

  // If we have an auth token, make sure it gets propagated.
  map<CStdString, CStdString> options;
  theFullURL.GetOptions(options);
  finalURL = CheckAuthToken(options, finalURL, "X-Plex-Token");
  
  //CLog::Log(LOGNOTICE, "Processed [%s] + [%s] => [%s]\n", parent.c_str(), url.c_str(), finalURL.c_str());
  return finalURL;
}

bool CPlexDirectory::IsHomeVideoSection(const CStdString &url)
{
  boost::mutex::scoped_lock lk(g_homeVideoMapMutex);

  CURL cu(url);

  bool found = false;
  BOOST_FOREACH(CStdString k, g_homeVideoMap)
  {
    if (boost::starts_with(cu.GetUrlWithoutOptions(), k))
      found = true;
  }

  return found;
}

void CPlexDirectory::AddHomeVideoSection(const CStdString &url)
{
  if (IsHomeVideoSection(url))
    return;

  boost::mutex::scoped_lock lk(g_homeVideoMapMutex);

  CURL cu(url);

  g_homeVideoMap.push_back(cu.GetUrlWithoutOptions());
}
