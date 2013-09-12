/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "GameFileLoader.h"
#include "GameClient.h"
#include "games/libretro/libretro_wrapped.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/Crc32.h"
#include "utils/log.h"
#include "utils/StdString.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"

#include <limits>

#define MAX_SAVESTATE_CRC_LENGTH   40 * 1024 * 1024 // 40 MiB

using namespace ADDON;
using namespace GAME_INFO;
using namespace GAMES;
using namespace XFILE;
using namespace std;

CGameFile::CGameFile(TYPE type, const string &originalPath, const string &translatedPath /* = "" */)
  : m_type(type),
    m_strOriginalPath(originalPath),
    m_strTranslatedPath(translatedPath),
    m_bIsLoaded(false),
    m_bIsCRCed(false)
{
  if (m_strTranslatedPath.empty())
    m_strTranslatedPath = m_strOriginalPath; // no translation, just use original path
}

void CGameFile::Reset()
{
  m_type = TYPE_INVALID;
  m_strOriginalPath.clear();
  m_strTranslatedPath.clear();
  m_data.clear();
  m_bIsLoaded = false;
  m_strCRC.clear();
  m_bIsCRCed = false;
}

const std::vector<uint8_t> &CGameFile::Buffer()
{
  if (!m_bIsLoaded)
  {
    m_bIsLoaded = true;
    if (m_type == TYPE_DATA)
      Read(m_strTranslatedPath, m_data);
    else
      CLog::Log(LOGERROR, "GameFile: Not loading %s, absolute path requested!", m_strTranslatedPath.c_str());
  }
  return m_data;
}

const string &CGameFile::CRC()
{
  if (!m_bIsCRCed)
  {
    Crc32 crc;
    if (m_type == TYPE_PATH)
    {
      /*
       * Loosely tests if CGameFileLoaderUseParentZip was the chosen strategy. If
       * this returns true, the original path is a better CRC candidate because
       * it lets us CRC the actual game file instead of a zip.
       */
      bool useOriginalPath = (StringUtils::EqualsNoCase(URIUtils::GetExtension(m_strTranslatedPath), ".zip") &&
                              !StringUtils::EqualsNoCase(URIUtils::GetExtension(m_strOriginalPath), ".zip"));
      vector<uint8_t> data;
      if (useOriginalPath)
        Read(m_strOriginalPath, data);
      else
        Read(m_strTranslatedPath, data);

      if (!data.empty() && data.size() <= MAX_SAVESTATE_CRC_LENGTH)
      {
        m_bIsCRCed = true;
        crc.Compute(reinterpret_cast<char*>(data.data()), data.size());
      }
    }
    else
    {
      // If we're loading from a buffer, then CRC that data
      if (!Buffer().empty() && Buffer().size() <= MAX_SAVESTATE_CRC_LENGTH)
      {
        m_bIsCRCed = true;
        crc.Compute(reinterpret_cast<char*>(m_data.data()), m_data.size());
      }
    }

    if (m_bIsCRCed)
      m_strCRC = StringUtils::Format("%08x", (unsigned __int32)crc);
    
    m_bIsCRCed = true; // don't try again
  }
  return m_strCRC;
}

/* static */
void CGameFile::Read(const std::string &path, std::vector<uint8_t> &data)
{
  // Load the file from the vfs
  CFile vfsFile;
  if (!vfsFile.Open(path))
  {
    CLog::Log(LOGERROR, "GameFile: XBMC cannot open file \"%s\"", path.c_str());
    return;
  }

  int64_t length = vfsFile.GetLength();

  // Check for file size overflow (libretro accepts files <= size_t max)
  if (length <= 0 || (uint64_t)length > (uint64_t)std::numeric_limits<size_t>::max())
  {
    CLog::Log(LOGERROR, "GameFile: Invalid file size: %"PRId64" bytes", length);
    return;
  }

  data.resize((size_t)length);

  if (vfsFile.Read(data.data(), length) != length)
  {
    CLog::Log(LOGERROR, "GameFile: XBMC failed to read game data");
    // Fullfill post-condition: m_data.size() must return zero
    data.clear();
    return;
  }

  CLog::Log(LOGDEBUG, "GameFile: loaded file from VFS (filesize: %lu KiB)", data.size() / 1024);
}

bool CGameFile::ToInfo(LIBRETRO::retro_game_info &info)
{
  switch (m_type)
  {
  case TYPE_PATH:
    info.path = m_strTranslatedPath.c_str();
    info.data = NULL;
    info.size = 0;
    info.meta = NULL;
    return true;
  case TYPE_DATA:
    if (!Buffer().empty())
    {
      info.path = NULL; // NULL forces the game client to load from memory
      info.data = m_data.data();
      info.size = m_data.size();
      info.meta = NULL;
      return true;
    }
    break;
  default:
    break;
  }
  return false;
}

