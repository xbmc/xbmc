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

#pragma once

#include <vector>
#include <map>
#include "utils/StdString.h"

class CGUIListItem;

namespace INFO
{
/*!
 \ingroup info
 \brief Base class, wrapping boolean conditions and expressions
 */
class InfoBool
{
public:
  InfoBool(const CStdString &expression, int context)
    : m_value(false),
      m_context(context),
      m_expression(expression),
      m_lastUpdate(0)
  {
  };

  virtual ~InfoBool() {};

  /*! \brief Get the value of this info bool
   This is called to update (if necessary) and fetch the value of the info bool
   \param time current time (used to test if we need to update yet)
   \param item the item used to evaluate the bool
   */
  inline bool Get(unsigned int time, const CGUIListItem *item = NULL)
  {
    if (item)
      Update(item);
    else if (time - m_lastUpdate > 0)
    {
      Update(NULL);
      m_lastUpdate = time;
    }
    return m_value;
  }

  bool operator==(const InfoBool &right) const
  {
    return (m_context == right.m_context && 
            m_expression.CompareNoCase(right.m_expression) == 0);
  }

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(const CGUIListItem *item) {};

protected:

  bool m_value;                ///< current value
  int m_context;               ///< contextual information to go with the condition

private:
  CStdString m_expression;     ///< original expression
  unsigned int m_lastUpdate;   ///< last update time (to determine dirty status)
};

/*! \brief Class to wrap active boolean conditions
 */
class InfoSingle : public InfoBool
{
public:
  InfoSingle(const CStdString &condition, int context);
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
  InfoExpression(const CStdString &expression, int context);
  virtual ~InfoExpression() {};

  virtual void Update(const CGUIListItem *item);
private:
  void Parse(const CStdString &expression);
  bool Evaluate(const CGUIListItem *item, bool &result);
  short GetOperator(const char ch) const;

  std::vector<short> m_postfix;         ///< the postfix form of the expression (operators and operand indicies)
  std::vector<unsigned int> m_operands; ///< the operands in the expression
};

};
