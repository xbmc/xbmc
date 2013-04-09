#include "PlexFile.h"
#include "Client/PlexServerManager.h"
#include "utils/log.h"
#include "settings/GUISettings.h"

using namespace XFILE;

CPlexFile::CPlexFile(void) : CCurlFile()
{
  SetRequestHeader("X-Plex-Version", PLEX_VERSION);
  SetRequestHeader("X-Plex-Client-Platform", PlexUtils::GetMachinePlatform());
  SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));
  SetRequestHeader("X-Plex-Platform", PlexUtils::GetMachinePlatform());
  SetRequestHeader("X-Plex-Provides", "player");
  SetRequestHeader("X-Plex-Product", "Plex for Home Theater");
}

bool
CPlexFile::BuildHTTPURL(CURL& url)
{
  CURL newUrl;

  /* Resolve the correct URL */
  CStdString uuid = url.GetHostName();
  CPlexServerPtr server = g_plexServerManager.FindByUUID(uuid);
  if (!server)
  {
    /* Ouch, this should not happen! */
    CLog::Log(LOGWARNING, "CPlexFile::BuildHTTPURL tried to lookup server %s but it was not found!", uuid.c_str());
    return false;
  }

  newUrl = server->BuildURL(url.GetFileName());
  CLog::Log(LOGDEBUG, "CPlexFile::BuildHTTPURL translated '%s' to '%s'", url.Get().c_str(), newUrl.Get().c_str());
  url = newUrl;

  return true;
}

bool
CPlexFile::Open(const CURL &url)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Open(newUrl);
  return false;
}

int
CPlexFile::Stat(const CURL &url, struct stat *buffer)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Stat(newUrl, buffer);
  return false;
}

bool
CPlexFile::Exists(const CURL &url)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Exists(newUrl);
  return false;
}
