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
  void Parse(const std::string &expression);
  bool Evaluate(const CGUIListItem *item, bool &result);
  short GetOperator(const char ch) const;

  std::vector<short> m_postfix;         ///< the postfix form of the expression (operators and operand indicies)
  std::vector<InfoPtr> m_operands;      ///< the operands in the expression
};

};
