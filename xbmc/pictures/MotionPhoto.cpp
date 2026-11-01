/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MotionPhoto.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/IRunnable.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

using namespace XFILE;

namespace
{
constexpr std::string_view CLIP_FOLDER = "special://temp/motionphotos/";

// The XMP packet is near the start of the file, after the EXIF data
constexpr int64_t XMP_SEARCH_SIZE = 256 * 1024;
// Apple's maker note is in the EXIF data at the start of the file
constexpr int64_t MAKER_NOTE_SEARCH_SIZE = 64 * 1024;
// The maker note tag holding the ID a Live Photo's still shares with its clip
constexpr uint16_t APPLE_CONTENT_IDENTIFIER = 0x0011;

std::string_view GetXmpValue(std::string_view xmp, std::string_view name)
{
  const std::string attribute = std::string(name) + "=\"";
  size_t start = xmp.find(attribute);
  if (start != std::string_view::npos)
  {
    start += attribute.size();
    const size_t end = xmp.find('"', start);
    if (end != std::string_view::npos)
      return xmp.substr(start, end - start);
  }

  const std::string element = "<" + std::string(name) + ">";
  start = xmp.find(element);
  if (start != std::string_view::npos)
  {
    start += element.size();
    const size_t end = xmp.find('<', start);
    if (end != std::string_view::npos)
      return xmp.substr(start, end - start);
  }

  return {};
}

uint64_t ToNumber(std::string_view value)
{
  uint64_t number = 0;
  std::from_chars(value.data(), value.data() + value.size(), number);
  return number;
}

/*!
 * \brief Size of the clip at the end of a Motion Photo, or 0 if the XMP doesn't describe one
 */
uint64_t GetClipSize(std::string_view xmp)
{
  // Motion Photo format: the clip is the last item of the container directory
  if (GetXmpValue(xmp, "GCamera:MotionPhoto") == "1" ||
      GetXmpValue(xmp, "Camera:MotionPhoto") == "1")
  {
    const size_t semantic = xmp.find("Item:Semantic=\"MotionPhoto\"");
    if (semantic == std::string_view::npos)
      return 0;

    const size_t start = xmp.rfind('<', semantic);
    const size_t end = xmp.find('>', semantic);
    if (start == std::string_view::npos || end == std::string_view::npos)
      return 0;

    return ToNumber(GetXmpValue(xmp.substr(start, end - start), "Item:Length"));
  }

  // Older MicroVideo format
  if (GetXmpValue(xmp, "GCamera:MicroVideo") == "1")
    return ToNumber(GetXmpValue(xmp, "GCamera:MicroVideoOffset"));
  if (GetXmpValue(xmp, "Camera:MicroVideo") == "1")
    return ToNumber(GetXmpValue(xmp, "Camera:MicroVideoOffset"));

  return 0;
}

/*!
 * \brief Whether a picture is the still of an Apple Live Photo
 */
bool IsLivePhotoStill(const std::string& path)
{
  CFile file;
  if (!file.Open(path))
    return false;

  std::string head(static_cast<size_t>(std::min(file.GetLength(), MAKER_NOTE_SEARCH_SIZE)), '\0');
  if (head.empty() || file.Read(head.data(), head.size()) != static_cast<ssize_t>(head.size()))
    return false;

  // "Apple iOS", a version and the byte order, then the tags
  const size_t note = head.find(std::string_view("Apple iOS\0", 10));
  if (note == std::string::npos || note + 16 > head.size())
    return false;

  const std::string_view bytes = std::string_view(head).substr(note);
  const bool bigEndian = bytes.substr(12, 2) == "MM";
  const auto read16 = [bytes, bigEndian](size_t offset)
  {
    const auto first = static_cast<uint8_t>(bytes[offset]);
    const auto second = static_cast<uint8_t>(bytes[offset + 1]);
    return static_cast<uint16_t>(bigEndian ? first << 8 | second : second << 8 | first);
  };

  const size_t count = read16(14);
  for (size_t i = 0; i < count && 16 + (i + 1) * 12 <= bytes.size(); ++i)
  {
    if (read16(16 + i * 12) == APPLE_CONTENT_IDENTIFIER)
      return true;
  }
  return false;
}

class CLivePhotoCheck : public IRunnable
{
public:
  explicit CLivePhotoCheck(std::vector<std::string> pictures)
    : m_pictures(std::move(pictures)),
      m_isLivePhoto(m_pictures.size(), false)
  {
  }

  void Run() override
  {
    for (size_t i = 0; i < m_pictures.size() && !m_cancelled; ++i)
      m_isLivePhoto[i] = IsLivePhotoStill(m_pictures[i]);
  }

  void Cancel() override { m_cancelled = true; }

  bool IsLivePhoto(size_t i) const { return m_isLivePhoto[i]; }

private:
  const std::vector<std::string> m_pictures;
  std::vector<bool> m_isLivePhoto;
  std::atomic<bool> m_cancelled{false};
};

std::string GetLivePhotoKey(const std::string& path)
{
  std::string key = path;
  URIUtils::RemoveExtension(key);
  StringUtils::ToLower(key);
  return key;
}
} // namespace

