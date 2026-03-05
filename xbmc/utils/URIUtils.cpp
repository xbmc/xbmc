/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URIUtils.h"

#include "FileItem.h"
#include "PasswordManager.h"
#include "ServiceBroker.h"
#include "StringUtils.h"
#include "URL.h"
#ifdef HAVE_LIBBLURAY
#include "filesystem/BlurayDirectory.h"
#endif
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "network/DNSNameCache.h"
#include "network/Network.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "settings/AdvancedSettings.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"

#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

#include "application/Application.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace PVR;
using namespace XFILE;

const CAdvancedSettings* URIUtils::m_advancedSettings = nullptr;

namespace
{

constexpr std::string_view DecodeURLSpecialChars{"+%"};

// Lookup table for URL encoding. This is more efficient than using fmt::format
// for such a simple operation and this function has been identified as a hot path
// during library scans; especially encoding the contents of nfo files into URL
// parameters.
constexpr std::array<const char*, 256> hex_chars{
    // clang-format off
    "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07", "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f",
    "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f",
    "%20", "%21", "%22", "%23", "%24", "%25", "%26", "%27", "%28", "%29", "%2a", "%2b", "%2c", "%2d", "%2e", "%2f",
    "%30", "%31", "%32", "%33", "%34", "%35", "%36", "%37", "%38", "%39", "%3a", "%3b", "%3c", "%3d", "%3e", "%3f",
    "%40", "%41", "%42", "%43", "%44", "%45", "%46", "%47", "%48", "%49", "%4a", "%4b", "%4c", "%4d", "%4e", "%4f",
    "%50", "%51", "%52", "%53", "%54", "%55", "%56", "%57", "%58", "%59", "%5a", "%5b", "%5c", "%5d", "%5e", "%5f",
    "%60", "%61", "%62", "%63", "%64", "%65", "%66", "%67", "%68", "%69", "%6a", "%6b", "%6c", "%6d", "%6e", "%6f",
    "%70", "%71", "%72", "%73", "%74", "%75", "%76", "%77", "%78", "%79", "%7a", "%7b", "%7c", "%7d", "%7e", "%7f",
    "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f",
    "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f",
    "%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%a7", "%a8", "%a9", "%aa", "%ab", "%ac", "%ad", "%ae", "%af",
    "%b0", "%b1", "%b2", "%b3", "%b4", "%b5", "%b6", "%b7", "%b8", "%b9", "%ba", "%bb", "%bc", "%bd", "%be", "%bf",
    "%c0", "%c1", "%c2", "%c3", "%c4", "%c5", "%c6", "%c7", "%c8", "%c9", "%ca", "%cb", "%cc", "%cd", "%ce", "%cf",
    "%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7", "%d8", "%d9", "%da", "%db", "%dc", "%dd", "%de", "%df",
    "%e0", "%e1", "%e2", "%e3", "%e4", "%e5", "%e6", "%e7", "%e8", "%e9", "%ea", "%eb", "%ec", "%ed", "%ee", "%ef",
    "%f0", "%f1", "%f2", "%f3", "%f4", "%f5", "%f6", "%f7", "%f8", "%f9", "%fa", "%fb", "%fc", "%fd", "%fe", "%ff",
    // clang-format on
};

std::optional<char> DecodeOctlet(std::string_view& encoded)
{
  if (encoded.length() < 2)
    return {};

  uint8_t decimal{};
  const auto res = std::from_chars(encoded.data(), encoded.data() + 2, decimal, 16);

  if (res.ec != std::errc() || res.ptr != encoded.data() + 2)
    return {};

  encoded = encoded.substr(2);
  return decimal;
}

} // Unnamed namespace

std::string URIUtils::URLDecode(std::string_view encoded)
{
  /* result will always be less than or equal to source */
  std::string decodedUrl{};
  decodedUrl.reserve(encoded.length());

  while (true)
  {
    const auto special = encoded.find_first_of(DecodeURLSpecialChars);
    decodedUrl += encoded.substr(0, special);

    if (special == std::string::npos)
      break;

    const char specialChar = encoded[special];
    encoded = encoded.substr(special + 1);

    if (specialChar == '+')
      decodedUrl += ' ';
    else
      decodedUrl += DecodeOctlet(encoded).value_or('%'); // Decoded octet triplet '%2f'
  }

  return decodedUrl;
}

std::string URIUtils::URLEncode(std::string_view decoded, std::string_view URLSpec)
{
  std::string result;
  result.reserve(decoded.size() * 2);

  for (const auto& ch : decoded)
  {
    if (StringUtils::isasciialphanum(ch) || URLSpec.find(ch) != std::string::npos)
      result += ch;
    else
      result.append(hex_chars[static_cast<unsigned char>(ch)], 3);
  }
  return result;
}

void URIUtils::RegisterAdvancedSettings(const CAdvancedSettings& advancedSettings)
{
  m_advancedSettings = &advancedSettings;
}

void URIUtils::UnregisterAdvancedSettings()
{
  m_advancedSettings = nullptr;
}

/* returns filename extension including period of filename */
std::string URIUtils::GetExtension(const CURL& url)
{
  return url.GetExtension();
}

static constexpr int NO_EXTENSION{-1};
enum class FindExtensions : uint8_t
{
  ONLY_IN_LIST,
  ALL_EXTENSIONS
};
enum class FinalDot : uint8_t
{
  IGNORE_FINAL_DOT,
  CONSIDER_FINAL_DOT
};

