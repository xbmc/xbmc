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
#include <utility>
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

  /*!
   \brief Clear all include components (defaults, constants, variables, expressions and includes)
  */
  void Clear();

  /*!
   \brief Load all include components (defaults, constants, variables, expressions and includes)
    from the given \code{file}.
   
   \param file the file to load
   \return true if the file was loaded otherwise false
   */
  bool Load(const std::string &file);

  /*!
   \brief Resolve all include components (defaults, constants, variables, expressions and includes)
   for the given \code{node}. Place the conditions specified for <include> elements in \code{includeConditions}.

   \param node the node from where we start to resolve the include components
   \param includeConditions a map that holds the conditions for resolved includes
   */
  void Resolve(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* includeConditions = NULL);

  /*!
   \brief Create a skin variable for the given \code{name} within the given \code{context}.

   \param name the name of the skin variable
   \param context the context where the variable is created in
   \return skin variable
   */
  const INFO::CSkinVariableString* CreateSkinVariable(const std::string& name, int context);

private:
  enum ResolveParamsResult
  {
    NO_PARAMS_FOUND,
    PARAMS_RESOLVED,
    SINGLE_UNDEFINED_PARAM_RESOLVED
  };

  bool HasLoaded(const std::string &file) const;

  void LoadDefaults(const TiXmlElement *node);
  void LoadIncludes(const TiXmlElement *node);
  void LoadVariables(const TiXmlElement *node);
  void LoadConstants(const TiXmlElement *node);
  void LoadExpressions(const TiXmlElement *node);

  void SetDefaults(TiXmlElement *node);
  void ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = NULL);
  void ResolveConstants(TiXmlElement *node);
  void ResolveExpressions(TiXmlElement *node);

  typedef std::map<std::string, std::string> Params;
  static bool GetParameters(const TiXmlElement *include, const char *valueAttribute, Params& params);
  static void ResolveParametersForNode(TiXmlElement *node, const Params& params);
  static ResolveParamsResult ResolveParameters(const std::string& strInput, std::string& strOutput, const Params& params);

  std::string ResolveConstant(const std::string &constant) const;
  std::string ResolveExpressions(const std::string &expression) const;

  std::vector<std::string> m_files;
  std::map<std::string, std::pair<TiXmlElement, Params>> m_includes;
  std::map<std::string, TiXmlElement> m_defaults;
  std::map<std::string, TiXmlElement> m_skinvariables;
  std::map<std::string, std::string> m_constants;
  std::map<std::string, std::string> m_expressions;

  std::set<std::string> m_constantAttributes;
  std::set<std::string> m_constantNodes;

  std::set<std::string> m_expressionAttributes;
  std::set<std::string> m_expressionNodes;
};
