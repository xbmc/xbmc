/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIControlFactory.h
\brief
*/

#include "GUIControl.h"
#include "utils/ColorUtils.h"
#include "utils/MovingSpeed.h"

#include <string>
#include <vector>

class CTextureInfo; // forward
class CAspectRatio;
class TiXmlNode;
class CGUIAction;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
class CGUIInfoLabel;
}
} // namespace GUILIB
} // namespace KODI

/*!
 \ingroup controls
 \brief
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CGUIControl* Create(int parentID,
                      const CRect& rect,
                      TiXmlElement* pControlNode,
                      bool insideContainer = false);

  /*! \brief translate from control name to control type
   \param type name of the control
   \return type of control
   */
  static CGUIControl::GUICONTROLTYPES TranslateControlType(const std::string& type);

  /*! \brief translate from control type to control name
   \param type type of the control
   \return name of control
   */
  static std::string TranslateControlType(CGUIControl::GUICONTROLTYPES type);

  static bool GetAspectRatio(const TiXmlNode* pRootNode,
                             const char* strTag,
                             CAspectRatio& aspectRatio);
  static bool GetInfoTexture(const TiXmlNode* pRootNode,
                             const char* strTag,
                             CTextureInfo& image,
                             KODI::GUILIB::GUIINFO::CGUIInfoLabel& info,
                             int parentID);
  static bool GetTexture(const TiXmlNode* pRootNode, const char* strTag, CTextureInfo& image);
  static bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwAlignment);
  static bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwAlignment);
  static bool GetAnimations(TiXmlNode* control,
                            const CRect& rect,
                            int context,
                            std::vector<CAnimation>& animation);

  /*! \brief Create an info label from an XML element
   Processes XML elements of the form
    <xmltag fallback="fallback_value">info_value</xmltag>
   where info_value may use $INFO[], $LOCALIZE[], $NUMBER[] etc.
   If either the fallback_value or info_value are natural numbers they are interpreted
   as ids for lookup in strings.po. The fallback attribute is optional.
   \param element XML element to process
   \param infoLabel Returned infoLabel
   \param parentID The parent id
   \return true if a valid info label was read, false otherwise
   */

  /*! \brief Parse a position string
   Handles strings of the form
   ###   number of pixels
   ###r  number of pixels measured from the right
   ###%  percentage of parent size
   \param pos the string to parse.
   \param parentSize the size of the parent.
   \sa GetPosition
   */
  static float ParsePosition(const char* pos, const float parentSize);

  static bool GetInfoLabelFromElement(const TiXmlElement* element,
                                      KODI::GUILIB::GUIINFO::CGUIInfoLabel& infoLabel,
                                      int parentID);
  static void GetInfoLabel(const TiXmlNode* pControlNode,
                           const std::string& labelTag,
                           KODI::GUILIB::GUIINFO::CGUIInfoLabel& infoLabel,
                           int parentID);
  static void GetInfoLabels(const TiXmlNode* pControlNode,
                            const std::string& labelTag,
                            std::vector<KODI::GUILIB::GUIINFO::CGUIInfoLabel>& infoLabels,
                            int parentID);
  static bool GetColor(const TiXmlNode* pRootNode, const char* strTag, UTILS::COLOR::Color& value);
  static bool GetInfoColor(const TiXmlNode* pRootNode,
                           const char* strTag,
                           KODI::GUILIB::GUIINFO::CGUIInfoColor& value,
                           int parentID);
  static std::string FilterLabel(const std::string& label);
  static bool GetConditionalVisibility(const TiXmlNode* control, std::string& condition);
  static bool GetActions(const TiXmlNode* pRootNode, const char* strTag, CGUIAction& actions);
  static void GetRectFromString(const std::string& string, CRect& rect);
  static bool GetHitRect(const TiXmlNode* pRootNode, CRect& rect, const CRect& parentRect);
  static bool GetScroller(const TiXmlNode* pControlNode,
                          const std::string& scrollerTag,
                          CScroller& scroller);

private:
  static std::string GetType(const TiXmlElement* pControlNode);
  static bool GetMovingSpeedConfig(const TiXmlNode* pRootNode,
                                   const char* strTag,
                                   UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg);
  static bool GetConditionalVisibility(const TiXmlNode* control,
                                       std::string& condition,
                                       std::string& allowHiddenFocus);
  bool GetString(const TiXmlNode* pRootNode, const char* strTag, std::string& strString);
  static bool GetFloatRange(const TiXmlNode* pRootNode,
                            const char* strTag,
                            float& iMinValue,
                            float& iMaxValue,
                            float& iIntervalValue);
  static bool GetIntRange(const TiXmlNode* pRootNode,
                          const char* strTag,
                          int& iMinValue,
                          int& iMaxValue,
                          int& iIntervalValue);

  /*! \brief Get the value of a position tag from XML
   Handles both absolute and relative values.
   \param node the <control> XML node.
   \param tag the XML node to parse.
   \param parentSize the size of the parent, for relative positioning.
   \param value [out] the returned value.
   \sa ParsePosition, GetDimension, GetDimensions.
   */
  static bool GetPosition(const TiXmlNode* node,
                          const char* tag,
                          const float parentSize,
                          float& value);

  /*! \brief grab a dimension out of the XML

   Supports plain reading of a number (or constant) and, in addition allows "auto" as the value
   for the dimension, whereby value is set to the max attribute (if it exists) and min is set the min
   attribute (if it exists) or 1.  Auto values are thus detected by min != 0.

   \param node the <control> XML node to read
   \param strTag tag within node to read
   \param parentSize the size of the parent for relative sizing.
   \param value value to set, or maximum value if using auto
   \param min minimum value - set != 0 if auto is used.
   \return true if we found and read the tag.
   \sa GetPosition, GetDimensions, ParsePosition.
   */
  static bool GetDimension(
      const TiXmlNode* node, const char* strTag, const float parentSize, float& value, float& min);

  /*! \brief Retrieve the dimensions for a control.

   Handles positioning based on at least 2 of left/right/center/width.

   \param node the <control> node describing the control.
   \param leftTag the tag that holds the left field.
   \param rightTag the tag that holds the right field.
   \param centerLeftTag the tag that holds the center left field.
   \param centerRightTag the tag that holds the center right field.
   \param widthTag the tag holding the width.
   \param parentSize the size of the parent, for relative sizing.
   \param pos [out] the discovered position.
   \param width [out] the discovered width.
   \param min_width [out] the discovered minimum width.
   \return true if we can successfully derive the position and size, false otherwise.
   \sa GetDimension, GetPosition, ParsePosition.
   */
  static bool GetDimensions(const TiXmlNode* node,
                            const char* leftTag,
                            const char* rightTag,
                            const char* centerLeftTag,
                            const char* centerRightTag,
                            const char* widthTag,
                            const float parentSize,
                            float& left,
                            float& width,
                            float& min_width);
};
