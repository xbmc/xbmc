#ifndef CGUIControlFactory_H
#define CGUIControlFactory_H
#pragma once
#include "GUICOntrol.h"
#include "tinyxml/tinyxml.h"

#include <string>
using namespace std;
class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CGUIControl* Create(DWORD dwParentId,const TiXmlNode* pControlNode, CGUIControl* pReference);
private:
	bool GetDWORD(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwDWORDValue);
	bool GetHex(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwHexValue);
	bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
	bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
	bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
	bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
	bool GetAlignment(const TiXmlNode* pRootNode, const char* strTag, DWORD& dwAlignment);
};
#endif