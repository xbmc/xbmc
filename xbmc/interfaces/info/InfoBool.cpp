/*
 *      Copyright (C) 2013 Team XBMC
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

#include "InfoBool.h"
#include "utils/StringUtils.h"

namespace INFO
{
  InfoBool::InfoBool(const std::string &expression, int context, unsigned int &refreshCounter)
    : m_value(false),
      m_context(context),
      m_listItemDependent(false),
      m_expression(expression),
      m_parentRefreshCounter(refreshCounter)
  {
    StringUtils::ToLower(m_expression);
  }
}
