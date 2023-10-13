/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BooleanLogic.h"

#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <memory>

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
      CLog::Log(LOGDEBUG, "CBooleanLogicValue: invalid negated value \"{}\"", strNegated);
      return false;
    }
  }

  return true;
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
        CLog::Log(LOGDEBUG, "CBooleanLogicOperation: failed to deserialize <{}> definition", tag);
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
          CLog::Log(LOGDEBUG, "CBooleanLogicOperation: failed to deserialize <{}> definition", tag);
          return false;
        }

        m_values.push_back(value);
      }
      else if (operationNode->Type() == TiXmlNode::TINYXML_ELEMENT)
        CLog::Log(LOGDEBUG, "CBooleanLogicOperation: unknown <{}> definition encountered", tag);
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
    m_operation = std::make_shared<CBooleanLogicOperation>();

    if (m_operation == NULL)
      return false;
  }

  return m_operation->Deserialize(node);
}
