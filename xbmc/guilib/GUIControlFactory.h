/*!
\file GuiControlFactory.h
\brief
*/

#ifndef GUI_CONTROL_FACTORY_H
#define GIU_CONTROL_FACTORY_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIControl.h"

class CTextureInfo; // forward
class CAspectRatio;
class CGUIInfoLabel;
class TiXmlNode;
class CGUIAction;

/*!
 \ingroup controls
 \brief
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CGUIControl* Create(int parentID, const CRect &rect, TiXmlElement* pControlNode, bool insideContainer = false);

  /*! \brief translate from control name to control type
   \param type name of the control
   \return type of control
   */
  static CGUIControl::GUICONTROLTYPES TranslateControlType(const CStdString &type);

  /*! \brief translate from control type to control name
   \param type type of the control
   \return name of control
   */
  static CStdString TranslateControlType(CGUIControl::GUICONTROLTYPES type);

  /*! \brief grab a dimension out of the XML
   
   Supports plain reading of a number (or constant) and, in addition allows "auto" as the value
   for the dimension, whereby value is set to the max attribute (if it exists) and min is set the min
   attribute (if it exists) or 1.  Auto values are thus detected by min != 0.

   \param pRootNode XML node to read
   \param strTag tag within pRootNode to read
   \param value value to set, or maximum value if using auto
   \param min minimum value - set != 0 if auto is used.
   \return true if we found and read the tag.
   */
  static bool GetDimension(const TiXmlNode* pRootNode, const char* strTag, float &value, float &min);
  static bool GetAspectRatio(const TiXmlNode* pRootNode, const char* strTag, CAspectRatio &aspectRatio);
  static bool GetInfoTexture(const TiXmlNode* pRootNode, const char* strTag, CTextureInfo &image, CGUIInfoLabel &info, int parentID);
  static bool GetTexture(const TiXmlNode* pRootNode, const char* strTag, CTextureInfo &image);
  static bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwAlignment);
  static bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwAlignment);
  static bool GetAnimations(TiXmlNode *control, const CRect &rect, int context, std::vector<CAnimation> &animation);

  /*! \brief Create an info label from an XML element
   Processes XML elements of the form
    <xmltag fallback="fallback_value">info_value</xmltag>
   where info_value may use $INFO[], $LOCALIZE[], $NUMBER[] etc.
   If either the fallback_value or info_value are natural numbers they are interpreted
   as ids for lookup in strings.xml. The fallback attribute is optional.
   \param element XML element to process
   \param infoLabel returned infoLabel
   \return true if a valid info label was read, false otherwise
   */
  static bool GetInfoLabelFromElement(const TiXmlElement *element, CGUIInfoLabel &infoLabel, int parentID);
  static void GetInfoLabel(const TiXmlNode *pControlNode, const CStdString &labelTag, CGUIInfoLabel &infoLabel, int parentID);
  static void GetInfoLabels(const TiXmlNode *pControlNode, const CStdString &labelTag, std::vector<CGUIInfoLabel> &infoLabels, int parentID);
  static bool GetColor(const TiXmlNode* pRootNode, const char* strTag, color_t &value);
  static bool GetInfoColor(const TiXmlNode* pRootNode, const char* strTag, CGUIInfoColor &value, int parentID);
  static CStdString FilterLabel(const CStdString &label);
  static bool GetConditionalVisibility(const TiXmlNode* control, CStdString &condition);
  static bool GetActions(const TiXmlNode* pRootNode, const char* strTag, CGUIAction& actions);
  static void GetRectFromString(const CStdString &string, CRect &rect);
  static bool GetHitRect(const TiXmlNode* pRootNode, CRect &rect);
  static bool GetScroller(const TiXmlNode *pControlNode, const CStdString &scrollerTag, CScroller& scroller);
private:
  static CStdString GetType(const TiXmlElement *pControlNode);
  static bool GetConditionalVisibility(const TiXmlNode* control, CStdString &condition, CStdString &allowHiddenFocus);
  bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strString);
  bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
  bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
};
#endif
