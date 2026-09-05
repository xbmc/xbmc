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
#include <cmath>
#include <limits>
#include <mutex>

namespace KODI::UTILS
{

namespace
{

constexpr const char* DEFINITION_FILE = "aspectratios.xml";

//! \brief One row of the built-in vocabulary, held as literals so it costs nothing until used.
struct BuiltInEntry
{
  float ratio;
  std::string_view name;
  bool detect;
  bool declare;
};

/*!
 * \brief The vocabulary Kodi answers with when no definition has been read.
 *
 * Ascending order is required: classification cuts between adjacent entries at their geometric
 * mean. A rare ratio is withheld from detection where a measurement landing on it wrongly would
 * claim a narrower rectangle than the truth; every ratio stays declarable.
 */
constexpr auto BUILT_IN = std::to_array<BuiltInEntry>({
    // Square is a failure signature rather than a ratio: equal bars on all four sides means the
    // frame was never resolved.
    {1.00f, "", false, true},
    {1.19f, "Movietone", true, true},
    // Exact fractions are held as such: at the two-decimal nominal, 16:9 would put every
    // full-frame measurement a couple of pixels inside its own frame. Labels and keys round.
    {4.0f / 3.0f, "4:3", true, true},
    {1.37f, "Academy", true, true},
    {1.43f, "IMAX 70mm", true, true},
    // A stills ratio, essentially nothing being delivered at it. Undetectable, so a failed
    // measurement has nowhere to land in the wide gap between 1.43 and 1.66.
    {1.50f, "3:2", false, true},
    {1.66f, "", true, true},
    {16.0f / 9.0f, "16:9", true, true},
    {1.85f, "Flat", true, true},
    {1.90f, "IMAX digital", true, true}, // also the DCI full container
    {2.00f, "Univisium", true, true},
    {2.20f, "Todd-AO", true, true},
    // The original CinemaScope ratio and modern anamorphic scope. Close together and routinely
    // conflated, which is why both carry a name rather than only the numbers.
    {2.35f, "CinemaScope", true, true},
    {2.40f, "Scope", true, true},
    // Both are delivered, and both are rare enough that a measurement landing here is more
    // likely an overshoot on the entry below.
    {2.55f, "CinemaScope 55", false, true},
    {2.76f, "Ultra Panavision 70", false, true},
});

//! \brief The label for a ratio, which is the ratio written out. Derived rather than stored, and
//! not settable from a definition file - every existing consumer takes the label to be a number.
std::string LabelFor(float ratio)
{
  return StringUtils::Format("{:.2f}", ratio);
}

struct Vocabulary
{
  std::vector<AspectRatioEntry> entries;
  float tolerance{CAspectRatioVocabulary::DEFAULT_TOLERANCE};
};

Vocabulary BuiltInVocabulary()
{
  Vocabulary vocabulary;
  vocabulary.entries.reserve(BUILT_IN.size());
  for (const BuiltInEntry& entry : BUILT_IN)
  {
    vocabulary.entries.emplace_back(AspectRatioEntry{
        entry.ratio, LabelFor(entry.ratio), std::string(entry.name), entry.detect, entry.declare});
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

  // Comparing squares avoids a square root per entry and keeps the cutoffs derived from the
  // table rather than held alongside it.
  const float squared = ratio * ratio;

  const AspectRatioEntry* previous = nullptr;
  for (const AspectRatioEntry& entry : vocabulary.entries)
  {
    if (!Usable(entry, use))
      continue;

    if (previous && squared < previous->ratio * entry.ratio)
      return previous;

    previous = &entry;
  }

  return previous;
}

//! \brief Read a boolean attribute, leaving \p value alone when the attribute is absent, so
//! that a definition can change one property of an entry without restating the others.
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

//! \brief Merge one <ratio> into \p vocabulary. Returns false on anything it cannot read, a
//! partly understood definition being one nobody has tested against.
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
  // saying 1.78 changes the 16:9 entry rather than standing beside it. It also keeps every key
  // distinct - two entries inside one hundredth would share a key.
  const auto existing = std::ranges::find_if(
      vocabulary.entries, [ratio](const auto& entry)
      { return CAspectRatioVocabulary::Key(entry.ratio) == CAspectRatioVocabulary::Key(ratio); });

  AspectRatioEntry entry = existing != vocabulary.entries.end()
                               ? *existing
                               : AspectRatioEntry{ratio, LabelFor(ratio), {}, true, true};

  if (!ReadFlag(element, "detect", entry.detect) || !ReadFlag(element, "declare", entry.declare))
  {
    CLog::LogF(LOGERROR, "<ratio value=\"{}\"> has a flag that is neither true nor false", value);
    return false;
  }

  if (const char* name = element->Attribute("name"))
    entry.name = name;

  if (existing != vocabulary.entries.end())
    *existing = std::move(entry);
  else
    vocabulary.entries.emplace_back(std::move(entry));

  return true;
}

} // unnamed namespace

std::vector<AspectRatioEntry> CAspectRatioVocabulary::Entries()
{
  std::unique_lock lock(StoreLock());
  return Store().entries;
}

std::vector<AspectRatioEntry> CAspectRatioVocabulary::EntriesFor(AspectRatioUse use)
{
  std::unique_lock lock(StoreLock());

  std::vector<AspectRatioEntry> entries;
  entries.reserve(Store().entries.size());
  for (const AspectRatioEntry& entry : Store().entries)
  {
    if (Usable(entry, use))
      entries.emplace_back(entry);
  }
  return entries;
}

float CAspectRatioVocabulary::Tolerance()
{
  std::unique_lock lock(StoreLock());
  return Store().tolerance;
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
  const Vocabulary& vocabulary = Store();

  const AspectRatioEntry* chosen = nullptr;
  for (const AspectRatioEntry& entry : vocabulary.entries)
  {
    if (!Usable(entry, use) || Distance(ratio, entry.ratio) > vocabulary.tolerance)
      continue;

    if (!chosen)
    {
      chosen = &entry;
      continue;
    }

    // Distance() answers infinity for a towardRatio that is not a ratio, making the room's
    // preference a tie everywhere and leaving the measurement's own proximity to decide.
    const float held = Distance(chosen->ratio, towardRatio);
    const float offered = Distance(entry.ratio, towardRatio);
    if (offered < held ||
        (offered == held && Distance(ratio, entry.ratio) < Distance(ratio, chosen->ratio)))
      chosen = &entry;
  }

  if (!chosen)
    return std::nullopt;

  return *chosen;
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

  // Matched inside the lock rather than over a copy: this is reached from the resolver on every
  // refresh to compare integers, where the copy cost thirty-two string allocations.
  for (const AspectRatioEntry& entry : Store().entries)
  {
    if (Key(entry.ratio) == key)
      return entry.ratio;
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

  // Merged into a copy so that a document rejected part way through leaves nothing behind.
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

  // Classification cuts between neighbours, so the order is part of the data. Sorted here
  // rather than demanded of the file.
  std::ranges::sort(merged.entries, {}, &AspectRatioEntry::ratio);

  std::unique_lock lock(StoreLock());
  Store() = std::move(merged);
  return true;
}

void CAspectRatioVocabulary::Reset()
{
  std::unique_lock lock(StoreLock());
  Store() = BuiltInVocabulary();
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

  // The viewer's own, applied over the shipped one so that changing a ratio does not mean owning
  // the whole list. Absent for almost everyone, so not worth a log line.
  const auto settings = CServiceBroker::GetSettingsComponent();
  if (settings && settings->GetProfileManager())
    apply(settings->GetProfileManager()->GetUserDataItem(DEFINITION_FILE), false);
}

} // namespace KODI::UTILS
