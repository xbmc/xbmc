/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentGeometryRecord.h"

#include "filesystem/File.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <cmath>

#include <sys/stat.h>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

bool IsRatio(float aspect)
{
  return aspect > 0.0f && !std::isinf(aspect);
}

} // unnamed namespace

FileIdentity GetFileIdentity(const std::string& path)
{
  struct __stat64 st = {};
  if (XFILE::CFile::Stat(path, &st) != 0)
    return {};

  // Some filesystems report no modification time but a usable creation time.
  int64_t time{st.st_mtime};
  if (time == 0)
    time = st.st_ctime;

  if (time == 0 && st.st_size == 0)
    return {};

  return {static_cast<int64_t>(st.st_size), time};
}

float StoredAspect(float aspect)
{
  return std::round(aspect * 100.0f) / 100.0f;
}

std::string EncodeContentAspects(const std::vector<float>& aspects)
{
  std::string packed;
  for (const float aspect : aspects)
  {
    if (!packed.empty())
      packed += ';';
    packed += StringUtils::Format("{:.2f}", aspect);
  }
  return packed;
}

std::vector<float> DecodeContentAspects(const std::string& packed)
{
  std::vector<float> aspects;
  for (const std::string& value : StringUtils::Split(packed, ';'))
  {
    const float aspect{StoredAspect(StringUtils::ToFloat(value))};
    if (IsRatio(aspect))
      aspects.push_back(aspect);
  }
  return aspects;
}

ContentGeometryState StateOf(const ContentGeometryRecord& record)
{
  return record.algorithmVersion < CONTENT_GEOMETRY_ALGORITHM_VERSION ? ContentGeometryState::STALE
                                                                      : ContentGeometryState::VALID;
}

float WidestAspect(const ContentGeometryRecord& record)
{
  return record.aspects.empty() ? 0.0f
                                : *std::max_element(record.aspects.begin(), record.aspects.end());
}

bool NeedsContentGeometry(const std::optional<ContentGeometryRecord>& stored,
                          const FileIdentity& identity)
{
  if (!stored)
    return true;

  if (stored->algorithmVersion < CONTENT_GEOMETRY_ALGORITHM_VERSION)
    return true;

  return !stored->identity.Matches(identity);
}

void Archive(CArchive& ar, ContentGeometryRecord& record)
{
  if (ar.IsStoring())
  {
    ar << static_cast<int>(record.aspects.size());
    for (const float aspect : record.aspects)
      ar << aspect;
    ar << record.algorithmVersion;
    ar << record.identity.size;
    ar << record.identity.time;
  }
  else
  {
    int count{0};
    ar >> count;
    record.aspects.assign(std::max(count, 0), 0.0f);
    for (float& aspect : record.aspects)
      ar >> aspect;
    ar >> record.algorithmVersion;
    ar >> record.identity.size;
    ar >> record.identity.time;
  }
}

void SaveContentGeometryXML(TiXmlNode& movie, const ContentGeometryRecord& record)
{
  if (!record.HasReading())
    return;

  TiXmlElement geometry("contentgeometry");
  for (const float aspect : record.aspects)
  {
    TiXmlElement element("aspect");
    element.InsertEndChild(TiXmlText(StringUtils::Format("{:.2f}", aspect)));
    geometry.InsertEndChild(element);
  }

  movie.InsertEndChild(geometry);
}

std::optional<ContentGeometryRecord> LoadContentGeometryXML(const TiXmlElement& movie)
{
  const TiXmlElement* geometry{movie.FirstChildElement("contentgeometry")};
  if (!geometry)
    return std::nullopt;

  ContentGeometryRecord record;
  for (const TiXmlElement* element = geometry->FirstChildElement("aspect"); element;
       element = element->NextSiblingElement("aspect"))
  {
    if (!element->GetText())
      continue;

    const float aspect{StoredAspect(StringUtils::ToFloat(element->GetText()))};
    if (IsRatio(aspect))
      record.aspects.push_back(aspect);
  }

  if (!record.HasReading())
    return std::nullopt;

  return record;
}

} // namespace KODI::VIDEO::GEOMETRY
