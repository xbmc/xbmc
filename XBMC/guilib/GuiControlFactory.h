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
  CGUIControl* Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference, RESOLUTION res);
private:
	bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue);
	bool GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue);
	bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
	bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
	bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
	bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
  bool GetMultipleString(const TiXmlNode* pRootNode, const char* strTag, CStdStringArray& vecStringValue);
	bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringPath);
	bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
	bool GetAlignmentY(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
	bool GetFloatRange(const TiXmlNode* pRootNode, const char* strTag, float& iMinValue, float& iMaxValue, float& iIntervalValue);
	bool GetIntRange(const TiXmlNode* pRootNode, const char* strTag, int& iMinValue, int& iMaxValue, int& iIntervalValue);
};
#endif
