#pragma once
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

#include <string>
#include <vector>

#include <memory>

#include "utils/IXmlDeserializable.h"

typedef enum {
  BooleanLogicOperationOr = 0,
  BooleanLogicOperationAnd
} BooleanLogicOperation;

class CBooleanLogicValue : public IXmlDeserializable
{
public:
  CBooleanLogicValue(const std::string &value = "", bool negated = false)
    : m_value(value), m_negated(negated)
  { }
  virtual ~CBooleanLogicValue() { }

  virtual bool Deserialize(const TiXmlNode *node);

  virtual const std::string& GetValue() const { return m_value; }
  virtual bool IsNegated() const { return m_negated; }
  virtual const char* GetTag() const { return "value"; }

  virtual void SetValue(const std::string &value) { m_value = value; }
  virtual void SetNegated(bool negated) { m_negated = negated; }

protected:
  std::string m_value;
  bool m_negated;
};

typedef std::shared_ptr<CBooleanLogicValue> CBooleanLogicValuePtr;
typedef std::vector<CBooleanLogicValuePtr> CBooleanLogicValues;

class CBooleanLogicOperation;
typedef std::shared_ptr<CBooleanLogicOperation> CBooleanLogicOperationPtr;
typedef std::vector<CBooleanLogicOperationPtr> CBooleanLogicOperations;

class CBooleanLogicOperation : public IXmlDeserializable
{
public:
  CBooleanLogicOperation(BooleanLogicOperation op = BooleanLogicOperationAnd)
    : m_operation(op)
  { }
  virtual ~CBooleanLogicOperation();

  virtual bool Deserialize(const TiXmlNode *node);

  virtual BooleanLogicOperation GetOperation() const { return m_operation; }
  virtual const CBooleanLogicOperations& GetOperations() const { return m_operations; }
  virtual const CBooleanLogicValues& GetValues() const { return m_values; }

  virtual void SetOperation(BooleanLogicOperation op) { m_operation = op; }

protected:
  virtual CBooleanLogicOperation* newOperation() { return new CBooleanLogicOperation(); }
  virtual CBooleanLogicValue* newValue() { return new CBooleanLogicValue(); }
  
  BooleanLogicOperation m_operation;
  CBooleanLogicOperations m_operations;
  CBooleanLogicValues m_values;
};

class CBooleanLogic : public IXmlDeserializable
{
public:
  CBooleanLogic() { }
  virtual ~CBooleanLogic() { }

  virtual bool Deserialize(const TiXmlNode *node);

  virtual const CBooleanLogicOperationPtr& Get() const { return m_operation; }
  virtual CBooleanLogicOperationPtr Get() { return m_operation; }

protected:
  CBooleanLogicOperationPtr m_operation;
};
