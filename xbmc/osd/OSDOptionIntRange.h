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
#include "IOSDOption.h"
#include "IExecutor.h"
#include "GUISliderControl.h"
namespace OSD
{
class COSDOptionIntRange :
      public IOSDOption
{
public:
  COSDOptionIntRange(int iAction, int iHeading, bool bPercent);
  COSDOptionIntRange(int iAction, int iHeading, bool bPercent, int iStart, int iEnd, int iInterval, int iValue);
  COSDOptionIntRange(const COSDOptionIntRange& option);
  const OSD::COSDOptionIntRange& operator = (const COSDOptionIntRange& option);


  virtual ~COSDOptionIntRange(void);
  virtual IOSDOption* Clone() const;
  virtual void Draw(int x, int y, bool bFocus = false, bool bSelected = false);
  virtual bool OnAction(IExecutor& executor, const CAction& action);

  int GetValue() const;
  virtual int GetMessage() const { return m_iAction;};
  virtual void SetValue(int iValue){m_iValue = iValue;};
  virtual void SetLabel(const CStdString& strLabel){};
private:
  CGUISliderControl m_slider;
  bool m_bPercent;
  int m_iAction;
  int m_iMin;
  int m_iMax;
  int m_iInterval;
  int m_iValue;
};
};
