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

#include <string>
#include <memory>

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
  InfoBool(const std::string &expression, int context);
  virtual ~InfoBool() {};

  /*! \brief Set the info bool dirty.
   Will cause the info bool to be re-evaluated next call to Get()
   */
  void SetDirty()
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

  bool operator==(const InfoBool &right) const
  {
    return (m_context == right.m_context &&
            m_expression == right.m_expression);
  }

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(const CGUIListItem *item) {};

  const std::string &GetExpression() const { return m_expression; }
  bool ListItemDependent() const { return m_listItemDependent; }
protected:

  bool m_value;                ///< current value
  int m_context;               ///< contextual information to go with the condition
  bool m_listItemDependent;    ///< do not cache if a listitem pointer is given

private:
  std::string  m_expression;   ///< original expression
  bool         m_dirty;        ///< whether we need an update
};

typedef std::shared_ptr<InfoBool> InfoPtr;
};
