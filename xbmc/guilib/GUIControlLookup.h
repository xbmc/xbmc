/*!
\file GUIControlLookup.h
\brief
*/

#pragma once

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

#include "GUIControl.h"

class CGUIControlLookup : public CGUIControl
{
public:
  CGUIControlLookup()
    : CGUIControl() {};
  CGUIControlLookup(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height) {};

  virtual CGUIControl *GetControl(int id, std::vector<CGUIControl*> *idCollector = nullptr);
protected:
  typedef std::multimap<int, CGUIControl *> LookupMap;

  /*!
  \brief Check whether a given control is valid
  Runs through controls and returns whether this control is valid.  Only functional
  for controls with non-zero id.
  \param control to check
  \return true if the control is valid, false otherwise.
  */
  bool IsValidControl(const CGUIControl *control) const;
  std::pair<LookupMap::const_iterator, LookupMap::const_iterator> GetLookupControls(int controlId) const
  {
    return m_lookup.equal_range(controlId);
  };

  // fast lookup by id
  void AddLookup(CGUIControl *control);
  void RemoveLookup(CGUIControl *control);
  const LookupMap &GetLookup() const { return m_lookup; };
  void ClearLookup() { m_lookup.clear(); };
private:
  LookupMap m_lookup;
};
