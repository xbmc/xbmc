#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <map>
#include <set>
#include <string>
#include <vector>
#include "interfaces/info/InfoBool.h"

// forward definitions
class TiXmlElement;
namespace INFO
{
  class CSkinVariableString;
}

class CGUIIncludes
{
public:
  CGUIIncludes();
  ~CGUIIncludes();

  void ClearIncludes();
  bool LoadIncludes(const std::string &includeFile);
  bool LoadIncludesFromXML(const TiXmlElement *root);

  /*! \brief Resolve <include>name</include> tags recursively for the given XML element
   Replaces any instances of <include file="foo">bar</include> with the value of the include
   "bar" from the include file "foo".
   \param node an XML Element - all child elements are traversed.
   */
  void ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = NULL);
  const INFO::CSkinVariableString* CreateSkinVariable(const std::string& name, int context);

private:
  enum ResolveParamsResult
  {
    NO_PARAMS_FOUND,
    PARAMS_RESOLVED,
    SINGLE_UNDEFINED_PARAM_RESOLVED
  };

  void ResolveIncludesForNode(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = NULL);
  typedef std::map<std::string, std::string> Params;
  static bool GetParameters(const TiXmlElement *include, const char *valueAttribute, Params& params);
  static void ResolveParametersForNode(TiXmlElement *node, const Params& params);
  static ResolveParamsResult ResolveParameters(const std::string& strInput, std::string& strOutput, const Params& params);
  std::string ResolveConstant(const std::string &constant) const;
  bool HasIncludeFile(const std::string &includeFile) const;
  std::map<std::string, std::pair<TiXmlElement, Params>> m_includes;
  std::map<std::string, TiXmlElement> m_defaults;
  std::map<std::string, TiXmlElement> m_skinvariables;
  std::map<std::string, std::string> m_constants;
  std::vector<std::string> m_files;
  typedef std::vector<std::string>::const_iterator iFiles;

  std::set<std::string> m_constantAttributes;
  std::set<std::string> m_constantNodes;
};
