#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

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

#include "VoiceCommunicator.h"

// Enumeration for communicator-related events
enum VOICE_COMMUNICATOR_EVENT
{
  VOICE_COMMUNICATOR_INSERTED,
  VOICE_COMMUNICATOR_REMOVED,
};

// Callback function signature for communicator-related events
typedef VOID (*PFNCOMMUNICATORCALLBACK)( DWORD dwPort, VOICE_COMMUNICATOR_EVENT event, VOID* pContext );

// Callback function signature for voice data
typedef VOID (*PFNVOICEDATACALLBACK)( DWORD dwPort, DWORD dwSize, VOID* pvData, VOID* pContext );

// With everything running at DPC, we only need 2 packets to ping-pong
static const DWORD NUM_PACKETS = 2;


struct VOICE_MANAGER_CONFIG
{
  DWORD dwVoicePacketTime;      // Packet time, in ms
  DWORD dwMaxRemotePlayers;     // Maximum # of remote players
  DWORD dwMaxStoredPackets;     // Maximum # of stored encoded packets

  LPDIRECTSOUND8 pDSound;                // DirectSound object
  LPDSEFFECTIMAGEDESC pEffectImageDesc;       // DSP Effect Image Desc
  DWORD dwFirstSRCEffectIndex;  // Effect index of first SRC effect in DSP

  // Will need callbacks for notifying of certain events
  VOID* pCallbackContext;
  PFNCOMMUNICATORCALLBACK pfnCommunicatorCallback;
  PFNVOICEDATACALLBACK pfnVoiceDataCallback;
};

// Typedefs and class/struct declaration
typedef std::vector<DWORD> MuteList;
struct REMOTE_CHATTER;

class CVoiceManager
{
public:
  CVoiceManager();
  ~CVoiceManager();

  // Methods for controlling overall voice state
  HRESULT Initialize( VOICE_MANAGER_CONFIG* pConfig );
  HRESULT Shutdown();
  VOID EnterChatSession();
  VOID LeaveChatSession();
  BOOL IsInChatSession() { return m_bIsInChatSession; }

  // Common methods
  HRESULT ReceivePacket( DWORD dwPlayer, VOID* pvData, INT nSize );
  HRESULT ProcessVoice();

  // Methods for managing the chatter list, including muting
  HRESULT AddChatter( DWORD dwPlayer );
  HRESULT RemoveChatter( DWORD dwPlayer );
  HRESULT ResetChatter( DWORD dwPlayer );
  HRESULT MutePlayer( DWORD dwPlayer, DWORD dwControllerPort );
  HRESULT UnMutePlayer( DWORD dwPlayer, DWORD dwControllerPort );
  BOOL IsPlayerMuted( DWORD dwPlayer, DWORD dwControllerPort );
  HRESULT RemoteMutePlayer( DWORD dwPlayer, DWORD dwControllerPort );
  HRESULT UnRemoteMutePlayer( DWORD dwPlayer, DWORD dwControllerPort );
  BOOL IsPlayerRemoteMuted( DWORD dwPlayer, DWORD dwControllerPort );

  // Methods for getting information about voice state
  BOOL IsCommunicatorInserted( DWORD dwControllerPort ) { return m_dwConnectedCommunicators & ( 1 << dwControllerPort ); }
  BOOL IsPlayerRegistered( DWORD dwPlayer );
  BOOL IsPlayerTalking( DWORD dwPlayer );

  // Methods for controlling communicator operation
  HRESULT EnableCommunicator( DWORD dwControllerPort, BOOL bEnabled );
  HRESULT SetLoopback( DWORD dwControllerPort, BOOL bLoopback );
  HRESULT SetVoiceThroughSpeakers( BOOL bEnabled );

