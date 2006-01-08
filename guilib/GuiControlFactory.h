/*!
\file GuiControlFactory.h
\brief 
*/

#ifndef CGUIControlFactory_H
#define CGUIControlFactory_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CGUIControl* Create(DWORD dwParentId, const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res);
//PRE_SKIN_VERSION_2_0_COMPATIBILITY
  bool GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus, bool &startHidden);
//  bool GetConditionalVisibility(const TiXmlNode* control, int &condition, bool &allowHiddenFocus);
  bool GetConditionalVisibility(const TiXmlNode* control, int &condition);
  bool GetAnimations(const TiXmlNode *control, vector<CAnimation> &animation, RESOLUTION res);
private:
  bool GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, CStdStringArray& vecStringValue);
  bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath);
  bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
  bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
  bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
};
#endif
