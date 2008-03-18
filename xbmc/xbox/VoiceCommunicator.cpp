#include "stdafx.h"
#include "VoiceCommunicator.h"
#include "VoiceManager.h"
#include <xvoice.h>


// Defined in VoiceManager.cpp
extern const DWORD VOICE_SAMPLING_RATE;

CVoiceCommunicator::CVoiceCommunicator()
{
  m_pMicrophoneXMO = NULL;
  m_pHeadphoneXMO = NULL;
}




CVoiceCommunicator::~CVoiceCommunicator()
{
  assert( m_pMicrophoneXMO == NULL &&
          m_pHeadphoneXMO == NULL );

  assert( m_pbMicrophoneBuffer == NULL &&
          m_pbHeadphoneBuffer == NULL );
}



HRESULT CVoiceCommunicator::Initialize( CVoiceManager* pManager )
{
  m_pManager = pManager;

  // Allocate microphone buffer
  m_pbMicrophoneBuffer = new BYTE[ m_pManager->GetPacketSize() * m_pManager->GetNumPackets() ];
  if ( !m_pbMicrophoneBuffer )
    return E_OUTOFMEMORY;

  // Allocate headphone buffer
  m_pbHeadphoneBuffer = new BYTE[ m_pManager->GetPacketSize() * m_pManager->GetNumPackets() ];
  if ( !m_pbHeadphoneBuffer )
    return E_OUTOFMEMORY;

  // Create the encoder
  speex_bits_init( &m_bits );
  m_enc_state = speex_encoder_init( &speex_nb_mode );
  m_bitrate = 8192;
  speex_encoder_ctl( m_enc_state, SPEEX_SET_BITRATE, &m_bitrate );
  speex_encoder_ctl( m_enc_state, SPEEX_GET_FRAME_SIZE, &m_enc_frame_size );

  // Set the volume ramp time to 0
  m_dwRampTime = 0;

  return S_OK;
}


