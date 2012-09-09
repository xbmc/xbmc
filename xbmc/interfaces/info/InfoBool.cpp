/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "InfoBool.h"
#include <stack>
#include "utils/log.h"
#include "GUIInfoManager.h"

using namespace std;
using namespace INFO;

InfoSingle::InfoSingle(const CStdString &expression, int context)
: InfoBool(expression, context)
{
  m_condition = g_infoManager.TranslateSingleString(expression);
}

void InfoSingle::Update(const CGUIListItem *item)
{
  m_value = g_infoManager.GetBool(m_condition, m_context, item);
}

InfoExpression::InfoExpression(const CStdString &expression, int context)
: InfoBool(expression, context)
{
  Parse(expression);
}

void InfoExpression::Update(const CGUIListItem *item)
{
  Evaluate(item, m_value);
}

#define OPERATOR_LB   5
#define OPERATOR_RB   4
#define OPERATOR_NOT  3
#define OPERATOR_AND  2
#define OPERATOR_OR   1

short InfoExpression::GetOperator(const char ch) const
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
    return 0;
}

void InfoExpression::Parse(const CStdString &expression)
{
  stack<char> operators;
  CStdString operand;
  for (unsigned int i = 0; i < expression.size(); i++)
  {
    if (GetOperator(expression[i]))
    {
      // cleanup any operand, translate and put into our expression list
      if (!operand.IsEmpty())
      {
        unsigned int info = g_infoManager.Register(operand, m_context);
        if (info)
        {
          m_postfix.push_back(m_operands.size());
          m_operands.push_back(info);
        }
        operand.clear();
      }
      // handle closing parenthesis
      if (expression[i] == ']')
      {
        while (operators.size())
        {
          char oper = operators.top();
          operators.pop();

          if (oper == '[')
            break;

          m_postfix.push_back(-GetOperator(oper)); // negative denotes operator
        }
      }
      else
      {
        // all other operators we pop off the stack any operator
        // that has a higher priority than the one we have.
        while (!operators.empty() && GetOperator(operators.top()) > GetOperator(expression[i]))
        {
          // only handle parenthesis once they're closed.
          if (operators.top() == '[' && expression[i] != ']')
            break;

          m_postfix.push_back(-GetOperator(operators.top()));  // negative denotes operator
          operators.pop();
        }
        operators.push(expression[i]);
      }
    }
    else
    {
      operand += expression[i];
    }
  }

  if (!operand.empty())
  {
    unsigned int info = g_infoManager.Register(operand, m_context);
    if (info)
    {
      m_postfix.push_back(m_operands.size());
      m_operands.push_back(info);
    }
  }

  // finish up by adding any operators
  while (!operators.empty())
  {
    m_postfix.push_back(-GetOperator(operators.top()));  // negative denotes operator
    operators.pop();
  }

  // test evaluate
  bool test;
  if (!Evaluate(NULL, test))
    CLog::Log(LOGERROR, "Error evaluating boolean expression %s", expression.c_str());
}

bool InfoExpression::Evaluate(const CGUIListItem *item, bool &result)
{
  stack<bool> save;
  for (vector<short>::const_iterator it = m_postfix.begin(); it != m_postfix.end(); ++it)
  {
    short expr = *it;
    if (expr == -OPERATOR_NOT)
    { // NOT the top item on the stack
      if (save.size() < 1) return false;
      bool expr = save.top();
      save.pop();
      save.push(!expr);
    }
    else if (expr == -OPERATOR_AND)
    { // AND the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left && right);
    }
    else if (expr == -OPERATOR_OR)
    { // OR the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left || right);
    }
    else  // operand
      save.push(g_infoManager.GetBoolValue(m_operands[expr], item));
  }
  if (save.size() != 1)
    return false;
  result = save.top();
  return true;
}

