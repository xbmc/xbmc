#pragma once
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

#include "GUIRenderingControl.h"
#include "addons/IAddon.h"

class CGUIVisualisationControl : public CGUIRenderingControl
{
public:
  CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIVisualisationControl(const CGUIVisualisationControl &from);
  virtual CGUIVisualisationControl *Clone() const { return new CGUIVisualisationControl(*this); }; //! @todo check for naughties
  virtual void FreeResources(bool immediately = false);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
private:
  bool m_bAttemptedLoad;
  ADDON::VizPtr m_addon;
};
