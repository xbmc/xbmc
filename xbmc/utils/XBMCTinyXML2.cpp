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
#include "language/LangInfo.h"
#include "utils/CharsetConverter.h"
#include "utils/CharsetDetection.h"
#include "utils/StringUtils.h"
#include "utils/Utf8Utils.h"
#include "utils/log.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace
{
static constexpr int MAX_ENTITY_LENGTH = 8; // size of largest entity "&#xNNNN;"
static constexpr size_t BUFFER_SIZE = 4096;

// The document is UTF-8 after conversion, so it must not be saved with the original encoding
void SetDeclaredEncodingToUtf8(tinyxml2::XMLDocument& doc)
{
  tinyxml2::XMLNode* node = doc.FirstChild();
  tinyxml2::XMLDeclaration* declaration = node ? node->ToDeclaration() : nullptr;
  if (!declaration)
    return;

  std::string value = declaration->Value();
  const size_t encoding = value.find("encoding");
  if (encoding == std::string::npos)
    return;

  const size_t start = value.find_first_of("\"'", encoding);
  if (start == std::string::npos)
    return;

  const size_t end = value.find(value[start], start + 1);
  if (end == std::string::npos)
    return;

  value.replace(start + 1, end - start - 1, "UTF-8");
  declaration->SetValue(value.c_str());
}
} // namespace

bool CXBMCTinyXML2::LoadFile(const std::string& filename)
{
  m_filename = filename;

  XFILE::CFile file;
  std::vector<uint8_t> buffer;

  if (file.LoadFile(filename, buffer) <= 0)
  {
    // SetError private in tinyxml2 - replacement?
    // tinyxml2::XMLDocument::SetError(tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED, NULL, NULL);
    return false;
  }

  return Parse(std::string(reinterpret_cast<char*>(buffer.data()), buffer.size()),
               file.GetProperty(XFILE::FileProperty::CONTENT_CHARSET));
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
  m_usedCharset = "UTF-8";
  return InternalParse(inputdata);
}

bool CXBMCTinyXML2::Parse(std::string&& inputdata)
{
  m_usedCharset = "UTF-8";
  return InternalParse(std::move(inputdata));
}

bool CXBMCTinyXML2::Parse(const std::string& inputdata, const std::string& dataCharset)
{
  std::string suggestedCharset{dataCharset};
  StringUtils::ToUpper(suggestedCharset);
  m_usedCharset.clear();

  if (!suggestedCharset.empty() && TryParse(inputdata, suggestedCharset))
    return true;

  std::string detectedCharset;
  const auto source = [this]
  { return m_filename.empty() ? "XML data" : "file \"" + m_filename + "\""; };
  const auto logCharsetFallback = [&]
  {
    if (!suggestedCharset.empty())
      CLog::Log(LOGWARNING,
                "CXBMCTinyXML2::Parse: \"{}\" charset was used instead of suggested charset "
                "\"{}\" for {}",
                m_usedCharset, suggestedCharset, source());
    else if (!detectedCharset.empty())
      CLog::Log(LOGWARNING,
                "CXBMCTinyXML2::Parse: \"{}\" charset was used instead of detected charset "
                "\"{}\" for {}",
                m_usedCharset, detectedCharset, source());
  };

  if (CCharsetDetection::DetectXmlEncoding(inputdata, detectedCharset) &&
      TryParse(inputdata, detectedCharset))
  {
    if (!suggestedCharset.empty())
      logCharsetFallback();
    return true;
  }

  if (suggestedCharset != "UTF-8" && detectedCharset != "UTF-8" &&
      CUtf8Utils::isValidUtf8(inputdata) && TryParse(inputdata, "UTF-8"))
  {
    logCharsetFallback();
    return true;
  }

  if (TryParse(inputdata, g_langInfo.GetGuiCharSet()))
  {
    logCharsetFallback();
    return true;
  }

  // can't detect correct data charset, try to process data as is
  if (InternalParse(inputdata))
  {
    if (!suggestedCharset.empty())
      CLog::Log(LOGWARNING,
                "CXBMCTinyXML2::Parse: Processed {} as unknown encoding instead of suggested "
                "\"{}\"",
                source(), suggestedCharset);
    else if (!detectedCharset.empty())
      CLog::Log(LOGWARNING,
                "CXBMCTinyXML2::Parse: Processed {} as unknown encoding instead of detected "
                "\"{}\"",
                source(), detectedCharset);
    return true;
  }

  return false;
}

bool CXBMCTinyXML2::TryParse(const std::string& inputdata, const std::string& tryDataCharset)
{
  bool parsed;
  if (tryDataCharset.empty() || tryDataCharset == "UTF-8")
    parsed = InternalParse(inputdata);
  else
  {
    std::string converted;
    /* some wrong conversions can leave US-ASCII XML header and structure untouched but break non-English data
     * so conversion must fail on wrong character and then other encodings will be tried */
    if (!g_charsetConverter.ToUtf8(tryDataCharset, inputdata, converted, true) || converted.empty())
      return false;

    parsed = InternalParse(std::move(converted));
    if (parsed)
      SetDeclaredEncodingToUtf8(*this);
  }

  if (!parsed)
    return false;

  m_usedCharset = tryDataCharset;
  return true;
}

bool CXBMCTinyXML2::InternalParse(std::string_view inputdata)
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

bool CXBMCTinyXML2::InternalParse(std::string&& inputdata)
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
