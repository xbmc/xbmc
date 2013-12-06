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
#include <map>
#include "utils/StdString.h"
#include "boost/shared_ptr.hpp"

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
    : m_listItemDependent(false),
      m_value(false),
      m_context(context),
      m_expression(expression),
      m_dirty(true)
  {
  };

  virtual ~InfoBool() {};

  /*! \brief Mark this info bool as dirty
   */
  void SetDirty(void)
  {
    m_dirty = true;
  }

  /*! \brief Get the value of this info bool
   This is called to update (if dirty) and fetch the value of the info bool
   \param item the item used to evaluate the bool
   */
  inline bool Get(const CGUIListItem *item = NULL)
  {
    if (item && m_listItemDependent)
      Update(item);
    else if (m_dirty)
    {
      Update(NULL);
      m_dirty = false;
    }
    return m_value;
  }

  bool operator==(const InfoBool &right) const;

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(const CGUIListItem *item) {};

  const std::string &GetExpression() const { return m_expression; }

  bool m_listItemDependent;    ///< do not cache if a listitem pointer is given

protected:

  bool m_value;                ///< current value
  int m_context;               ///< contextual information to go with the condition

private:
  CStdString m_expression;     ///< original expression
  bool         m_dirty;        ///< whether we need an update
};

typedef boost::shared_ptr<InfoBool> InfoPtr;

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
  std::vector<InfoPtr> m_operands;      ///< the operands in the expression
};

};
