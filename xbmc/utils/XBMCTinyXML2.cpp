/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBMCTinyXML2.h"

#include "RegExp.h"
#include "filesystem/File.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace
{
static constexpr int MAX_ENTITY_LENGTH = 8; // size of largest entity "&#xNNNN;"
static constexpr size_t BUFFER_SIZE = 4096;
} // namespace

bool CXBMCTinyXML2::LoadFile(const std::string& filename)
{
  XFILE::CFile file;
  std::vector<uint8_t> buffer;

  if (file.LoadFile(filename, buffer) <= 0)
  {
    // SetError private in tinyxml2 - replacement?
    // tinyxml2::XMLDocument::SetError(tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED, NULL, NULL);
    return false;
  }

  Parse(std::string_view(reinterpret_cast<char*>(buffer.data()), buffer.size()));

  if (Error())
    return false;
  return true;
}

bool CXBMCTinyXML2::LoadFile(FILE* file)
{
  std::string data;
  char buf[BUFFER_SIZE] = {};
  size_t result;
  while ((result = fread(buf, 1, BUFFER_SIZE, file)) > 0)
    data.append(buf, result);
  return Parse(std::move(data));
}

bool CXBMCTinyXML2::SaveFile(const std::string& filename) const
{
  XFILE::CFile file;
  if (file.OpenForWrite(filename, true))
  {
    tinyxml2::XMLPrinter printer;
    Accept(&printer);
    const ssize_t sizeToWrite = printer.CStrSize() - 1; // strip trailing '\0'
    bool suc = file.Write(printer.CStr(), sizeToWrite) == sizeToWrite;
    if (suc)
      file.Flush();

    return suc;
  }
  return false;
}

bool CXBMCTinyXML2::Parse(std::string_view inputdata)
{
  // TinyXML2 only uses UTF-8 charset
  // Preprocess string, replacing '&' with '&amp; for invalid XML entities
  size_t pos = inputdata.find('&');
  if (pos == std::string::npos)
  {
    return (tinyxml2::XML_SUCCESS ==
            tinyxml2::XMLDocument::Parse(
                inputdata.data(), inputdata.size())); // nothing to fix, process data directly
  }

  return ParseHelper(pos, std::string{inputdata});
}

bool CXBMCTinyXML2::Parse(std::string&& inputdata)
{
  // TinyXML2 only uses UTF-8 charset
  // Preprocess string, replacing '&' with '&amp; for invalid XML entities
  size_t pos = inputdata.find('&');
  if (pos == std::string::npos)
  {
    return (tinyxml2::XML_SUCCESS ==
            tinyxml2::XMLDocument::Parse(
                inputdata.c_str(), inputdata.size())); // nothing to fix, process data directly
  }

  return ParseHelper(pos, std::move(inputdata));
}

bool CXBMCTinyXML2::ParseHelper(size_t pos, std::string&& inputdata)
{
  CRegExp re(false, CRegExp::asciiOnly,
             "^&(amp|lt|gt|quot|apos|#x[a-fA-F0-9]{1,4}|#[0-9]{1,5});.*");
  do
  {
    if (re.RegFind(inputdata, static_cast<unsigned int>(pos), MAX_ENTITY_LENGTH) < 0)
      inputdata.insert(pos + 1, "amp;");
    pos = inputdata.find('&', pos + 1);
  } while (pos != std::string::npos);

  return (tinyxml2::XML_SUCCESS ==
          tinyxml2::XMLDocument::Parse(inputdata.c_str(), inputdata.size()));
}
