/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoExpression.h"

#include "GUIInfoManager.h"
#include "utils/log.h"

#include <list>
#include <memory>
#include <stack>

using namespace INFO;

void InfoSingle::Initialize(CGUIInfoManager* infoMgr)
{
  InfoBool::Initialize(infoMgr);
  m_condition = m_infoMgr->TranslateSingleString(m_expression, m_listItemDependent);
}

void InfoSingle::Update(int contextWindow, const CGUIListItem* item)
{
  // use propagated context in case this info has the default context (i.e. if not tied to a specific window)
  // its value might depend on the context in which the evaluation was called
  int context = m_context == DEFAULT_CONTEXT ? contextWindow : m_context;
  m_value = m_infoMgr->GetBool(m_condition, context, item);
}

void InfoExpression::Initialize(CGUIInfoManager* infoMgr)
{
  InfoBool::Initialize(infoMgr);
  if (!Parse(m_expression))
  {
    CLog::Log(LOGERROR, "Error parsing boolean expression {}", m_expression);
    m_expression_tree = std::make_shared<InfoLeaf>(m_infoMgr->Register("false", 0), false);
  }
}

void InfoExpression::Update(int contextWindow, const CGUIListItem* item)
{
  // use propagated context in case this info expression has the default context (i.e. if not tied to a specific window)
  // its value might depend on the context in which the evaluation was called
  int context = m_context == DEFAULT_CONTEXT ? contextWindow : m_context;
  m_value = m_expression_tree->Evaluate(context, item);
}

/* Expressions are rewritten at parse time into a form which favours the
 * formation of groups of associative nodes. These groups are then reordered at
 * evaluation time such that nodes whose value renders the evaluation of the
 * remainder of the group unnecessary tend to be evaluated first (these are
 * true nodes for OR subexpressions, or false nodes for AND subexpressions).
 * The end effect is to minimise the number of leaf nodes that need to be
 * evaluated in order to determine the value of the expression. The runtime
 * adaptability has the advantage of not being customised for any particular skin.
 *
 * The modifications to the expression at parse time fall into two groups:
 * 1) Moving logical NOTs so that they are only applied to leaf nodes.
 *    For example, rewriting ![A+B]|C as !A|!B|C allows reordering such that
 *    any of the three leaves can be evaluated first.
 * 2) Combining adjacent AND or OR operations such that each path from the root
 *    to a leaf encounters a strictly alternating pattern of AND and OR
 *    operations. So [A|B]|[C|D+[[E|F]|G] becomes A|B|C|[D+[E|F|G]].
 */

bool InfoExpression::InfoLeaf::Evaluate(int contextWindow, const CGUIListItem* item)
{
  return m_invert ^ m_info->Get(contextWindow, item);
}

InfoExpression::InfoAssociativeGroup::InfoAssociativeGroup(
    node_type_t type,
    const InfoSubexpressionPtr &left,
    const InfoSubexpressionPtr &right)
    : m_type(type)
{
  AddChild(right);
  AddChild(left);
}

void InfoExpression::InfoAssociativeGroup::AddChild(const InfoSubexpressionPtr &child)
{
  m_children.push_front(child); // largely undoes the effect of parsing right-associative
}

void InfoExpression::InfoAssociativeGroup::Merge(const std::shared_ptr<InfoAssociativeGroup>& other)
{
  m_children.splice(m_children.end(), other->m_children);
}

bool InfoExpression::InfoAssociativeGroup::Evaluate(int contextWindow, const CGUIListItem* item)
{
  /* Handle either AND or OR by using the relation
   * A AND B == !(!A OR !B)
   * to convert ANDs into ORs
   */
  std::list<InfoSubexpressionPtr>::iterator last = m_children.end();
  std::list<InfoSubexpressionPtr>::iterator it = m_children.begin();
  bool use_and = (m_type == NODE_AND);
  bool result = use_and ^ (*it)->Evaluate(contextWindow, item);
  while (!result && ++it != last)
  {
    result = use_and ^ (*it)->Evaluate(contextWindow, item);
    if (result)
    {
      /* Move this child to the head of the list so we evaluate faster next time */
      m_children.push_front(*it);
      m_children.erase(it);
    }
  }
  return use_and ^ result;
}

/* Expressions are parsed using the shunting-yard algorithm. Binary operators
 * (AND/OR) are treated as right-associative so that we don't need to make a
 * special case for the unary NOT operator. This has no effect upon the answers
 * generated, though the initial sequence of evaluation of leaves may be
 * different from what you might expect.
 */

InfoExpression::operator_t InfoExpression::GetOperator(char ch)
{
  if (ch == '[')
    return OPERATOR_LB;
  else if (ch == ']')
    return OPERATOR_RB;
  else if (ch == '!')
    return OPERATOR_NOT;
  else if (ch == '+')
    return OPERATOR_AND;
  else if (ch == '|')
    return OPERATOR_OR;
  else
    return OPERATOR_NONE;
}

