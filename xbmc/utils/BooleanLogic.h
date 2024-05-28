/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IXmlDeserializable.h"

#include <memory>
#include <string>
#include <vector>

typedef enum {
  BooleanLogicOperationOr = 0,
  BooleanLogicOperationAnd
} BooleanLogicOperation;

class TiXmlNode;

class CBooleanLogicValue : public IXmlDeserializable
{
public:
  CBooleanLogicValue(const std::string &value = "", bool negated = false)
    : m_value(value), m_negated(negated)
  { }
  ~CBooleanLogicValue() override = default;

  bool Deserialize(const TiXmlNode *node) override;

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
  explicit CBooleanLogicOperation(BooleanLogicOperation op = BooleanLogicOperationAnd)
    : m_operation(op)
  { }
  ~CBooleanLogicOperation() override = default;

  bool Deserialize(const TiXmlNode *node) override;

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
protected:
  /* make sure nobody deletes a pointer to this class */
  ~CBooleanLogic() override = default;

public:
  bool Deserialize(const TiXmlNode *node) override;

  const CBooleanLogicOperationPtr& Get() const { return m_operation; }
  CBooleanLogicOperationPtr Get() { return m_operation; }

protected:
  CBooleanLogicOperationPtr m_operation;
};
