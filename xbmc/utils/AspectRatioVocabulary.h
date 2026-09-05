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

  //! \brief The ratio written out, and what every existing consumer means by a ratio's label.
  std::string label;

  //! \brief What the ratio is called, empty when it has no established name. Not localised - it
  //! is served unchanged to skins and to JSON-RPC, which branch on it.
  std::string name;

  //! \brief May be the answer to a measurement.
  bool detect{false};

  //! \brief May be offered for a user to declare by hand.
  bool declare{false};
};

//! \brief Which entries an operation may choose from.
enum class AspectRatioUse
{
  //! \brief The whole vocabulary. Reported labels classify against this, so that the label for
  //! a given ratio never depends on how the ratio was arrived at.
  Any,

  Detect,
  Declare,
};

/*!
 * \brief The vocabulary of aspect ratios Kodi recognises.
 *
 * The single source for the labels skins display, the list a declaration is chosen from, and the
 * check that decides whether a measured ratio corresponds to any real ratio at all. Answers from
 * a built-in table until Load() replaces it, and again if loading fails.
 */
class CAspectRatioVocabulary
{
public:
  //! \brief Largest distance from an entry that still counts as that entry, in log space. Set
  //! wide enough for an off-nominal encode: 1888x1080 is 1.748, 1.7% from the 1.78 entry.
  static constexpr float DEFAULT_TOLERANCE = 0.02f;

  //! \brief The whole vocabulary, in ascending order.
  static std::vector<AspectRatioEntry> Entries();

  //! \brief The entries usable for \p use, in ascending order.
  static std::vector<AspectRatioEntry> EntriesFor(AspectRatioUse use);

  //! \brief The tolerance in force, which a loaded definition may set.
  static float Tolerance();

  //! \brief Distance between two ratios, in log space, so it does not depend on where on the
  //! scale they sit.
  static float Distance(float a, float b);

  /*!
   * \brief The closest entry to \p ratio, which is always some entry. The cutoff between two
   *        adjacent entries is their geometric mean.
   *
   * \return nothing only when \p ratio is not a ratio, or nothing is usable for \p use.
   */
  static std::optional<AspectRatioEntry> Nearest(float ratio,
                                                 AspectRatioUse use = AspectRatioUse::Any);

  //! \brief The closest entry to \p ratio, if \p ratio is within the tolerance in force. Unlike
  //! Nearest(), this rejects a ratio that corresponds to no real ratio.
  static std::optional<AspectRatioEntry> Match(float ratio,
                                               AspectRatioUse use = AspectRatioUse::Any);

  /*!
   * \brief The closest entry to \p ratio, if \p ratio is within \p tolerance of it.
   *
   * Tolerance is applied after classification rather than as a window around each entry, the
   * windows of adjacent entries overlapping at any useful tolerance - 2.35 and 2.40 are 2.1%
   * apart.
   */
  static std::optional<AspectRatioEntry> Match(float ratio, AspectRatioUse use, float tolerance);

  /*!
   * \brief The entry a measurement resolves to, erring toward the shape the room rests at.
   *
   * Rejects exactly as Match() does. Where more than one entry is within tolerance the cut is
   * made toward \p towardRatio rather than at the geometric mean, so that the error is always
   * into the masking rather than onto the wall.
   *
   * \param towardRatio the resting shape. Zero or negative means no preference, and the
   *        nearest entry wins exactly as it does for Match().
   */
  static std::optional<AspectRatioEntry> Resolve(float ratio,
                                                 AspectRatioUse use,
                                                 float towardRatio);

  //! \brief The label Kodi reports for \p ratio, or empty when \p ratio is not a ratio.
  static std::string Label(float ratio);

  //! \brief What Kodi calls \p ratio, empty when it has no name or is not a ratio.
  static std::string Name(float ratio);

  //! \brief A ratio as the hundredths a setting or a list control stores it as. Zero is not a
  //! ratio, and is left free for a caller expressing the absence of a choice.
  static int Key(float ratio);

  /*!
   * \brief The vocabulary's own ratio for \p key, rather than one reconstructed from it, so that
   *        a definition file changing an entry moves everything that chose it.
   *
   * \return zero when no entry has that key
   */
  static float RatioForKey(int key);

  //! \brief How an entry reads where a viewer picks one: the number, then the name. Not
  //! translated, for the same reason the name is not.
  static std::string ChoiceLabel(const AspectRatioEntry& entry);

  //! \brief Append the declarable entries as ChoiceLabel()/Key() choices, after whatever
  //! zero-keyed lead entry the caller supplied.
  static void AppendDeclareChoices(std::vector<IntegerSettingOption>& list);

  //! \brief Read the shipped definition, then the viewer's, and adopt the result. Call once,
  //! before anything classifies a ratio.
  static void Load();

  /*!
   * \brief Apply one definition document over the vocabulary in force.
   *
   * An entry whose ratio matches one already present replaces it; any other is added, so a
   * viewer can change one ratio without restating the rest. All or nothing: a document that is
   * malformed anywhere leaves the vocabulary untouched.
   *
   * \return false if \p xml was rejected, in which case nothing changed.
   */
  static bool Apply(std::string_view xml);

  //! \brief Discard anything loaded and go back to the built-in vocabulary.
  static void Reset();
};

} // namespace KODI::UTILS
