/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PictureXmp.h"

#include "filesystem/File.h"

#include <charconv>

namespace
{
// The XMP packet comes after the EXIF data, which can't be larger than 64 KiB
constexpr size_t XMP_SEARCH_SIZE = 256 * 1024;

uint64_t ToNumber(std::string_view value)
{
  uint64_t number = 0;
  std::from_chars(value.data(), value.data() + value.size(), number);
  return number;
}
} // namespace

namespace KODI::PICTURE::XMP
{
std::string Read(XFILE::CFile& file)
{
  std::string head(XMP_SEARCH_SIZE, '\0');
  const ssize_t read = file.Read(head.data(), head.size());
  if (read <= 0)
    return {};
  head.resize(static_cast<size_t>(read));

  const size_t start = head.find("<x:xmpmeta");
  if (start == std::string::npos)
    return {};
  const size_t end = head.find("</x:xmpmeta>", start);
  if (end == std::string::npos)
    return {};

  return head.substr(start, end - start);
}

std::string_view GetValue(std::string_view xmp, std::string_view name)
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
    // A value per colour channel is a list; the first one stands for all of them
    if (xmp.substr(start).starts_with("<rdf:Seq>"))
    {
      constexpr std::string_view listItem = "<rdf:li>";
      start = xmp.find(listItem, start);
      if (start == std::string_view::npos)
        return {};
      start += listItem.size();
    }
    const size_t end = xmp.find('<', start);
    if (end != std::string_view::npos)
      return xmp.substr(start, end - start);
  }

  return {};
}

uint64_t GetUnsigned(std::string_view xmp, std::string_view name)
{
  return ToNumber(GetValue(xmp, name));
}

std::vector<ContainerItem> GetContainerItems(std::string_view xmp)
{
  std::vector<ContainerItem> items;

  constexpr std::string_view itemTag = "<Container:Item";
  for (size_t start = xmp.find(itemTag); start != std::string_view::npos;
       start = xmp.find(itemTag, start + itemTag.size()))
  {
    const size_t end = xmp.find('>', start);
    if (end == std::string_view::npos)
      break;

    const std::string_view item = xmp.substr(start, end - start);
    items.push_back({std::string(GetValue(item, "Item:Semantic")),
                     ToNumber(GetValue(item, "Item:Length")),
                     ToNumber(GetValue(item, "Item:Padding"))});
  }

  return items;
}
} // namespace KODI::PICTURE::XMP
