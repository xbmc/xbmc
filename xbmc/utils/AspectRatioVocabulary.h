/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/SettingDefinitions.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace KODI::UTILS
{

//! \brief A ratio Kodi recognises, what it is called, and where it may be used.
struct AspectRatioEntry
{
  float ratio{0.0f};
  std::string label; //!< the ratio written out
  std::string name; //!< what it is called, empty for none. Not localised - clients branch on it
  bool detect{false}; //!< may be the answer to a measurement
  bool declare{false}; //!< may be offered for a user to declare by hand
};

//! \brief Which entries an operation may choose from.
enum class AspectRatioUse
{
  Any, //!< the whole vocabulary, which every reported label classifies against
  Detect,
  Declare,
};

//! \brief The vocabulary of aspect ratios Kodi recognises, read from a definition file by
//! Load(). Empty until then, and if the shipped definition cannot be read: there is one
//! statement of what the ratios are, and it is the file.
class CAspectRatioVocabulary
{
public:
  //! \brief Largest distance from an entry that still counts as that entry, in log space.
  static constexpr float DEFAULT_TOLERANCE = 0.02f;

  //! \brief The whole vocabulary, in ascending order.
  static std::vector<AspectRatioEntry> Entries();

  //! \brief The entries usable for \p use, in ascending order.
  static std::vector<AspectRatioEntry> EntriesFor(AspectRatioUse use);

  //! \brief The tolerance in force, which a loaded definition may set.
  static float Tolerance();

  //! \brief Distance between two ratios, in log space.
  static float Distance(float a, float b);

  //! \brief The closest entry to \p ratio, cutting between adjacent entries at their geometric
  //! mean. Always some entry unless \p ratio is not a ratio.
  static std::optional<AspectRatioEntry> Nearest(float ratio,
                                                 AspectRatioUse use = AspectRatioUse::Any);

  //! \brief The closest entry to \p ratio if it is within the tolerance in force. Unlike
  //! Nearest(), rejects a ratio matching no entry.
  static std::optional<AspectRatioEntry> Match(float ratio,
                                               AspectRatioUse use = AspectRatioUse::Any);

  //! \brief The closest entry to \p ratio if it is within \p tolerance of it.
  static std::optional<AspectRatioEntry> Match(float ratio, AspectRatioUse use, float tolerance);

  //! \brief The entry a measurement resolves to. Where several are within tolerance the cut is
  //! made toward \p towardRatio rather than at the geometric mean; zero for no preference.
  static std::optional<AspectRatioEntry> Resolve(float ratio,
                                                 AspectRatioUse use,
                                                 float towardRatio);

  //! \brief Resolve() for a caller that wants only the ratio, answering identically.
  static std::optional<float> ResolveRatio(float ratio, AspectRatioUse use, float towardRatio);

  //! \brief The label Kodi reports for \p ratio, or empty when \p ratio is not a ratio.
  static std::string Label(float ratio);

  //! \brief What Kodi calls \p ratio, empty when it has no name or is not a ratio.
  static std::string Name(float ratio);

  //! \brief A ratio as the hundredths a setting stores it as. Zero is not a ratio, and is left
  //! free for a caller expressing no choice.
  static int Key(float ratio);

  //! \brief The vocabulary's own ratio for \p key, not one reconstructed from it. Zero when no
  //! entry has it.
  static float RatioForKey(int key);

  //! \brief How an entry reads where a viewer picks one: the number, then the name.
  static std::string ChoiceLabel(const AspectRatioEntry& entry);

  //! \brief Append the declarable entries as ChoiceLabel()/Key() choices.
  static void AppendDeclareChoices(std::vector<IntegerSettingOption>& list);

  //! \brief Read the shipped definition, then the viewer's. Call once, before anything
  //! classifies a ratio.
  static void Load();

  //! \brief Apply one definition over the vocabulary in force, replacing entries it names and
  //! adding the rest. All or nothing: a malformed document changes nothing and returns false.
  static bool Apply(std::string_view xml);

  //! \brief Discard anything loaded, leaving nothing to classify against.
  static void Reset();
};

} // namespace KODI::UTILS