namespace
{
/*! \brief Finds the extension (if any) in a given file path.
   \param path The file path.
   \param extensions List of '.' prefixed lowercase extensions separated with '|'. 
                     If there is no list then all extensions are found,
                     along with selected compound archive extensions.
   \param extensionsToFind Whether to find only extensions in the given list or all extensions.
   \param finalDotAction Action to take if there is a final dot in the filename.
   \return The position of the extension in the path, or NO_EXTENSION if none found.
 */
int FindExtension(const std::string& path,
                  std::string_view extensions = "",
                  FindExtensions extensionsToFind = FindExtensions::ALL_EXTENSIONS,
                  FinalDot finalDotAction = FinalDot::IGNORE_FINAL_DOT)
{
  if (path.empty())
    return NO_EXTENSION;

  // Special directories
  const size_t separator{path.find_last_of("/\\")};
  const std::string last{path.substr(separator == std::string::npos ? 0 : separator + 1)};
  if (last == "." || last == "..")
    return NO_EXTENSION;

  // Single trailing dot - no extension
  if (path.back() == '.')
    return finalDotAction == FinalDot::IGNORE_FINAL_DOT ? NO_EXTENSION
                                                        : static_cast<int>(path.size() - 1);

  const size_t period{path.find_last_of('.')};
  if (period == std::string::npos || (separator != std::string::npos && period < separator))
    return NO_EXTENSION; // Separator after last period means no extension
  if (period == 0 || (separator != std::string::npos && period == separator + 1))
    return NO_EXTENSION; // No extension (a leading dot only)

  // If no extensions are passed then the routine generically removes all extensions
  // However it is almost impossible to determine what is a double dot/compound extension and
  //  what is part of the filename (with dots) so we use a list of known important (to Kodi - ie. archives)
  //  compound extensions to check against.
  auto exts{StringUtils::Split(
      !extensions.empty()
          ? extensions
          : CServiceBroker::GetFileExtensionProvider().GetCompoundArchiveExtensions(),
      '|')};

  // Compound extensions first (otherwise .tar.gz could be detected as .gz only)
  std::ranges::sort(exts, std::greater{},
                    [](std::string_view s) { return std::ranges::count(s, '.'); });

  const std::string file{StringUtils::ToLower(last)};
  for (auto& ext : exts)
  {
    if (!ext.empty())
    {
      if (const size_t start{ext.find('.')}; start != std::string::npos && start > 0)
        ext.erase(0, start);
      if (file.ends_with(ext))
        return static_cast<int>(path.size() - ext.size());
    }
  }

  if (extensionsToFind == FindExtensions::ONLY_IN_LIST)
    return NO_EXTENSION;

  // Single dot extension
  return static_cast<int>(period);
}
} // namespace

std::string URIUtils::GetExtension(const std::string& path)
{
  if (IsURL(path))
  {
    CURL url(path);
    return url.GetExtension();
  }

  if (const int extension{FindExtension(path)}; extension != NO_EXTENSION)
    return path.substr(extension);
  return {};
}

bool URIUtils::HasExtension(const std::string& path)
{
  if (IsURL(path))
  {
    CURL url(path);
    return HasExtension(url.GetFileName());
  }

  return FindExtension(path) != NO_EXTENSION;
}

bool URIUtils::HasExtension(const CURL& url, std::string_view strExtensions)
{
  return url.HasExtension(strExtensions);
}

bool URIUtils::HasExtension(const std::string& path, std::string_view extensions)
{
  if (IsURL(path))
  {
    const CURL url(path);
    return HasExtension(url.GetFileName(), extensions);
  }

  return FindExtension(path, extensions, FindExtensions::ONLY_IN_LIST) != NO_EXTENSION;
}

void URIUtils::RemoveExtension(std::string& path)
{
  if (IsURL(path))
  {
    CURL url(path);
    path = url.GetFileName();
    RemoveExtension(path);
    url.SetFileName(path);
    path = url.Get();
    return;
  }

  // Extensions to remove
  const std::string extensions{
      CServiceBroker::GetFileExtensionProvider().GetPictureExtensions() +
      CServiceBroker::GetFileExtensionProvider().GetMusicExtensions() +
      CServiceBroker::GetFileExtensionProvider().GetVideoExtensions() +
      CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions() +
      CServiceBroker::GetFileExtensionProvider().GetCompoundArchiveExtensions() +
      CServiceBroker::GetFileExtensionProvider().GetArchiveExtensions() +
      "|.py|.xml|.milk|.xbt|.cdg"
#ifdef TARGET_DARWIN
      + "|.app|.applescript|.workflow"
#endif
  };

  if (const int extension{FindExtension(path, extensions, FindExtensions::ONLY_IN_LIST,
                                        FinalDot::CONSIDER_FINAL_DOT)};
      extension != NO_EXTENSION)
  {
    path.resize(extension);
  }
}

std::string URIUtils::ReplaceExtension(const std::string& path, const std::string& newExtension)
{
  if (IsURL(path))
  {
    CURL url(path);
    url.SetFileName(ReplaceExtension(url.GetFileName(), newExtension));
    return url.Get();
  }

  const int extension{
      FindExtension(path, "", FindExtensions::ALL_EXTENSIONS, FinalDot::CONSIDER_FINAL_DOT)};
  std::string_view base{path};
  if (extension != NO_EXTENSION)
    base = base.substr(0, extension);
  std::string result;
  result.reserve(base.size() + newExtension.size());
  result.append(base);
  result.append(newExtension);
  return result;
}

bool URIUtils::HasPluginPath(const CFileItem& item)
{
  return IsPlugin(item.GetPath()) || IsPlugin(item.GetDynPath());
}

std::string URIUtils::GetFileName(const CURL& url)
{
  return GetFileName(url.GetFileName());
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
std::string URIUtils::GetFileName(const std::string& strFileNameAndPath)
{
  if(IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return GetFileName(url.GetFileName());
  }

  int i = strFileNameAndPath.size() - 1;
  while (i >= 0)
  {
    const char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1))
      break;
    else
      i--;
  }
  return strFileNameAndPath.substr(i + 1);
}

std::string URIUtils::GetFileOrFolderName(std::string_view path)
{
  if (path.empty())
    return {};

  constexpr char separators[] = "/\\";

  auto idx = path.find_last_of(separators);
  if (idx == path.size() - 1)
  {
    path.remove_suffix(1);
    idx = path.find_last_of(separators);
  }

  if (idx == std::string_view::npos)
    return std::string{path};
  else
    return std::string{path.substr(idx + 1)};
}

void URIUtils::Split(const std::string& strFileNameAndPath,
                     std::string& strPath, std::string& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  int i = strFileNameAndPath.size() - 1;
  while (i >= 0)
  {
    const char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1))
      break;
    else
      i--;
  }

  // take left including the directory separator
  strPath = strFileNameAndPath.substr(0, i+1);
  // everything to the right of the directory separator
  strFileName = strFileNameAndPath.substr(i+1);

  // if actual uri, ignore options
  if (IsURL(strFileNameAndPath))
  {
    size_t extras = strFileName.find_last_of("?|");
    if (extras != std::string::npos)
      strFileName.resize(extras);
  }
}

