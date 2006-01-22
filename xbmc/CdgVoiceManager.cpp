#include "stdafx.h"
#include "CdgVoiceManager.h"


//CdgChatter
CCdgChatter::CCdgChatter()
{
  m_dwPort = -1;
  m_dwPacketSize = 0;
  m_dwCompressedSize = 0;
  m_dwNumCompletedPackets = 0;
  m_DeviceType = VOICE_COMMUNICATOR;
  m_pVoiceMask = NULL;
  m_pbTempBuffer = NULL;
  m_pbCompTempBuffer = NULL;
  m_pbDriftBuffer = NULL;
  m_pbStreamBuffer = NULL;
  m_pbMicrophoneBuffer = NULL;
  m_pbCompletedPackets = NULL;
  m_dwTempStatus = NULL;
  ZeroMemory(&m_wfx, sizeof(WAVEFORMATEX) );
  m_lVolume = VOLUME_MINIMUM;
  m_pVoiceManager = NULL;      // Pointer to CVoiceManager
  m_pOutputStream = NULL;      // DSound mixing stream

  m_pMicrophoneXMO = NULL;
  m_pEncoderXMO = NULL;
  m_pDecoderXMO = NULL;
}
CCdgChatter::~CCdgChatter()
{
  Shutdown();
}
HRESULT CCdgChatter::Initialize(CCdgVoiceManager* pManager, DWORD dwPort, CDG_DEVICE_TYPE device)
{
  CSingleLock lock (m_CritSection);
  m_pVoiceManager = pManager;
  m_dwPort = dwPort;
  m_DeviceType = device;
  HRESULT hr = S_OK;

  LoadSettings();

  // Allocate space to store completed packets until game retrieves them
  if ( !m_pbCompletedPackets)
    m_pbCompletedPackets = new BYTE[ m_pVoiceManager->m_cfg.dwMaxStoredPackets * m_dwPacketSize ];
  if ( !m_pbCompletedPackets ) return E_OUTOFMEMORY;

  if ( ! m_pbTempBuffer)
    m_pbTempBuffer = new BYTE[ m_dwPacketSize];
  if ( ! m_pbTempBuffer) return E_OUTOFMEMORY;

  if (m_pVoiceMask)
  {
    if ( ! m_pbCompTempBuffer)
      m_pbCompTempBuffer = new BYTE[ m_dwCompressedSize ];
    if ( ! m_pbCompTempBuffer) return E_OUTOFMEMORY;
  }

  if (!m_pbMicrophoneBuffer)
    m_pbMicrophoneBuffer = new BYTE[ m_dwPacketSize * m_pVoiceManager->m_dwNumPackets ];
  if ( !m_pbMicrophoneBuffer ) return E_OUTOFMEMORY;
  if (m_pVoiceMask)
  {
    // Create the encoder
    if ( FAILED( XVoiceCreateOneToOneEncoder( &m_pEncoderXMO ) ) )
      return E_OUTOFMEMORY;
    // Create the Decoder
    if ( FAILED( XVoiceCreateOneToOneDecoder( &m_pDecoderXMO ) ) )
      return E_OUTOFMEMORY;
    if (m_pEncoderXMO)
      m_pEncoderXMO->SetVoiceMask(0, m_pVoiceMask);
  }

  // Create the microphone device
  PXPP_DEVICE_TYPE ppDevice;
  if (m_DeviceType == HIFI_MICROPHONE)
    ppDevice = XDEVICE_TYPE_HIGHFIDELITY_MICROPHONE;
  else if (m_DeviceType == VOICE_COMMUNICATOR)
    ppDevice = XDEVICE_TYPE_VOICE_MICROPHONE;

  hr = XVoiceCreateMediaObjectEx( ppDevice, m_dwPort,
                                  m_pVoiceManager->m_dwNumPackets, &m_wfx,
                                  CdgMicrophoneCallback, this, &m_pMicrophoneXMO );
  if ( FAILED( hr ) ) return E_FAIL;

  // To reset the microphone, we just need to submit all our packets
  XMEDIAPACKET xmp;
  for ( DWORD i = 0; i < m_pVoiceManager->m_dwNumPackets; i++ )
  {
    GetMicrophonePacket( &xmp, i );
    // This will only fail if the communicator has been removed
    if ( FAILED( SubmitMicrophonePacket( &xmp ) ) )
    {
      return E_FAIL;
    }
  }
  // Set up the voice stream
  DSMIXBINVOLUMEPAIR dsmbvp = { DSMIXBIN_FRONT_CENTER, DSBVOLUME_MAX };
  DSMIXBINS dsmb = { 1, &dsmbvp};
  DSSTREAMDESC dssd = {0};
  dssd.dwMaxAttachedPackets = m_pVoiceManager->m_dwNumPackets;
  dssd.lpwfxFormat = &m_wfx;
  dssd.lpMixBins = &dsmb;
  dssd.dwFlags = DSSTREAMCAPS_ACCURATENOTIFY;
  dssd.lpfnCallback = CdgStreamCallback;
  dssd.lpvContext = (void*) this;

  // Create a stream with the specified context
  hr = DirectSoundCreateStream( &dssd, & m_pOutputStream );
  if ( FAILED( hr ) ) return hr;
  // Set the stream headroom to 0
  m_pOutputStream->SetHeadroom( 0 );
  m_pOutputStream->SetVolume(m_lVolume);

  // Allocate buffer for stream data
  if ( ! m_pbStreamBuffer )
    m_pbStreamBuffer = new BYTE[ m_dwPacketSize * m_pVoiceManager->m_dwNumPackets];
  if ( ! m_pbStreamBuffer ) return E_OUTOFMEMORY;

  // Allocate drift compensation buffer and fill with silence
  if ( ! m_pbDriftBuffer )
    m_pbDriftBuffer = new BYTE[ m_dwPacketSize ];
  if ( ! m_pbDriftBuffer ) return E_OUTOFMEMORY;
  ZeroMemory( m_pbDriftBuffer, m_dwPacketSize);

  // Start the stream with silence from the drift compensation buffer
  m_dwTempStatus = XMEDIAPACKET_STATUS_PENDING;
  for ( DWORD i = 0; i < m_pVoiceManager->m_dwNumPackets; i++ )
    SubmitStreamPacket(i);
  return S_OK;
}
void CCdgChatter::Shutdown()
{
  CSingleLock lock (m_CritSection);
  if ( m_pOutputStream )
  {
    m_pOutputStream->Flush();
    m_pOutputStream->Pause( DSSTREAMPAUSE_PAUSE );
    DWORD dwStatus;
    do {m_pOutputStream->GetStatus(&dwStatus); }
    while (dwStatus == DSSTREAMSTATUS_PLAYING);
    m_pOutputStream->Release();
  }
  m_pOutputStream = NULL;
  if ( m_pMicrophoneXMO )
    m_pMicrophoneXMO->Release();
  m_pMicrophoneXMO = NULL;
  if ( m_pbMicrophoneBuffer )
    delete[] m_pbMicrophoneBuffer;
  m_pbMicrophoneBuffer = NULL;
  if ( m_pEncoderXMO )
    m_pEncoderXMO->Release();
  m_pEncoderXMO = NULL;
  if ( m_pDecoderXMO )
    m_pDecoderXMO->Release();
  m_pDecoderXMO = NULL;
  if ( m_pVoiceMask )
    delete m_pVoiceMask;
  m_pVoiceMask = NULL;
  if ( m_pbCompletedPackets )
    delete[] m_pbCompletedPackets;
  m_pbCompletedPackets = NULL;
  if ( m_pbTempBuffer)
    delete[] m_pbTempBuffer;
  m_pbTempBuffer = NULL;
  if ( m_pbCompTempBuffer)
    delete[] m_pbCompTempBuffer;
  m_pbCompTempBuffer = NULL;
  if ( m_pbStreamBuffer )
    delete[] m_pbStreamBuffer;
  m_pbStreamBuffer = NULL;
  if ( m_pbDriftBuffer )
    delete[] m_pbDriftBuffer;
  m_pbDriftBuffer = NULL;
}
void CCdgChatter::LoadSettings()
{
  // Get the Max volume
  CStdString strSetting = "Karaoke.Volume";
  int iPercent = g_guiSettings.GetInt(strSetting);
  if (iPercent < 0) iPercent = 0;
  if (iPercent > 100) iPercent = 100;
  float fHardwareVolume = ((float)iPercent) / 100.0f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM;
  m_lVolume = (long)fHardwareVolume;
  //Load the voice mask
  strSetting.Format("Karaoke.Port%iVoiceMask", m_dwPort);
  strSetting = g_guiSettings.GetString(strSetting);
  if (strSetting.CompareNoCase("None") == 0)
  {
    if (m_pVoiceMask)
      delete m_pVoiceMask;
    m_pVoiceMask = NULL;
  }
  else
  {
    if (!m_pVoiceMask)
      m_pVoiceMask = new XVOICE_MASK;
    if (!m_pVoiceMask) return ;
    memcpy(m_pVoiceMask, &g_stSettings.m_karaokeVoiceMask[m_dwPort], sizeof(XVOICE_MASK));
  }
  //Calculate other useful constants
  DWORD dwSamplingRate, dwVoicePacketTime;
  if ( !m_pVoiceMask && m_DeviceType == HIFI_MICROPHONE)
    dwSamplingRate = m_pVoiceManager->m_dwHiFiSamplingRate;
  else
    dwSamplingRate = m_pVoiceManager->m_dwSamplingRate;
  dwVoicePacketTime = m_pVoiceManager->m_cfg.dwVoicePacketTime;
  m_dwPacketSize = ( dwVoicePacketTime * dwSamplingRate / 1000 ) * 2;
  m_dwCompressedSize = dwVoicePacketTime * 8 / 20 + 2;  // The +2 comes from the encoder
  m_wfx.cbSize = 0;
  m_wfx.nChannels = 1;
  m_wfx.nSamplesPerSec = dwSamplingRate;
  m_wfx.wBitsPerSample = 16;
  m_wfx.nBlockAlign = m_wfx.wBitsPerSample / 8 * m_wfx.nChannels;
  m_wfx.nAvgBytesPerSec = m_wfx.nSamplesPerSec * m_wfx.nBlockAlign;
  m_wfx.wFormatTag = WAVE_FORMAT_PCM;
}

