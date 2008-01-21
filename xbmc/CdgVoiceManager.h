#ifndef CDGVOICEMANAGER_H
#define CDGVOICEMANAGER_H

#include <xvoice.h>
#include "utils/SingleLock.h"

enum CDG_DEVICE_TYPE
{
  VOICE_COMMUNICATOR,
  HIFI_MICROPHONE,
};

enum CDG_VOICE_DEVICE_EVENT
{
  CDG_VOICE_DEVICE_INSERTED,
  CDG_VOICE_DEVICE_REMOVED,
};
// Callback function signature for voice device-related events
typedef VOID (*PFNCDGVOICEDEVICECALLBACK)( DWORD dwPort, CDG_DEVICE_TYPE DeviceType, CDG_VOICE_DEVICE_EVENT event, VOID* pContext );
typedef VOID (*PFNCDGVOICEDATACALLBACK)( DWORD dwPort, DWORD dwSize, VOID* pvData, VOID* pContext );

struct CDG_VOICE_MANAGER_CONFIG
{
  DWORD dwVoicePacketTime;      // Packet time, in ms
  DWORD dwMaxStoredPackets;     // Maximum # of stored encoded packets per voice device
  LPDIRECTSOUND8 pDSound;                // DirectSound object
  // Will need callbacks for notifying of certain events
  VOID* pCallbackContext;
  PFNCDGVOICEDEVICECALLBACK pfnVoiceDeviceCallback;
  PFNCDGVOICEDATACALLBACK pfnVoiceDataCallback;
};

class CCdgVoiceManager;
class CCdgChatter
{
public:
  CCdgChatter();
  ~CCdgChatter();
  HRESULT Initialize(CCdgVoiceManager* pManager, DWORD dwPort, CDG_DEVICE_TYPE device);
  HRESULT ProcessVoice(PFNCDGVOICEDATACALLBACK pfnVoiceDataCallback, VOID* pCallbackContext);
  void SetVolume(long lVol);
  void Shutdown();
private:
  void LoadSettings();
  HRESULT GetDriftCompensationPacket( XMEDIAPACKET* pPacket );
  HRESULT GetTemporaryPacket( XMEDIAPACKET* pPacket);
  HRESULT GetCompTemporaryPacket( XMEDIAPACKET* pPacket);
  HRESULT OnCompletedPacket(VOID* pvData, DWORD dwSize );

  HRESULT GetStreamPacket( XMEDIAPACKET* pPacket, DWORD dwIndex );
  HRESULT SubmitStreamPacket( DWORD dwIndex );
  HRESULT StreamPacketCallback( LPVOID pPacketContext, DWORD dwStatus );
  HRESULT GetMicrophonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex );
  HRESULT SubmitMicrophonePacket( XMEDIAPACKET* pPacket );
  HRESULT MicrophonePacketCallback( LPVOID pPacketContext, DWORD dwStatus );

  // DPC callback functions
  friend VOID CALLBACK CdgMicrophoneCallback( LPVOID, LPVOID, DWORD );
  friend VOID CALLBACK CdgStreamCallback( LPVOID, LPVOID, DWORD );

  DWORD m_dwPort;
  DWORD m_dwPacketSize;
  DWORD m_dwCompressedSize;
  DWORD m_dwNumCompletedPackets;
  DWORD m_dwTempStatus;
  CDG_DEVICE_TYPE m_DeviceType;
  BYTE* m_pbTempBuffer;
  BYTE* m_pbCompTempBuffer;
  BYTE* m_pbDriftBuffer;
  BYTE* m_pbStreamBuffer;
  BYTE* m_pbCompletedPackets;
  BYTE* m_pbMicrophoneBuffer;
  WAVEFORMATEX m_wfx;
  long m_lVolume;
  CCdgVoiceManager* m_pVoiceManager;      // Pointer to CVoiceManager
  LPDIRECTSOUNDSTREAM m_pOutputStream;      // DSound mixing stream
  CCriticalSection m_CritSection;

  XMediaObject* m_pMicrophoneXMO;
  LPXVOICEDECODER m_pDecoderXMO;
  LPXVOICEENCODER m_pEncoderXMO;
  XVOICE_MASK * m_pVoiceMask;
};

class CCdgVoiceManager
{
public:
  friend CCdgChatter;
  CCdgVoiceManager();
  ~CCdgVoiceManager();
  void Initialize( CDG_VOICE_MANAGER_CONFIG* pConfig );
  HRESULT EnableVoiceDevice( DWORD dwPort, bool bEnabled );
  void SetVolume(DWORD dwPort, int iPercent);
  BOOL IsCommunicatorInserted( DWORD dwPort ) { return m_dwConnectedCommunicators & ( 1 << dwPort ); }
  BOOL IsHiFiMicrophoneInserted( DWORD dwPort ) { return m_dwConnectedHiFiMicrophones & ( 1 << dwPort ); }
  HRESULT ProcessVoice();
  void Shutdown();
protected:
  // Internal-only functions for dealing with communicators
  HRESULT OnVoiceDeviceInserted(DWORD dwPort , CDG_DEVICE_TYPE DeviceType );
  HRESULT OnVoiceDeviceRemoved(DWORD dwPort , CDG_DEVICE_TYPE DeviceType );
  HRESULT CheckDeviceChanges();

  // Copy of configuration struct passed in to Initialize()
  CDG_VOICE_MANAGER_CONFIG m_cfg;
  CCdgChatter m_Chatters[XGetPortCount()];
  bool m_bEnabled[XGetPortCount()];
  // Handy data to keep cached
  DWORD m_dwNumPackets;
  DWORD m_dwSamplingRate;
  DWORD m_dwHiFiSamplingRate;
  // Communicator info
  DWORD m_dwConnectedCommunicators;
  DWORD m_dwConnectedHiFiMicrophones;
  DWORD m_dwMicrophoneState;
  DWORD m_dwHeadphoneState;
  DWORD m_dwHiFiMicrophoneState;
};


#endif // CDGVOICEMANAGER_H
