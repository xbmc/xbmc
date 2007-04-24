#pragma once

// forward definitions
class TiXmlElement;

class CGUIIncludes
{
public:
  CGUIIncludes();
  ~CGUIIncludes();

  void ClearIncludes();
  bool LoadIncludes(const CStdString &includeFile);
  void ResolveIncludes(TiXmlElement *node, const CStdString &type);
  bool LoadIncludesFromXML(const TiXmlElement *root);

private:
  bool HasIncludeFile(const CStdString &includeFile) const;
  map<CStdString, TiXmlElement> m_includes;
  map<CStdString, TiXmlElement> m_defaults;
  vector<CStdString> m_files;
  typedef vector<CStdString>::const_iterator iFiles;
};