void CCdgChatter::SetVolume(long lVol)
{
  CSingleLock lock (m_CritSection);
  m_lVolume = lVol;
  if (m_pOutputStream)
    m_pOutputStream->SetVolume(m_lVolume);
}
HRESULT CCdgChatter::ProcessVoice(PFNCDGVOICEDATACALLBACK pfnVoiceDataCallback, VOID* pCallbackContext)
{
  CSingleLock lock (m_CritSection);
  if ( !pfnVoiceDataCallback || !m_pbCompletedPackets) return S_OK;
  // Fire callbacks for all buffered packets
  for ( DWORD j = 0; j < m_dwNumCompletedPackets; j++ )
  {
    BYTE* pPacket = (BYTE*)( m_pbCompletedPackets + j * m_dwPacketSize );
    pfnVoiceDataCallback( m_dwPort, m_dwPacketSize, pPacket, pCallbackContext );
  }
  m_dwNumCompletedPackets = 0;
  return S_OK;
}
HRESULT CCdgChatter::GetDriftCompensationPacket(XMEDIAPACKET* pPacket)
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbDriftBuffer;
  pPacket->dwMaxSize = m_dwPacketSize;
  return S_OK;
}



HRESULT CCdgChatter::GetTemporaryPacket( XMEDIAPACKET* pPacket )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbTempBuffer;
  pPacket->dwMaxSize = m_dwPacketSize;
  pPacket->pdwStatus = &m_dwTempStatus;
  return S_OK;
}



