/*
 *  LAME MP3 encoder for DirectShow
 *  Advanced property page
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <streams.h>
#include <olectl.h>
#include <commctrl.h>
#include "iaudioprops.h"
#include "mpegac.h"
#include "resource.h"
#include "PropPage_adv.h"
#include "Reg.h"

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

// Strings which apear in comboboxes
const char *chChMode[4] = {
    "Mono",
    "Standard stereo",
    "Joint stereo",
    "Dual channel"};

////////////////////////////////////////////////////////////////
// CreateInstance
////////////////////////////////////////////////////////////////
CUnknown *CMpegAudEncPropertyPageAdv::CreateInstance( LPUNKNOWN punk, HRESULT *phr )
{
    CMpegAudEncPropertyPageAdv *pNewObject
        = new CMpegAudEncPropertyPageAdv( punk, phr );

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
}

////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////
CMpegAudEncPropertyPageAdv::CMpegAudEncPropertyPageAdv(LPUNKNOWN punk, HRESULT *phr) :
    CBasePropertyPage(NAME("Encoder Advanced Property Page"), punk, IDD_ADVPROPS, IDS_AUDIO_ADVANCED_TITLE),
    m_pAEProps(NULL)
{
    ASSERT(phr);

    InitCommonControls();
}

//
// OnConnect
//
// Give us the filter to communicate with
HRESULT CMpegAudEncPropertyPageAdv::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pAEProps == NULL);

    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAudioEncoderProperties,(void **)&m_pAEProps);
    if (FAILED(hr) || !m_pAEProps)
        return E_NOINTERFACE;

    ASSERT(m_pAEProps);

    // Get current filter state
//    m_pAEProps->LoadAudioEncoderPropertiesFromRegistry();

    m_pAEProps->get_EnforceVBRmin(&m_dwEnforceVBRmin);
    m_pAEProps->get_VoiceMode(&m_dwVoiceMode);
    m_pAEProps->get_KeepAllFreq(&m_dwKeepAllFreq);
    m_pAEProps->get_StrictISO(&m_dwStrictISO);
    m_pAEProps->get_NoShortBlock(&m_dwNoShortBlock);
    m_pAEProps->get_XingTag(&m_dwXingTag);
    m_pAEProps->get_ChannelMode(&m_dwChannelMode);
    m_pAEProps->get_ForceMS(&m_dwForceMS);
    m_pAEProps->get_ModeFixed(&m_dwModeFixed);
    m_pAEProps->get_SampleOverlap(&m_dwOverlap);
    m_pAEProps->get_SetDuration(&m_dwSetStop);

    return NOERROR;
}

//
// OnDisconnect
//
// Release the interface

HRESULT CMpegAudEncPropertyPageAdv::OnDisconnect()
{
    // Release the interface
    if (m_pAEProps == NULL)
        return E_UNEXPECTED;

    m_pAEProps->set_EnforceVBRmin(m_dwEnforceVBRmin);
    m_pAEProps->set_VoiceMode(m_dwVoiceMode);
    m_pAEProps->set_KeepAllFreq(m_dwKeepAllFreq);
    m_pAEProps->set_StrictISO(m_dwStrictISO);
    m_pAEProps->set_NoShortBlock(m_dwNoShortBlock);
    m_pAEProps->set_XingTag(m_dwXingTag);
    m_pAEProps->set_ChannelMode(m_dwChannelMode);
    m_pAEProps->set_ForceMS(m_dwForceMS);
    m_pAEProps->set_ModeFixed(m_dwModeFixed);
    m_pAEProps->set_SampleOverlap(m_dwOverlap);
    m_pAEProps->set_SetDuration(m_dwSetStop);
    m_pAEProps->SaveAudioEncoderPropertiesToRegistry();

    m_pAEProps->Release();
    m_pAEProps = NULL;

    return NOERROR;
}

//
// OnActivate
//
// Called on dialog creation

HRESULT CMpegAudEncPropertyPageAdv::OnActivate(void)
{
    InitPropertiesDialog(m_hwnd);

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT CMpegAudEncPropertyPageAdv::OnDeactivate(void)
{
    return NOERROR;
}

////////////////////////////////////////////////////////////////
// OnReceiveMessage - message handler function
////////////////////////////////////////////////////////////////
BOOL CMpegAudEncPropertyPageAdv::OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RADIO_STEREO:
        case IDC_RADIO_JSTEREO:
        case IDC_RADIO_DUAL:
        case IDC_RADIO_MONO:
            {

                DWORD dwChannelMode = LOWORD(wParam) - IDC_RADIO_STEREO;
                CheckRadioButton(hwnd, IDC_RADIO_STEREO, IDC_RADIO_MONO, LOWORD(wParam));

                if (dwChannelMode == MPG_MD_JOINT_STEREO)
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK_FORCE_MS),TRUE);
                else
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK_FORCE_MS),FALSE);

                m_pAEProps->set_ChannelMode(dwChannelMode);
                SetDirty();
            }
            break;

        case IDC_CHECK_ENFORCE_MIN:
            m_pAEProps->set_EnforceVBRmin(IsDlgButtonChecked(hwnd, IDC_CHECK_ENFORCE_MIN));
            SetDirty();
            break;

        case IDC_CHECK_VOICE:
            m_pAEProps->set_VoiceMode(IsDlgButtonChecked(hwnd, IDC_CHECK_VOICE));
            SetDirty();
            break;

        case IDC_CHECK_KEEP_ALL_FREQ:
            m_pAEProps->set_KeepAllFreq(IsDlgButtonChecked(hwnd, IDC_CHECK_KEEP_ALL_FREQ));
            SetDirty();
            break;

        case IDC_CHECK_STRICT_ISO:
            m_pAEProps->set_StrictISO(IsDlgButtonChecked(hwnd, IDC_CHECK_STRICT_ISO));
            SetDirty();
            break;

        case IDC_CHECK_DISABLE_SHORT_BLOCK:
            m_pAEProps->set_NoShortBlock(IsDlgButtonChecked(hwnd, IDC_CHECK_DISABLE_SHORT_BLOCK));
            SetDirty();
            break;

        case IDC_CHECK_XING_TAG:
            m_pAEProps->set_XingTag(IsDlgButtonChecked(hwnd, IDC_CHECK_XING_TAG));
            SetDirty();
            break;

        case IDC_CHECK_FORCE_MS:
            m_pAEProps->set_ForceMS(IsDlgButtonChecked(hwnd, IDC_CHECK_FORCE_MS));
            SetDirty();
            break;

        case IDC_CHECK_MODE_FIXED:
            m_pAEProps->set_ModeFixed(IsDlgButtonChecked(hwnd, IDC_CHECK_MODE_FIXED));
            SetDirty();
            break;

        case IDC_CHECK_OVERLAP:
            m_pAEProps->set_SampleOverlap(IsDlgButtonChecked(hwnd, IDC_CHECK_OVERLAP));
            SetDirty();
            break;

        case IDC_CHECK_STOP:
            m_pAEProps->set_SetDuration(IsDlgButtonChecked(hwnd, IDC_CHECK_STOP));
            SetDirty();
            break;
        }

        return TRUE;

    case WM_DESTROY:
        return TRUE;

    default:
        return FALSE;
    }

    return TRUE;
}

//
// OnApplyChanges
//
HRESULT CMpegAudEncPropertyPageAdv::OnApplyChanges()
{
    m_pAEProps->get_EnforceVBRmin(&m_dwEnforceVBRmin);
    m_pAEProps->get_VoiceMode(&m_dwVoiceMode);
    m_pAEProps->get_KeepAllFreq(&m_dwKeepAllFreq);
    m_pAEProps->get_StrictISO(&m_dwStrictISO);
    m_pAEProps->get_ChannelMode(&m_dwChannelMode);
    m_pAEProps->get_ForceMS(&m_dwForceMS);
    m_pAEProps->get_NoShortBlock(&m_dwNoShortBlock);
    m_pAEProps->get_XingTag(&m_dwXingTag);
    m_pAEProps->get_ModeFixed(&m_dwModeFixed);
    m_pAEProps->get_SampleOverlap(&m_dwOverlap);
    m_pAEProps->get_SetDuration(&m_dwSetStop);
    m_pAEProps->SaveAudioEncoderPropertiesToRegistry();

    m_pAEProps->ApplyChanges();

    return S_OK;
}

//
// Initialize dialogbox controls with proper values
//
void CMpegAudEncPropertyPageAdv::InitPropertiesDialog(HWND hwndParent)
{
    EnableControls(hwndParent, TRUE);

    //
    // initialize radio bottons
    //
    DWORD dwChannelMode;
    m_pAEProps->get_ChannelMode(&dwChannelMode);
    CheckRadioButton(hwndParent, IDC_RADIO_STEREO, IDC_RADIO_MONO, IDC_RADIO_STEREO + dwChannelMode);

    if (dwChannelMode == MPG_MD_JOINT_STEREO)
        EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_FORCE_MS), TRUE);
    else
        EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_FORCE_MS), FALSE);

    //
    // initialize checkboxes
    //
    DWORD dwEnforceVBRmin;
    m_pAEProps->get_EnforceVBRmin(&dwEnforceVBRmin);
    CheckDlgButton(hwndParent, IDC_CHECK_ENFORCE_MIN, dwEnforceVBRmin ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwVoiceMode;
    m_pAEProps->get_VoiceMode(&dwVoiceMode);
    CheckDlgButton(hwndParent, IDC_CHECK_VOICE, dwVoiceMode ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwKeepAllFreq;
    m_pAEProps->get_KeepAllFreq(&dwKeepAllFreq);
    CheckDlgButton(hwndParent, IDC_CHECK_KEEP_ALL_FREQ, dwKeepAllFreq ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwStrictISO;
    m_pAEProps->get_StrictISO(&dwStrictISO);
    CheckDlgButton(hwndParent, IDC_CHECK_STRICT_ISO, dwStrictISO ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwNoShortBlock;
    m_pAEProps->get_NoShortBlock(&dwNoShortBlock);
    CheckDlgButton(hwndParent, IDC_CHECK_DISABLE_SHORT_BLOCK, dwNoShortBlock ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwXingEnabled;
    m_pAEProps->get_XingTag(&dwXingEnabled);
    CheckDlgButton(hwndParent, IDC_CHECK_XING_TAG, dwXingEnabled ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwForceMS;
    m_pAEProps->get_ForceMS(&dwForceMS);
    CheckDlgButton(hwndParent, IDC_CHECK_FORCE_MS, dwForceMS ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwModeFixed;
    m_pAEProps->get_ModeFixed(&dwModeFixed);
    CheckDlgButton(hwndParent, IDC_CHECK_MODE_FIXED, dwModeFixed ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwOverlap;
    m_pAEProps->get_SampleOverlap(&dwOverlap);
    CheckDlgButton(hwndParent, IDC_CHECK_OVERLAP, dwOverlap ? BST_CHECKED : BST_UNCHECKED);

    DWORD dwStopTime;
    m_pAEProps->get_SetDuration(&dwStopTime);
    CheckDlgButton(hwndParent, IDC_CHECK_STOP, dwStopTime ? BST_CHECKED : BST_UNCHECKED);
}


////////////////////////////////////////////////////////////////
// EnableControls
////////////////////////////////////////////////////////////////
void CMpegAudEncPropertyPageAdv::EnableControls(HWND hwndParent, bool bEnable)
{
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_ENFORCE_MIN), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_RADIO_STEREO), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_RADIO_JSTEREO), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_RADIO_DUAL), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_RADIO_MONO), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_FORCE_MS), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_VOICE), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_KEEP_ALL_FREQ), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_STRICT_ISO), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_DISABLE_SHORT_BLOCK), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_XING_TAG), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_MODE_FIXED), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_OVERLAP), bEnable);
    EnableWindow(GetDlgItem(hwndParent, IDC_CHECK_STOP), bEnable);
}

//
// SetDirty
//
// notifies the property page site of changes

void CMpegAudEncPropertyPageAdv::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