std::vector<std::string> URIUtils::SplitPath(const std::string& path)
{
  const CURL url{path};

  // Split the filename portion of the URL up into separate directories
  const char sep{url.GetDirectorySeparator()};
  std::vector<std::string> dirs{StringUtils::Split(url.GetFileName(), std::string_view{&sep, 1})};

  // Prepend root path if present
  if (auto root{url.GetWithoutFilename()}; !root.empty())
    dirs.insert(dirs.begin(), std::move(root));

  // Remove trailing empty token
  if (dirs.size() > 1 && dirs.back().empty())
    dirs.pop_back();

  return dirs;
}

void URIUtils::GetCommonPath(std::string& parent, std::string_view path)
{
  const size_t maxCompare{std::min(parent.size(), path.size())};
  size_t i{0};
  while (i < maxCompare && std::tolower(parent[i]) == std::tolower(path[i]))
    ++i;

  parent.erase(i);

  if (!HasSlashAtEnd(parent))
  {
    parent = GetDirectory(parent);
    AddSlashAtEnd(parent);
  }
}

bool URIUtils::HasParentInHostname(const CURL& url)
{
  return url.HasParentInHostname();
}

bool URIUtils::HasEncodedHostname(const CURL& url)
{
  return url.HasEncodedHostname();
}

bool URIUtils::HasEncodedFilename(const CURL& url)
{
  const std::string prot2 = url.GetTranslatedProtocol();

  // For now assume only (quasi) http internet streams use URL encoding
  return CURL::IsProtocolEqual(prot2, "http")  ||
         CURL::IsProtocolEqual(prot2, "https");
}

