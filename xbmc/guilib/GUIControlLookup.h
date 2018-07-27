/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"

class CGUIControlLookup : public CGUIControl
{
public:
  CGUIControlLookup() = default;
  CGUIControlLookup(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height) {}
  CGUIControlLookup(const CGUIControlLookup &from)
    : CGUIControl(from) {}
  ~CGUIControlLookup(void) override = default;

  CGUIControl *GetControl(int id, std::vector<CGUIControl*> *idCollector = nullptr) override;
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
  void RemoveLookup();
  const LookupMap &GetLookup() const { return m_lookup; }
  void ClearLookup() { m_lookup.clear(); }
private:
  LookupMap m_lookup;
};
