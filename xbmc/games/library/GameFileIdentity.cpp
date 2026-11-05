/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameFileIdentity.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/VFSEntry.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/ZipManager.h"
#include "utils/Digest.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <vector>

#include <zlib.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1 MiB

constexpr std::array<const char*, 2> sheetExtensions{".cue", ".gdi"};
constexpr std::array<const char*, 3> programExtensions{".exe", ".com", ".bat"};

constexpr std::string_view SATURN_MAGIC = "SEGA SEGASATURN";
constexpr std::string_view MEGACD_MAGIC = "SEGADISCSYSTEM";
constexpr std::string_view DREAMCAST_MAGIC = "SEGA SEGAKATANA";

//! A track that a cue sheet or GDI file names has its header near the start.
//! Looking further into one in an archive would decompress the whole track.
constexpr uint64_t MAX_ARCHIVED_TRACK_SCAN = 16 * 1024 * 1024; // 16 MiB

// The protocol that lists an archive's files: zip's own, or the one the VFS
// add-on that opens the extension registers, such as vfs.libarchive's for 7z
// and tar and vfs.rar's for rar
std::string ArchiveProtocol(const std::string& path)
{
  const std::string ext = StringUtils::ToLower(URIUtils::GetExtension(path));
  if (ext == ".zip")
    return "zip";
  if (ext.empty() || !CServiceBroker::IsServiceManagerUp())
    return "";

  for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
  {
    if (!vfsAddon->HasFileDirectories())
      continue;
    const std::vector<std::string> extensions = StringUtils::Split(vfsAddon->GetExtensions(), "|");
    const std::vector<std::string> protocols = StringUtils::Split(vfsAddon->GetProtocols(), "|");
    if (!protocols.empty() && std::ranges::find(extensions, ext) != extensions.end())
      return protocols.front();
  }
  return "";
}

bool HasExtension(const std::string& path, const auto& extensions)
{
  const std::string ext = StringUtils::ToLower(URIUtils::GetExtension(path));
  return std::ranges::any_of(extensions, [&ext](const char* e) { return ext == e; });
}

// A folder inside an archive, listed the way CGameFileIdentity::ArchiveFiles()
// lists the archive
bool ListArchiveFolder(const std::string& folder, CFileItemList& items)
{
  return XFILE::CDirectory::GetDirectory(
      folder, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
}

// The cue sheet or GDI file among an archive's files. A disc packed with its
// folder keeps it below the top.
std::string FindSheet(const CFileItemList& items)
{
  for (const auto& item : items)
  {
    if (!item->IsFolder() && HasExtension(item->GetPath(), sheetExtensions))
      return item->GetPath();
  }
  for (const auto& item : items)
  {
    CFileItemList inner;
    if (item->IsFolder() && ListArchiveFolder(item->GetPath(), inner))
    {
      if (std::string sheet = FindSheet(inner); !sheet.empty())
        return sheet;
    }
  }
  return "";
}

// An archive's files, with those in its folders
void AddFiles(const CFileItemList& items, std::vector<std::shared_ptr<CFileItem>>& files)
{
  for (const auto& item : items)
  {
    CFileItemList inner;
    if (!item->IsFolder())
      files.push_back(item);
    else if (ListArchiveFolder(item->GetPath(), inner))
      AddFiles(inner, files);
  }
}

std::string ReadHead(const std::string& path, size_t bytes)
{
  XFILE::CFile file;
  if (!file.Open(path, XFILE::READ_NO_CACHE))
    return "";

  std::string data;
  data.resize(bytes);
  size_t got = 0;
  while (got < bytes)
  {
    const ssize_t n = file.Read(data.data() + got, bytes - got);
    if (n <= 0)
      break;
    got += static_cast<size_t>(n);
  }
  data.resize(got);
  return data;
}

// Streams a zip member through the decompressor rather than letting the zip
// backend copy it whole into special://temp first, which it does for anything
// deflated over 4 MB and never cleans up. Everything here reads front to back,
// so nothing needs to seek. Other archives are read by VFS add-ons, which take
// the option as part of the name and open nothing.
std::string Uncached(const std::string& memberPath)
{
  CURL url(memberPath);
  if (url.IsProtocol("zip"))
    url.SetOptions("?cache=no");
  return url.Get();
}

// Empty until the whole field has been read
std::string HeaderField(const std::string& data, size_t offset, size_t length)
{
  if (offset + length > data.size())
    return "";
  std::string value = data.substr(offset, length);
  const size_t nul = value.find('\0');
  if (nul != std::string::npos)
    value.erase(nul);
  StringUtils::Trim(value);
  return value;
}
} // namespace

