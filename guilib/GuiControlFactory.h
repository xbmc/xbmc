#ifndef CGUIControlFactory_H
#define CGUIControlFactory_H
#pragma once

#include "GUICOntrol.h"
#include "tinyxml/tinyxml.h"

class CGUIControlFactory
{
public:
  CGUIControlFactory(void);
  virtual ~CGUIControlFactory(void);
  CGUIControl* Create(DWORD dwParentId,const TiXmlNode* pControlNode);
};
#endif