std::string URIUtils::GetParentPath(const std::string& strPath)
{
  std::string strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

bool URIUtils::GetParentPath(const std::string& strPath, std::string& strParent)
{
  strParent.clear();

  CURL url(strPath);
  std::string strFile = url.GetFileName();
  if (url.HasParentInHostname() && strFile.empty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.IsProtocol("bluray"))
  {
    const CURL url2(url.GetHostName()); // strip bluray://
    if (url2.IsProtocol("udf"))
    {
      strFile = url2.GetHostName(); // strip udf://
      return GetParentPath(strFile, strParent);
    }
    strParent = url2.Get();
    return !strParent.empty();
  }
  else if (IsBDFile(strPath) || IsDVDFile(strPath))
  {
    std::string folder{GetDirectory(strPath)};
    RemoveSlashAtEnd(folder);
    const std::string lastFolder{GetFileName(folder)};
    if (StringUtils::EqualsNoCase(lastFolder, "VIDEO_TS") ||
        StringUtils::EqualsNoCase(lastFolder, "BDMV"))
      strParent = GetParentPath(folder); // go back up another one
    else
      strParent = folder;
    return !strParent.empty();
  }
  else if (IsArchive(url))
  {
    strParent = GetDirectory(url.GetHostName());
    return !strParent.empty();
  }
  else if (url.IsProtocol("stack"))
  {
    strParent = CStackDirectory::GetParentPath(url.Get());
    return !strParent.empty();
  }
  else if (url.IsProtocol("multipath"))
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.IsProtocol("plugin"))
  {
    if (!url.GetOptions().empty())
    {
      //! @todo Make a new python call to get the plugin content type and remove this temporary hack
      // When a plugin provides multiple types, it has "plugin://addon.id/?content_type=xxx" root URL
      if (url.GetFileName().empty() && url.HasOption("content_type") && url.GetOptions().find('&') == std::string::npos)
        url.SetHostName("");
      //
      url.SetOptions("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetFileName().empty())
    {
      url.SetFileName("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetHostName().empty())
    {
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return true;  // already at root
  }
  else if (url.IsProtocol("special"))
  {
    if (HasSlashAtEnd(strFile))
      strFile.erase(strFile.size() - 1);
    if(strFile.rfind('/') == std::string::npos)
      return false;
  }
  else if (strFile.empty())
  {
    if (!url.GetHostName().empty())
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile.erase(strFile.size() - 1);
  }

  size_t iPos = strFile.rfind('/');
  if (iPos == std::string::npos)
  {
    iPos = strFile.rfind('\\');
  }
  if (iPos == std::string::npos)
  {
    url.SetFileName("");
    strParent = url.Get();
    return true;
  }

  strFile.erase(iPos);

  AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  strParent = url.Get();
  return true;
}

std::string URIUtils::GetBasePath(const std::string& strPath)
{
  std::string strCheck{strPath};
  if (IsStack(strPath))
    return CStackDirectory::GetBasePath(strPath);

  if (IsBDFile(strCheck) || IsDVDFile(strCheck))
    return GetDiscBasePath(strCheck);

#ifdef HAVE_LIBBLURAY
  if (IsBlurayPath(strCheck))
    return CBlurayDirectory::GetBasePath(CURL(strCheck));
#endif

  if (const CURL url(strPath); IsArchive(url))
  {
    if (const std::string & hostname{url.GetHostName()}; !hostname.empty())
      strCheck = hostname;
  }

  std::string strDirectory = GetDirectory(strCheck);

  return strDirectory;
}

bool URIUtils::IsDiscPath(const std::string& path)
{
  std::string folder{path};
  RemoveSlashAtEnd(folder);
  folder = GetFileName(folder);
  return StringUtils::EqualsNoCase(folder, "VIDEO_TS") || StringUtils::EqualsNoCase(folder, "BDMV");
}

std::string URIUtils::GetDiscBase(const std::string& file)
{
  std::string discFile{IsBlurayPath(file) ? GetDiscFile(file) : file};
  if (IsDiscImage(discFile))
    return discFile; // return .ISO

  return GetParentPath(discFile);
}

std::string URIUtils::GetDiscBasePath(const std::string& file)
{
  std::string base{GetDiscBase(file)};
  if (IsDiscImage(base))
    return GetDirectory(base);
  return base;
}

std::string URIUtils::RemoveDiscPath(const std::string& path)
{
  std::string base{};
  if (IsBDFile(path) || IsDVDFile(path))
  {
    std::string folder{GetDirectory(path)};
    RemoveSlashAtEnd(folder);
    const std::string lastFolder{GetFileName(folder)};
    if (StringUtils::EqualsNoCase(lastFolder, "VIDEO_TS") ||
        StringUtils::EqualsNoCase(lastFolder, "BDMV"))
      base = GetDirectory(folder); // go back up another one
    else
      base = folder;
  }
  return base;
}

std::string URIUtils::GetDiscFile(const std::string& path)
{
  if (!IsBlurayPath(path))
    return {};

  const CURL url(path);
  const CURL url2(url.GetHostName()); // strip bluray://

  if (url2.IsProtocol("udf"))
    return url2.GetHostName(); // ISO so strip udf:// before return
  return AddFileToFolder(url2.Get(), "BDMV", "index.bdmv"); // BDMV
}

std::string URIUtils::GetDiscUnderlyingFile(const CURL& url)
{
  if (!url.IsProtocol("bluray"))
    return {};

  const std::string& host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  if (host.empty() || filename.empty())
    return {};
  return AddFileToFolder(host, filename);
}

std::string URIUtils::GetBlurayRootPath(const std::string& path)
{
  return AddFileToFolder(GetBlurayPath(path), "root");
}

std::string URIUtils::GetBlurayTitlesPath(const std::string& path)
{
  return AddFileToFolder(GetBlurayPath(path), "root", "titles");
}

std::string URIUtils::GetBlurayEpisodePath(const std::string& path, int season, int episode)
{
  return AddFileToFolder(GetBlurayPath(path), "root", "episode", std::to_string(season),
                         std::to_string(episode));
}

std::string URIUtils::GetBlurayAllEpisodesPath(const std::string& path)
{
  return AddFileToFolder(GetBlurayPath(path), "root", "episode", "all");
}

std::string URIUtils::GetBlurayPlaylistPath(const std::string& path, int playlist /* = -1 */)
{
  return AddFileToFolder(GetBlurayPath(path), "BDMV", "PLAYLIST",
                         playlist != -1 ? StringUtils::Format("{:05}.mpls", playlist) : "");
}

std::string URIUtils::GetBlurayPath(const std::string& path)
{
  if (IsBlurayPath(path))
  {
    // Already bluray:// path
    CURL url(path);
    url.SetFileName("");
    return url.Get();
  }

  std::string newPath{};
  if (IsDiscImage(path))
  {
    CURL url("udf://");
    url.SetHostName(path);
    newPath = url.Get();
  }
  else if (IsBDFile(path))
    newPath = GetDiscBasePath(path);

  if (!newPath.empty())
  {
    CURL url("bluray://");
    url.SetHostName(newPath);
    newPath = url.Get();
  }

  return newPath;
}

int URIUtils::GetBlurayPlaylistFromPath(const std::string& path)
{
  int playlist{-1};
  if (IsBlurayPath(path))
  {
    CRegExp regex{true, CRegExp::autoUtf8, R"(\/(\d{5}).mpls$)"};
    if (regex.RegFind(path) != -1)
      playlist = std::stoi(regex.GetMatch(1));
  }
  return playlist;
}

bool URIUtils::CompareDiscPaths(const std::string& path1, const std::string& path2)
{
  std::string base1{GetDiscBase(path1)};
  std::string base2{GetDiscBase(path2)};
  return PathEquals(base1, base2, true, true);
}

std::string URIUtils::GetTitleTrailingPartNumberRegex()
{
  return m_advancedSettings->m_titleTrailingPartNumberRegExp;
}

std::string URIUtils::GetTrailingPartNumberRegex()
{
  return m_advancedSettings->m_trailingPartNumberRegExp;
}

std::string URLEncodePath(const std::string& strPath)
{
  std::vector<std::string> segments = StringUtils::Split(strPath, "/");
  for (std::vector<std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Encode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URLDecodePath(const std::string& strPath)
{
  std::vector<std::string> segments = StringUtils::Split(strPath, "/");
  for (std::vector<std::string>::iterator i = segments.begin(); i != segments.end(); ++i)
    *i = CURL::Decode(*i);

  return StringUtils::Join(segments, "/");
}

std::string URIUtils::ChangeBasePath(const std::string &fromPath, const std::string &fromFile, const std::string &toPath, const bool &bAddPath /* = true */)
{
  std::string toFile = fromFile;

  // Convert back slashes to forward slashes, if required
  if (IsDOSPath(fromPath) && !IsDOSPath(toPath))
    StringUtils::Replace(toFile, "\\", "/");

  // Handle difference in URL encoded vs. not encoded
  if ( HasEncodedFilename(CURL(fromPath))
   && !HasEncodedFilename(CURL(toPath)) )
  {
    toFile = URLDecodePath(toFile); // Decode path
  }
  else if (!HasEncodedFilename(CURL(fromPath))
         && HasEncodedFilename(CURL(toPath)) )
  {
    toFile = URLEncodePath(toFile); // Encode path
  }

  // Convert forward slashes to back slashes, if required
  if (!IsDOSPath(fromPath) && IsDOSPath(toPath))
    StringUtils::Replace(toFile, "/", "\\");

  if (bAddPath)
    return AddFileToFolder(toPath, toFile);

  return toFile;
}

CURL URIUtils::SubstitutePath(const CURL& url, bool reverse /* = false */)
{
  const std::string pathToUrl = url.Get();
  return CURL(SubstitutePath(pathToUrl, reverse));
}

std::string URIUtils::SubstitutePath(const std::string& strPath, bool reverse /* = false */)
{
  if (!m_advancedSettings)
  {
    // path substitution not needed / not working during Kodi bootstrap.
    return strPath;
  }

  for (const auto& pathPair : m_advancedSettings->m_pathSubstitutions)
  {
    const std::string fromPath = reverse ? pathPair.second : pathPair.first;
    std::string toPath = reverse ? pathPair.first : pathPair.second;

    if (strncmp(strPath.c_str(), fromPath.c_str(), HasSlashAtEnd(fromPath) ? fromPath.size() - 1 : fromPath.size()) == 0)
    {
      if (strPath.size() > fromPath.size())
      {
        std::string strSubPathAndFileName = strPath.substr(fromPath.size());
        return ChangeBasePath(fromPath, strSubPathAndFileName, toPath); // Fix encoding + slash direction
      }
      else
      {
        return toPath;
      }
    }
  }
  return strPath;
}

bool URIUtils::IsProtocol(const std::string& url, const std::string &type)
{
  return StringUtils::StartsWithNoCase(url, type + "://");
}

bool URIUtils::PathHasParent(std::string path, std::string parent, bool translate /* = false */)
{
  if (translate)
  {
    path = CSpecialProtocol::TranslatePath(path);
    parent = CSpecialProtocol::TranslatePath(parent);
  }

  if (parent.empty())
    return false;

  if (path == parent)
    return true;

  // Make sure parent has a trailing slash
  AddSlashAtEnd(parent);

  return StringUtils::StartsWith(path, parent);
}

bool URIUtils::PathEquals(std::string path1, std::string path2, bool ignoreTrailingSlash /* = false */, bool ignoreURLOptions /* = false */)
{
  if (ignoreURLOptions)
  {
    path1 = CURL(path1).GetWithoutOptions();
    path2 = CURL(path2).GetWithoutOptions();
  }

  if (ignoreTrailingSlash)
  {
    RemoveSlashAtEnd(path1);
    RemoveSlashAtEnd(path2);
  }

  return (path1 == path2);
}

bool URIUtils::IsRemote(const std::string& strFile)
{
  if (IsCDDA(strFile) || IsISO9660(strFile))
    return false;

  if (IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsRemote(CSpecialProtocol::TranslatePath(strFile));

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    std::vector<std::string> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  CURL url(strFile);
  if (url.HasParentInHostname())
    return IsRemote(url.GetHostName());

  if (url.IsAddonsPath())
    return false;

  if (url.IsSourcesPath())
    return false;

  if (url.IsVideoDb() || url.IsMusicDb())
    return false;

  if (url.IsLibraryFolder())
    return false;

  if (url.IsPlugin())
    return false;

  if (url.IsAndroidApp())
    return false;

  if (!url.IsLocal())
    return true;

  return false;
}

bool URIUtils::IsOnDVD(const std::string& strFile)
{
  if (IsProtocol(strFile, "dvd"))
    return true;

  if (IsProtocol(strFile, "udf"))
    return true;

  if (IsProtocol(strFile, "iso9660"))
    return true;

  if (IsProtocol(strFile, "cdda"))
    return true;

#if defined(TARGET_WINDOWS_STORE)
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
#elif defined(TARGET_WINDOWS_DESKTOP)
  using KODI::PLATFORM::WINDOWS::ToW;
  if (strFile.size() >= 2 && strFile.substr(1, 1) == ":")
    return (GetDriveType(ToW(strFile.substr(0, 3)).c_str()) == DRIVE_CDROM);
#endif
  return false;
}

bool URIUtils::IsOnLAN(const std::string& strPath, LanCheckMode lanCheckMode)
{
  if(IsMultiPath(strPath))
    return IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath), lanCheckMode);

  if(IsStack(strPath))
    return IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath), lanCheckMode);

  if(IsSpecial(strPath))
    return IsOnLAN(CSpecialProtocol::TranslatePath(strPath), lanCheckMode);

  if(IsPlugin(strPath))
    return false;

  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if (url.HasParentInHostname())
    return IsOnLAN(url.GetHostName(), lanCheckMode);

  if(!IsRemote(strPath))
    return false;

  const std::string& host = url.GetHostName();

  return IsHostOnLAN(host, lanCheckMode);
}

static bool addr_match(uint32_t addr, const char* target, const char* submask)
{
  uint32_t addr2 = ntohl(inet_addr(target));
  uint32_t mask = ntohl(inet_addr(submask));
  return (addr & mask) == (addr2 & mask);
}

bool URIUtils::IsHostOnLAN(const std::string& host, LanCheckMode lanCheckMode)
{
  if (host.empty())
    return false;

  // assume a hostname without dot's
  // is local (smb netbios hostnames)
  if(host.find('.') == std::string::npos)
    return true;

  uint32_t address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    std::string ip;
    auto cache = CServiceBroker::GetDNSNameCache();
    if (cache && cache->Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address != INADDR_NONE)
  {
    if (lanCheckMode ==
        LanCheckMode::
            ANY_PRIVATE_SUBNET) // check if in private range, ref https://en.wikipedia.org/wiki/Private_network
    {
      if (
        addr_match(address, "192.168.0.0", "255.255.0.0") ||
        addr_match(address, "10.0.0.0", "255.0.0.0") ||
        addr_match(address, "172.16.0.0", "255.240.0.0")
        )
        return true;
    }
    // check if we are on the local subnet
    if (!CServiceBroker::GetNetwork().GetFirstConnectedInterface())
      return false;

    if (CServiceBroker::GetNetwork().HasInterfaceForIP(address))
      return true;
  }

  return false;
}

bool URIUtils::IsMultiPath(const std::string& strPath)
{
  return IsProtocol(strPath, "multipath");
}

bool URIUtils::IsHD(const std::string& strFileName)
{
  CURL url(strFileName);

  if (IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));

  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if (url.HasParentInHostname())
    return IsHD(url.GetHostName());

  return url.GetProtocol().empty() || url.IsProtocol("file") || url.IsProtocol("win-lib") ||
         url.IsProtocol("resource");
}

bool URIUtils::IsDVD(const std::string& strFile)
{
  std::string strFileLow = strFile;
  StringUtils::ToLower(strFileLow);
  if (strFileLow.find("video_ts.ifo") != std::string::npos && IsOnDVD(strFile))
    return true;

#if defined(TARGET_WINDOWS)
  if (IsProtocol(strFile, "dvd"))
    return true;

  if(strFile.size() < 2 || (strFile.substr(1) != ":\\" && strFile.substr(1) != ":"))
    return false;

#ifndef TARGET_WINDOWS_STORE
  if(GetDriveType(KODI::PLATFORM::WINDOWS::ToW(strFile).c_str()) == DRIVE_CDROM)
    return true;
#endif
#else
  if (strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool URIUtils::IsStack(const std::string& strFile)
{
  return IsProtocol(strFile, "stack");
}

bool URIUtils::IsFavourite(const std::string& strFile)
{
  return IsProtocol(strFile, "favourites");
}

bool URIUtils::IsRAR(const std::string& strFile)
{
  std::string strExtension = GetExtension(strFile);

  if (strExtension == ".001" && !StringUtils::EndsWithNoCase(strFile, ".ts.001"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".cbr"))
    return true;

  if (StringUtils::EqualsNoCase(strExtension, ".rar"))
    return true;

  return false;
}

bool URIUtils::IsInArchive(const std::string &strFile)
{
  CURL url(strFile);

  bool archiveProto = url.IsProtocol("archive") && !url.GetFileName().empty();
  return archiveProto || IsInZIP(strFile) || IsInRAR(strFile) || IsInAPK(strFile);
}

bool URIUtils::IsInAPK(const std::string& strFile)
{
  CURL url(strFile);

  return url.IsProtocol("apk") && !url.GetFileName().empty();
}

bool URIUtils::IsInZIP(const std::string& strFile)
{
  CURL url(strFile);

  if (url.GetFileName().empty())
    return false;

  if (url.IsProtocol("archive"))
    return IsZIP(url.GetHostName());

  return url.IsProtocol("zip");
}

bool URIUtils::IsInRAR(const std::string& strFile)
{
  CURL url(strFile);

  if (url.GetFileName().empty())
    return false;

  if (url.IsProtocol("archive"))
    return IsRAR(url.GetHostName());

  return url.IsProtocol("rar");
}

bool URIUtils::IsAPK(const std::string& strFile)
{
  return HasExtension(strFile, ".apk");
}

bool URIUtils::IsZIP(const std::string& strFile) // also checks for comic books!
{
  return HasExtension(strFile, ".zip|.cbz");
}

bool URIUtils::IsArchive(const std::string& strFile)
{
  return HasExtension(strFile, ".zip|.rar|.apk|.cbz|.cbr");
}

bool URIUtils::IsArchive(const CURL& url)
{
  return url.IsProtocol("archive") || url.IsProtocol("zip") || url.IsProtocol("rar");
}

bool URIUtils::IsDiscImage(const std::string& file)
{
  return HasExtension(file, ".img|.iso|.nrg|.udf");
}

bool URIUtils::IsDiscImageStack(const std::string& file)
{
  if (IsStack(file))
  {
    std::vector<std::string> paths;
    CStackDirectory::GetPaths(file, paths);
    for (const std::string& path : paths)
      if (IsDiscImage(path) || IsDVDFile(path) || IsBDFile(path))
        return true;
  }
  return false;
}

bool URIUtils::IsSpecial(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsSpecial(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "special");
}

bool URIUtils::IsPlugin(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("plugin");
}

bool URIUtils::IsScript(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("script");
}

bool URIUtils::IsAddonsPath(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("addons");
}

bool URIUtils::IsSourcesPath(const std::string& strPath)
{
  CURL url(strPath);
  return url.IsProtocol("sources");
}

bool URIUtils::IsCDDA(const std::string& strFile)
{
  return IsProtocol(strFile, "cdda");
}

bool URIUtils::IsISO9660(const std::string& strFile)
{
  return IsProtocol(strFile, "iso9660");
}

bool URIUtils::IsSmb(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsSmb(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsSmb(CSpecialProtocol::TranslatePath(strFile));

  if (const CURL url{strFile}; url.HasParentInHostname())
    return IsSmb(url.GetHostName());

  return IsProtocol(strFile, "smb");
}

bool URIUtils::IsURL(const std::string& strFile)
{
  return strFile.find("://") != std::string::npos;
}

bool URIUtils::IsFTP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsFTP(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsFTP(CSpecialProtocol::TranslatePath(strFile));

  if (const CURL url{strFile}; url.HasParentInHostname())
    return IsFTP(url.GetHostName());

  return IsProtocol(strFile, "ftp") ||
         IsProtocol(strFile, "ftps");
}

bool URIUtils::IsHTTP(const std::string& strFile, bool bTranslate /* = false */)
{
  if (IsStack(strFile))
    return IsHTTP(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsHTTP(CSpecialProtocol::TranslatePath(strFile));

  CURL url(strFile);
  if (url.HasParentInHostname())
    return IsHTTP(url.GetHostName());

  const std::string strProtocol = (bTranslate ? url.GetTranslatedProtocol() : url.GetProtocol());

  return (strProtocol == "http" || strProtocol == "https");
}

bool URIUtils::IsUDP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsUDP(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "udp");
}

bool URIUtils::IsTCP(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsTCP(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "tcp");
}

bool URIUtils::IsPVR(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVR(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr");
}

bool URIUtils::IsPVRChannel(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRChannel(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr") && CPVRChannelsPath(strFile).IsChannel();
}

bool URIUtils::IsPVRRadioChannel(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRRadioChannel(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsProtocol(strFile, "pvr"))
  {
    const CPVRChannelsPath path{strFile};
    return path.IsChannel() && path.IsRadio();
  }
  return false;
}

bool URIUtils::IsPVRChannelGroup(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRChannelGroup(CStackDirectory::GetFirstStackedFile(strFile));

  return IsProtocol(strFile, "pvr") && CPVRChannelsPath(strFile).IsChannelGroup();
}

bool URIUtils::IsPVRGuideItem(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsPVRGuideItem(CStackDirectory::GetFirstStackedFile(strFile));

  return StringUtils::StartsWithNoCase(strFile, "pvr://guide");
}

bool URIUtils::IsDAV(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsDAV(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsDAV(CSpecialProtocol::TranslatePath(strFile));

  if (const CURL url{strFile}; url.HasParentInHostname())
    return IsDAV(url.GetHostName());

  return IsProtocol(strFile, "dav") ||
         IsProtocol(strFile, "davs");
}

bool URIUtils::IsInternetStream(const std::string &path, bool bStrictCheck /* = false */)
{
  const CURL pathToUrl(path);
  return IsInternetStream(pathToUrl, bStrictCheck);
}

bool URIUtils::IsInternetStream(const CURL& url, bool bStrictCheck /* = false */)
{
  if (url.GetProtocol().empty())
    return false;

  // there's nothing to stop internet streams from being stacked
  if (url.IsProtocol("stack"))
    return IsInternetStream(CStackDirectory::GetFirstStackedFile(url.Get()), bStrictCheck);

  // Only consider "streamed" filesystems internet streams when being strict
  if (bStrictCheck && IsStreamedFilesystem(url.Get()))
    return true;

  // Check for true internetstreams
  const std::string& protocol = url.GetProtocol();
  if (CURL::IsProtocolEqual(protocol, "http") || CURL::IsProtocolEqual(protocol, "https") ||
      CURL::IsProtocolEqual(protocol, "tcp") || CURL::IsProtocolEqual(protocol, "udp") ||
      CURL::IsProtocolEqual(protocol, "rtp") || CURL::IsProtocolEqual(protocol, "sdp") ||
      CURL::IsProtocolEqual(protocol, "mms") || CURL::IsProtocolEqual(protocol, "mmst") ||
      CURL::IsProtocolEqual(protocol, "mmsh") || CURL::IsProtocolEqual(protocol, "rtsp") ||
      CURL::IsProtocolEqual(protocol, "rtmp") || CURL::IsProtocolEqual(protocol, "rtmpt") ||
      CURL::IsProtocolEqual(protocol, "rtmpe") || CURL::IsProtocolEqual(protocol, "rtmpte") ||
      CURL::IsProtocolEqual(protocol, "rtmps") || CURL::IsProtocolEqual(protocol, "shout") ||
      CURL::IsProtocolEqual(protocol, "rss") || CURL::IsProtocolEqual(protocol, "rsss"))
    return true;

  return false;
}

bool URIUtils::IsStreamedFilesystem(const std::string& strPath)
{
  CURL url(strPath);

  if (url.GetProtocol().empty())
    return false;

  if (url.IsProtocol("stack"))
    return IsStreamedFilesystem(CStackDirectory::GetFirstStackedFile(strPath));

  if (IsUPnP(strPath) || IsFTP(strPath) || IsHTTP(strPath, true))
    return true;

  //! @todo sftp/ssh special case has to be handled by vfs addon
  if (url.IsProtocol("sftp") || url.IsProtocol("ssh"))
    return true;

  return false;
}

bool URIUtils::IsNetworkFilesystem(const std::string& strPath)
{
  CURL url(strPath);

  if (url.GetProtocol().empty())
    return false;

  if (url.IsProtocol("stack"))
    return IsNetworkFilesystem(CStackDirectory::GetFirstStackedFile(strPath));

  if (IsStreamedFilesystem(strPath))
    return true;

  if (IsSmb(strPath) || IsNfs(strPath))
    return true;

  return false;
}

bool URIUtils::IsUPnP(const std::string& strFile)
{
  return IsProtocol(strFile, "upnp");
}

bool URIUtils::IsLiveTV(const std::string& strFile)
{
  std::string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  if (StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
      !StringUtils::StartsWith(strFileWithoutSlash, "pvr://recordings"))
    return true;

  return false;
}

bool URIUtils::IsPVRRecording(const std::string& strFile)
{
  std::string strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  return StringUtils::EndsWithNoCase(strFileWithoutSlash, ".pvr") &&
         StringUtils::StartsWith(strFile, "pvr://recordings");
}

bool URIUtils::IsPVRRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings");
}

bool URIUtils::IsPVRTVRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings/tv");
}

bool URIUtils::IsPVRRadioRecordingFileOrFolder(const std::string& strFile)
{
  return StringUtils::StartsWith(strFile, "pvr://recordings/radio");
}

bool URIUtils::IsMusicDb(const std::string& strFile)
{
  return IsProtocol(strFile, "musicdb");
}

bool URIUtils::IsNfs(const std::string& strFile)
{
  if (IsStack(strFile))
    return IsNfs(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsSpecial(strFile))
    return IsNfs(CSpecialProtocol::TranslatePath(strFile));

  if (const CURL url{strFile}; url.HasParentInHostname())
    return IsNfs(url.GetHostName());

  return IsProtocol(strFile, "nfs");
}

bool URIUtils::IsVideoDb(const std::string& strFile)
{
  return IsProtocol(strFile, "videodb");
}

bool URIUtils::IsBlurayPath(const std::string& strFile)
{
  return IsProtocol(strFile, "bluray");
}

bool URIUtils::IsBlurayMenuPath(const std::string& file)
{
  return IsBlurayPath(file) && GetFileName(file) == "menu";
}

bool URIUtils::IsOpticalMediaFile(const std::string& file)
{
  return IsBDFile(file) || IsDVDFile(file);
}

bool URIUtils::IsBDFile(const std::string& file)
{
  const std::string fileName{GetFileName(file)};
  return StringUtils::EqualsNoCase(fileName, "index.bdmv") ||
         StringUtils::EqualsNoCase(fileName, "MovieObject.bdmv") ||
         StringUtils::EqualsNoCase(fileName, "INDEX.BDM") ||
         StringUtils::EqualsNoCase(fileName, "MOVIEOBJ.BDM");
}

bool URIUtils::IsDVDFile(const std::string& file)
{
  const std::string fileName{GetFileName(file)};
  return StringUtils::EqualsNoCase(fileName, "video_ts.ifo") ||
         (StringUtils::StartsWithNoCase(fileName, "vts_") &&
          StringUtils::EndsWithNoCase(fileName, "_0.ifo") && fileName.length() == 12);
}

bool URIUtils::IsAndroidApp(const std::string &path)
{
  return IsProtocol(path, "androidapp");
}

bool URIUtils::IsLibraryFolder(const std::string& strFile)
{
  CURL url(strFile);
  return url.IsProtocol("library");
}

bool URIUtils::IsLibraryContent(const std::string &strFile)
{
  return (IsProtocol(strFile, "library") ||
          IsProtocol(strFile, "videodb") ||
          IsProtocol(strFile, "musicdb") ||
          StringUtils::EndsWith(strFile, ".xsp"));
}

bool URIUtils::IsDOSPath(const std::string& path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

bool URIUtils::IsAbsolutePOSIXPath(const std::string& path)
{
  if (path.size() > 1 && path[0] == '/')
    return true;

  return false;
}

std::string URIUtils::AppendSlash(std::string strFolder)
{
  AddSlashAtEnd(strFolder);
  return strFolder;
}

void URIUtils::AddSlashAtEnd(std::string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if(!file.empty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
    }
    return;
  }

  if (!HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

bool URIUtils::HasSlashAtEnd(const std::string& strFile, bool checkURL /* = false */)
{
  if (strFile.empty()) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    const std::string& file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(std::string& strFolder)
{
  // performance optimization. pvr guide items are mass objects, uri never has a slash at end, and this method is quite expensive...
  if (IsPVRGuideItem(strFolder))
    return;

  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if (!file.empty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().empty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}

bool URIUtils::CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2)
{
  std::string strc1 = strPath1, strc2 = strPath2;
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return StringUtils::EqualsNoCase(strc1, strc2);
}


std::string URIUtils::FixSlashesAndDups(const std::string& path, const char slashCharacter /* = '/' */, const size_t startFrom /*= 0*/)
{
  const size_t len = path.length();
  if (startFrom >= len)
    return path;

  std::string result(path, 0, startFrom);
  result.reserve(len);

  const char* const str = path.c_str();
  size_t pos = startFrom;
  do
  {
    if (str[pos] == '\\' || str[pos] == '/')
    {
      result.push_back(slashCharacter);  // append one slash
      pos++;
      // skip any following slashes
      while (str[pos] == '\\' || str[pos] == '/') // str is null-terminated, no need to check for buffer overrun
        pos++;
    }
    else
      result.push_back(str[pos++]);   // append current char and advance pos to next char

  } while (pos < len);

  return result;
}


std::string URIUtils::CanonicalizePath(const std::string& path, const char slashCharacter /*= '\\'*/)
{
  assert(slashCharacter == '\\' || slashCharacter == '/');

  if (path.empty())
    return path;

  const std::string slashStr(1, slashCharacter);
  std::vector<std::string> pathVec, resultVec;
  StringUtils::Tokenize(path, pathVec, slashStr);

  for (std::vector<std::string>::const_iterator it = pathVec.begin(); it != pathVec.end(); ++it)
  {
    if (*it == ".")
    { /* skip - do nothing */ }
    else if (*it == ".." && !resultVec.empty() && resultVec.back() != "..")
      resultVec.pop_back();
    else
      resultVec.push_back(*it);
  }

  std::string result;
  if (path[0] == slashCharacter)
    result.push_back(slashCharacter); // add slash at the begin

  result += StringUtils::Join(resultVec, slashStr);

  if (path[path.length() - 1] == slashCharacter  && !result.empty() && result[result.length() - 1] != slashCharacter)
    result.push_back(slashCharacter); // add slash at the end if result isn't empty and result isn't "/"

  return result;
}

std::string URIUtils::AddFileToFolder(const std::string& strFolder,
                                const std::string& strFile)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      url.SetFileName(AddFileToFolder(url.GetFileName(), strFile));
      return url.Get();
    }
  }

  std::string strResult = strFolder;
  if (!strResult.empty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (!strFile.empty() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.substr(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    StringUtils::Replace(strResult, '\\', '/');
  else
    StringUtils::Replace(strResult, '/', '\\');

  return strResult;
}

std::string URIUtils::GetDirectory(const std::string &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == std::string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == std::string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}

CURL URIUtils::CreateArchivePath(const std::string& type,
                                 const CURL& archiveUrl,
                                 const std::string& pathInArchive,
                                 const std::string& password)
{
  CURL url;
  url.SetProtocol(type);
  if (!password.empty())
    url.SetUserName(password);
  url.SetHostName(archiveUrl.Get());

  /* NOTE: on posix systems, the replacement of \ with / is incorrect.
     Ideally this would not be done. We need to check that the ZipManager
     code (and elsewhere) doesn't pass in non-posix paths.
   */
  std::string strBuffer(pathInArchive);
  StringUtils::Replace(strBuffer, '\\', '/');
  StringUtils::TrimLeft(strBuffer, "/");
  url.SetFileName(strBuffer);

  return url;
}

std::string URIUtils::GetRealPath(const std::string &path)
{
  if (path.empty())
    return path;

  CURL url(path);
  url.SetHostName(GetRealPath(url.GetHostName()));
  url.SetFileName(resolvePath(url.GetFileName()));

  return url.Get();
}

std::string URIUtils::resolvePath(const std::string &path)
{
  if (path.empty())
    return path;

  size_t posSlash = path.find('/');
  size_t posBackslash = path.find('\\');
  std::string delim = posSlash < posBackslash ? "/" : "\\";
  std::vector<std::string> parts = StringUtils::Split(path, delim);
  std::vector<std::string> realParts;

  for (std::vector<std::string>::const_iterator part = parts.begin(); part != parts.end(); ++part)
  {
    if (part->empty() || part->compare(".") == 0)
      continue;

    // go one level back up
    if (part->compare("..") == 0)
    {
      if (!realParts.empty())
        realParts.pop_back();
      continue;
    }

    realParts.push_back(*part);
  }

  std::string realPath;
  // re-add any / or \ at the beginning
  for (std::string::const_iterator itPath = path.begin(); itPath != path.end(); ++itPath)
  {
    if (*itPath != delim.at(0))
      break;

    realPath += delim;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) && !realPath.empty() &&
      realPath.at(realPath.size() - 1) != delim.at(0))
    realPath += delim;

  return realPath;
}

bool URIUtils::UpdateUrlEncoding(std::string &strFilename)
{
  if (strFilename.empty())
    return false;

  CURL url(strFilename);
  // if this is a stack:// URL we need to work with its filename
  if (URIUtils::IsStack(strFilename))
  {
    std::vector<std::string> files;
    if (!CStackDirectory::GetPaths(strFilename, files))
      return false;

    for (std::vector<std::string>::iterator file = files.begin(); file != files.end(); ++file)
      UpdateUrlEncoding(*file);

    std::string stackPath;
    if (!CStackDirectory::ConstructStackPath(files, stackPath))
      return false;

    url.Parse(stackPath);
  }
  // if the protocol has an encoded hostname we need to work with its hostname
  else if (URIUtils::HasEncodedHostname(url))
  {
    std::string hostname = url.GetHostName();
    UpdateUrlEncoding(hostname);
    url.SetHostName(hostname);
  }
  else
    return false;

  std::string newFilename = url.Get();
  if (newFilename == strFilename)
    return false;

  strFilename = newFilename;
  return true;
}

CURL URIUtils::AddCredentials(CURL url)
{
  if (CPasswordManager::GetInstance().IsURLSupported(url) && url.GetUserName().empty())
    CPasswordManager::GetInstance().AuthenticateURL(url);
  return url;
}