std::string CGameFileIdentity::NormaliseSerial(std::string serial)
{
  StringUtils::Trim(serial);
  if (serial.starts_with("GM "))
    serial.erase(0, 3);
  // Saturn appends a version; Mega-CD a region suffix. The first word is the serial.
  const size_t space = serial.find_first_of(" \t");
  if (space != std::string::npos)
    serial.erase(space);

  std::string out;
  for (const char c : serial)
  {
    if (std::isalnum(static_cast<unsigned char>(c)))
      out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return out;
}

std::string CGameFileIdentity::FirstTrack(const std::string& sheetPath)
{
  const std::string sheet = ReadHead(sheetPath, 64 * 1024);
  if (sheet.empty())
    return "";

  const std::string folder = URIUtils::GetDirectory(sheetPath);
  const std::string ext = StringUtils::ToLower(URIUtils::GetExtension(sheetPath));

  if (ext == ".gdi")
  {
    // Line: <track> <lba> <type> <sector size> <file> <offset>; track 3 holds the
    // data. A file name with spaces is quoted.
    CRegExp gdi(true);
    if (gdi.RegComp("(?m)^[ \\t]*3[ \\t]+[0-9]+[ \\t]+[0-9]+[ \\t]+[0-9]+[ \\t]+"
                    "(?:\"([^\"]+)\"|([^ \\t\\r\\n]+))") &&
        gdi.RegFind(sheet) >= 0)
    {
      const std::string quoted = gdi.GetMatch(1);
      return URIUtils::AddFileToFolder(folder, quoted.empty() ? gdi.GetMatch(2) : quoted);
    }
    return "";
  }

  CRegExp re(true);
  if (re.RegComp("FILE\\s+\"([^\"]+)\"") && re.RegFind(sheet) >= 0)
    return URIUtils::AddFileToFolder(folder, re.GetMatch(1));
  if (re.RegComp("FILE\\s+(\\S+)") && re.RegFind(sheet) >= 0)
    return URIUtils::AddFileToFolder(folder, re.GetMatch(1));
  return "";
}

std::string CGameFileIdentity::SerialFromHeader(const std::string& data,
                                                size_t at,
                                                DiscHeader header)
{
  if (header == DiscHeader::DREAMCAST)
  {
    // "MK-51171" or "T40201N", sometimes with a second field after a space
    std::string serial = HeaderField(data, at + 0x40, 0x10);
    const size_t space = serial.find(' ');
    if (space != std::string::npos)
      serial.erase(space);
    return NormaliseSerial(serial);
  }

  if (header == DiscHeader::SATURN)
    return NormaliseSerial(HeaderField(data, at + 0x20, 0x10));

  std::string serial = HeaderField(data, at + 0x180, 0x10);
  if (serial.starts_with("GM "))
    serial.erase(0, 3);
  const size_t suffix = serial.find("-00");
  if (suffix != std::string::npos)
    serial.erase(suffix);
  return NormaliseSerial(serial);
}

std::string CGameFileIdentity::ReadDiscSerial(const std::string& trackPath, uint64_t maxRead)
{
  XFILE::CFile file;
  if (!file.Open(trackPath, XFILE::READ_NO_CACHE))
    return "";

  // The system area sits at the start of a plain track, but a container such
  // as DiscJuggler's puts the data track wherever it lands: measured on real
  // images, anywhere from 1 MB to 460 MB in. So the file is read through until
  // the header appears and reading stops the moment it does, which is no
  // dearer than hashing the file and usually far cheaper.
  static constexpr std::array<std::pair<std::string_view, DiscHeader>, 3> magics{{
      {DREAMCAST_MAGIC, DiscHeader::DREAMCAST},
      {SATURN_MAGIC, DiscHeader::SATURN},
      {MEGACD_MAGIC, DiscHeader::MEGA_CD},
  }};

  // PlayStation names itself in text rather than in a header
  CRegExp playStation(true);
  if (!playStation.RegComp("BOOT\\s*=\\s*cdrom:\\\\?([A-Z]{4})[_-]?([0-9]{3})\\.?([0-9]{2})"))
    return "";

  // Enough of the last chunk is kept for a header that straddles the join
  static constexpr size_t OVERLAP = 0x400;

  std::vector<char> buffer(CHUNK_SIZE);
  std::string window;
  uint64_t read = 0;

  while (read < maxRead)
  {
    const ssize_t n = file.Read(buffer.data(), buffer.size());
    if (n <= 0)
      break;
    read += static_cast<uint64_t>(n);
    window.append(buffer.data(), static_cast<size_t>(n));

    for (const auto& [magic, header] : magics)
    {
      const size_t at = window.find(magic);
      if (at == std::string::npos)
        continue;
      const std::string serial = SerialFromHeader(window, at, header);
      if (!serial.empty())
        return serial;
    }

    if (playStation.RegFind(window) >= 0)
      return NormaliseSerial(playStation.GetMatch(1) + playStation.GetMatch(2) +
                             playStation.GetMatch(3));

    if (window.size() > OVERLAP)
      window.erase(0, window.size() - OVERLAP);
  }

  return "";
}

bool CGameFileIdentity::HashWhole(const std::string& path, GameFile& file, uint64_t maxSize)
{
  XFILE::CFile in;
  if (!in.Open(path, XFILE::READ_NO_CACHE))
    return false;

  // The catalogues key ROMs by the CRC-32 that PKZIP and the No-Intro sets use.
  // Kodi's own Crc32 is the non-reflected variant and matches none of them.
  uLong crc = crc32(0UL, nullptr, 0);
  KODI::UTILITY::CDigest md5(KODI::UTILITY::CDigest::Type::MD5);
  std::vector<char> buffer(CHUNK_SIZE);
  uint64_t total = 0;

  while (true)
  {
    const ssize_t n = in.Read(buffer.data(), buffer.size());
    if (n < 0)
      return false;
    if (n == 0)
      break;
    crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), static_cast<uInt>(n));
    md5.Update(buffer.data(), static_cast<size_t>(n));
    total += static_cast<uint64_t>(n);
    if (total > maxSize)
      return false;
  }

  // A read that ends early hashes part of the file, which matches nothing
  if (file.size != 0 && total != file.size)
    return false;

  file.crc32 = StringUtils::Format("{:08x}", static_cast<uint32_t>(crc));
  file.md5 = StringUtils::ToLower(md5.Finalize());
  if (file.size == 0)
    file.size = total;
  return true;
}

