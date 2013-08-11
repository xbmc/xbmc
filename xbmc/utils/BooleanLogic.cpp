/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "BooleanLogic.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

bool CBooleanLogicValue::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (node->FirstChild() != NULL && node->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
    m_value = node->FirstChild()->ValueStr();

  m_negated = false;
  const char *strNegated = elem->Attribute("negated");
  if (strNegated != NULL)
  {
    if (StringUtils::EqualsNoCase(strNegated, "true"))
      m_negated = true;
    else if (!StringUtils::EqualsNoCase(strNegated, "false"))
    {
      CLog::Log(LOGDEBUG, "CBooleanLogicValue: invalid negated value \"%s\"", strNegated);
      return false;
    }
  }

  return true;
}

CBooleanLogicOperation::~CBooleanLogicOperation()
{
  m_operations.clear();
  m_values.clear();
}

bool CBooleanLogicOperation::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  // check if this is a simple operation with a single value directly expressed
  // in the parent tag
  if (node->FirstChild() == NULL || node->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
  {
    CBooleanLogicValuePtr value = CBooleanLogicValuePtr(newValue());
    if (value == NULL || !value->Deserialize(node))
    {
      CLog::Log(LOGDEBUG, "CBooleanLogicOperation: failed to deserialize implicit boolean value definition");
      return false;
    }

    m_values.push_back(value);
    return true;
  }

  const TiXmlNode *operationNode = node->FirstChild();
  while (operationNode != NULL)
  {
    std::string tag = operationNode->ValueStr();
    if (StringUtils::EqualsNoCase(tag, "and") || StringUtils::EqualsNoCase(tag, "or"))
    {
      CBooleanLogicOperationPtr operation = CBooleanLogicOperationPtr(newOperation());
      if (operation == NULL)
        return false;

      operation->SetOperation(StringUtils::EqualsNoCase(tag, "and") ? BooleanLogicOperationAnd : BooleanLogicOperationOr);
      if (!operation->Deserialize(operationNode))
      {
        CLog::Log(LOGDEBUG, "CBooleanLogicOperation: failed to deserialize <%s> definition", tag.c_str());
        return false;
      }

      m_operations.push_back(operation);
    }
    else
    {
      CBooleanLogicValuePtr value = CBooleanLogicValuePtr(newValue());
      if (value == NULL)
        return false;

      if (StringUtils::EqualsNoCase(tag, value->GetTag()))
      {
        if (!value->Deserialize(operationNode))
        {
          CLog::Log(LOGDEBUG, "CBooleanLogicOperation: failed to deserialize <%s> definition", tag.c_str());
          return false;
        }

        m_values.push_back(value);
      }
      else if (operationNode->Type() == TiXmlNode::TINYXML_ELEMENT)
        CLog::Log(LOGDEBUG, "CBooleanLogicOperation: unknown <%s> definition encountered", tag.c_str());
    }

    operationNode = operationNode->NextSibling();
  }

  return true;
}

bool CBooleanLogic::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  if (m_operation == NULL)
  {
    m_operation = CBooleanLogicOperationPtr(new CBooleanLogicOperation());

    if (m_operation == NULL)
      return false;
  }

  return m_operation->Deserialize(node);
}
