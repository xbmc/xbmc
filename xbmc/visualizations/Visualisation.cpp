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
#include "system.h"
#include "Visualisation.h"
#include "fft.h"
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "MusicInfoTag.h"
#include "Settings.h"
#include "WindowingFactory.h"
#include "Util.h"
#ifdef _LINUX
#include <dlfcn.h>
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/File.h"
#endif
#include "../utils/AddonHelpers.h"

using namespace std;
using namespace MUSIC_INFO;
using namespace ADDON;

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
  if (iSize<0)
  {
    return;
  }

  if (iBitsPerSample == 16)
  {
    iSize /= 2;
    for (int i = 0; i < iSize && i < m_iLen; i++)
    { // 16 bit -> convert to short directly
      m_pBuffer[i] = ((short *)psBuffer)[i];
    }
  }
  else if (iBitsPerSample == 8)
  {
    for (int i = 0; i < iSize && i < m_iLen; i++)
    { // 8 bit -> convert to signed short by multiplying by 256
      m_pBuffer[i] = ((short)((char *)psBuffer)[i]) << 8;
    }
  }
  else // assume 24 bit data
  {
    iSize /= 3;
    for (int i = 0; i < iSize && i < m_iLen; i++)
    { // 24 bit -> ignore least significant byte and convert to signed short
      m_pBuffer[i] = (((int)psBuffer[3 * i + 1]) << 0) + (((int)((char *)psBuffer)[3 * i + 2]) << 8);
    }
  }
  for (int i = iSize; i < m_iLen;++i) m_pBuffer[i] = 0;
}

bool CVisualisation::Create(int x, int y, int w, int h)
{
  /* Allocate the callback table to save all the pointers
     to the helper callback functions */
  m_callbacks = new AddonCB;

  /* PVR Helper functions */
  m_callbacks->userData     = this;
  m_callbacks->addonData    = (CAddon*) this;

  /* Write XBMC Global Add-on function addresses to callback table */
  CAddonUtils::CreateAddOnCallbacks(m_callbacks);

  m_pInfo = new VIS_PROPS;
  m_pInfo->hdl = m_callbacks;
  m_pInfo->x = x;
  m_pInfo->y = y;
  m_pInfo->width = w;
  m_pInfo->height = h;
  m_pInfo->pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;

  m_pInfo->name = strdup(Name().c_str());
  m_pInfo->presets = strdup(_P(Path()).c_str());
  m_pInfo->profile = strdup(_P(Profile()).c_str());

  if (CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>::Create())
  {
    // Start the visualisation
    CStdString strFile = CUtil::GetFileName(g_application.CurrentFile());
    CLog::Log(LOGDEBUG, "Visualisation::Start()\n");
    try
    {
      m_pStruct->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile);
    } catch (std::exception e)
    {
      CLog::Log(LOGERROR, "ADDON: Exception");
      return false;
    }
    m_initialized = true;

    GetPresets();

    CreateBuffers();
    if (g_application.m_pPlayer)
    {
      g_application.m_pPlayer->RegisterAudioCallback(this);
    }

    return true;
  }
  return false;
}

void CVisualisation::Destroy()
{
  /* Release Callback table in memory */
  delete m_callbacks;
  m_callbacks = NULL;
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  if (m_initialized)
  {
    try
    {
      m_pStruct->Start(iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
    } catch (std::exception e)
    {
      CLog::Log(LOGERROR, "ADDON: Exception");
    }
  }
}

void CVisualisation::AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  if (m_initialized)
  {
    try
    {
      m_pStruct->AudioData(pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);
    }
    catch (std::exception e)
    {
      CLog::Log(LOGERROR, "Exception in Visualisation::AudioData()");
    }
  }
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  g_graphicsContext.BeginPaint();
  if (m_initialized)
  {
    try
    {
      m_pStruct->Render();
    } catch (std::exception e)
    {
    }
  }
  g_graphicsContext.EndPaint();
}

void CVisualisation::Stop()
{
  if (g_application.m_pPlayer) g_application.m_pPlayer->UnRegisterAudioCallback();
  if (m_initialized)
  {
    try
    {
      m_pStruct->Stop();
    } catch (std::exception e)
    {
    }
  }
}