  static BOOL IsHeadsetConnected();

protected:
  // Helper methods, primarily for use by CVoiceCommunicator class
  friend CVoiceCommunicator;
  HRESULT GetSRCInfo( DWORD dwControllerPort, DWORD* pdwBufferSize, VOID** ppvBufferData, DWORD* pdwWritePosition );
  HRESULT SetSRCGain( DWORD dwControllerPort, FLOAT fGain );
  DWORD GetNumPackets() { return NUM_PACKETS; }
  DWORD GetPacketSize() { return m_dwPacketSize; }
  const WAVEFORMATEX* GetWaveFormat() { return &m_wfx; }

  // DPC callback functions
  friend VOID CALLBACK StreamCallback( LPVOID, LPVOID, DWORD );
  friend VOID CALLBACK MicrophoneCallback( LPVOID, LPVOID, DWORD );
  friend VOID CALLBACK HeadphoneCallback( LPVOID, LPVOID, DWORD );

  // VoiceManager implementations of DPC callbacks
  HRESULT StreamPacketCallback( REMOTE_CHATTER* pChatter, LPVOID pPacketContext, DWORD dwStatus );
  HRESULT MicrophonePacketCallback( DWORD dwPort, LPVOID pPacketContext, DWORD dwStatus );
  HRESULT HeadphonePacketCallback( DWORD dwPort, LPVOID pPacketContext, DWORD dwStatus );

  // Internal-only functions for dealing with communicators
  HRESULT OnCompletedPacket( DWORD dwControllerPort, VOID* pvData, DWORD dwSize );
  HRESULT OnCommunicatorInserted( DWORD dwControllerPort );
  HRESULT OnCommunicatorRemoved( DWORD dwControllerPort );

  static VOID CheckDeviceChangesLite();
  HRESULT CheckDeviceChanges();

  // Internal-only functions for dealing with chatters
  HRESULT InitChatter( REMOTE_CHATTER* pChatter,
                       DSSTREAMDESC* pdssd );
  DWORD ChatterIndexFromDUID( DWORD dwPlayer);
  HRESULT RecalculateMixBins( REMOTE_CHATTER* pChatter );

  // Helper functions for filling out XMEDIAPACKETs
  HRESULT GetDriftCompensationPacket( XMEDIAPACKET* pPacket, REMOTE_CHATTER* pChatter );
  HRESULT GetTemporaryPacket( XMEDIAPACKET* pPacket );
  HRESULT GetStreamPacket( XMEDIAPACKET* pPacket, REMOTE_CHATTER* pChatter, DWORD dwIndex );
  HRESULT SubmitStreamPacket( DWORD dwIndex, REMOTE_CHATTER* pChatter );

  // Copy of configuration struct passed in to Initialize()
  VOICE_MANAGER_CONFIG m_cfg;

  // Handy data to keep cached
  WAVEFORMATEX m_wfx;
  DWORD m_dwPacketSize;
  DWORD m_dwBufferSize;
  DWORD m_dwCompressedSize;
  DWORD m_adwHeadphoneSends[ XGetPortCount() ];

  // Chatter list
  REMOTE_CHATTER* m_pChatters;
  BOOL m_bIsInChatSession;
  BOOL m_bResetDeviceState;
  REMOTE_CHATTER* m_pLoopbackChatters;

  // Communicator info
  DWORD m_dwConnectedCommunicators;
  static DWORD m_dwMicrophoneState;
  static DWORD m_dwHeadphoneState;
  DWORD m_dwLoopback;
  DWORD m_dwEnabled;
  CVoiceCommunicator m_aVoiceCommunicators[XGetPortCount()];
  BOOL m_bVoiceThroughSpeakers;

  // Number of 20ms frames where silence has been detected continously.
  INT m_nSilentFrameCounter;

  // Temporary buffer to hold a compressed packet
  BYTE* m_pbTempEncodedPacket;

  // Buffer to hold completed, encoded packets
  DWORD m_dwCompletedPacketSize;
  DWORD m_dwNumCompletedPackets;
  BYTE* m_pbCompletedPackets;

  // Mute lists
  MuteList m_MuteList[XGetPortCount()];
  MuteList m_RemoteMuteList[XGetPortCount()];
};


// Single global instance of class is in cpp file
extern CVoiceManager g_VoiceManager;

#endif // VOICEMANAGER_H
