#include "stdafx.h"
#include "SettingsControls.h"

CBaseSettingControl::CBaseSettingControl(DWORD dwID, CSetting *pSetting)
{
	m_dwID = dwID;
	m_pSetting = pSetting;
}

CRadioButtonSettingControl::CRadioButtonSettingControl(CGUIRadioButtonControl *pRadioButton, DWORD dwID, CSetting *pSetting)
:CBaseSettingControl(dwID, pSetting)
{
	m_pRadioButton = pRadioButton;
	m_pRadioButton->SetID(dwID);
	Update();
}

CRadioButtonSettingControl::~CRadioButtonSettingControl()
{
}

void CRadioButtonSettingControl::OnClick()
{
	((CSettingBool *)m_pSetting)->SetData(!((CSettingBool *)m_pSetting)->GetData());
}

void CRadioButtonSettingControl::Update()
{
	m_pRadioButton->SetSelected(((CSettingBool *)m_pSetting)->GetData());
}

CSpinExSettingControl::CSpinExSettingControl(CGUISpinControlEx *pSpin, DWORD dwID, CSetting *pSetting)
:CBaseSettingControl(dwID, pSetting)
{
	m_pSpin = pSpin;
	m_pSpin->SetID(dwID);
	if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
	{
		CSettingFloat *pSettingFloat = (CSettingFloat *)pSetting;
		m_pSpin->SetType(SPIN_CONTROL_TYPE_FLOAT);
		m_pSpin->SetFloatRange(pSettingFloat->m_fMin, pSettingFloat->m_fMax);
		m_pSpin->SetFloatInterval(pSettingFloat->m_fStep);
	}
	else if (pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_INT)
	{
		CSettingInt *pSettingInt = (CSettingInt *)pSetting;
		m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
		m_pSpin->Clear();
		CStdString strLabel;
		for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i+= pSettingInt->m_iStep)
		{
			if (pSettingInt->m_iFormat>-1)
			{
				CStdString strFormat=g_localizeStrings.Get(pSettingInt->m_iFormat);
				strLabel.Format(strFormat, i);
			}
			else
				strLabel.Format(pSettingInt->m_strFormat, i);
			m_pSpin->AddLabel(strLabel, (i-pSettingInt->m_iMin)/pSettingInt->m_iStep);
		}
	}
	else // if (pSetting->GetControlType() == SPIN_CONTROL_TEXT)
	{
		m_pSpin->SetType(SPIN_CONTROL_TYPE_TEXT);
		m_pSpin->Clear();
	}
	Update();
}

CSpinExSettingControl::~CSpinExSettingControl()
{
}

void CSpinExSettingControl::OnClick()
{
	if (m_pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
		((CSettingFloat *)m_pSetting)->SetData(m_pSpin->GetFloatValue());
	else
	{
		if (m_pSetting->GetType() != SETTINGS_TYPE_STRING)
		{
			CSettingInt *pSettingInt = (CSettingInt *)m_pSetting;
			pSettingInt->SetData(m_pSpin->GetValue() * pSettingInt->m_iStep + pSettingInt->m_iMin);
		}
	}
}

void CSpinExSettingControl::Update()
{
	if (m_pSetting->GetControlType() == SPIN_CONTROL_FLOAT)
	{
		CSettingFloat *pSettingFloat = (CSettingFloat *)m_pSetting;
		m_pSpin->SetFloatValue(pSettingFloat->GetData());
	}
	else if (m_pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || m_pSetting->GetControlType() == SPIN_CONTROL_INT)
	{
		CSettingInt *pSettingInt = (CSettingInt *)m_pSetting;
		m_pSpin->SetValue((pSettingInt->GetData()-pSettingInt->m_iMin)/pSettingInt->m_iStep);
	}
}

CButtonSettingControl::CButtonSettingControl(CGUIButtonControl *pButton, DWORD dwID, CSetting *pSetting)
:CBaseSettingControl(dwID, pSetting)
{
	m_pButton = pButton;
	m_pButton->SetID(dwID);
	Update();
}

CButtonSettingControl::~CButtonSettingControl()
{
}

void CButtonSettingControl::OnClick()
{
	// grab the onscreen keyboard
	CStdString keyboardInput(((CSettingString *)m_pSetting)->GetData());
	if (m_pSetting->GetControlType() == BUTTON_CONTROL_INPUT || m_pSetting->GetControlType() == BUTTON_CONTROL_HIDDEN_INPUT || m_pSetting->GetControlType() == BUTTON_CONTROL_IP_INPUT)
	{
		if (CGUIDialogKeyboard::ShowAndGetInput(keyboardInput, ((CSettingString *)m_pSetting)->m_bAllowEmpty))
		{
			if (m_pSetting->GetControlType() == BUTTON_CONTROL_IP_INPUT)
			{	// check if we have a valid ip...
				if (!IsValidIPAddress(keyboardInput))
					return;
			}
		}
		else
			return;
	}
	((CSettingString *)m_pSetting)->SetData(keyboardInput);
	Update();
}

void CButtonSettingControl::Update()
{
	if (m_pSetting->GetControlType() == BUTTON_CONTROL_STANDARD)
		return;
	CStdString strText = ((CSettingString *)m_pSetting)->GetData();
	if (m_pSetting->GetControlType() == BUTTON_CONTROL_HIDDEN_INPUT)
	{
		int iNumChars = strText.size();
		strText.Empty();
		for (int i=0; i<iNumChars; i++)
			strText += '*';
	}
	m_pButton->SetText2(strText);
}


bool CButtonSettingControl::IsValidIPAddress(const CStdString &strIP)
{
	const char* s = strIP.c_str();
	bool legalFormat = true;
	bool numSet = false;
	int num = 0;
	int dots = 0;

	while (*s != '\0')
	{
		if (*s == '.')
		{
			++dots;

			// There must be a number before a .
			if (!numSet)
			{
				legalFormat = false;
				break;
			}

			if (num > 255)
			{
				legalFormat = false;
				break;
			}

			num = 0;
			numSet = false;
		}
		else if  (*s >= '0' && *s <= '9')
		{
			num = (num * 10) + (*s - '0');
			numSet = true;
		}
		else
		{
			legalFormat = false;
			break;
		}

		++s;
	}

	if (legalFormat)
	{
		if (!numSet)
		{
			legalFormat = false;
		}

		if (num > 255 || dots != 3)
		{
			legalFormat = false;
		}
	}

	if (!legalFormat)
	{
		CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		dlg->SetHeading( 257 );
		dlg->SetLine( 0, 724 );
		dlg->SetLine( 1, 725 );
		dlg->SetLine( 2, L"" );
		dlg->DoModal( m_gWindowManager.GetActiveWindow() );
	}
	return legalFormat;
}