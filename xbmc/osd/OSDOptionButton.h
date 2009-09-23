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

namespace OSD
{
class COSDOptionButton :
      public IOSDOption
{
public:
  COSDOptionButton(int iAction, int iHeading);
  COSDOptionButton(const COSDOptionButton& option);
  const OSD::COSDOptionButton& operator = (const COSDOptionButton& option);

  virtual ~COSDOptionButton(void);
  virtual IOSDOption* Clone() const;
  virtual void Draw(int x, int y, bool bFocus = false, bool bSelected = false);
  virtual bool OnAction(IExecutor& executor, const CAction& action);
  virtual int GetMessage() const { return m_iAction;};
  virtual void SetLabel(const CStdString& strValue);
  virtual void SetValue(int iValue){};
private:
  int m_iAction;
  CStdString m_strValue;
};
};
