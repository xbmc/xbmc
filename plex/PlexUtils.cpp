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

using namespace std;
using namespace boost;


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
  if (url.GetProtocol() == "plex" || url.GetPort() == 32400 || url.GetOptions().find("X-Plex-Token") != -1)
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
#ifdef _WIN32

int usleep(useconds_t useconds)
{
  Sleep(useconds / 1000);
  return 0;
}

#endif
