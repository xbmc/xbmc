#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "Utility/sha1.hpp"

#include "PlexUtils.h"
#include "File.h"
#include "URIUtils.h"
#include "StackDirectory.h"
#include "URL.h"
#include "TextureCache.h"

#include "SystemInfo.h"

#include "utils/StringUtils.h"
#include "addons/Skin.h"
#include "Client/PlexMediaServerClient.h"
#include "PlexApplication.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "GUI/GUIPlexMediaWindow.h"

#include "File.h"

#ifdef TARGET_DARWIN_OSX
#include <CoreServices/CoreServices.h>
#endif

#ifdef TARGET_LINUX // For uname()
#include <sys/utsname.h>
#endif

using namespace std;
using namespace boost;


///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexUtils::IsLocalNetworkIP(const CStdString &host)
{
  bool isLocal = false;
  if (starts_with(host, "10.") || starts_with(host, "192.168.") || starts_with(host, "127.0.0.1"))
  {
    isLocal = true;
  }
  else if (starts_with(host, "172."))
  {
    vector<string> elems = StringUtils::Split(host, ".");
    if (elems.size() == 4)
    {
      int secondOct = lexical_cast<int>(elems[1]);
      if (secondOct >= 16 && secondOct <= 31)
        isLocal = true;
    }
  }
  return isLocal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::CacheImageUrl(const string &url)
{
  bool needsRecache = false;
  std::string returnFile = CTextureCache::Get().CheckCachedImage(url, false, needsRecache);
  if (returnFile.empty())
  {
    CTextureDetails details;
    if (CTextureCache::Get().CacheImage(url, details))
      returnFile = details.file;
  }
  return returnFile;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::CacheImageUrlAsync(const std::string &url)
{
  bool needsRecache = false;
  std::string returnFile = CTextureCache::Get().CheckCachedImage(url, false, needsRecache);
  if (returnFile.empty())
  {
    CTextureCache::Get().BackgroundCacheImage(url);
    return "";
  }
  return returnFile;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int64_t PlexUtils::Size(const CStdString& strFileName)
{
  struct __stat64 buffer;
  if (XFILE::CFile::Stat(strFileName, &buffer) == 0)
    return buffer.st_size;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexUtils::AppendPathToURL(CURL& baseURL, const string& relativePath)
{
  string filename = baseURL.GetFileName();
  if (boost::ends_with(filename, "/") == false)
    filename += "/";
  filename += relativePath;
  baseURL.SetFileName(filename);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
string PlexUtils::AppendPathToURL(const string& baseURL, const string& relativePath)
{
  string ret = baseURL;
  string args;

  // If there are arguments, strip them for now.
  size_t q = ret.find("?");
  if (q != string::npos)
  {
    args = ret.substr(q+1);
    ret = ret.substr(0, q);
  }

  // Make sure there is a trailing slash.
  if (boost::ends_with(ret, "/") == false)
    ret += "/";

  // Add the path.
  ret += relativePath;

  // Add arguments.
  if (args.empty() == false)
  {
    if (ret.find("?") == string::npos)
      ret += "?" + args;
    else
      ret += "&" + args;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
int PlexUtils::FileAge(const CStdString &strFileName)
{
  struct __stat64 stat;
  if(XFILE::CFile::Stat(strFileName, &stat) == 0)
    return time(0) - (int)stat.st_mtime;
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
bool PlexUtils::IsValidIP(const std::string& address)
{
  bool valid = false;
  
  vector<string> octets;
  split(octets, address, is_any_of("."));
  if (octets.size() == 4)
  {
    BOOST_FOREACH(string octet, octets)
    {
      try { int i = lexical_cast<int>(octet); if (i < 0 || i > 255) return false; }
      catch (...) { return false; }
    }
    
    valid = true;
  }    
  
  return valid;
}

///////////////////////////////////////////////////////////////////////////////
string PlexUtils::GetHostName()
{
  char hostname[256];
  gethostname(hostname, 256);
  string friendlyName = hostname;
  boost::replace_last(friendlyName, ".local", "");
  
  return friendlyName;
}

///////////////////////////////////////////////////////////////////////////////
bool PlexUtils::IsPlexMediaServer(const CStdString& strFile)
{
  CURL url(strFile);
  if (url.GetProtocol() == "plexserver" || url.GetProtocol() == "plex")
    return true;

  // A stack can also come from the Plex Media Servers.
  if (URIUtils::IsStack(strFile))
  {
    XFILE::CStackDirectory dir;
    CStdString firstFile = dir.GetFirstStackedFile(strFile);
    return IsPlexMediaServer(firstFile);
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
bool PlexUtils::IsPlexWebKit(const CStdString& strFile)
{
  return strFile.Find("/:/webkit") != -1;
}

///////////////////////////////////////////////////////////////////////////////
string PlexUtils::GetMachinePlatform()
{
#ifdef TARGET_WINDOWS
  return "Windows";
#elif TARGET_LINUX
  return "Linux";
#elif TARGET_DARWIN_OSX
  return "MacOSX";
#elif TARGET_DARWIN_IOS_ATV
  return "AppleTV2";
#elif TARGET_RPI
  return "RaspberryPI";
#else
  return "Unknown";
#endif
}

///////////////////////////////////////////////////////////////////////////////
string PlexUtils::GetMachinePlatformVersion()
{
  string ver;

#if TARGET_WINDOWS

  DWORD dwVersion = GetVersion();
  DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
  DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
  DWORD dwBuildNumber  = (DWORD)(HIWORD(dwVersion));

  char str[256];
  sprintf(str, "%d.%d (Build %d)", dwMajorVersion, dwMinorVersion, dwBuildNumber);
  ver = str;

#elif TARGET_LINUX

  struct utsname buf;
  if (uname(&buf) == 0)
  {
    ver = buf.release;
    ver = " (" + string(buf.version) + ")";
  }

#elif TARGET_DARWIN_OSX

  // TODO: Gestalt() is deprecated in 10.8!

  SInt32 res = 0;
  Gestalt(gestaltSystemVersionMajor, &res);
  ver = boost::lexical_cast<string>(res) + ".";

  Gestalt(gestaltSystemVersionMinor, &res);
  ver += boost::lexical_cast<string>(res) + ".";

  Gestalt(gestaltSystemVersionBugFix, &res);
  ver += boost::lexical_cast<string>(res);

#else

  ver = "Unknown";

#endif

  return ver;
}

///////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::GetStreamChannelName(CFileItemPtr item)
{
  int64_t channels = item->GetProperty("channels").asInteger();

  if (channels == 1)
    return "Mono";
  else if (channels == 2)
    return "Stereo";

  return boost::lexical_cast<std::string>(channels - 1) + ".1";
}

///////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::GetStreamCodecName(CFileItemPtr item)
{
  std::string codec = item->GetProperty("codec").asString();
  if (codec == "dca")
    return "DTS";

  return boost::to_upper_copy(codec);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr PlexUtils::GetItemSelectedStreamOfType(const CFileItem &fileItem, int streamType)
{
  int selectedItem = 0;
  if (fileItem.HasProperty("selectedMediaItem"))
    selectedItem = fileItem.GetProperty("selectedMediaItem").asInteger();

  if (fileItem.m_mediaItems.size() == 0 ||
      fileItem.m_mediaItems.size() < selectedItem)
    return CFileItemPtr();

  CFileItemPtr item = fileItem.m_mediaItems[selectedItem];

  if (!item)
    return CFileItemPtr();

  CFileItemPtr part;
  if (item->m_mediaParts.size() > 0)
    part = item->m_mediaParts[0];

  if (part)
    return GetSelectedStreamOfType(part, streamType);

  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////
CFileItemPtr PlexUtils::GetSelectedStreamOfType(CFileItemPtr mediaPart, int streamType)
{
  BOOST_FOREACH(CFileItemPtr stream, mediaPart->m_mediaPartStreams)
  {
    int64_t _streamType = stream->GetProperty("streamType").asInteger();
    bool selected = (stream->IsSelected() || streamType == PLEX_STREAM_VIDEO);
    if (_streamType == streamType && selected)
      return stream;
  }

  return CFileItemPtr();
}

////////////////////////////////////////////////////////////////////////////////////////
bool PlexUtils::PlexMediaStreamCompare(CFileItemPtr stream1, CFileItemPtr stream2)
{
  if (stream1->GetProperty("streamType").asInteger() == stream2->GetProperty("streamType").asInteger() &&
      stream1->GetProperty("language").asString() == stream2->GetProperty("language").asString() &&
      stream1->GetProperty("codec").asString() == stream2->GetProperty("codec").asString() &&
      stream1->GetProperty("index").asInteger() == stream2->GetProperty("index").asInteger() &&
      stream1->GetProperty("channels").asInteger() == stream2->GetProperty("channels").asInteger())
    return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr PlexUtils::GetStreamByID(CFileItemPtr item, int streamType, int plexStreamID)
{
  BOOST_FOREACH(CFileItemPtr it, item->m_mediaItems)
  {
    BOOST_FOREACH(CFileItemPtr part, it->m_mediaParts)
    {
      BOOST_FOREACH(CFileItemPtr stream, part->m_mediaPartStreams)
      {
        if (stream->GetProperty("streamType").asInteger() == streamType &&
            stream->GetProperty("id").asInteger() == plexStreamID)
        {
          return stream;
        }
      }
    }
  }
  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////
void PlexUtils::SetSelectedStream(CFileItemPtr item, CFileItemPtr stream)
{
  BOOST_FOREACH(CFileItemPtr item, item->m_mediaItems)
  {
    BOOST_FOREACH(CFileItemPtr part, item->m_mediaParts)
    {
      if (stream->GetProperty("streamType").asInteger() == PLEX_STREAM_SUBTITLE &&
          stream->GetProperty("id").asInteger() == 0)
      {
        /* this means we reset the stream back to none */
        stream->Select(true);
        g_plexApplication.mediaServerClient->SelectStream(item, part->GetProperty("id").asInteger(), 0, -1);
      }
      
      BOOST_FOREACH(CFileItemPtr str, part->m_mediaPartStreams)
      {
        if (PlexMediaStreamCompare(stream, str))
        {
          int subId = -1;
          int audioId = -1;
          if (str->GetProperty("streamType").asInteger() == PLEX_STREAM_AUDIO)
            audioId = str->GetProperty("id").asInteger();
          else if (str->GetProperty("streamType").asInteger() == PLEX_STREAM_SUBTITLE)
            subId = str->GetProperty("id").asInteger();
          
          g_plexApplication.mediaServerClient->SelectStream(item, part->GetProperty("id").asInteger(), subId, audioId);
          
          str->Select(true);
          str->SetProperty("select", true);
        }
        else if (stream->GetProperty("streamType").asInteger() == str->GetProperty("streamType").asInteger())
        {
          str->Select(false);
          str->SetProperty("select", false);
        }
      }
    }
  }
  
  if (stream->GetProperty("streamType").asInteger() == PLEX_STREAM_AUDIO)
    item->SetProperty("selectedAudioStream", GetPrettyStreamName(*item.get(), true));
  else if (stream->GetProperty("streamType").asInteger() == PLEX_STREAM_SUBTITLE)
    item->SetProperty("selectedSubtitleStream", GetPrettyStreamName(*item.get(), false));
}

///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

int usleep(useconds_t useconds)
{
  Sleep(useconds / 1000);
  return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
bool PlexUtils::CurrentSkinHasPreplay()
{
  return g_SkinInfo->HasSkinFile("PlexPreplayVideo.xml");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexUtils::CurrentSkinHasFilters()
{
return g_SkinInfo->HasSkinFile("DialogFilters.xml");}

////////////////////////////////////////////////////////////////////////////////////////
#include "filesystem/SpecialProtocol.h"

std::string PlexUtils::GetPlexCrashPath()
{
  return CSpecialProtocol::TranslatePath("special://temp/CrashReports");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString PlexUtils::GetPrettyStreamNameFromStreamItem(CFileItemPtr stream)
{
  CStdString name;

  if (stream->HasProperty("language") && !stream->GetProperty("language").asString().empty())
  {
    name = stream->GetProperty("language").asString();
    if (stream->GetProperty("streamType").asInteger() == PLEX_STREAM_AUDIO)
    {
      name += " (" + GetStreamCodecName(stream) + " " + GetStreamChannelName(stream) + ")";
    }
    else if (stream->HasProperty("format"))
    {
      name += " (" + boost::to_upper_copy(stream->GetProperty("format").asString());
      if (stream->GetProperty("forced").asBoolean())
        name += " " + g_localizeStrings.Get(52503) + ")";
      else
        name += ")";

    }
  }
  else
    name = g_localizeStrings.Get(1446);

  return name;
}

////////////////////////////////////////////////////////////////////////////////////////
CStdString PlexUtils::GetPrettyStreamName(const CFileItem &fileItem, bool audio)
{
  CFileItemPtr selectedStream;
  int numStreams = 0;
  
  if (fileItem.m_mediaItems.size() < 1 || fileItem.m_mediaItems[0]->m_mediaParts.size() < 1)
    return g_localizeStrings.Get(231);
  
  CFileItemPtr part = fileItem.m_mediaItems[0]->m_mediaParts[0];
  for (int y = 0; y < part->m_mediaPartStreams.size(); y ++)
  {
    CFileItemPtr stream = part->m_mediaPartStreams[y];
    int64_t streamType = stream->GetProperty("streamType").asInteger();
    if ((audio && streamType == PLEX_STREAM_AUDIO) ||
        (!audio && streamType == PLEX_STREAM_SUBTITLE))
    {
      if (stream->IsSelected())
        selectedStream = stream;
      
      numStreams ++;
    }
  }
  
  CStdString name;
  
  if (selectedStream)
    name = GetPrettyStreamNameFromStreamItem(selectedStream);
  else
    name = g_localizeStrings.Get(231);
  
  return name;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString PlexUtils::GetSHA1SumFromURL(const CURL &url)
{
  SHA1 sha;
  XFILE::CFile file;

  if (file.Open(url.Get()))
  {
    uint8_t buffer[4096];
    int read = file.Read(buffer, 4096);

    while (read != 0)
    {
      sha.update(buffer, read);
      read = file.Read(buffer, 4096);
    }

    return sha.end().hex();
  }

  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString PlexUtils::GetXMLString(const CXBMCTinyXML &document)
{
  CXBMCTinyXML ldoc(document);

  TiXmlPrinter printer;
  printer.SetIndent("  ");
  ldoc.Accept(&printer);

  return printer.Str();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool PlexUtils::MakeWakeupPipe(SOCKET *pipe)
{
#ifdef TARGET_POSIX
  if (::pipe(pipe) != 0)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to create a POSIX pipe");
    return false;
  }
#else
  pipe[0] = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (pipe[0] == -1)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to create UDP socket");
    return false;
  }

  struct sockaddr_in inAddr;
  struct sockaddr addr;

  memset((char*)&inAddr, 0, sizeof(inAddr));
  memset((char*)&addr, 0, sizeof(addr));

  inAddr.sin_family = AF_INET;
  inAddr.sin_port = htons(0);
  inAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  int y = 1;
  if (::setsockopt(pipe[0], SOL_SOCKET, SO_REUSEADDR, (const char*)&y, sizeof(y)) == -1)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to set socket options");
    return false;
  }

  if (::bind(pipe[0], (struct sockaddr *)&inAddr, sizeof(inAddr)) == -1)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to bind socket!");
    return false;
  }

  int len = sizeof(addr);
  if (::getsockname(pipe[0], &addr, &len) != 0)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to getsockname on socket");
    return false;
  }

  pipe[1] = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (pipe[1] == -1)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to create other end of UDP pipe");
    return false;
  }

  if (connect(pipe[1], &addr, len) != 0)
  {
    CLog::Log(LOGWARNING, "PlexUtils::MakeWakeupPipe failed to connect UDP pipe.");
    return false;
  }
#endif
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(HAVE_EXECINFO_H)
void PlexUtils::LogStackTrace(char *FuncName)
{
  void *buffer[100];
  char **strings;
  int  nptrs;

   nptrs = backtrace(buffer, 100);
   strings = backtrace_symbols(buffer, nptrs);
   if (strings)
   {
     CLog::Log(LOGDEBUG,"Stacktrace for function %s", FuncName);
     for (int j = 0; j < nptrs; j++)
         CLog::Log(LOGDEBUG,"%s\n", strings[j]);

     free(strings);
   }
}
#else
void PlexUtils::LogStackTrace(char *FuncName) {}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
ePlexMediaType PlexUtils::GetMediaTypeFromItem(CFileItemPtr item)
{
  EPlexDirectoryType plexType = item->GetPlexDirectoryType();

  switch(plexType)
  {
    case PLEX_DIR_TYPE_TRACK:
    case PLEX_DIR_TYPE_ALBUM:
    case PLEX_DIR_TYPE_ARTIST:
      return PLEX_MEDIA_TYPE_MUSIC;
    case PLEX_DIR_TYPE_VIDEO:
    case PLEX_DIR_TYPE_EPISODE:
    case PLEX_DIR_TYPE_CLIP:
    case PLEX_DIR_TYPE_MOVIE:
    case PLEX_DIR_TYPE_SEASON:
    case PLEX_DIR_TYPE_SHOW:
      return PLEX_MEDIA_TYPE_VIDEO;
    case PLEX_DIR_TYPE_IMAGE:
    case PLEX_DIR_TYPE_PHOTO:
    case PLEX_DIR_TYPE_PHOTOALBUM:
      return PLEX_MEDIA_TYPE_PHOTO;
    default:
      return PLEX_MEDIA_TYPE_UNKNOWN;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::GetMediaTypeString(ePlexMediaType type)
{
  switch(type)
  {
    case PLEX_MEDIA_TYPE_MUSIC:
      return "music";
    case PLEX_MEDIA_TYPE_PHOTO:
      return "photo";
    case PLEX_MEDIA_TYPE_VIDEO:
      return "video";
    default:
      return "unknown";
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ePlexMediaType PlexUtils::GetMediaTypeFromString(const std::string &typestr)
{
  if (typestr == "music")
    return PLEX_MEDIA_TYPE_MUSIC;
  if (typestr == "photo")
    return PLEX_MEDIA_TYPE_PHOTO;
  if (typestr == "video")
    return PLEX_MEDIA_TYPE_VIDEO;
  return PLEX_MEDIA_TYPE_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////////////
std::string PlexUtils::GetMediaStateString(ePlexMediaState state)
{
  CStdString strstate;
  switch (state) {
    case PLEX_MEDIA_STATE_STOPPED:
      strstate = "stopped";
      break;
    case PLEX_MEDIA_STATE_BUFFERING:
      strstate = "buffering";
      break;
    case PLEX_MEDIA_STATE_PLAYING:
      strstate = "playing";
      break;
    case PLEX_MEDIA_STATE_PAUSED:
      strstate = "paused";
      break;
  }
  return strstate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ePlexMediaState PlexUtils::GetMediaStateFromString(const std::string& statestr)
{
  if (statestr == "stopped")
    return PLEX_MEDIA_STATE_STOPPED;
  else if (statestr == "buffering")
    return PLEX_MEDIA_STATE_BUFFERING;
  else if (statestr == "playing")
    return PLEX_MEDIA_STATE_PLAYING;
  else if (statestr == "paused")
    return PLEX_MEDIA_STATE_PAUSED;

  return PLEX_MEDIA_STATE_STOPPED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long PlexUtils::GetFastHash(std::string Data)
{
  // DJB2 FastHash Method (http://www.cse.yorku.ca/~oz/hash.html)
  unsigned long hash = 5381;
  int c;
  const char* str = Data.c_str();

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}