HRESULT CVoiceCommunicator::Shutdown()
{
  OnRemoval();

  if ( m_pbMicrophoneBuffer )
  {
    delete[] m_pbMicrophoneBuffer;
    m_pbMicrophoneBuffer = NULL;
  }

  if ( m_pbHeadphoneBuffer )
  {
    delete[] m_pbHeadphoneBuffer;
    m_pbHeadphoneBuffer = NULL;
  }

  speex_bits_destroy(&m_bits);
  speex_encoder_destroy(m_enc_state);

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: ResetMicrophone
// Desc: Resets the voice communicator's microphone.  This should be called
//          when the communicator is connected and the devices have been
//          created
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::ResetMicrophone()
{
  // To reset the microphone, we just need to submit all our packets
  XMEDIAPACKET xmp;
  for ( DWORD i = 0; i < m_pManager->GetNumPackets(); i++ )
  {
    GetMicrophonePacket( &xmp, i );

    // This will only fail if the communicator has been removed
    if ( FAILED( SubmitMicrophonePacket( &xmp ) ) )
    {
      return E_FAIL;
    }
  }

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: ResetHeadphone
// Desc: Resets the voice communicator's headphone.  This should be called
//          when the communicator is connected and the devices have been
//          created
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::ResetHeadphone()
{
  // Reset our SRC pointer
  m_pManager->GetSRCInfo( (WORD)m_lSlot, NULL, NULL, &m_dwSRCReadPosition );

  // To reset the headphone, we fill up all non-pending packets, up
  // until the first pending one
  XMEDIAPACKET xmp;
  for ( DWORD i = 0; i < m_pManager->GetNumPackets(); i++ )
  {
    GetHeadphonePacket( &xmp, i );
    ZeroMemory( xmp.pvBuffer, m_pManager->GetPacketSize() );

    // This will only fail if the communicator has been removed
    if ( FAILED( SubmitHeadphonePacket( &xmp ) ) )
    {
      return E_FAIL;
    }
  }

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: MicrophoneCallback
// Desc: Microphone DPC callback function - called whenever a microphone
//          has completed a packet.  Just forwards the call on to the
//          CVoiceManager object
//-----------------------------------------------------------------------------
VOID CALLBACK MicrophoneCallback( LPVOID pContext, LPVOID pPacketContext, DWORD dwStatus )
{
  CVoiceCommunicator* pThis = (CVoiceCommunicator*)pContext;

  pThis->m_pManager->MicrophonePacketCallback( pThis->m_lSlot, pPacketContext, dwStatus );
}



//-----------------------------------------------------------------------------
// Name: HeadphoneCallback
// Desc: Headphone DPC callback function - called whenever a headphone
//          has completed a packet.  Just forwards the call on to the
//          CVoiceManager object
//-----------------------------------------------------------------------------
VOID CALLBACK HeadphoneCallback( LPVOID pContext, LPVOID pPacketContext, DWORD dwStatus )
{
  CVoiceCommunicator* pThis = (CVoiceCommunicator*)pContext;

  pThis->m_pManager->HeadphonePacketCallback( pThis->m_lSlot, pPacketContext, dwStatus );
}




//-----------------------------------------------------------------------------
// Name: OnInsertion
// Desc: Handles insertion of a voice communicator by creating the microphone
//          and headphone devices
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::OnInsertion( DWORD dwSlot )
{
  m_lSlot = LONG( dwSlot );

  HRESULT hr;
  // Create the headphone device
  hr = XVoiceCreateMediaObjectEx( XDEVICE_TYPE_VOICE_HEADPHONE,
                                  m_lSlot,
                                  m_pManager->GetNumPackets(),
                                  m_pManager->GetWaveFormat(),
                                  HeadphoneCallback,
                                  this,
                                  &m_pHeadphoneXMO );
  if ( FAILED( hr ) )
    return E_FAIL;

  // Create the microphone device
  hr = XVoiceCreateMediaObjectEx( XDEVICE_TYPE_VOICE_MICROPHONE,
                                  m_lSlot,
                                  m_pManager->GetNumPackets(),
                                  m_pManager->GetWaveFormat(),
                                  MicrophoneCallback,
                                  this,
                                  &m_pMicrophoneXMO );
  if ( FAILED( hr ) )
    return E_FAIL;

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OnRemoval
// Desc: Handles removal of a voice communicator by releasing the microphone
//          and headphone devices
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::OnRemoval()
{
  if ( m_pMicrophoneXMO )
  {
    m_pMicrophoneXMO->Release();
    m_pMicrophoneXMO = NULL;
  }

  if ( m_pHeadphoneXMO )
  {
    m_pHeadphoneXMO->Release();
    m_pHeadphoneXMO = NULL;
  }

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetMicrophonePacket
// Desc: Fills out an XMEDIAPACKET structure for the current microphone packet
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::GetMicrophonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbMicrophoneBuffer + dwIndex * m_pManager->GetPacketSize();
  pPacket->dwMaxSize = m_pManager->GetPacketSize();
  pPacket->pContext = (LPVOID)dwIndex;

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: SubmitMicrophonePacket
// Desc: Submits the XMEDIAPACKET to the microphone XMO
//-----------------------------------------------------------------------------
inline
HRESULT CVoiceCommunicator::SubmitMicrophonePacket( XMEDIAPACKET* pPacket )
{
  return m_pMicrophoneXMO->Process( NULL, pPacket );
}



//-----------------------------------------------------------------------------
// Name: GetHeadphonePacket
// Desc: Fills out an XMEDIAPACKET structure for the current headphone packet
//-----------------------------------------------------------------------------
HRESULT CVoiceCommunicator::GetHeadphonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbHeadphoneBuffer + dwIndex * m_pManager->GetPacketSize();
  pPacket->dwMaxSize = m_pManager->GetPacketSize();
  pPacket->pContext = (LPVOID)dwIndex;

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: SubmitHeadphonePacket
// Desc: Submits the XMEDIAPACKET to the headphone XMO
//-----------------------------------------------------------------------------
inline
HRESULT CVoiceCommunicator::SubmitHeadphonePacket( XMEDIAPACKET* pPacket )
{
  return m_pHeadphoneXMO->Process( pPacket, NULL );
}
