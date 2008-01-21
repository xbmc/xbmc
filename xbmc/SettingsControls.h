#pragma once

#include "GUISpinControlEx.h"
#include "GUIButtonControl.h"
#include "GUIRadioButtonControl.h"

class CBaseSettingControl
{
public:
  CBaseSettingControl(DWORD dwID, CSetting *pSetting);
  virtual ~CBaseSettingControl() {}
  virtual bool OnClick() { return false; };
  virtual void Update() {};
  DWORD GetID() { return m_dwID; };
  CSetting* GetSetting() { return m_pSetting; };

protected:
  DWORD m_dwID;
  CSetting* m_pSetting;
};

class CRadioButtonSettingControl : public CBaseSettingControl
{
public:
  CRadioButtonSettingControl(CGUIRadioButtonControl* pRadioButton, DWORD dwID, CSetting *pSetting);
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
  CSpinExSettingControl(CGUISpinControlEx* pSpin, DWORD dwID, CSetting *pSetting);
  virtual ~CSpinExSettingControl();
  virtual bool OnClick();
  virtual void Update();
private:
  CGUISpinControlEx *m_pSpin;
};

class CButtonSettingControl : public CBaseSettingControl
{
public:
  CButtonSettingControl(CGUIButtonControl* pButton, DWORD dwID, CSetting *pSetting);
  virtual ~CButtonSettingControl();
  virtual bool OnClick();
  virtual void Update();
private:
  bool IsValidIPAddress(const CStdString &strIP);
  CGUIButtonControl *m_pButton;
};

class CSeparatorSettingControl : public CBaseSettingControl
{
public:
  CSeparatorSettingControl(CGUIImage* pImage, DWORD dwID, CSetting *pSetting);
  virtual ~CSeparatorSettingControl();
  virtual bool OnClick() { return false; };
  virtual void Update() {};
private:
  CGUIImage *m_pImage;
};
