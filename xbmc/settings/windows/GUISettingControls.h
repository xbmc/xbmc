#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"

class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;

class CSetting;

class CGUIBaseSettingControl
{
public:
  CGUIBaseSettingControl(int id, CSetting *pSetting);
  virtual ~CGUIBaseSettingControl() {}
  virtual bool OnClick() { return false; };
  virtual void Update() {};
  int GetID() { return m_id; };
  CSetting* GetSetting() { return m_pSetting; };
  virtual bool NeedsUpdate() { return false; };   ///< Returns true if the control needs an update
  virtual void Reset() {}; ///< Resets the NeedsUpdate() state
  virtual void Clear()=0;  ///< Clears the attached control

  /*!
   \brief Specifies that this setting should update after a delay
   Useful for settings that have options to navigate through
   and may take a while, or require additional input to update
   once the final setting is chosen.  Settings default to updating
   instantly.
   \sa IsDelayed()
   */
  void SetDelayed() { m_delayed = true; };

  /*!
   \brief Returns whether this setting should have delayed update
   \return true if the setting's update should be delayed
   \sa SetDelayed()
   */
  bool IsDelayed() const { return m_delayed; };
protected:
  int m_id;
  CSetting* m_pSetting;
  bool m_delayed;
};

class CGUIRadioButtonSettingControl : public CGUIBaseSettingControl
{
public:
  CGUIRadioButtonSettingControl(CGUIRadioButtonControl* pRadioButton, int id, CSetting *pSetting);
  virtual ~CGUIRadioButtonSettingControl();
  virtual bool OnClick();
  virtual void Update();
  virtual void Clear() { m_pRadioButton = NULL; }
  void Select(bool bSelect);
private:
  CGUIRadioButtonControl *m_pRadioButton;
};

class CGUISpinExSettingControl : public CGUIBaseSettingControl
{
public:
  CGUISpinExSettingControl(CGUISpinControlEx* pSpin, int id, CSetting *pSetting);
  virtual ~CGUISpinExSettingControl();
  virtual bool OnClick();
  virtual void Update();
  virtual void Clear() { m_pSpin = NULL; }
private:
  CGUISpinControlEx *m_pSpin;
};

class CGUIButtonSettingControl : public CGUIBaseSettingControl
{
public:
  CGUIButtonSettingControl(CGUIButtonControl* pButton, int id, CSetting *pSetting);
  virtual ~CGUIButtonSettingControl();
  virtual bool OnClick();
  virtual void Update();
  virtual void Clear() { m_pButton = NULL; }
private:
  CGUIButtonControl *m_pButton;
};

class CGUIEditSettingControl : public CGUIBaseSettingControl
{
public:
  CGUIEditSettingControl(CGUIEditControl* pButton, int id, CSetting *pSetting);
  virtual ~CGUIEditSettingControl();
  virtual bool OnClick();
  virtual void Update();
  virtual bool NeedsUpdate() { return m_needsUpdate; };
  virtual void Reset() { m_needsUpdate = false; };
  virtual void Clear() { m_pEdit = NULL; }
private:
  bool IsValidIPAddress(const CStdString &strIP);
  CGUIEditControl *m_pEdit;
  bool m_needsUpdate;
};

class CGUISeparatorSettingControl : public CGUIBaseSettingControl
{
public:
  CGUISeparatorSettingControl(CGUIImage* pImage, int id, CSetting *pSetting);
  virtual ~CGUISeparatorSettingControl();
  virtual bool OnClick() { return false; };
  virtual void Update() {};
  virtual void Clear() { m_pImage = NULL; }
private:
  CGUIImage *m_pImage;
};
