/*
 *  LAME MP3 encoder for DirectShow
 *  Basic property page
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

class CMpegAudEncPropertyPage : public CBasePropertyPage 
{

public:
    static CUnknown *CreateInstance( LPUNKNOWN punk, HRESULT *phr );
    CMpegAudEncPropertyPage( LPUNKNOWN punk, HRESULT *phr );

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
    void    InitPropertiesDialog(HWND hwndParent);
    void    EnableControls(HWND hwndParent, bool bEnable);
    void    SetDirty(void);

    DWORD   m_dwBitrate;                //constant bit rate
    DWORD   m_dwVariable;               //flag - whether the variable bit rate set 
    DWORD   m_dwMin;                    //specify a minimum allowed bitrate
    DWORD   m_dwMax;                    //specify a maximum allowed bitrate
    DWORD   m_dwQuality;                //encoding quality
    DWORD   m_dwVBRq;                   //VBR quality setting (0=highest quality, 9=lowest) 
    DWORD   m_dwSampleRate;
    DWORD   m_dwChannelMode;
    DWORD   m_dwCRC;
    DWORD   m_dwForceMono;
    DWORD   m_dwCopyright;
    DWORD   m_dwOriginal;

    HWND    m_hwndQuality;               //Slider window handle

    int     m_srIdx;

    IAudioEncoderProperties *m_pAEProps;
};
