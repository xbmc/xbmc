#ifndef VOICECOMMUNICATOR_H
#define VOICECOMMUNICATOR_H

#include "../lib/libSpeex/Speex.h"

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
    CVoiceManager*  m_pManager;
    LONG            m_lSlot;
    XMediaObject*   m_pMicrophoneXMO;
    XMediaObject*   m_pHeadphoneXMO;

	SpeexBits		m_bits;
	int				m_bitrate;
	int				m_enc_frame_size;
	void*			m_enc_state;

    DWORD           m_dwSRCReadPosition;
    DWORD           m_dwRampTime;

    BYTE*           m_pbMicrophoneBuffer;
    BYTE*           m_pbHeadphoneBuffer;
};


#endif // VOICECOMMUNICATOR_H