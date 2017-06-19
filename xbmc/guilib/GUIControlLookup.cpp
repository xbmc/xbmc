/*
*      Copyright (C) 2017 Team XBMC
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

#include "GUIControlLookup.h"

CGUIControl *CGUIControlLookup::GetControl(int iControl, std::vector<CGUIControl*> *idCollector)
{
  if (idCollector)
    idCollector->clear();

  CGUIControl* pPotential(nullptr);

  LookupMap::const_iterator first = m_lookup.find(iControl);
  if (first != m_lookup.end())
  {
    LookupMap::const_iterator last = m_lookup.upper_bound(iControl);
    for (LookupMap::const_iterator i = first; i != last; ++i)
    {
      CGUIControl *control = i->second;
      if (control->IsVisible())
        return control;
      else if (idCollector)
        idCollector->push_back(control);
      else if (!pPotential)
        pPotential = control;
    }
  }
  return pPotential;
}

bool CGUIControlLookup::IsValidControl(const CGUIControl *control) const
{
  if (control->GetID())
  {
    for (const auto &i : m_lookup)
    {
      if (control == i.second)
        return true;
    }
  }
  return false;
}

void CGUIControlLookup::AddLookup(CGUIControl *control)
{
  CGUIControlLookup *lookupControl(dynamic_cast<CGUIControlLookup*>(control));
  
  if (lookupControl)
  { // first add all the subitems of this group (if they exist)
    const LookupMap &map(lookupControl->GetLookup());
    for (const auto &i : map)
      m_lookup.insert(m_lookup.upper_bound(i.first), std::make_pair(i.first, i.second));
  }
  if (control->GetID())
    m_lookup.insert(m_lookup.upper_bound(control->GetID()), std::make_pair(control->GetID(), control));
  // ensure that our size is what it should be
  if (m_parentControl && (lookupControl = dynamic_cast<CGUIControlLookup*>(m_parentControl)))
    lookupControl->AddLookup(control);
}

void CGUIControlLookup::RemoveLookup(CGUIControl *control)
{
  CGUIControlLookup *lookupControl(dynamic_cast<CGUIControlLookup*>(control));
  if (lookupControl)
  { // remove the group's lookup
    const LookupMap &map(lookupControl->GetLookup());
    for (const auto &i : map)
    { // remove this control
      for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); ++it)
      {
        if (i.second == it->second)
        {
          m_lookup.erase(it);
          break;
        }
      }
    }
  }
  // remove the actual control
  if (control->GetID())
  {
    for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); ++it)
    {
      if (control == it->second)
      {
        m_lookup.erase(it);
        break;
      }
    }
  }
  if (m_parentControl && (lookupControl = dynamic_cast<CGUIControlLookup*>(m_parentControl)))
    lookupControl->RemoveLookup(control);
}
