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

#pragma once

#include <vector>
#include <list>
#include <stack>
#include "InfoBool.h"

class CGUIListItem;

namespace INFO
{
/*! \brief Class to wrap active boolean conditions
 */
class InfoSingle : public InfoBool
{
public:
  InfoSingle(const std::string &condition, int context);
  virtual ~InfoSingle() {};

  virtual void Update(const CGUIListItem *item);
private:
  int m_condition;             ///< actual condition this represents
};

/*! \brief Class to wrap active boolean expressions
 */
class InfoExpression : public InfoBool
{
public:
  InfoExpression(const std::string &expression, int context);
  virtual ~InfoExpression() {};

  virtual void Update(const CGUIListItem *item);
private:
  typedef enum
  {
    OPERATOR_NONE  = 0,
    OPERATOR_LB,  // 1
    OPERATOR_RB,  // 2
    OPERATOR_OR,  // 3
    OPERATOR_AND, // 4
    OPERATOR_NOT, // 5
  } operator_t;

  typedef enum
  {
    NODE_LEAF,
    NODE_AND,
    NODE_OR,
  } node_type_t;

  // An abstract base class for nodes in the expression tree
  class InfoSubexpression
  {
  public:
    virtual ~InfoSubexpression(void) {}; // so we can destruct derived classes using a pointer to their base class
    virtual bool Evaluate(const CGUIListItem *item) = 0;
    virtual node_type_t Type() const=0;
  };

  typedef std::shared_ptr<InfoSubexpression> InfoSubexpressionPtr;

  // A leaf node in the expression tree
  class InfoLeaf : public InfoSubexpression
  {
  public:
    InfoLeaf(InfoPtr info, bool invert) : m_info(info), m_invert(invert) {};
    virtual bool Evaluate(const CGUIListItem *item);
    virtual node_type_t Type() const { return NODE_LEAF; };
  private:
    InfoPtr m_info;
    bool m_invert;
  };

  // A branch node in the expression tree
  class InfoAssociativeGroup : public InfoSubexpression
  {
  public:
    InfoAssociativeGroup(node_type_t type, const InfoSubexpressionPtr &left, const InfoSubexpressionPtr &right);
    void AddChild(const InfoSubexpressionPtr &child);
    void Merge(std::shared_ptr<InfoAssociativeGroup> other);
    virtual bool Evaluate(const CGUIListItem *item);
    virtual node_type_t Type() const { return m_type; };
  private:
    node_type_t m_type;
    std::list<InfoSubexpressionPtr> m_children;
  };

  static operator_t GetOperator(char ch);
  static void OperatorPop(std::stack<operator_t> &operator_stack, bool &invert, std::stack<InfoSubexpressionPtr> &nodes);
  bool Parse(const std::string &expression);
  InfoSubexpressionPtr m_expression_tree;
};

};
