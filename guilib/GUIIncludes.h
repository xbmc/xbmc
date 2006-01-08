#pragma once

#include "include.h"

class CGUIIncludes
{
public:
  CGUIIncludes();
  ~CGUIIncludes();

  bool LoadIncludes(const CStdString &includeFile);
  void ResolveIncludes(TiXmlElement *node);

private:
  map<CStdString, TiXmlElement> m_includes;
};