bool CGameFileIdentity::ListZip(const std::string& path, std::vector<ArchiveMember>& members)
{
  std::vector<SZipEntry> entries;
  if (!g_ZipManager.GetZipList(URIUtils::CreateArchivePath("zip", CURL(path), ""), entries))
    return false;

  for (const SZipEntry& entry : entries)
  {
    const std::string name = entry.name;
    if (name.empty() || name.back() == '/')
      continue;
    members.push_back({name, entry.usize, StringUtils::Format("{:08x}", entry.crc32)});
  }
  return true;
}

bool CGameFileIdentity::HashArchive(const std::string& path, GameFile& file, uint64_t maxSize)
{
  // A zip of several files may be an arcade set, which is known by all of them.
  // A zip of one keeps that file's CRC in its directory, which still names it
  // when it is too large to hash.
  std::vector<ArchiveMember> members;
  if (StringUtils::ToLower(URIUtils::GetExtension(path)) == ".zip")
    ListZip(path, members);
  if (members.size() > 1)
    file.members = members;

  if (HashLargestMember(path, file, maxSize))
    return true;

  if (members.size() != 1)
    return !file.members.empty();

  file.crc32 = members.front().crc32;
  file.size = members.front().size;
  return true;
}

bool CGameFileIdentity::ArchiveFiles(const std::string& path, CFileItemList& files)
{
  const std::string protocol = ArchiveProtocol(path);
  if (protocol.empty())
    return false;
  const std::string archiveUrl = URIUtils::CreateArchivePath(protocol, CURL(path), "").Get();

  // The members are wanted as files. Letting Kodi convert them to directories
  // offers each one to every VFS add-on in turn to ask whether it is itself an
  // archive, so one bad add-on takes the whole scan down with it: vfs.rar
  // segfaults on some names, and a library scan is thousands of chances to hit
  // one. Nothing here needs a nested archive opened, only the member list.
  return XFILE::CDirectory::GetDirectory(
      archiveUrl, files, "", XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_NO_FILE_INFO);
}

