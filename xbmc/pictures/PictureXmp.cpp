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
// XML whitespace, which pretty-printed XMP has between elements and around values
constexpr std::string_view XML_SPACE = " \t\r\n";

std::string_view Trim(std::string_view value)
{
  const size_t first = value.find_first_not_of(XML_SPACE);
  if (first == std::string_view::npos)
    return {};
  value = value.substr(first);
  return value.substr(0, value.find_last_not_of(XML_SPACE) + 1);
}

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
    start = xmp.find_first_not_of(XML_SPACE, start + element.size());
    if (start == std::string_view::npos)
      return {};
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
      return Trim(xmp.substr(start, end - start));
  }

  return {};
}

std::vector<std::string_view> GetValues(std::string_view xmp, std::string_view name)
{
  const std::string element = "<" + std::string(name) + ">";
  size_t start = xmp.find(element);
  if (start != std::string_view::npos)
    start = xmp.find_first_not_of(XML_SPACE, start + element.size());
  if (start == std::string_view::npos || !xmp.substr(start).starts_with("<rdf:Seq>"))
  {
    const std::string_view value = GetValue(xmp, name);
    if (value.empty())
      return {};
    return {value};
  }

  std::vector<std::string_view> values;
  const size_t end = xmp.find("</rdf:Seq>", start);
  if (end == std::string_view::npos)
    return values;
  constexpr std::string_view listItem = "<rdf:li>";
  for (size_t item = xmp.find(listItem, start); item != std::string_view::npos && item < end;
       item = xmp.find(listItem, item + listItem.size()))
  {
    const size_t valueStart = item + listItem.size();
    const size_t valueEnd = xmp.find('<', valueStart);
    if (valueEnd == std::string_view::npos)
      break;
    values.push_back(Trim(xmp.substr(valueStart, valueEnd - valueStart)));
  }
  return values;
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