void CVisualisation::GetInfo(VIS_INFO *info)
{
  // get info from vis
  if (m_initialized) m_pStruct->GetInfo(info);
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
  if (!m_initialized)
  {
    return false;
  }

  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  if (action != VIS_ACTION_NONE && m_pStruct->OnAction)
  {
    // if this is a VIS_ACTION_UPDATE_TRACK action, copy relevant
    // tags from CMusicInfoTag to VisTag
    if ( action == VIS_ACTION_UPDATE_TRACK && param )
    {
      const CMusicInfoTag* tag = (const CMusicInfoTag*)param;
      VisTrack track;
      track.title       = tag->GetTitle().c_str();
      track.artist      = tag->GetArtist().c_str();
      track.album       = tag->GetAlbum().c_str();
      track.albumArtist = tag->GetAlbumArtist().c_str();
      track.genre       = tag->GetGenre().c_str();
      track.comment     = tag->GetComment().c_str();
      track.lyrics      = tag->GetLyrics().c_str();
      track.trackNumber = tag->GetTrackNumber();
      track.discNumber  = tag->GetDiscNumber();
      track.duration    = tag->GetDuration();
      track.year        = tag->GetYear();
      track.rating      = tag->GetRating();

      return m_pStruct->OnAction(action, &track);
    }
    return m_pStruct->OnAction((int)action, param);
  }
  return false;
}

void CVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  if (!m_pStruct)
    return ;
  CLog::Log(LOGDEBUG, "OnInitialize() started");

  m_iChannels = iChannels;
  m_iSamplesPerSec = iSamplesPerSec;
  m_iBitsPerSample = iBitsPerSample;
  UpdateTrack();

  CLog::Log(LOGDEBUG, "OnInitialize() done");
}

void CVisualisation::OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)
{
  if (!m_pStruct)
    return ;
  if (!m_initialized) return ;

  // FIXME: iAudioDataLength should never be less than 0
  if (iAudioDataLength<0)
    return;

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
    AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, m_fFreq, AUDIO_BUFFER_SIZE);
  }
  else
  { // Transfer data to our visualisation
    AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, NULL, 0);
  }
  return ;
}

void CVisualisation::CreateBuffers()
{
  ClearBuffers();

  // Get the number of buffers from the current vis
  VIS_INFO info;
  m_pStruct->GetInfo(&info);
  m_iNumBuffers = info.iSyncDelay + 1;
  m_bWantsFreq = (info.bWantsFreq != 0);
  if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
    m_iNumBuffers = MAX_AUDIO_BUFFERS;
  if (m_iNumBuffers < 1)
    m_iNumBuffers = 1;
}

void CVisualisation::ClearBuffers()
{
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

bool CVisualisation::UpdateTrack()
{
  bool handled;
  if (m_initialized)
  {
    // get the current album art filename
    m_AlbumThumb = _P(g_infoManager.GetImage(MUSICPLAYER_COVER, WINDOW_INVALID));

    // get the current track tag
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();

    if (m_AlbumThumb == "DefaultAlbumCover.png")
      m_AlbumThumb = "";
    else
      CLog::Log(LOGDEBUG,"Updating visualisation albumart: %s", m_AlbumThumb.c_str());

    // inform the visualisation of the current album art
    if ( m_pStruct->OnAction( VIS_ACTION_UPDATE_ALBUMART,
      (void*)( m_AlbumThumb.c_str() ) ) )
      handled = true;

    // inform the visualisation of the current track's tag information
    if ( tag && m_pStruct->OnAction( VIS_ACTION_UPDATE_TRACK,
      (void*)tag ) )
      handled = true;
  }
  return handled;
}

bool CVisualisation::GetPresetList(std::vector<CStdString> &vecpresets)
{
  vecpresets = m_presets;
  return !m_presets.empty();
}

bool CVisualisation::GetPresets()
{
  m_presets.clear();
  char **presets = NULL;
  unsigned int entries = 0;
  try
  {
    entries = m_pStruct->GetPresets(&presets);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in Visualisation::GetPresets()");
    return false;
  }
  if (presets && entries > 0)
  {
    for (unsigned i=0; i < entries; i++)
    {
      if (presets[i])
      {
        m_presets.push_back(presets[i]);
      }
    }
  }
  return (!m_presets.empty());
}

bool CVisualisation::IsLocked()
{
  return m_pStruct->IsLocked();
}

unsigned CVisualisation::GetPreset()
{
  unsigned index = 0;
  try
  {
    index = m_pStruct->GetPreset();
  }
  catch(...)
  {
    return 0;
  }
  return index;
}

CStdString CVisualisation::GetPresetName()
{
  if (!m_presets.empty())
    return m_presets[GetPreset()];
  else
    return "";
}