bool CGameFileIdentity::HashLargestMember(const std::string& path, GameFile& file, uint64_t maxSize)
{
  // The largest member is the ROM
  CFileItemList members;
  if (!ArchiveFiles(path, members))
    return false;

  std::vector<std::shared_ptr<CFileItem>> files;
  AddFiles(members, files);

  std::shared_ptr<CFileItem> best;
  int count = 0;
  bool program = false;
  for (const auto& member : files)
  {
    ++count;
    program = program || HasExtension(member->GetPath(), programExtensions);
    if (!best || member->GetSize() > best->GetSize())
      best = member;
  }

  // An archive of a program and its files is an installed game. Its largest
  // file is one of the game's own, often one other games share, and no
  // catalogue knows the game by it.
  if (count > 1 && program)
    return false;
  if (!best || best->GetSize() > static_cast<int64_t>(maxSize))
    return false;

  GameFile member;
  member.size = static_cast<uint64_t>(best->GetSize());
  if (!HashWhole(Uncached(best->GetPath()), member, maxSize))
    return false;

  file.crc32 = member.crc32;
  file.md5 = member.md5;
  file.size = member.size;
  return true;
}

std::string CGameFileIdentity::ReadArchivedDiscSerial(const std::string& path)
{
  CFileItemList members;
  if (!ArchiveFiles(path, members))
    return "";

  // Only a disc whose cue sheet or GDI file names its data track is read. The
  // header of any other disc may not be one this knows.
  const std::string sheet = FindSheet(members);
  if (sheet.empty())
    return "";
  const std::string track = FirstTrack(sheet);
  return track.empty() ? "" : ReadDiscSerial(Uncached(track), MAX_ARCHIVED_TRACK_SCAN);
}

bool CGameFileIdentity::Identify(const std::string& path, GameFile& file, MediaFormat media)
{
  file = GameFile{};
  file.path = path;

  XFILE::CFile probe;
  struct __stat64 st = {};
  if (probe.Stat(path, &st) == 0)
    file.size = static_cast<uint64_t>(st.st_size);

  try
  {
    if (HasExtension(path, sheetExtensions))
    {
      const std::string track = FirstTrack(path);
      if (!track.empty())
        file.serial = ReadDiscSerial(track);
      return !file.serial.empty();
    }

    const uint64_t limit = media == MediaFormat::UNKNOWN ? MAX_HASH_SIZE : MAX_ROM_HASH_SIZE;

    if (!ArchiveProtocol(path).empty())
    {
      if (media != MediaFormat::DISC)
        return HashArchive(path, file, limit);
      file.serial = ReadArchivedDiscSerial(path);
      return !file.serial.empty();
    }

    // A disc is known by the serial printed in its own header. Hashing the
    // container matches nothing: the catalogues key discs by serial, and every
    // rip of one game differs by however it was made.
    // A file too large for a cartridge is taken for a disc only when the
    // platform's media isn't known.
    if (media == MediaFormat::DISC || (media == MediaFormat::UNKNOWN && file.size > MAX_HASH_SIZE))
    {
      file.serial = ReadDiscSerial(path);
      return !file.serial.empty();
    }

    if (file.size > limit)
      return false;

    return HashWhole(path, file, limit);
  }
  catch (...)
  {
    CLog::Log(LOGWARNING, "GAME: Could not identify {}", path);
  }
  return false;
}
