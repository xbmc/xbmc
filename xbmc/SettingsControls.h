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

#include "StdString.h"

class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;

class CSetting;

class CBaseSettingControl
{
public:
  CBaseSettingControl(int id, CSetting *pSetting);
  virtual ~CBaseSettingControl() {}
  virtual bool OnClick() { return false; };
  virtual void Update() {};
  int GetID() { return m_id; };
  CSetting* GetSetting() { return m_pSetting; };
  virtual bool NeedsUpdate() { return false; };   ///< Returns true if the control needs an update
  virtual void Reset() {}; ///< Resets the NeedsUpdate() state
protected:
  int m_id;
  CSetting* m_pSetting;
};

class CRadioButtonSettingControl : public CBaseSettingControl
{
public:
  CRadioButtonSettingControl(CGUIRadioButtonControl* pRadioButton, int id, CSetting *pSetting);
  virtual ~CRadioButtonSettingControl();
  virtual bool OnClick();
  virtual void Update();
  void Select(bool bSelect);
private:
  CGUIRadioButtonControl *m_pRadioButton;
};

class CSpinExSettingControl : public CBaseSettingControl
{
public:
  CSpinExSettingControl(CGUISpinControlEx* pSpin, int id, CSetting *pSetting);
  virtual ~CSpinExSettingControl();
  virtual bool OnClick();
  virtual void Update();
private:
  CGUISpinControlEx *m_pSpin;
};

class CButtonSettingControl : public CBaseSettingControl
{
public:
  CButtonSettingControl(CGUIButtonControl* pButton, int id, CSetting *pSetting);
  virtual ~CButtonSettingControl();
  virtual bool OnClick();
  virtual void Update();
private:
  CGUIButtonControl *m_pButton;
};

class CEditSettingControl : public CBaseSettingControl
{
public:
  CEditSettingControl(CGUIEditControl* pButton, int id, CSetting *pSetting);
  virtual ~CEditSettingControl();
  virtual bool OnClick();
  virtual void Update();
  virtual bool NeedsUpdate() { return m_needsUpdate; };
  virtual void Reset() { m_needsUpdate = false; };
private:
  bool IsValidIPAddress(const CStdString &strIP);
  CGUIEditControl *m_pEdit;
  bool m_needsUpdate;
};

class CSeparatorSettingControl : public CBaseSettingControl
{
public:
  CSeparatorSettingControl(CGUIImage* pImage, int id, CSetting *pSetting);
  virtual ~CSeparatorSettingControl();
  virtual bool OnClick() { return false; };
  virtual void Update() {};
private:
  CGUIImage *m_pImage;
};
