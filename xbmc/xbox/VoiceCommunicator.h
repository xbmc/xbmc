#ifndef VOICECOMMUNICATOR_H
#define VOICECOMMUNICATOR_H

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

#include "lib/libSpeex/Speex.h"

class CVoiceManager;

class CVoiceCommunicator
{
public:
  friend CVoiceManager;

  CVoiceCommunicator();
  ~CVoiceCommunicator();

  HRESULT Initialize( CVoiceManager* pManager );
  HRESULT Shutdown();

  HRESULT ResetMicrophone();
  HRESULT ResetHeadphone();
  HRESULT OnInsertion( DWORD dwSlot );
  HRESULT OnRemoval();

  HRESULT GetMicrophonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex );
  HRESULT SubmitMicrophonePacket( XMEDIAPACKET* pPacket );

  HRESULT GetHeadphonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex );
  HRESULT SubmitHeadphonePacket( XMEDIAPACKET* pPacket );

  friend VOID CALLBACK MicrophoneCallback( LPVOID, LPVOID, DWORD );
  friend VOID CALLBACK HeadphoneCallback( LPVOID, LPVOID, DWORD );

private:
  CVoiceManager* m_pManager;
  LONG m_lSlot;
  XMediaObject* m_pMicrophoneXMO;
  XMediaObject* m_pHeadphoneXMO;

  SpeexBits m_bits;
  int m_bitrate;
  int m_enc_frame_size;
  void* m_enc_state;

  DWORD m_dwSRCReadPosition;
  DWORD m_dwRampTime;

  BYTE* m_pbMicrophoneBuffer;
  BYTE* m_pbHeadphoneBuffer;
};


#endif // VOICECOMMUNICATOR_H
