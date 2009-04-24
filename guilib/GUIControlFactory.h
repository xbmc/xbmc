/*!
\file GuiControlFactory.h
\brief
*/

#ifndef GUI_CONTROL_FACTORY_H
#define GIU_CONTROL_FACTORY_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIControl.h"

class CTextureInfo; // forward
class CAspectRatio;
class CGUIInfoLabel;
class TiXmlNode;

/*!
 \ingroup controls
 \brief
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  static CStdString GetType(const TiXmlElement *pControlNode);
  CGUIControl* Create(DWORD dwParentId, const FRECT &rect, TiXmlElement* pControlNode, bool insideContainer = false);
  void ScaleElement(TiXmlElement *element, RESOLUTION fileRes, RESOLUTION destRes);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value);
  static bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& value);
  static bool GetAspectRatio(const TiXmlNode* pRootNode, const char* strTag, CAspectRatio &aspectRatio);
  static bool GetInfoTexture(const TiXmlNode* pRootNode, const char* strTag, CTextureInfo &image, CGUIInfoLabel &info);
  static bool GetTexture(const TiXmlNode* pRootNode, const char* strTag, CTextureInfo &image);
  static bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  static bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  static bool GetAnimations(const TiXmlNode *control, const FRECT &rect, std::vector<CAnimation> &animation);
  static void GetInfoLabel(const TiXmlNode *pControlNode, const CStdString &labelTag, CGUIInfoLabel &infoLabel);
  static void GetInfoLabels(const TiXmlNode *pControlNode, const CStdString &labelTag, std::vector<CGUIInfoLabel> &infoLabels);
  static bool GetColor(const TiXmlNode* pRootNode, const char* strTag, DWORD &value);
  static bool GetInfoColor(const TiXmlNode* pRootNode, const char* strTag, CGUIInfoColor &value);
  static CStdString FilterLabel(const CStdString &label);
  static bool GetConditionalVisibility(const TiXmlNode* control, int &condition);
  static bool GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, std::vector<CStdString>& vecStringValue);
  static void GetRectFromString(const CStdString &string, FRECT &rect);
private:
  bool GetNavigation(const TiXmlElement *node, const char *tag, DWORD &direction, std::vector<CStdString> &actions);
  bool GetCondition(const TiXmlNode *control, const char *tag, int &condition);
  static bool GetConditionalVisibility(const TiXmlNode* control, int &condition, CGUIInfoBool &allowHiddenFocus);
  bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strString);
  bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
  bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
  bool GetHitRect(const TiXmlNode* pRootNode, CRect &rect);
};
#endif
