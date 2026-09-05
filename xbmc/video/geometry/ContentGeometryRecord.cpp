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
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "video/geometry/GeometryTransforms.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <limits>

#include <sys/stat.h>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

CVariant RectToVariant(const CRectInt& rect)
{
  CVariant value{CVariant::VariantTypeObject};
  value["x"] = rect.x1;
  value["y"] = rect.y1;
  value["width"] = rect.Width();
  value["height"] = rect.Height();
  return value;
}

CRectInt RectFromVariant(const CVariant& value)
{
  return OriginSizeRect(value["x"].asInteger32(0), value["y"].asInteger32(0),
                        value["width"].asInteger32(0), value["height"].asInteger32(0));
}

// Decoding falls back to each field's default, so a row written before a field existed
// still decodes.

CVariant CombinerParamsToVariant(const CombinerParams& params)
{
  CVariant value{CVariant::VariantTypeObject};
  value["tolerance"] = params.tolerance;
  value["minconfidence"] = params.minConfidence;
  value["minstationarysamples"] = params.minStationarySamples;
  value["variesshare"] = params.variesShare;
  value["minrivalsamples"] = params.minRivalSamples;
  value["distinctaspectshare"] = params.distinctAspectShare;
  return value;
}

CombinerParams CombinerParamsFromVariant(const CVariant& value)
{
  CombinerParams params;
  params.tolerance = value["tolerance"].asUnsignedInteger32(params.tolerance);
  params.minConfidence = value["minconfidence"].asFloat(params.minConfidence);
  params.minStationarySamples =
      value["minstationarysamples"].asUnsignedInteger32(params.minStationarySamples);
  params.variesShare = value["variesshare"].asFloat(params.variesShare);
  params.minRivalSamples = value["minrivalsamples"].asUnsignedInteger32(params.minRivalSamples);
  params.distinctAspectShare = value["distinctaspectshare"].asFloat(params.distinctAspectShare);
  return params;
}

CVariant SamplingParamsToVariant(const SamplingParams& params)
{
  CVariant value{CVariant::VariantTypeObject};
  value["points"] = params.points;
  value["leadinseconds"] = params.leadInSeconds;
  value["leadoutseconds"] = params.leadOutSeconds;
  value["shorttitlewindow"] = params.shortTitleWindow;
  value["picturesperpoint"] = params.picturesPerPoint;
  value["escalatedpoints"] = params.escalatedPoints;
  value["escalatediscardshare"] = params.escalateDiscardShare;
  return value;
}