/* static */
bool CGameFileLoader::CanOpen(const CGameClient &gc, const CFileItem &file)
{
  // Check gameclient
  if (file.HasProperty("gameclient") && file.GetProperty("gameclient").asString() != gc.ID())
    return false;

  if (gc.GetExtensions().empty() && gc.GetPlatforms().empty())
    return true; // Client provided us with *no* useful information. Be optimistic.

  // Check platform
  if (!gc.GetPlatforms().empty() && file.GetGameInfoTag())
  {
    GamePlatform id = CGameInfoTagLoader::GetPlatformInfoByName(file.GetGameInfoTag()->GetPlatform()).id;
    if (id != PLATFORM_UNKNOWN)
      if (std::find(gc.GetPlatforms().begin(), gc.GetPlatforms().end(), id) == gc.GetPlatforms().end())
        return false;
  }

  CGameFileLoaderUseHD        hd;
  CGameFileLoaderUseParentZip outerzip;
  CGameFileLoaderUseVFS       vfs;
  CGameFileLoaderEnterZip     innerzip;

  CGameFileLoader *strategies[] = { &hd, &outerzip, &vfs, &innerzip };
  CGameFile dummy;

  for (unsigned int i = 0; i < sizeof(strategies) / sizeof(strategies[0]); i++)
    if (strategies[i]->CanLoad(gc, file, dummy))
      return true;
  return false;
}

/* static */
bool CGameFileLoader::GetEffectiveRomPath(const string &zipPath, const set<string> &validExts, string &effectivePath)
{
  // Default case: effective zip file is the zip file itself
  effectivePath = zipPath;

  // If it's not a zip file, we can't open and explore...
  if (!URIUtils::GetExtension(zipPath).Equals(".zip"))
    return false;

  // Enumerate the zip directory, looking for valid extensions
  CStdString strUrl;
  URIUtils::CreateArchivePath(strUrl, "zip", zipPath, "");

  CStdString strValidExts;
  for (set<string>::const_iterator it = validExts.begin(); it != validExts.end(); it++)
    strValidExts += *it + "|";

  CFileItemList itemList;
  if (CDirectory::GetDirectory(strUrl, itemList, strValidExts, DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO) && itemList.Size())
  {
    // Use the first file discovered
    effectivePath = itemList[0]->GetPath();
    return true;
  }
  return false;
}

/* static */
bool CGameFileLoader::IsExtensionValid(const string &ext, const set<string> &setExts)
{
  if (setExts.empty())
    return true; // Be optimistic :)
  if (ext.empty())
    return false;

  // Convert to lower case and canonicalize with a leading "."
  string ext2(ext);
  StringUtils::ToLower(ext2);
  if (ext2[0] != '.')
    ext2.insert(0, ".");
  return setExts.find(ext2) != setExts.end();
}

bool CGameFileLoaderUseHD::CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: Testing if we can load game from hard drive");

  // Make sure the file is local
  if (!file.GetAsUrl().GetProtocol().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: File is not local (or is inside an archive)");
    return false;
  }

  // Make sure the extension is valid
  if (!IsExtensionValid(URIUtils::GetExtension(file.GetPath()), gc.GetExtensions()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseHD: Extension %s is not valid", URIUtils::GetExtension(file.GetPath()).c_str());
    return false;
  }

  result = CGameFile(CGameFile::TYPE_PATH, file.GetPath());
  return true;
}

bool CGameFileLoaderUseVFS::CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Testing if we can load game from VFS");

  // Obvious check
  if (!gc.AllowVFS())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Game client does not allow VFS");
    return false;
  }

  // Make sure the extension is valid
  CStdString ext = URIUtils::GetExtension(file.GetPath());
  if (!IsExtensionValid(ext, gc.GetExtensions()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseVFS: Extension %s is not valid", ext.c_str());
    return false;
  }

  result = CGameFile(CGameFile::TYPE_DATA, file.GetPath());
  return true;
}

bool CGameFileLoaderUseParentZip::CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Testing if the game is in a zip");

  // Can't use parent zip if file isn't a child file of a .zip folder
  if (!URIUtils::IsInZIP(file.GetPath()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Game is not in a zip file");
    return false;
  }

  if (!IsExtensionValid(".zip", gc.GetExtensions()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: This game client does not support zip files");
    return false;
  }

  // Make sure we're in the root folder of the zip (no parent folder)
  CURL parentURL(URIUtils::GetParentPath(file.GetPath()));
  if (!parentURL.GetFileName().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Game is not in the root folder of the zip");
    return false;
  }

  // Make sure the container zip is on the local hard disk (or not inside another zip)
  if (!CURL(parentURL.GetHostName()).GetProtocol().empty())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Zip file is not on the local hard disk");
    return false;
  }

  // Make sure the extension is valid
  if (!IsExtensionValid(URIUtils::GetExtension(file.GetPath()), gc.GetExtensions()))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyUseParentZip: Extension %s is not valid", URIUtils::GetExtension(file.GetPath()).c_str());
    return false;
  }

  // Found our file
  result = CGameFile(CGameFile::TYPE_PATH, file.GetPath(), parentURL.GetHostName());
  return true;
}

bool CGameFileLoaderEnterZip::CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result)
{
  CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: Testing if the file is a zip containing a game");

  // Must be a zip file, clearly
  if (!URIUtils::GetExtension(file.GetPath()).Equals(".zip"))
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: File is not a zip");
    return false;
  }

  // Must support loading from the vfs
  if (!gc.AllowVFS())
  {
    CLog::Log(LOGDEBUG, "GameClient::CStrategyEnterZip: Game client does not allow VFS");
    return false;
  }

  // Look for an internal file. This will screen against valid extensions.
  CStdString internalFile;
  if (!GetEffectiveRomPath(file.GetPath(), gc.GetExtensions(), internalFile))
  {
    CLog::Log(LOGINFO, "GameClient::CStrategyEnterZip: Zip does not contain a file with a valid extension");
    return false;
  }

  result = CGameFile(CGameFile::TYPE_DATA, file.GetPath(), internalFile);
  return true;
}
