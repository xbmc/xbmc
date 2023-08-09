/*
 *  Copyright (C) 2013-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


/**
 * \file media\hwdecoder\DecoderFilterManager.cpp
 * \brief Implements CDecoderFilterManager class.
 *
 */

#include "DecoderFilterManager.h"

#include "Util.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "filesystem/File.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

static const char* TAG_ROOT = "decoderfilter";
static const char* TAG_FILTER = "filter";
static const char* TAG_NAME = "name";
static const char* TAG_GENERAL = "allowed";
static const char* TAG_STILLS = "stills-allowed";
static const char* TAG_DVD = "dvd-allowed";
static const char* TAG_MINHEIGHT = "min-height";
static const char* HWDFFileName = "special://masterprofile/decoderfilter.xml";

CDecoderFilter::CDecoderFilter(const std::string& name, uint32_t flags, int minHeight)
  : m_name(name)
  , m_flags(flags)
  , m_minHeight(minHeight)
{
}

bool CDecoderFilter::isValid(const CDVDStreamInfo& streamInfo) const
{
  uint32_t flags = FLAG_GENERAL_ALLOWED;

  if (streamInfo.stills)
    flags |= FLAG_STILLS_ALLOWED;

  if (streamInfo.dvd)
    flags |= FLAG_DVD_ALLOWED;

  if ((flags & m_flags) != flags)
    return false;

  // remove codec pitch for comparison
  if (m_minHeight && (streamInfo.height & ~31) < m_minHeight)
    return false;

  return true;
}

bool CDecoderFilter::Load(const tinyxml2::XMLNode* node)
{
  bool flagBool = false;

  XMLUtils::GetString(node, TAG_NAME, m_name);
  XMLUtils::GetBoolean(node, TAG_GENERAL, flagBool);
  if (flagBool)
    m_flags |= FLAG_GENERAL_ALLOWED;
  flagBool = false;

  XMLUtils::GetBoolean(node, TAG_STILLS, flagBool);
  if (flagBool)
    m_flags |= FLAG_STILLS_ALLOWED;
  flagBool = false;

  XMLUtils::GetBoolean(node, TAG_DVD, flagBool);
  if (flagBool)
    m_flags |= FLAG_DVD_ALLOWED;

  XMLUtils::GetInt(node, TAG_MINHEIGHT, m_minHeight);

  return true;
}

bool CDecoderFilter::Save(tinyxml2::XMLNode* node) const
{
  // Now write each of the pieces of information we need...
  XMLUtils::SetString(node, TAG_NAME, m_name);
  XMLUtils::SetBoolean(node, TAG_GENERAL, (m_flags & FLAG_GENERAL_ALLOWED) != 0);
  XMLUtils::SetBoolean(node, TAG_STILLS, (m_flags & FLAG_STILLS_ALLOWED) != 0);
  XMLUtils::SetBoolean(node, TAG_DVD, (m_flags & FLAG_DVD_ALLOWED) != 0);
  XMLUtils::SetInt(node, TAG_MINHEIGHT, m_minHeight);
  return true;
}


/****************************************/

bool CDecoderFilterManager::isValid(const std::string& name, const CDVDStreamInfo& streamInfo)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  std::set<CDecoderFilter>::const_iterator filter(m_filters.find(name));
  return filter != m_filters.end() ? filter->isValid(streamInfo) : m_filters.empty();
}

void CDecoderFilterManager::add(const CDecoderFilter& filter)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  std::pair<std::set<CDecoderFilter>::iterator, bool> res = m_filters.insert(filter);
  m_dirty = m_dirty || res.second;
}

bool CDecoderFilterManager::Load()
{
  std::unique_lock<CCriticalSection> lock(m_critical);

  m_filters.clear();

  std::string fileName = CUtil::TranslateSpecialSource(HWDFFileName);
  if (!XFILE::CFile::Exists(fileName))
    return true;

  CLog::LogF(LOGINFO, "loading filters from {}", fileName);

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(fileName))
  {
    CLog::LogF(LOGERROR, "error loading: line {}, {}", xmlDoc.ErrorLineNum(), xmlDoc.ErrorStr());
    return false;
  }

  const auto* rootElement = xmlDoc.RootElement();
  if (rootElement == nullptr || !StringUtils::EqualsNoCase(rootElement->Value(), TAG_ROOT))
  {
    CLog::LogF(LOGERROR, "invalid root element ({})", rootElement ? rootElement->Value() : "");
    return false;
  }

  const auto* tagFilter = rootElement->FirstChildElement(TAG_FILTER);
  while (tagFilter != nullptr)
  {
    CDecoderFilter filter("");
    if (filter.Load(tagFilter))
      m_filters.insert(filter);
    tagFilter = tagFilter->NextSiblingElement(TAG_FILTER);
  }
  return true;
}

bool CDecoderFilterManager::Save() const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (!m_dirty || m_filters.empty())
    return true;

  CXBMCTinyXML2 doc;
  auto* xmlRootElement = doc.NewElement(TAG_ROOT);
  if (xmlRootElement == nullptr)
    return false;

  auto* root = doc.InsertEndChild(xmlRootElement);
  if (root == nullptr)
    return false;

  for (const CDecoderFilter& filter : m_filters)
  {
    // Write the resolution tag
    auto* filterElem = doc.NewElement(TAG_FILTER);
    if (filterElem == nullptr)
      return false;

    auto* node = root->InsertEndChild(filterElem);
    if (node == nullptr)
      return false;

    filter.Save(node);
  }
  std::string fileName = CUtil::TranslateSpecialSource(HWDFFileName);
  return doc.SaveFile(fileName);
}
