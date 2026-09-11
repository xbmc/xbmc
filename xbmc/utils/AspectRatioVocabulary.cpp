/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AspectRatioVocabulary.h"

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>

namespace KODI::UTILS
{

namespace
{

constexpr const char* DEFINITION_FILE = "aspectratios.xml";

//! \brief One row of the built-in vocabulary.
struct BuiltInEntry
{
  float ratio;
  std::string_view name;
  bool detect;
  bool declare;
};

//! \brief The vocabulary Kodi answers with when no definition has been read. Ascending order
//! is required: classification cuts between adjacent entries at their geometric mean.
constexpr auto BUILT_IN = std::to_array<BuiltInEntry>({
    // Square is a failure signature rather than a ratio: bars on all four sides.
    {1.00f, "", false, true},
    {1.19f, "Movietone", true, true},
    // Exact fractions, not the two-decimal nominal. Labels and keys round.
    {4.0f / 3.0f, "4:3", true, true},
    {1.37f, "Academy", true, true},
    {1.43f, "IMAX 70mm", true, true},
    // A stills ratio nothing is delivered at.
    {1.50f, "3:2", false, true},
    {1.66f, "", true, true},
    {16.0f / 9.0f, "16:9", true, true},
    {1.85f, "Flat", true, true},
    {1.90f, "IMAX digital", true, true}, // also the DCI full container
    {2.00f, "Univisium", true, true},
    {2.20f, "Todd-AO", true, true},
    // The original CinemaScope ratio and modern anamorphic scope, routinely conflated.
    {2.35f, "CinemaScope", true, true},
    {2.40f, "Scope", true, true},
    // Delivered, but rare enough that a measurement landing here is likely an overshoot.
    {2.55f, "CinemaScope 55", false, true},
    {2.76f, "Ultra Panavision 70", false, true},
});

//! \brief The label for a ratio, which is the ratio written out. Not settable from a
//! definition file.
std::string LabelFor(float ratio)
{
  return StringUtils::Format("{:.2f}", ratio);
}

//! \brief An entry with the log of its ratio, which is what classification compares in. An
//! entry's ratio never changes once it is in the store, so the log is taken here rather than
//! on every query.
struct VocabularyEntry
{
  AspectRatioEntry entry;
  float logRatio{0.0f};
};

struct Vocabulary
{
  std::vector<VocabularyEntry> entries;
  float tolerance{CAspectRatioVocabulary::DEFAULT_TOLERANCE};
};

VocabularyEntry Hold(AspectRatioEntry entry)
{
  const float logRatio = std::log(entry.ratio);
  return {std::move(entry), logRatio};
}

Vocabulary BuiltInVocabulary()
{
  Vocabulary vocabulary;
  vocabulary.entries.reserve(BUILT_IN.size());
  for (const BuiltInEntry& entry : BUILT_IN)
  {
    vocabulary.entries.emplace_back(Hold({entry.ratio, LabelFor(entry.ratio),
                                          std::string(entry.name), entry.detect, entry.declare}));
  }
  return vocabulary;
}

std::mutex& StoreLock()
{
  static std::mutex lock;
  return lock;
}

Vocabulary& Store()
{
  static Vocabulary store{BuiltInVocabulary()};
  return store;
}

//! \brief The tolerance in force, mirrored out of the store. Read without the lock, written
//! under it.
std::atomic<float>& CachedTolerance()
{
  static std::atomic<float> tolerance{CAspectRatioVocabulary::DEFAULT_TOLERANCE};
  return tolerance;
}

bool Usable(const AspectRatioEntry& entry, AspectRatioUse use)
{
  switch (use)
  {
    case AspectRatioUse::Detect:
      return entry.detect;
    case AspectRatioUse::Declare:
      return entry.declare;
    case AspectRatioUse::Any:
    default:
      return true;
  }
}

const AspectRatioEntry* NearestIn(const Vocabulary& vocabulary, float ratio, AspectRatioUse use)
{
  if (ratio <= 0.0f)
    return nullptr;

  const float squared = ratio * ratio;

  const AspectRatioEntry* previous = nullptr;
  for (const VocabularyEntry& held : vocabulary.entries)
  {
    if (!Usable(held.entry, use))
      continue;

    if (previous && squared < previous->ratio * held.entry.ratio)
      return previous;

    previous = &held.entry;
  }

  return previous;
}

/*!
 * \brief The entry \p ratio resolves to, erring toward \p towardRatio. Null when it
 * corresponds to no usable entry.
 *
 * Every comparison is a difference of logs, so the only logarithms taken are of the two
 * ratios the caller supplied. This runs against every decoded picture during live detection.
 */
const AspectRatioEntry* ResolveIn(const Vocabulary& vocabulary,
                                  float ratio,
                                  AspectRatioUse use,
                                  float towardRatio)
{
  if (!(ratio > 0.0f))
    return nullptr;

  const float wanted = std::log(ratio);

  // With no resting shape every entry is equally far from it, which leaves the nearest-to-the
  // -reading tie-break as the whole rule - and that is what Match() answers.
  const bool errs = towardRatio > 0.0f;
  const float toward = errs ? std::log(towardRatio) : 0.0f;

  const AspectRatioEntry* chosen = nullptr;
  float chosenOffset = 0.0f; //!< how far the chosen entry is from the reading
  float chosenPull = 0.0f; //!< how far it is from the resting shape

  for (const VocabularyEntry& held : vocabulary.entries)
  {
    if (!Usable(held.entry, use))
      continue;

    const float offset = std::fabs(wanted - held.logRatio);
    if (offset > vocabulary.tolerance)
      continue;

    const float pull =
        errs ? std::fabs(toward - held.logRatio) : std::numeric_limits<float>::infinity();

    if (!chosen || pull < chosenPull || (pull == chosenPull && offset < chosenOffset))
    {
      chosen = &held.entry;
      chosenOffset = offset;
      chosenPull = pull;
    }
  }

  return chosen;
}

//! \brief Read a boolean attribute, leaving \p value alone when it is absent.
bool ReadFlag(const tinyxml2::XMLElement* element, const char* attribute, bool& value)
{
  const char* text = element->Attribute(attribute);
  if (!text)
    return true;

  if (StringUtils::EqualsNoCase(text, "true"))
    value = true;
  else if (StringUtils::EqualsNoCase(text, "false"))
    value = false;
  else
    return false;

  return true;
}

//! \brief Merge one <ratio> into \p vocabulary, false on anything it cannot read.
bool MergeRatio(const tinyxml2::XMLElement* element, Vocabulary& vocabulary)
{
  double value{0.0};
  if (element->QueryDoubleAttribute("value", &value) != tinyxml2::XML_SUCCESS)
  {
    CLog::LogF(LOGERROR, "<ratio> without a readable value attribute");
    return false;
  }

  const float ratio{static_cast<float>(value)};
  if (!(ratio > 0.0f) || std::isinf(ratio))
  {
    CLog::LogF(LOGERROR, "<ratio> value {} is not a ratio", value);
    return false;
  }

  // Entries are identified at the two decimals their labels and keys are written at, so a file
  // saying 1.78 changes the 16:9 entry rather than standing beside it.
  const auto existing = std::ranges::find_if(
      vocabulary.entries,
      [ratio](const VocabularyEntry& held)
      {
        return CAspectRatioVocabulary::Key(held.entry.ratio) == CAspectRatioVocabulary::Key(ratio);
      });

  AspectRatioEntry entry = existing != vocabulary.entries.end()
                               ? existing->entry
                               : AspectRatioEntry{ratio, LabelFor(ratio), {}, true, true};

  if (!ReadFlag(element, "detect", entry.detect) || !ReadFlag(element, "declare", entry.declare))
  {
    CLog::LogF(LOGERROR, "<ratio value=\"{}\"> has a flag that is neither true nor false", value);
    return false;
  }

  if (const char* name = element->Attribute("name"))
    entry.name = name;

  // An existing entry keeps its own ratio, so the log it holds is still the right one.
  if (existing != vocabulary.entries.end())
    *existing = Hold(std::move(entry));
  else
    vocabulary.entries.emplace_back(Hold(std::move(entry)));

  return true;
}

} // unnamed namespace

std::vector<AspectRatioEntry> CAspectRatioVocabulary::Entries()
{
  return EntriesFor(AspectRatioUse::Any);
}

std::vector<AspectRatioEntry> CAspectRatioVocabulary::EntriesFor(AspectRatioUse use)
{
  std::unique_lock lock(StoreLock());

  std::vector<AspectRatioEntry> entries;
  entries.reserve(Store().entries.size());
  for (const VocabularyEntry& held : Store().entries)
  {
    if (Usable(held.entry, use))
      entries.emplace_back(held.entry);
  }
  return entries;
}

float CAspectRatioVocabulary::Tolerance()
{
  return CachedTolerance().load(std::memory_order_relaxed);
}

float CAspectRatioVocabulary::Distance(float a, float b)
{
  if (a <= 0.0f || b <= 0.0f)
    return std::numeric_limits<float>::infinity();

  return std::fabs(std::log(a / b));
}

std::optional<AspectRatioEntry> CAspectRatioVocabulary::Nearest(float ratio, AspectRatioUse use)
{
  std::unique_lock lock(StoreLock());

  const AspectRatioEntry* nearest = NearestIn(Store(), ratio, use);
  if (!nearest)
    return std::nullopt;

  return *nearest;
}

std::optional<AspectRatioEntry> CAspectRatioVocabulary::Match(float ratio, AspectRatioUse use)
{
  return Match(ratio, use, Tolerance());
}

std::optional<AspectRatioEntry> CAspectRatioVocabulary::Match(float ratio,
                                                              AspectRatioUse use,
                                                              float tolerance)
{
  const std::optional<AspectRatioEntry> nearest = Nearest(ratio, use);
  if (!nearest || Distance(ratio, nearest->ratio) > tolerance)
    return std::nullopt;

  return nearest;
}

std::optional<AspectRatioEntry> CAspectRatioVocabulary::Resolve(float ratio,
                                                                AspectRatioUse use,
                                                                float towardRatio)
{
  std::unique_lock lock(StoreLock());

  const AspectRatioEntry* chosen = ResolveIn(Store(), ratio, use, towardRatio);
  if (!chosen)
    return std::nullopt;

  return *chosen;
}

std::optional<float> CAspectRatioVocabulary::ResolveRatio(float ratio,
                                                          AspectRatioUse use,
                                                          float towardRatio)
{
  std::unique_lock lock(StoreLock());

  const AspectRatioEntry* chosen = ResolveIn(Store(), ratio, use, towardRatio);
  if (!chosen)
    return std::nullopt;

  return chosen->ratio;
}

std::string CAspectRatioVocabulary::Label(float ratio)
{
  const std::optional<AspectRatioEntry> nearest = Nearest(ratio);
  return nearest ? nearest->label : std::string();
}

std::string CAspectRatioVocabulary::Name(float ratio)
{
  const std::optional<AspectRatioEntry> nearest = Nearest(ratio);
  return nearest ? nearest->name : std::string();
}

int CAspectRatioVocabulary::Key(float ratio)
{
  return static_cast<int>(std::lround(ratio * 100.0f));
}

void CAspectRatioVocabulary::AppendDeclareChoices(std::vector<IntegerSettingOption>& list)
{
  for (const AspectRatioEntry& entry : EntriesFor(AspectRatioUse::Declare))
    list.emplace_back(ChoiceLabel(entry), Key(entry.ratio));
}

float CAspectRatioVocabulary::RatioForKey(int key)
{
  std::unique_lock lock(StoreLock());

  for (const VocabularyEntry& held : Store().entries)
  {
    if (Key(held.entry.ratio) == key)
      return held.entry.ratio;
  }

  return 0.0f;
}

std::string CAspectRatioVocabulary::ChoiceLabel(const AspectRatioEntry& entry)
{
  if (entry.name.empty())
    return entry.label;

  return StringUtils::Format("{} - {}", entry.label, entry.name);
}

bool CAspectRatioVocabulary::Apply(std::string_view xml)
{
  CXBMCTinyXML2 document;
  if (!document.Parse(xml) || document.Error())
  {
    CLog::LogF(LOGERROR, "aspect ratio definition is not valid XML: {}", document.ErrorStr());
    return false;
  }

  const tinyxml2::XMLElement* root = document.RootElement();
  if (!root || !StringUtils::EqualsNoCase(root->Value(), "aspectratios"))
  {
    CLog::LogF(LOGERROR, "aspect ratio definition has no <aspectratios> root");
    return false;
  }

  Vocabulary merged;
  {
    std::unique_lock lock(StoreLock());
    merged = Store();
  }

  if (const char* tolerance = root->Attribute("tolerance"))
  {
    double value{0.0};
    if (root->QueryDoubleAttribute("tolerance", &value) != tinyxml2::XML_SUCCESS ||
        !(value > 0.0) || value >= 1.0)
    {
      CLog::LogF(LOGERROR, "tolerance \"{}\" is not a fraction greater than zero", tolerance);
      return false;
    }
    merged.tolerance = static_cast<float>(value);
  }

  for (const tinyxml2::XMLElement* element = root->FirstChildElement("ratio"); element;
       element = element->NextSiblingElement("ratio"))
  {
    if (!MergeRatio(element, merged))
      return false;
  }

  if (merged.entries.empty())
  {
    CLog::LogF(LOGERROR, "aspect ratio definition would leave no ratios at all");
    return false;
  }

  std::ranges::sort(merged.entries, {},
                    [](const VocabularyEntry& held) { return held.entry.ratio; });

  std::unique_lock lock(StoreLock());
  Store() = std::move(merged);
  CachedTolerance().store(Store().tolerance, std::memory_order_relaxed);
  return true;
}

void CAspectRatioVocabulary::Reset()
{
  std::unique_lock lock(StoreLock());
  Store() = BuiltInVocabulary();
  CachedTolerance().store(Store().tolerance, std::memory_order_relaxed);
}

void CAspectRatioVocabulary::Load()
{
  Reset();

  const auto apply = [](const std::string& path, bool required)
  {
    XFILE::CFile file;
    std::vector<uint8_t> buffer;
    if (file.LoadFile(path, buffer) <= 0)
    {
      if (required)
        CLog::LogF(LOGWARNING, "{} could not be read, keeping the built-in ratios", path);
      return;
    }

    const std::string_view xml{reinterpret_cast<const char*>(buffer.data()), buffer.size()};
    if (CAspectRatioVocabulary::Apply(xml))
      CLog::LogF(LOGDEBUG, "applied aspect ratios from {}", path);
    else
      CLog::LogF(LOGERROR, "{} was rejected, keeping the ratios already in force", path);
  };

  apply(std::string("special://xbmc/system/") + DEFINITION_FILE, true);

  // The viewer's own, over the shipped one.
  const auto settings = CServiceBroker::GetSettingsComponent();
  if (settings && settings->GetProfileManager())
    apply(settings->GetProfileManager()->GetUserDataItem(DEFINITION_FILE), false);
}

} // namespace KODI::UTILS
