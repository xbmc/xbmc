#include "stdafx.h"
#include "GUIVisualisationControl.h"
#include "../xbmc/GUIUserMessages.h"
#include "../xbmc/application.h"
#include "../xbmc/util.h"
#include "../xbmc/visualizations/VisualisationFactory.h"
#include "../xbmc/visualizations/fft.h"

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CAudioBuffer::CAudioBuffer(int iSize)
{
  m_iLen = iSize;
  m_pBuffer = new short[iSize];
}

CAudioBuffer::~CAudioBuffer()
{
  delete [] m_pBuffer;
}

const short* CAudioBuffer::Get() const
{
  return m_pBuffer;
}

void CAudioBuffer::Set(const unsigned char* psBuffer, int iSize, int iBitsPerSample)
{
  if (iBitsPerSample == 16)
  {
    iSize /= 2;
    for (int i = 0; i < iSize, i < m_iLen; i++)
    { // 16 bit -> convert to short directly
      m_pBuffer[i] = ((short *)psBuffer)[i];
    }
  }
  else if (iBitsPerSample == 8)
  {
    for (int i = 0; i < iSize, i < m_iLen; i++)
    { // 8 bit -> convert to signed short by multiplying by 256
      m_pBuffer[i] = ((short)((char *)psBuffer)[i]) << 8;
    }
  }
  else // assume 24 bit data
  {
    iSize /= 3;
    for (int i = 0; i < iSize, i < m_iLen; i++)
    { // 24 bit -> ignore least significant byte and convert to signed short
      m_pBuffer[i] = (((int)psBuffer[3 * i + 1]) << 0) + (((int)((char *)psBuffer)[3 * i + 2]) << 8);
    }
  }
  for (int i = iSize; i < m_iLen;++i) m_pBuffer[i] = 0;
}

CGUIVisualisationControl::CGUIVisualisationControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_pVisualisation = NULL;
  m_iNumBuffers = 0;
  m_currentVis = "";
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::~CGUIVisualisationControl(void)
{
}

void CGUIVisualisationControl::FreeVisualisation()
{
  // tell our app that we're going
  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, 0, 0);
  g_graphicsContext.SendMessage(msg);

  CSingleLock lock (m_critSection);
  CLog::Log(LOGDEBUG, "FreeVisualisation() started");
  m_bInitialized = false;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->UnRegisterAudioCallback();
  if (m_pVisualisation)
  {
    OutputDebugString("Visualisation::Stop()\n");
    m_pVisualisation->Stop();

    OutputDebugString("delete Visualisation()\n");
    delete m_pVisualisation;

    g_graphicsContext.ApplyStateBlock();
  }
  m_pVisualisation = NULL;
  ClearBuffers();
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

void CGUIVisualisationControl::LoadVisualisation()
{
  CLog::Log(LOGDEBUG, "LoadVisualisation() started");
  CSingleLock lock (m_critSection);
  if (m_pVisualisation)
  {
    m_pVisualisation->Stop();
    delete m_pVisualisation;
    g_graphicsContext.ApplyStateBlock();
  }
  m_pVisualisation = NULL;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->UnRegisterAudioCallback();

  m_bInitialized = false;
  CVisualisationFactory factory;
  CStdString strVisz;
  OutputDebugString("Load Visualisation\n");
  m_currentVis = g_guiSettings.GetString("MyMusic.Visualisation");
  if (m_currentVis.Equals("None"))
    return;
  strVisz.Format("Q:\\visualisations\\%s", m_currentVis.c_str());
  m_pVisualisation = factory.LoadVisualisation(strVisz.c_str());
  if (m_pVisualisation)
  {
    OutputDebugString("Visualisation::Create()\n");
    g_graphicsContext.CaptureStateBlock();
    m_pVisualisation->Create(GetXPosition(), GetYPosition(), GetWidth(), GetHeight());
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->RegisterAudioCallback(this);

    // Create new audio buffers
    CreateBuffers();
  }
  CLog::Log(LOGDEBUG, "LoadVisualisation() done");

  // tell our app that we're back
  CGUIMessage msg(GUI_MSG_VISUALISATION_LOADED, 0, 0, 0, 0, m_pVisualisation);
  g_graphicsContext.SendMessage(msg);
}

void CGUIVisualisationControl::Render()
{
  if (!UpdateVisibility())
  {
    if (m_bInitialized)
      FreeVisualisation();
    return;
  }
  if (!m_bInitialized)
  { // check if we need to load
    if (g_application.IsPlayingAudio())
    {
      LoadVisualisation();
    }
    return;
  }
  else
  { // check if we need to unload
    if (!g_application.IsPlayingAudio())
    { // deinit
      FreeVisualisation();
      return;
    }
    else if (!m_currentVis.Equals(g_guiSettings.GetString("MyMusic.Visualisation")))
    { // vis changed - reload
      LoadVisualisation();
      return;
    }
  }
  CSingleLock lock (m_critSection);
  if (m_pVisualisation)
  {
    if (m_bInitialized)
    {
      // set the viewport
      g_graphicsContext.SetViewPort((float)m_iPosX, (float)m_iPosY, (float)m_dwWidth, (float)m_dwHeight);
      try
      {
        m_pVisualisation->Render();
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "Exception in Visualisation::Render()");
      }
      // clear the viewport
      g_graphicsContext.RestoreViewPort();
    }
  }
}


void CGUIVisualisationControl::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  CSingleLock lock (m_critSection);
  if (!m_pVisualisation)
    return ;
  CLog::Log(LOGDEBUG, "OnInitialize() started");

  m_bInitialized = true;
  m_iChannels = iChannels;
  m_iSamplesPerSec = iSamplesPerSec;
  m_iBitsPerSample = iBitsPerSample;

  // Start the visualisation (this loads settings etc.)
  CStdString strFile = CUtil::GetFileName(g_application.CurrentFile());
  OutputDebugString("Visualisation::Start()\n");
  m_pVisualisation->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile);
  m_bInitialized = true;
  CLog::Log(LOGDEBUG, "OnInitialize() done");
}

void CGUIVisualisationControl::OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)
{
  CSingleLock lock (m_critSection);
  if (!m_pVisualisation)
    return ;
  if (!m_bInitialized) return ;

  // Save our audio data in the buffers
  auto_ptr<CAudioBuffer> pBuffer ( new CAudioBuffer(2*AUDIO_BUFFER_SIZE) );
  pBuffer->Set(pAudioData, iAudioDataLength, m_iBitsPerSample);
  m_vecBuffers.push_back( pBuffer.release() );

  if ( (int)m_vecBuffers.size() < m_iNumBuffers) return ;

  auto_ptr<CAudioBuffer> ptrAudioBuffer ( m_vecBuffers.front() );
  m_vecBuffers.pop_front();
  // Fourier transform the data if the vis wants it...
  if (m_bWantsFreq)
  {
    // Convert to floats
    const short* psAudioData = ptrAudioBuffer->Get();
    for (int i = 0; i < 2*AUDIO_BUFFER_SIZE; i++)
    {
      m_fFreq[i] = (float)psAudioData[i];
    }

    // FFT the data
    twochanwithwindow(m_fFreq, AUDIO_BUFFER_SIZE);

    // Normalize the data
    float fMinData = (float)AUDIO_BUFFER_SIZE * AUDIO_BUFFER_SIZE * 3 / 8 * 0.5 * 0.5; // 3/8 for the Hann window, 0.5 as minimum amplitude
    float fInvMinData = 1.0f/fMinData;
    for (int i = 0; i < AUDIO_BUFFER_SIZE + 2; i++)
    {
      m_fFreq[i] *= fInvMinData;
    }

    // Transfer data to our visualisation
    try
    {
      m_pVisualisation->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, m_fFreq, AUDIO_BUFFER_SIZE);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception in Visualisation::AudioData()");
    }
  }
  else
  { // Transfer data to our visualisation
    try
    {
      m_pVisualisation->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, NULL, 0);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception in Visualisation::AudioData()");
    }
  }
  return ;
}

bool CGUIVisualisationControl::OnAction(const CAction &action)
{
  if (!m_pVisualisation) return false;
  enum CVisualisation::VIS_ACTION visAction = CVisualisation::VIS_ACTION_NONE;
  if (action.wID == ACTION_VIS_PRESET_NEXT)
    visAction = CVisualisation::VIS_ACTION_NEXT_PRESET;
  else if (action.wID == ACTION_VIS_PRESET_PREV)
    visAction = CVisualisation::VIS_ACTION_PREV_PRESET;
  else if (action.wID == ACTION_VIS_PRESET_LOCK)
    visAction = CVisualisation::VIS_ACTION_LOCK_PRESET;
  else if (action.wID == ACTION_VIS_PRESET_RANDOM)
    visAction = CVisualisation::VIS_ACTION_RANDOM_PRESET;
  else if (action.wID == ACTION_VIS_RATE_PRESET_PLUS)
    visAction = CVisualisation::VIS_ACTION_RATE_PRESET_PLUS;
  else if (action.wID == ACTION_VIS_RATE_PRESET_MINUS)
    visAction = CVisualisation::VIS_ACTION_RATE_PRESET_MINUS;

  return m_pVisualisation->OnAction(visAction);
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_GET_VISUALISATION)
  {
    message.SetLPVOID(m_pVisualisation);
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_VISUALISATION_ACTION)
  {
    CAction action;
    action.wID = (WORD)message.GetParam1();
    return OnAction(action);
  }
  return CGUIControl::OnMessage(message);
}

void CGUIVisualisationControl::CreateBuffers()
{
  CSingleLock lock (m_critSection);
  ClearBuffers();

  // Get the number of buffers from the current vis
  VIS_INFO info;
  m_pVisualisation->GetInfo(&info);
  m_iNumBuffers = info.iSyncDelay + 1;
  m_bWantsFreq = info.bWantsFreq;
  if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
    m_iNumBuffers = MAX_AUDIO_BUFFERS;

  if (m_iNumBuffers < 1)
    m_iNumBuffers = 1;
}


void CGUIVisualisationControl::ClearBuffers()
{
  CSingleLock lock (m_critSection);
  m_bWantsFreq = false;
  m_iNumBuffers = 0;

  while (m_vecBuffers.size() > 0)
  {
    CAudioBuffer* pAudioBuffer = m_vecBuffers.front();
    delete pAudioBuffer;
    m_vecBuffers.pop_front();
  }
  for (int j = 0; j < AUDIO_BUFFER_SIZE*2; j++)
  {
    m_fFreq[j] = 0.0f;
  }
}

void CGUIVisualisationControl::FreeResources()
{
  FreeVisualisation();
  CGUIControl::FreeResources();
}