namespace KODI::PICTURE
{
void GroupLivePhotos(CFileItemList& items)
{
  std::map<std::string, std::shared_ptr<CFileItem>, std::less<>> pictures;
  for (const auto& item : items)
  {
    if (!item->IsFolder() && item->IsPicture())
      pictures.try_emplace(GetLivePhotoKey(item->GetPath()), item);
  }

  if (pictures.empty())
    return;

  // Clips paired with a picture by name, last first so they can be removed by index
  std::vector<std::pair<int, std::shared_ptr<CFileItem>>> clips;
  std::vector<std::string> paths;
  for (int i = items.Size() - 1; i >= 0; --i)
  {
    const std::shared_ptr<CFileItem> item = items[i];
    if (item->IsFolder() || !URIUtils::HasExtension(item->GetPath(), ".mov"))
      continue;

    const auto picture = pictures.find(GetLivePhotoKey(item->GetPath()));
    if (picture == pictures.end())
      continue;

    clips.emplace_back(i, picture->second);
    paths.emplace_back(picture->second->GetPath());
  }

  if (clips.empty())
    return;

  // Reading the pictures can take a while on a network share
  CLivePhotoCheck check(std::move(paths));
  if (CServiceBroker::GetGUI() && CServiceBroker::GetAppMessenger()->IsProcessThread())
    CGUIDialogBusy::Wait(&check, 100, true);
  else
    check.Run();

  for (size_t i = 0; i < clips.size(); ++i)
  {
    if (!check.IsLivePhoto(i))
      continue;

    const auto& [index, picture] = clips[i];
    picture->SetProperty("motionclip", items[index]->GetPath());
    items.Remove(index);
  }
}

std::string ExtractMotionPhotoClip(const std::string& path)
{
  if (!URIUtils::HasExtension(path, ".jpg|.jpeg"))
    return {};

  CFile file;
  if (!file.Open(path))
    return {};

  const int64_t fileSize = file.GetLength();
  std::string head(static_cast<size_t>(std::min(fileSize, XMP_SEARCH_SIZE)), '\0');
  if (head.empty() || file.Read(head.data(), head.size()) != static_cast<ssize_t>(head.size()))
    return {};

  const size_t xmpStart = head.find("<x:xmpmeta");
  if (xmpStart == std::string::npos)
    return {};
  const size_t xmpEnd = head.find("</x:xmpmeta>", xmpStart);
  if (xmpEnd == std::string::npos)
    return {};

  const uint64_t clipSize = GetClipSize(std::string_view(head).substr(xmpStart, xmpEnd - xmpStart));
  if (clipSize == 0 || clipSize >= static_cast<uint64_t>(fileSize))
    return {};

  const int64_t clipStart = fileSize - static_cast<int64_t>(clipSize);
  std::array<char, 8> boxHeader{};
  if (file.Seek(clipStart) != clipStart ||
      file.Read(boxHeader.data(), boxHeader.size()) != static_cast<ssize_t>(boxHeader.size()) ||
      std::string_view(boxHeader.data() + 4, 4) != "ftyp")
  {
    CLog::Log(LOGDEBUG, "MotionPhoto: {} has no MP4 where its XMP says the clip is", path);
    return {};
  }

  const std::string clipPath =
      StringUtils::Format("{}{:08x}-{}.mp4", CLIP_FOLDER, Crc32::Compute(path), clipSize);
  // special:// Exists() still asks the directory cache, which can outlive the folder
  if (CFile::Exists(CSpecialProtocol::TranslatePath(clipPath), false))
    return clipPath;

  // Only a complete copy gets the clip's name, so a copy cut short is never used
  const std::string partPath = clipPath + ".part";
  CDirectory::Create(std::string(CLIP_FOLDER));
  CFile clip;
  if (file.Seek(clipStart) != clipStart || !clip.OpenForWrite(partPath, true))
    return {};

  std::array<char, 64 * 1024> buffer;
  uint64_t remaining = clipSize;
  while (remaining > 0)
  {
    const size_t chunk = static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
    const ssize_t read = file.Read(buffer.data(), chunk);
    if (read <= 0 || clip.Write(buffer.data(), static_cast<size_t>(read)) != read)
      break;
    remaining -= static_cast<uint64_t>(read);
  }
  clip.Close();

  if (remaining > 0 || !CFile::Rename(partPath, clipPath))
  {
    CLog::Log(LOGERROR, "MotionPhoto: failed to copy the clip from {}", path);
    CFile::Delete(partPath);
    return {};
  }

  return clipPath;
}

void ClearMotionPhotoClips()
{
  // the real path for the same reason as in ExtractMotionPhotoClip()
  const std::string folder = CSpecialProtocol::TranslatePath(std::string(CLIP_FOLDER));
  if (CDirectory::Exists(folder, false))
    CDirectory::RemoveRecursive(folder);
}
} // namespace KODI::PICTURE