void InfoExpression::OperatorPop(std::stack<operator_t> &operator_stack, bool &invert, std::stack<InfoSubexpressionPtr> &nodes)
{
  operator_t op2 = operator_stack.top();
  operator_stack.pop();
  if (op2 == OPERATOR_NOT)
  {
    invert = !invert;
  }
  else
  {
    // At this point, it can only be OPERATOR_AND or OPERATOR_OR
    if (invert)
      op2 = (operator_t) (OPERATOR_AND ^ OPERATOR_OR ^ op2);
    node_type_t new_type = op2 == OPERATOR_AND ? NODE_AND : NODE_OR;

    InfoSubexpressionPtr right = nodes.top();
    nodes.pop();
    InfoSubexpressionPtr left = nodes.top();

    node_type_t right_type = right->Type();
    node_type_t left_type = left->Type();

    // Combine associative operations into the same node where possible
    if (left_type == new_type && right_type == new_type)
      /* For example:        AND
       *                   /     \                ____ AND ____
       *                AND       AND     ->     /    /   \    \
       *               /   \     /   \         leaf leaf leaf leaf
       *             leaf leaf leaf leaf
       */
      std::static_pointer_cast<InfoAssociativeGroup>(left)->Merge(std::static_pointer_cast<InfoAssociativeGroup>(right));
    else if (left_type == new_type)
      /* For example:        AND                    AND
       *                   /     \                /  |  \
       *                AND       OR      ->   leaf leaf OR
       *               /   \     /   \                  /   \
       *             leaf leaf leaf leaf              leaf leaf
       */
      std::static_pointer_cast<InfoAssociativeGroup>(left)->AddChild(right);
    else
    {
      nodes.pop();
      if (right_type == new_type)
      {
        /* For example:        AND                       AND
         *                   /     \                   /  |  \
         *                OR        AND     ->      OR  leaf leaf
         *               /   \     /   \           /   \
         *             leaf leaf leaf leaf       leaf leaf
         */
        std::static_pointer_cast<InfoAssociativeGroup>(right)->AddChild(left);
        nodes.push(right);
      }
      else
        /* For example:        AND              which can't be simplified, and
         *                   /     \            requires a new AND node to be
         *                OR        OR          created with the two OR nodes
         *               /   \     /   \        as children
         *             leaf leaf leaf leaf
         */
        nodes.push(std::make_shared<InfoAssociativeGroup>(new_type, left, right));
    }
  }
}

bool InfoExpression::Parse(const std::string &expression)
{
  const char *s = expression.c_str();
  std::string operand;
  std::stack<operator_t> operator_stack;
  bool invert = false;
  std::stack<InfoSubexpressionPtr> nodes;
  // The next two are for syntax-checking purposes
  bool after_binaryoperator = true;
  int bracket_count = 0;
  char c;
  // Skip leading whitespace - don't want it to count as an operand if that's all there is
  while (isspace((unsigned char)(c=*s)))
    s++;

  while ((c = *s++) != '\0')
  {
    operator_t op;
    if ((op = GetOperator(c)) != OPERATOR_NONE)
    {
      // Character is an operator
      if ((!after_binaryoperator && (c == '!' || c == '[')) ||
          (after_binaryoperator && (c == ']' || c == '+' || c == '|')))
      {
        CLog::Log(LOGERROR, "Misplaced {}", c);
        return false;
      }
      if (c == '[')
        bracket_count++;
      else if (c == ']' && bracket_count-- == 0)
      {
        CLog::Log(LOGERROR, "Unmatched ]");
        return false;
      }
      if (!operand.empty())
      {
        InfoPtr info = m_infoMgr->Register(operand, m_context);
        if (!info)
        {
          CLog::Log(LOGERROR, "Bad operand '{}'", operand);
          return false;
        }
        /* Propagate any listItem dependency from the operand to the expression */
        m_listItemDependent |= info->ListItemDependent();
        nodes.push(std::make_shared<InfoLeaf>(info, invert));
        /* Reuse operand string for next operand */
        operand.clear();
      }

      // Handle any higher-priority stacked operators, except when the new operator is left-bracket.
      // For a right-bracket, this will stop with the matching left-bracket at the top of the operator stack.
      if (op != OPERATOR_LB)
      {
        while (!operator_stack.empty() && operator_stack.top() > op)
          OperatorPop(operator_stack, invert, nodes);
      }
      if (op == OPERATOR_RB)
        operator_stack.pop(); // remove the matching left-bracket
      else
        operator_stack.push(op);
      if (op == OPERATOR_NOT)
        invert = !invert;

      if (c == '+' || c == '|')
        after_binaryoperator = true;
      // Skip trailing whitespace - don't want it to count as an operand if that's all there is
      while (isspace((unsigned char)(c=*s))) s++;
    }
    else
    {
      // Character is part of operand
      operand += c;
      after_binaryoperator = false;
    }
  }
  if (bracket_count > 0)
  {
    CLog::Log(LOGERROR, "Unmatched [");
    return false;
  }
  if (after_binaryoperator)
  {
    CLog::Log(LOGERROR, "Missing operand");
    return false;
  }
  if (!operand.empty())
  {
    InfoPtr info = m_infoMgr->Register(operand, m_context);
    if (!info)
    {
      CLog::Log(LOGERROR, "Bad operand '{}'", operand);
      return false;
    }
    /* Propagate any listItem dependency from the operand to the expression */
    m_listItemDependent |= info->ListItemDependent();
    nodes.push(std::make_shared<InfoLeaf>(info, invert));
  }
  while (!operator_stack.empty())
    OperatorPop(operator_stack, invert, nodes);

  m_expression_tree = nodes.top();
  return true;
}