HRESULT CCdgChatter::GetCompTemporaryPacket( XMEDIAPACKET* pPacket )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbCompTempBuffer;
  pPacket->dwMaxSize = m_dwCompressedSize;
  return S_OK;
}



HRESULT CCdgChatter::GetStreamPacket( XMEDIAPACKET* pPacket, DWORD dwIndex )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbStreamBuffer + dwIndex * m_dwPacketSize;
  pPacket->dwMaxSize = m_dwPacketSize;
  pPacket->pContext = (LPVOID)dwIndex;
  ZeroMemory( pPacket->pvBuffer, pPacket->dwMaxSize );
  return S_OK;
}



HRESULT CCdgChatter::GetMicrophonePacket( XMEDIAPACKET* pPacket, DWORD dwIndex )
{
  ZeroMemory( pPacket, sizeof( XMEDIAPACKET ) );
  pPacket->pvBuffer = m_pbMicrophoneBuffer + dwIndex * m_dwPacketSize;
  pPacket->dwMaxSize = m_dwPacketSize;
  pPacket->pContext = (LPVOID)dwIndex;
  return S_OK;
}



inline
HRESULT CCdgChatter::SubmitStreamPacket(DWORD dwIndex )
{
  XMEDIAPACKET xmp;
  GetStreamPacket(&xmp, dwIndex );
  memcpy( xmp.pvBuffer, m_pbDriftBuffer, m_dwPacketSize );
  return m_pOutputStream->Process( &xmp, NULL );
}



inline
HRESULT CCdgChatter::SubmitMicrophonePacket( XMEDIAPACKET* pPacket )
{
  return m_pMicrophoneXMO->Process( NULL, pPacket );
}



HRESULT CCdgChatter::MicrophonePacketCallback( LPVOID pPacketContext, DWORD dwStatus )
{
  // If the packet failed or was flushed, it means that we either released
  // the device, or the device was removed.
  if ( dwStatus != XMEDIAPACKET_STATUS_SUCCESS )
  {
    assert( dwStatus == XMEDIAPACKET_STATUS_FAILURE ||
            dwStatus == XMEDIAPACKET_STATUS_FLUSHED );

    return S_FALSE;
  }
  // Grab the completed microphone packet
  XMEDIAPACKET xmpMicrophone, xmpTemp;
  GetMicrophonePacket( &xmpMicrophone, (DWORD)pPacketContext );
  GetTemporaryPacket(&xmpTemp);
  if ( *xmpTemp.pdwStatus == XMEDIAPACKET_STATUS_PENDING)
  {
    if (! m_pVoiceMask)  //Do not Encode
    {
      // Detect silence
      short high = 0;
      int frameSize = m_dwPacketSize / 2;
      short* pSample = (short*)xmpMicrophone.pvBuffer;
      for (int i = 0; i < frameSize; i++)
      {
        if (pSample[i] > high)
          high = pSample[i];
      }
      if (high < 100)
        ZeroMemory(xmpTemp.pvBuffer, m_dwPacketSize);
      else
      {
        memcpy(xmpTemp.pvBuffer, xmpMicrophone.pvBuffer, m_dwPacketSize);
        OnCompletedPacket( xmpTemp.pvBuffer, m_dwPacketSize );
      }
    }
    else  //Encode
    {
      // Save floating point state
      XSaveFloatingPointStateForDpc();
      XMEDIAPACKET xmpCompTemp;
      GetCompTemporaryPacket(&xmpCompTemp);
      // Run the encoder
      DWORD dwCompressedSize;
      xmpCompTemp.pdwCompletedSize = & dwCompressedSize;
      HRESULT hr = m_pEncoderXMO->ProcessMultiple( 1, &xmpMicrophone, 1, &xmpCompTemp );
      assert( SUCCEEDED( hr ) );
      assert( dwCompressedSize == m_dwCompressedSize || dwCompressedSize == 0 );
      // If the compressed size is greater than zero, that means we detected
      // voice.  Otherwise, it was silence
      if ( dwCompressedSize > 0 )
      {
        hr = m_pDecoderXMO->ProcessMultiple( 1, &xmpCompTemp, 1, &xmpTemp );
        assert( SUCCEEDED( hr ) );
        OnCompletedPacket( xmpTemp.pvBuffer, m_dwPacketSize );
      }
      else
        ZeroMemory(xmpTemp.pvBuffer, m_dwPacketSize);
      // Restore floating point state
      XRestoreFloatingPointStateForDpc();
    }
    *xmpTemp.pdwStatus = XMEDIAPACKET_STATUS_SUCCESS;
  }
  // Re-submit the packet back to the microphone
  if ( FAILED( SubmitMicrophonePacket( &xmpMicrophone ) ) )
  {
    // This will only happen if the device has been removed,
    // in which case we'll pick it up on the next call to
    // CheckDeviceChanges
  }
  return S_OK;
}



HRESULT CCdgChatter::StreamPacketCallback( LPVOID pPacketContext, DWORD dwStatus )
{
  if ( dwStatus != XMEDIAPACKET_STATUS_SUCCESS )
  {
    // Streams don't fail
    assert( dwStatus == XMEDIAPACKET_STATUS_FLUSHED );
    return S_FALSE;
  }
  // Get the drift compensation packet
  XMEDIAPACKET xmpDriftPacket, xmpTemp;
  GetTemporaryPacket( &xmpTemp);
  GetDriftCompensationPacket( &xmpDriftPacket);
  if ( *xmpTemp.pdwStatus == XMEDIAPACKET_STATUS_SUCCESS)
  {
    memcpy(xmpDriftPacket.pvBuffer, xmpTemp.pvBuffer, m_dwPacketSize);
    *xmpTemp.pdwStatus = XMEDIAPACKET_STATUS_PENDING;
  }
  if ( FAILED( SubmitStreamPacket( (DWORD)pPacketContext ) ) )
  {
    // The only way this would fail is if we submitted too many packets
    assert( FALSE );
  }
  return S_OK;
}



HRESULT CCdgChatter::OnCompletedPacket( VOID* pvData, DWORD dwSize )
{
  // Store the completed packet until the game retrieves it
  if ( m_dwNumCompletedPackets < m_pVoiceManager->m_cfg.dwMaxStoredPackets )
  {
    BYTE* pPacket = (BYTE *)( m_pbCompletedPackets + m_dwNumCompletedPackets * m_dwPacketSize );
    memcpy( pPacket, pvData, dwSize);
    m_dwNumCompletedPackets++;
  }
  return S_OK;
}
VOID CALLBACK CdgStreamCallback( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus )
{
  CCdgChatter* pChatter = (CCdgChatter*)pStreamContext;
  pChatter->StreamPacketCallback( pPacketContext, dwStatus );
}



VOID CALLBACK CdgMicrophoneCallback( LPVOID pContext, LPVOID pPacketContext, DWORD dwStatus )
{
  CCdgChatter* pChatter = (CCdgChatter*)pContext;
  pChatter->MicrophonePacketCallback( pPacketContext, dwStatus );
}


//CdgVoiceManager
CCdgVoiceManager::CCdgVoiceManager()
{
  ZeroMemory( &m_cfg, sizeof( CDG_VOICE_MANAGER_CONFIG ) );
  ZeroMemory( m_bEnabled, XGetPortCount() *sizeof(bool) );
  // Set up communicators
  m_dwConnectedCommunicators = 0;
  m_dwConnectedHiFiMicrophones = 0;
  m_dwMicrophoneState = 0;
  m_dwHiFiMicrophoneState = 0;
  m_dwSamplingRate = 8000;
  m_dwHiFiSamplingRate = 48000;
  // With everything running at DPC, we only need 2 packets to ping-pong
  m_dwNumPackets = 2;
}




CCdgVoiceManager::~CCdgVoiceManager()
{
  Shutdown();
}


void CCdgVoiceManager::Initialize( CDG_VOICE_MANAGER_CONFIG* pConfig )
{
  // Voice packets must be a multiple of 20ms
  assert( pConfig->dwVoicePacketTime % 20 == 0 );
  // Grab the config parameters
  memcpy( &m_cfg, pConfig, sizeof( CDG_VOICE_MANAGER_CONFIG ) );

  //Update the starting voice device states
  DWORD dwComHeadMask = XGetDevices(XDEVICE_TYPE_VOICE_HEADPHONE);
  DWORD dwComMikeMask = XGetDevices(XDEVICE_TYPE_VOICE_MICROPHONE);
  DWORD dwHiFiMikeMask = XGetDevices(XDEVICE_TYPE_HIGHFIDELITY_MICROPHONE);

  for (int i = 0; i < XGetPortCount(); i++)
  {
    if ( ( (dwComHeadMask & (1 << i)) && (dwComMikeMask & (1 << i)) )
         || ( (dwComHeadMask & (1 << (i + 16))) && (dwComMikeMask & (1 << (i + 16))) ) )
      OnVoiceDeviceInserted(i, VOICE_COMMUNICATOR);
    else if ( dwHiFiMikeMask & (1 << i) || dwHiFiMikeMask & (1 << (i + 16)) )
      OnVoiceDeviceInserted(i, HIFI_MICROPHONE);
  }
}
void CCdgVoiceManager::Shutdown()
{
  // Tear down our chatters
  for ( DWORD i = 0; i < XGetPortCount(); i++ )
    m_Chatters[i].Shutdown();
}
HRESULT CCdgVoiceManager::CheckDeviceChanges()
{
  DWORD dwMicrophoneInsertions;
  DWORD dwMicrophoneRemovals;
  DWORD dwHeadphoneInsertions;
  DWORD dwHeadphoneRemovals;
  DWORD dwHiFiMicrophoneInsertions;
  DWORD dwHiFiMicrophoneRemovals;

  // Must call XGetDevice changes to track possible removal and insertion
  // in one frame
  XGetDeviceChanges( XDEVICE_TYPE_VOICE_MICROPHONE,
                     &dwMicrophoneInsertions,
                     &dwMicrophoneRemovals );
  XGetDeviceChanges( XDEVICE_TYPE_VOICE_HEADPHONE,
                     &dwHeadphoneInsertions,
                     &dwHeadphoneRemovals );
  XGetDeviceChanges( XDEVICE_TYPE_HIGHFIDELITY_MICROPHONE,
                     &dwHiFiMicrophoneInsertions,
                     &dwHiFiMicrophoneRemovals );

  // Update state for removals
  m_dwMicrophoneState &= ~( dwMicrophoneRemovals );
  m_dwHeadphoneState &= ~( dwHeadphoneRemovals );
  m_dwHiFiMicrophoneState &= ~( dwHiFiMicrophoneRemovals );

  // Then update state for new insertions
  m_dwMicrophoneState |= ( dwMicrophoneInsertions );
  m_dwHeadphoneState |= ( dwHeadphoneInsertions );
  m_dwHiFiMicrophoneState |= ( dwHiFiMicrophoneInsertions );

  for ( WORD i = 0; i < XGetPortCount(); i++ )
  {
    // If either the microphone or the headphone was
    // removed since last call, remove the communicator
    if ( m_dwConnectedCommunicators & ( 1 << i ) && ( ( dwMicrophoneRemovals & ( 1 << i ) ) || ( dwHeadphoneRemovals & ( 1 << i ) ) ) )
      OnVoiceDeviceRemoved( i, VOICE_COMMUNICATOR );
    // If both microphone and headphone are present, and
    // we didn't have a communicator here last frame, and
    // the communicator is enabled, then register the insertion
    if ( ( m_dwMicrophoneState & ( 1 << i ) ) && ( m_dwHeadphoneState & ( 1 << i ) ) &&
         !( m_dwConnectedCommunicators & ( 1 << i ) ) )
      OnVoiceDeviceInserted( i, VOICE_COMMUNICATOR );
    // If  the HiFi microphone was removed since last call, remove it
    if ( ( m_dwConnectedHiFiMicrophones & ( 1 << i )) && ( dwHiFiMicrophoneRemovals & ( 1 << i ) ) )
      OnVoiceDeviceRemoved( i, HIFI_MICROPHONE );
    // If the HiFi microphone is present, and
    // we didn't have a HiFi microphone here last frame, and
    // the microphone is enabled, then register the insertion
    if ( ( m_dwHiFiMicrophoneState & ( 1 << i ) ) && !( m_dwConnectedHiFiMicrophones & ( 1 << i ) ) )
      OnVoiceDeviceInserted( i, HIFI_MICROPHONE );
  }
  return S_OK;
}



HRESULT CCdgVoiceManager::OnVoiceDeviceInserted( DWORD dwPort, CDG_DEVICE_TYPE DeviceType )
{
  if (dwPort < 0 || dwPort >= XGetPortCount() ) return E_FAIL;
//  if ( !m_bEnabled[dwPort] ) return S_OK;

  HRESULT hr = m_Chatters[dwPort].Initialize(this, dwPort, DeviceType);
  if ( FAILED(hr) ) goto Cleanup;
  // Now that everything has succeeded, we can mark the device
  // as being present.
  if (DeviceType == VOICE_COMMUNICATOR)
    m_dwConnectedCommunicators |= ( 1 << dwPort );
  else if (DeviceType == HIFI_MICROPHONE)
    m_dwConnectedHiFiMicrophones |= ( 1 << dwPort );
  // Notify the title that the device was inserted
  if ( m_cfg.pfnVoiceDeviceCallback)
    m_cfg.pfnVoiceDeviceCallback( dwPort, DeviceType, CDG_VOICE_DEVICE_INSERTED, m_cfg.pCallbackContext );

Cleanup:
  if ( FAILED( hr ) )
  {
    // This could happen if the communicator gets removed immediately
    // after being inserted, and should be considered a normal scenario
    //m_aVoiceCommunicators[ dwPort ].OnRemoval();
    m_Chatters[dwPort].Shutdown();
  }
  return hr;
}



HRESULT CCdgVoiceManager::OnVoiceDeviceRemoved( DWORD dwPort, CDG_DEVICE_TYPE DeviceType )
{
  if (dwPort < 0 || dwPort >= XGetPortCount() ) return E_FAIL;
  // Mark the device as having been removed
  if (DeviceType == VOICE_COMMUNICATOR)
    m_dwConnectedCommunicators &= ~( 1 << dwPort );
  else if (DeviceType == HIFI_MICROPHONE)
    m_dwConnectedHiFiMicrophones &= ~( 1 << dwPort );
  m_Chatters[dwPort].Shutdown();
  // Notify the title that the device was removed
  if ( m_cfg.pfnVoiceDeviceCallback)
    m_cfg.pfnVoiceDeviceCallback( dwPort, DeviceType , CDG_VOICE_DEVICE_REMOVED, m_cfg.pCallbackContext );
  return S_OK;
}

HRESULT CCdgVoiceManager::EnableVoiceDevice( DWORD dwPort, bool bEnabled )
{
  if (dwPort < 0 || dwPort >= XGetPortCount() ) return E_FAIL;
  // All we need to do is set the flag - if a communicator is currently
  // plugged in, it will be picked up in the next call to
  // CheckDeviceChanges
/*  m_bEnabled[dwPort] = bEnabled;
  if ( !bEnabled )
  {
    // Pretend the communicator was removed.  Having the enabled flag
    // cleared will prevent it from being re-added in CheckDeviceChanges
    if ( m_dwConnectedCommunicators & ( 1 << dwPort ) )
      OnVoiceDeviceRemoved( dwPort, VOICE_COMMUNICATOR );
    else
      if ( m_dwConnectedHiFiMicrophones & ( 1 << dwPort ) )
        OnVoiceDeviceRemoved( dwPort, HIFI_MICROPHONE );
  }*/
  return S_OK;
}


void CCdgVoiceManager::SetVolume(DWORD dwPort, int iPercent)
{
  // convert the percentage to a mB (milliBell) value (*100 for dB)
  if (iPercent < 0) iPercent = 0;
  if (iPercent > 100) iPercent = 100;
  float fHardwareVolume = ((float)iPercent) / 100.0f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM;
  m_Chatters[dwPort].SetVolume((long) fHardwareVolume);
}
HRESULT CCdgVoiceManager::ProcessVoice()
{
  // Fire callbacks for all chatters
  if ( m_cfg.pfnVoiceDataCallback )
    for (DWORD i = 0; i < XGetPortCount(); i++)
      m_Chatters[i].ProcessVoice(m_cfg.pfnVoiceDataCallback, m_cfg.pCallbackContext);
  // Check for insertions and removals
  CheckDeviceChanges();
  return S_OK;
}


