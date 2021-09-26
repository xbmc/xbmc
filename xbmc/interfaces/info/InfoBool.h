/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

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
  InfoBool(const std::string &expression, int context, unsigned int &refreshCounter);
  virtual ~InfoBool() = default;

  virtual void Initialize() {}

  /*! \brief Get the value of this info bool
   This is called to update (if dirty) and fetch the value of the info bool
   \param item the item used to evaluate the bool
   */
  inline bool Get(const CGUIListItem *item = NULL)
  {
    if (item && m_listItemDependent)
      Update(item);
    else if (m_refreshCounter != m_parentRefreshCounter || m_refreshCounter == 0)
    {
      Update(NULL);
      m_refreshCounter = m_parentRefreshCounter;
    }
    return m_value;
  }

  bool operator==(const InfoBool &right) const
  {
    return (m_context == right.m_context &&
            m_expression == right.m_expression);
  }

  bool operator<(const InfoBool &right) const
  {
    if (m_context < right.m_context)
      return true;
    else if (m_context == right.m_context)
      return m_expression < right.m_expression;
    else
      return false;
  }

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(const CGUIListItem* item) {}

  const std::string &GetExpression() const { return m_expression; }
  bool ListItemDependent() const { return m_listItemDependent; }
protected:

  bool m_value;                ///< current value
  int m_context;               ///< contextual information to go with the condition
  bool m_listItemDependent;    ///< do not cache if a listitem pointer is given
  std::string  m_expression;   ///< original expression

private:
  unsigned int m_refreshCounter;
  unsigned int &m_parentRefreshCounter;
};

typedef std::shared_ptr<InfoBool> InfoPtr;
};