SamplingParams SamplingParamsFromVariant(const CVariant& value)
{
  SamplingParams params;
  params.points = value["points"].asUnsignedInteger32(params.points);
  params.leadInSeconds = value["leadinseconds"].asDouble(params.leadInSeconds);
  params.leadOutSeconds = value["leadoutseconds"].asDouble(params.leadOutSeconds);
  params.shortTitleWindow = value["shorttitlewindow"].asDouble(params.shortTitleWindow);
  params.picturesPerPoint = value["picturesperpoint"].asUnsignedInteger32(params.picturesPerPoint);
  params.escalatedPoints = value["escalatedpoints"].asUnsignedInteger32(params.escalatedPoints);
  params.escalateDiscardShare = value["escalatediscardshare"].asFloat(params.escalateDiscardShare);
  return params;
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

ContentGeometryState StateOf(const ContentGeometryRecord& record)
{
  return record.algorithmVersion < CONTENT_GEOMETRY_ALGORITHM_VERSION ? ContentGeometryState::STALE
                                                                      : ContentGeometryState::VALID;
}

float WidestAspect(const ContentGeometryRecord& record)
{
  if (!record.hasReading || record.coded.IsEmpty())
    return 0.0f;

  StreamGeometry measured;
  measured.coded = record.coded;
  measured.displayAspect = record.displayAspect;

  float widest{0.0f};
  const auto consider = [&](const CRectInt& shape)
  {
    if (!shape.IsEmpty())
      widest = std::max(widest, AspectOf(ToSquarePixels(shape, measured)));
  };

  // The two rectangles alongside the shapes rather than instead of them: a record made before
  // the sections were stored, or imported from an NFO, carries only the rectangle and extent.
  consider(record.rect);
  consider(record.envelope);
  for (const CRectInt& section : record.sections)
    consider(section);

  return widest;
}

bool NeedsContentGeometry(const ContentGeometryAttempt& attempt, const FileIdentity& identity)
{
  if (!attempt.exists)
    return true;

  if (attempt.algorithmVersion < CONTENT_GEOMETRY_ALGORITHM_VERSION)
    return true;

  return !attempt.identity.Matches(identity);
}

std::optional<ContentGeometryRecord> MergeDiscoveredGeometry(const ContentGeometryRecord& record,
                                                             const std::vector<CRectInt>& found,
                                                             const CombinerParams& params)
{
  if (found.empty() || !record.hasReading)
    return std::nullopt;

  ContentGeometryRecord merged{record};

  const auto known = [&merged, &params](const CRectInt& shape)
  {
    return std::any_of(merged.sections.begin(), merged.sections.end(),
                       [&shape, &params](const CRectInt& section)
                       { return EdgesWithin(section, shape, params.tolerance); });
  };

  bool changed{false};
  bool learnedShape{false};
  for (const CRectInt& shape : found)
  {
    if (shape.IsEmpty())
      continue;

    if (!known(shape))
    {
      merged.sections.push_back(shape);
      learnedShape = true;
      changed = true;
    }

    // Per axis: a title can contain a shape wider than anything sampled and another taller.
    const CRectInt widened = CRectInt{merged.envelope}.Union(shape);
    if (widened != merged.envelope)
    {
      merged.envelope = widened;
      changed = true;
    }
  }

  if (!changed)
    return std::nullopt;

  // Raised by a shape the record did not already hold, and never lowered - the stored value is
  // the combiner's, which weighs sample count and share together.
  //
  // The second shape, not the first: a record can arrive holding none, and a watch of a
  // one-shape title then learns that one shape.
  merged.varies = merged.varies || (learnedShape && merged.sections.size() > 1);
  return merged;
}

std::string EncodeContentGeometryDetails(const ContentGeometryDetails& details)
{
  CVariant value{CVariant::VariantTypeObject};
  value["detector"] = details.detector;
  value["usable"] = details.usable;
  value["discarded"] = details.discarded;
  value["unreadable"] = details.unreadable;

  value["combining"] = CombinerParamsToVariant(details.combining);
  value["sampling"] = SamplingParamsToVariant(details.sampling);

  CVariant samples{CVariant::VariantTypeArray};
  for (const auto& sample : details.samples)
  {
    CVariant entry{CVariant::VariantTypeObject};
    entry["rect"] = RectToVariant(sample.rect);
    entry["confidence"] = sample.confidence;
    entry["degenerate"] = sample.degenerate;
    entry["position"] = sample.position;
    samples.push_back(std::move(entry));
  }
  value["samples"] = std::move(samples);

  CVariant clusters{CVariant::VariantTypeArray};
  for (const auto& cluster : details.clusters)
  {
    CVariant entry{CVariant::VariantTypeObject};
    entry["rect"] = RectToVariant(cluster.rect);
    entry["samples"] = cluster.samples;
    entry["weight"] = cluster.weight;
    clusters.push_back(std::move(entry));
  }
  value["clusters"] = std::move(clusters);

  std::string json;
  if (!CJSONVariantWriter::Write(value, json, true))
    return {};

  return json;
}

std::string EncodeGeometrySections(const std::vector<CRectInt>& sections)
{
  std::string packed;
  for (const CRectInt& section : sections)
  {
    if (!packed.empty())
      packed += ';';

    packed += StringUtils::Format("{},{},{},{}", section.x1, section.y1, section.Width(),
                                  section.Height());
  }

  return packed;
}

std::vector<CRectInt> DecodeGeometrySections(const std::string& packed)
{
  std::vector<CRectInt> sections;

  const char* const end{packed.data() + packed.size()};
  const char* at{packed.data()};
  while (at < end)
  {
    std::array<int, 4> values{};
    size_t read{0};
    for (; read < values.size(); ++read)
    {
      const std::from_chars_result parsed{std::from_chars(at, end, values[read])};
      if (parsed.ec != std::errc{})
        break;

      at = parsed.ptr;
      if (at < end && (*at == ',' || *at == ';'))
        ++at;
    }

    if (read < values.size())
      break;

    // The stored form is an origin and a size where the rectangle is corner to corner, so the
    // far edge is a sum - and this column is user-editable text, where two in-range values need
    // not add to one. Taken wide and bounded, the rectangle's own type overflowing undefined.
    const int64_t x2{static_cast<int64_t>(values[0]) + values[2]};
    const int64_t y2{static_cast<int64_t>(values[1]) + values[3]};
    if (x2 < std::numeric_limits<int>::min() || x2 > std::numeric_limits<int>::max() ||
        y2 < std::numeric_limits<int>::min() || y2 > std::numeric_limits<int>::max())
      break;

    sections.push_back(CRectInt{values[0], values[1], static_cast<int>(x2), static_cast<int>(y2)});
  }

  return sections;
}

namespace
{

void ArchiveRect(CArchive& ar, CRectInt& rect)
{
  if (ar.IsStoring())
  {
    ar << rect.x1;
    ar << rect.y1;
    ar << rect.x2;
    ar << rect.y2;
  }
  else
  {
    ar >> rect.x1;
    ar >> rect.y1;
    ar >> rect.x2;
    ar >> rect.y2;
  }
}

//! \brief Write \p rect as <prefix>x/y/width/height elements under \p node.
void SaveRectXML(TiXmlElement& node, const std::string& prefix, const CRectInt& rect)
{
  XMLUtils::SetInt(&node, (prefix + "x").c_str(), rect.x1);
  XMLUtils::SetInt(&node, (prefix + "y").c_str(), rect.y1);
  XMLUtils::SetInt(&node, (prefix + "width").c_str(), rect.Width());
  XMLUtils::SetInt(&node, (prefix + "height").c_str(), rect.Height());
}

//! \brief Read SaveRectXML() back, each missing element answered from \p fallback.
CRectInt LoadRectXML(const TiXmlElement& node, const std::string& prefix, const CRectInt& fallback)
{
  int x{fallback.x1};
  int y{fallback.y1};
  int width{fallback.Width()};
  int height{fallback.Height()};
  XMLUtils::GetInt(&node, (prefix + "x").c_str(), x);
  XMLUtils::GetInt(&node, (prefix + "y").c_str(), y);
  XMLUtils::GetInt(&node, (prefix + "width").c_str(), width);
  XMLUtils::GetInt(&node, (prefix + "height").c_str(), height);
  return OriginSizeRect(x, y, width, height);
}

} // unnamed namespace

void Archive(CArchive& ar, ContentGeometryRecord& record)
{
  const auto io = [&ar](auto& field)
  {
    if (ar.IsStoring())
      ar << field;
    else
      ar >> field;
  };

  ArchiveRect(ar, record.coded);
  ArchiveRect(ar, record.rect);
  ArchiveRect(ar, record.envelope);

  if (ar.IsStoring())
  {
    ar << static_cast<int>(record.sections.size());
  }
  else
  {
    int sections{0};
    ar >> sections;
    record.sections.assign(sections, {});
  }
  for (CRectInt& section : record.sections)
    ArchiveRect(ar, section);

  io(record.displayAspect);
  io(record.varies);
  io(record.hasReading);
  io(record.confidence);
  io(record.algorithmVersion);
  io(record.identity.size);
  io(record.identity.time);
  io(record.computed);
}

void SaveContentGeometryXML(TiXmlNode& movie, const ContentGeometryRecord& record)
{
  TiXmlElement geometry("contentgeometry");
  XMLUtils::SetInt(&geometry, "codedwidth", record.coded.Width());
  XMLUtils::SetInt(&geometry, "codedheight", record.coded.Height());
  SaveRectXML(geometry, "", record.rect);
  SaveRectXML(geometry, "envelope", record.envelope);
  XMLUtils::SetFloat(&geometry, "displayaspect", record.displayAspect);
  XMLUtils::SetBoolean(&geometry, "varies", record.varies);
  XMLUtils::SetBoolean(&geometry, "hasreading", record.hasReading);
  XMLUtils::SetFloat(&geometry, "confidence", record.confidence);
  XMLUtils::SetInt(&geometry, "algorithmversion", record.algorithmVersion);
  XMLUtils::SetString(&geometry, "filesize", std::to_string(record.identity.size));
  XMLUtils::SetString(&geometry, "filemtime", std::to_string(record.identity.time));
  XMLUtils::SetString(&geometry, "computed", record.computed.GetAsDBDateTime());
  if (!record.sections.empty())
    XMLUtils::SetString(&geometry, "sections", EncodeGeometrySections(record.sections));

  movie.InsertEndChild(geometry);
}

std::optional<ContentGeometryRecord> LoadContentGeometryXML(const TiXmlElement& movie)
{
  const TiXmlElement* geometry{movie.FirstChildElement("contentgeometry")};
  if (!geometry)
    return std::nullopt;

  ContentGeometryRecord record;

  int codedWidth{0};
  int codedHeight{0};
  XMLUtils::GetInt(geometry, "codedwidth", codedWidth);
  XMLUtils::GetInt(geometry, "codedheight", codedHeight);
  record.coded = CRectInt{0, 0, codedWidth, codedHeight};
  record.rect = LoadRectXML(*geometry, "", {});

  // An NFO written before the envelope existed describes a rectangle and nothing wider, so
  // the rectangle is the envelope. Defaulting to an empty one instead would import a
  // measurement claiming no picture at all.
  record.envelope = LoadRectXML(*geometry, "envelope", record.rect);

  XMLUtils::GetFloat(geometry, "displayaspect", record.displayAspect);
  XMLUtils::GetBoolean(geometry, "varies", record.varies);
  XMLUtils::GetBoolean(geometry, "hasreading", record.hasReading);
  XMLUtils::GetFloat(geometry, "confidence", record.confidence);
  XMLUtils::GetInt(geometry, "algorithmversion", record.algorithmVersion);

  // Read as strings because a file size outgrows a 32-bit int routinely.
  if (std::string value; XMLUtils::GetString(geometry, "filesize", value))
    record.identity.size = std::strtoll(value.c_str(), nullptr, 10);
  if (std::string value; XMLUtils::GetString(geometry, "filemtime", value))
    record.identity.time = std::strtoll(value.c_str(), nullptr, 10);
  if (std::string value; XMLUtils::GetString(geometry, "computed", value))
    record.computed.SetFromDBDateTime(value);

  // An NFO written since the shapes travelled replaces them; one written before carries varies
  // and no sections, and leaves them alone.
  if (std::string value; XMLUtils::GetString(geometry, "sections", value))
    record.sections = DecodeGeometrySections(value);

  if (!record.IsValid())
    return std::nullopt;

  return record;
}

ContentGeometryDetails DecodeContentGeometryDetails(const std::string& json)
{
  ContentGeometryDetails details;

  CVariant value;
  if (json.empty() || !CJSONVariantParser::Parse(json, value) || !value.isObject())
    return details;

  details.detector = value["detector"].asString();
  details.usable = value["usable"].asUnsignedInteger32(0);
  details.discarded = value["discarded"].asUnsignedInteger32(0);
  details.unreadable = value["unreadable"].asUnsignedInteger32(0);

  details.combining = CombinerParamsFromVariant(value["combining"]);
  details.sampling = SamplingParamsFromVariant(value["sampling"]);

  const CVariant& samples{value["samples"]};
  for (auto it = samples.begin_array(); it != samples.end_array(); ++it)
  {
    GeometrySample sample;
    sample.rect = RectFromVariant((*it)["rect"]);
    sample.confidence = (*it)["confidence"].asFloat(0.0f);
    sample.degenerate = (*it)["degenerate"].asBoolean(false);
    sample.position = (*it)["position"].asDouble(0.0);
    details.samples.emplace_back(sample);
  }

  const CVariant& clusters{value["clusters"]};
  for (auto it = clusters.begin_array(); it != clusters.end_array(); ++it)
  {
    GeometryCluster cluster;
    cluster.rect = RectFromVariant((*it)["rect"]);
    cluster.samples = (*it)["samples"].asUnsignedInteger32(0);
    cluster.weight = (*it)["weight"].asFloat(0.0f);
    details.clusters.emplace_back(cluster);
  }

  return details;
}

} // namespace KODI::VIDEO::GEOMETRY
