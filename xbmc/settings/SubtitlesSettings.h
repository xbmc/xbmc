/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "utils/ColorUtils.h"
#include "utils/Observer.h"

#include <memory>
#include <string>

class CSetting;
class CSettings;
struct StringSettingOption;

namespace KODI
{
namespace SUBTITLES
{
// This is a placeholder to keep the fontname setting valid
// even if the default app font could be changed
constexpr const char* FONT_DEFAULT_FAMILYNAME = "DEFAULT";

enum class Align
{
  MANUAL = 0,
  BOTTOM_INSIDE,
  BOTTOM_OUTSIDE,
  TOP_INSIDE,
  TOP_OUTSIDE
};

enum class HorizontalAlign
{
  LEFT = 0,
  CENTER,
  RIGHT
};

enum class BackgroundType
{
  NONE = 0,
  SHADOW,
  BOX,
  SQUAREBOX
};

enum class FontStyle
{
  NORMAL = 0,
  BOLD,
  ITALIC,
  BOLD_ITALIC
};

enum class OverrideStyles
{
  DISABLED = 0,
  POSITIONS,
  STYLES,
  STYLES_POSITIONS
};

class CSubtitlesSettings : public ISettingCallback, public Observable
{
public:
  explicit CSubtitlesSettings(const std::shared_ptr<CSettings>& settings);
  ~CSubtitlesSettings() override;

  // Inherited from ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   * \brief Get subtitle alignment
   * \return The alignment
   */
  Align GetAlignment() const;

  /*!
   * \brief Set the subtitle alignment
   * \param align The alignment
   */
  void SetAlignment(Align align) const;

  /*!
   * \brief Get horizontal text alignment
   * \return The alignment
   */
  HorizontalAlign GetHorizontalAlignment() const;

  /*!
   * \brief Get font name
   * \return The font name
   */
  std::string GetFontName() const;

  /*!
   * \brief Get font style
   * \return The font style
   */
  FontStyle GetFontStyle() const;

  /*!
   * \brief Get font size
   * \return The font size in PX
   */
  int GetFontSize() const;

  /*!
   * \brief Get font color
   * \return The font color
   */
  UTILS::COLOR::Color GetFontColor() const;

  /*!
   * \brief Get font opacity
   * \return The font opacity in %
   */
  int GetFontOpacity() const;

  /*!
   * \brief Get border size
   * \return The border size in %
   */
  int GetBorderSize() const;

  /*!
   * \brief Get border color
   * \return The border color
   */
  UTILS::COLOR::Color GetBorderColor() const;

  /*!
   * \brief Get shadow size
   * \return The shadow size in %
   */
  int GetShadowSize() const;

  /*!
   * \brief Get shadow color
   * \return The shadow color
   */
  UTILS::COLOR::Color GetShadowColor() const;

  /*!
   * \brief Get shadow opacity
   * \return The shadow opacity in %
   */
  int GetShadowOpacity() const;

  /*!
   * \brief Get blur size
   * \return The blur size in %
   */
  int GetBlurSize() const;

  /*!
   * \brief Get line spacing
   * \return The line spacing
   */
  int GetLineSpacing() const;

  /*!
   * \brief Get background type
   * \return The background type
   */
  BackgroundType GetBackgroundType() const;

  /*!
   * \brief Get background color
   * \return The background color
   */
  UTILS::COLOR::Color GetBackgroundColor() const;

  /*!
   * \brief Get background opacity
   * \return The background opacity in %
   */
  int GetBackgroundOpacity() const;

  /*!
   * \brief Check if font override is enabled
   * \return True if fonts must be overridden, otherwise false
   */
  bool IsOverrideFonts() const;

  /*!
   * \brief Get override styles
   * \return The styles to be overridden
   */
  OverrideStyles GetOverrideStyles() const;

  /*!
   * \brief Get the subtitle vertical margin
   * \return The vertical margin in %
   */
  float GetVerticalMarginPerc() const;

  static void SettingOptionsSubtitleFontsFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<StringSettingOption>& list,
                                                std::string& current);

private:
  CSubtitlesSettings() = delete;

  const std::shared_ptr<CSettings> m_settings;
};

} // namespace SUBTITLES
} // namespace KODI
