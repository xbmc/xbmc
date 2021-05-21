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
#include "threads/SingleLock.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

static const char* TAG_ROOT = "decoderfilter";
static const char* TAG_FILTER = "filter";
static const char* TAG_NAME = "name";
static const char* TAG_GENERAL = "allowed";
static const char* TAG_STILLS = "stills-allowed";
static const char* TAG_DVD = "dvd-allowed";
static const char* TAG_MINHEIGHT = "min-height";
static const char* HWDFFileName = "special://masterprofile/decoderfilter.xml";
static const char* CLASSNAME = "CDecoderFilter";

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

  // remove codec pitch for comparision
  if (m_minHeight && (streamInfo.height & ~31) <= m_minHeight)
    return false;

  return true;
}

bool CDecoderFilter::Load(const TiXmlNode *node)
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

bool CDecoderFilter::Save(TiXmlNode *node) const
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
  CSingleLock lock(m_critical);
  std::set<CDecoderFilter>::const_iterator filter(m_filters.find(name));
  return filter != m_filters.end() ? filter->isValid(streamInfo) : m_filters.empty();
}

void CDecoderFilterManager::add(const CDecoderFilter& filter)
{
  CSingleLock lock(m_critical);
  std::pair<std::set<CDecoderFilter>::iterator, bool> res = m_filters.insert(filter);
  m_dirty = m_dirty || res.second;
}

bool CDecoderFilterManager::Load()
{
  CSingleLock lock(m_critical);

  m_filters.clear();

  std::string fileName = CUtil::TranslateSpecialSource(HWDFFileName);
  if (!XFILE::CFile::Exists(fileName))
    return true;

  CLog::Log(LOGINFO, "{}: loading filters from {}", CLASSNAME, fileName);

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(fileName))
  {
    CLog::Log(LOGERROR, "{}: error loading: line {}, {}", CLASSNAME, xmlDoc.ErrorRow(),
              xmlDoc.ErrorDesc());
    return false;
  }

  const TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement || !StringUtils::EqualsNoCase(pRootElement->ValueStr(), TAG_ROOT))
  {
    CLog::Log(LOGERROR, "{}: invalid root element ({})", CLASSNAME, pRootElement->ValueStr());
    return false;
  }

  const TiXmlElement *pFilter = pRootElement->FirstChildElement(TAG_FILTER);
  while (pFilter)
  {
    CDecoderFilter filter("");
    if (filter.Load(pFilter))
      m_filters.insert(filter);
    pFilter = pFilter->NextSiblingElement(TAG_FILTER);
  }
  return true;
}

bool CDecoderFilterManager::Save() const
{
  CSingleLock lock(m_critical);
  if (!m_dirty || m_filters.empty())
    return true;

  CXBMCTinyXML doc;
  TiXmlElement xmlRootElement(TAG_ROOT);
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  for (const CDecoderFilter& filter : m_filters)
  {
    // Write the resolution tag
    TiXmlElement filterElem(TAG_FILTER);
    TiXmlNode *pNode = pRoot->InsertEndChild(filterElem);
    if (pNode == NULL)
      return false;

    filter.Save(pNode);
  }
  std::string fileName = CUtil::TranslateSpecialSource(HWDFFileName);
  return doc.SaveFile(fileName);
}
