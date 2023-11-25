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
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. Defaults to nullptr.
   \return label (or image).
   */
  const std::string& GetItemLabel(const CGUIListItem* item,
                                  bool preferImage = false,
                                  std::string* fallback = nullptr) const;

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

  /*!
   * \brief Replaces instances of $FEATURE[feature name, controller ID] with
   *        the appropriate localized controller string
   *
   * \param label The text to replace
   *
   * \return text with any controller strings filled in
   */
  static std::string ReplaceControllerStrings(std::string&& label);

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

  /*! \brief Parse a provided label string into the list of info portions that may compose the label. Info portions mean the portions of complex
  infolabels along with their composition operations (and, or, etc) or skin variables.
  The label is also rewritten to resolve localized strings and other dynamic expressions
   \param label [in, out] the string label in question (e.g. "$LOCALIZE[xxx]")
   \param infoPortion [in, out]  the list of info portions that may have been parsed from the string
   \param context the context where the info expressions need to be evaluated (currently window ids)
  */
  void Parse(const std::string& label, std::vector<CInfoPortion>& infoPortion, int context);

  /*! \brief return (and cache) built label from info portions.
   \param rebuild whether we need to rebuild the label
   \sa GetLabel, GetItemLabel
   */
  const std::string& CacheLabel(bool rebuild) const;

  /*! \brief Rebuild a label value, based on the provided already resolved info portions (a localized string, multiple infolabels, etc)
   \param label[in, out] label value where to store the processed result
   \param infoPortion the list of info portions 
   */
  void RebuildLabel(std::string& label, const std::vector<CInfoPortion>& infoPortion) const;

  /*! \brief Checks if a given label needs to be updated, based on the provided info portions (a localized string, multiple infolabels, etc)
   \param context the context where the info expressions need to be evaluated (currently window ids)
   \param preferImages caller is specifically wanting an image rather than a label.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. This is used by infoproviders to
   to re-write the fallback label
   \param infoPortion the list of info portions 
   \return true if an update is needed, false otherwise
   */
  bool LabelNeedsUpdate(int context,
                        bool preferImages,
                        std::string* fallback,
                        const std::vector<CInfoPortion>& infoPortion) const;
  /*! \brief Checks if a given item label needs to be updated, based on the provided info portions (a localized string, multiple infolabels, etc)
   \param item listItem in question.
   \param preferImages caller is specifically wanting an image rather than a label.
   \param fallback if non-NULL, is set to an alternate value to use should the actual value be not appropriate. This is used by infoproviders to
   to re-write the fallback label
   \param infoPortion the list of info portions 
   \return true if an update is needed, false otherwise
   */
  bool ItemLabelNeedsUpdate(const CGUIListItem* item,
                            bool preferImages,
                            std::string* fallback,
                            const std::vector<CInfoPortion>& infoPortion) const;

  mutable bool        m_dirty = false;
  mutable std::string m_label;
  mutable std::string m_fallback;
  std::vector<CInfoPortion> m_infoLabel;
  std::vector<CInfoPortion> m_infoFallback;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI

