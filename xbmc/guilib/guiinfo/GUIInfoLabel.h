/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIInfoLabel.h
\brief
*/

#include "interfaces/info/Info.h"

#include <functional>
#include <string>
#include <vector>

class CGUIListItem;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfoLabel
{
public:
  CGUIInfoLabel() = default;
  CGUIInfoLabel(const std::string& label,
                const std::string& fallback = "",
                int context = INFO::DEFAULT_CONTEXT);

  void SetLabel(const std::string& label,
                const std::string& fallback,
                int context = INFO::DEFAULT_CONTEXT);

  /*!
   \brief Gets a label (or image) for a given window context from the info manager.
   \param contextWindow the context in which to evaluate the expression.
   \param preferImage caller is specifically wanting an image rather than a label. Defaults to false.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. Defaults to NULL.
   \return label (or image).
   */
  const std::string &GetLabel(int contextWindow, bool preferImage = false, std::string *fallback = NULL) const;

  /*!
   \brief Gets the label and returns it as an int value
   \param contextWindow the context in which to evaluate the expression.
   \return int value.
   \sa GetLabel
   */
  int GetIntValue(int contextWindow) const;

  /*!
   \brief Gets a label (or image) for a given listitem from the info manager.
   \param item listitem in question.
   \param preferImage caller is specifically wanting an image rather than a label. Defaults to false.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. Defaults to NULL.
   \return label (or image).
   */
  const std::string &GetItemLabel(const CGUIListItem *item, bool preferImage = false, std::string *fallback = NULL) const;

  bool IsConstant() const;
  bool IsEmpty() const;

  const std::string& GetFallback() const { return m_fallback; }

  static std::string GetLabel(const std::string& label,
                              int contextWindow,
                              bool preferImage = false);
  static std::string GetItemLabel(const std::string &label, const CGUIListItem *item, bool preferImage = false);

  /*!
   \brief Replaces instances of $LOCALIZE[number] with the appropriate localized string
   \param label text to replace
   \return text with any localized strings filled in.
   */
  static std::string ReplaceLocalize(const std::string &label);

  /*!
   \brief Replaces instances of $ADDON[id number] with the appropriate localized addon string
   \param label text to replace
   \return text with any localized strings filled in.
   */
  static std::string ReplaceAddonStrings(std::string &&label);

  typedef std::function<std::string(const std::string&)> StringReplacerFunc;

  /*!
   \brief Replaces instances of $strKeyword[value] with the appropriate resolved string
   \param strInput text to replace
   \param strKeyword keyword to look for
   \param func function that does the actual replacement of each bracketed value found
   \param strOutput the output string
   \return whether anything has been replaced.
   */
  static bool ReplaceSpecialKeywordReferences(const std::string &strInput, const std::string &strKeyword, const StringReplacerFunc &func, std::string &strOutput);

  /*!
   \brief Replaces instances of $strKeyword[value] with the appropriate resolved string in-place
   \param work text to replace in-place
   \param strKeyword keyword to look for
   \param func function that does the actual replacement of each bracketed value found
   \return whether anything has been replaced.
   */
  static bool ReplaceSpecialKeywordReferences(std::string &work, const std::string &strKeyword, const StringReplacerFunc &func);

private:
  void Parse(const std::string &label, int context);

  /*! \brief return (and cache) built label from info portions.
   \param rebuild whether we need to rebuild the label
   \sa GetLabel, GetItemLabel
   */
  const std::string &CacheLabel(bool rebuild) const;

  class CInfoPortion
  {
  public:
    CInfoPortion(int info, const std::string &prefix, const std::string &postfix, bool escaped = false);
    bool NeedsUpdate(const std::string &label) const;
    std::string Get() const;
    int m_info;
  private:
    bool m_escaped;
    mutable std::string m_label;
    std::string m_prefix;
    std::string m_postfix;
  };

  mutable bool        m_dirty = false;
  mutable std::string m_label;
  std::string m_fallback;
  std::vector<CInfoPortion> m_info;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI

