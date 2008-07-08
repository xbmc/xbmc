#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIBaseContainer.h"
#include "GUIEPGGridItem.h"
#include "TVDatabase.h"

class CGUIEPGGridContainer : public CGUIBaseContainer
{
public:
  CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
  virtual ~CGUIEPGGridContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  const int GetNumChannels()   { return m_numChannels;   }
  
 /* virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;*/
  
protected:
  //virtual void Scroll(int amount);
  void SetCursor(int cursor);
  bool MoveLeft(bool wrapAround);
  bool MoveRight(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool MoveDown(bool wrapAround);
  //virtual void ValidateOffset();
  //virtual void SelectItem(int item);
  //void CalculateLayout();

private:
  int m_numChannels;
  std::vector<CGUIEPGGridItem> m_epgItems;
};
