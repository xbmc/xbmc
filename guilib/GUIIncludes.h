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
  bool ResolveConstant(const CStdString &constant, float &value);
  bool LoadIncludesFromXML(const TiXmlElement *root);

private:
  bool HasIncludeFile(const CStdString &includeFile) const;
  std::map<CStdString, TiXmlElement> m_includes;
  std::map<CStdString, TiXmlElement> m_defaults;
  std::map<CStdString, float> m_constants;
  std::vector<CStdString> m_files;
  typedef std::vector<CStdString>::const_iterator iFiles;
};

