#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "PlexUtils.h"
#include "File.h"
#include "URIUtils.h"
#include "StackDirectory.h"
#include "URL.h"
#include "TextureCache.h"

#include "SystemInfo.h"

#include "utils/StringUtils.h"

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

///////////////////////////////////////////////////////////////////////////////
CFileItemPtr PlexUtils::GetSelectedStreamOfType(CFileItemPtr mediaPart, int streamType)
{
  BOOST_FOREACH(CFileItemPtr stream, mediaPart->m_mediaPartStreams)
  {
    int64_t _streamType = stream->GetProperty("streamType").asInteger();
    bool selected = stream->GetProperty("selected").asBoolean();
    if (_streamType == streamType && selected)
      return stream;
  }

  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////
void PlexUtils::SetSelectedStream(CFileItemPtr mediaPart, int streamType, int id)
{
  BOOST_FOREACH(CFileItemPtr stream, mediaPart->m_mediaPartStreams)
  {
    if (stream->GetProperty("streamType").asInteger() == streamType && stream->GetProperty("id").asInteger() == id)
      stream->SetProperty("selected", true);
    else
      stream->SetProperty("selected", false);
  }
}

///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

int usleep(useconds_t useconds)
{
  Sleep(useconds / 1000);
  return 0;
}

#endif
