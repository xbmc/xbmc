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
#include "stdafx.h"
// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "Visualisation.h"
#include "visualizations/fft.h"
#include "../addons/include/xbmc_vis_types.h"
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "MusicInfoTag.h"
#include "Settings.h"
#include "URL.h"

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

//////////////////////////////////////////////////////////////////////
// CVisualisation
//////////////////////////////////////////////////////////////////////

CVisualisation::~CVisualisation()
{}

bool CVisualisation::Create(int x, int y, int w, int h)
{
  m_pInfo = new VIS_PROPS;
  m_pInfo->x = x;
  m_pInfo->y = y;
  m_pInfo->width = w;
  m_pInfo->height = h;
  m_pInfo->pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;

  if (CAddonDll<DllVisualisation, Visualisation, VIS_PROPS>::Create())
  {
    // Start the visualisation (this loads settings etc.)
    CStdString strFile = CUtil::GetFileName(g_application.CurrentFile());
    CLog::Log(LOGDEBUG, "Visualisation::Start()\n");
    m_pStruct->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile);
    m_initialized = true;

    CreateBuffers();
    if (g_application.m_pPlayer)
    {
      g_application.m_pPlayer->RegisterAudioCallback(this);
    }

    return true;
  }
  return false;
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  if (m_initialized) m_pStruct->Start(iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
}

void CVisualisation::AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  if (m_initialized) m_pStruct->AudioData(const_cast<short*>(pAudioData), iAudioDataLength, pFreqData, iFreqDataLength);
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  g_graphicsContext.BeginPaint();
  if (m_initialized) m_pStruct->Render();
  g_graphicsContext.EndPaint();
}

void CVisualisation::Stop()
{
  // ask visz. to cleanup
  if (m_initialized) m_pStruct->Stop();
}

void CVisualisation::GetInfo(VIS_INFO *info)
{
  // get info from vis
  if (m_initialized) m_pStruct->GetInfo(info);
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
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
      viz_track_t track = viz_track_create();

      viz_track_set_title(track, tag->GetTitle().c_str());
      viz_track_set_artist(track, tag->GetArtist().c_str());
      viz_track_set_album(track, tag->GetAlbum().c_str());
      viz_track_set_albumartist(track, tag->GetAlbumArtist().c_str());
      viz_track_set_genre(track, tag->GetGenre().c_str());
      viz_track_set_comment(track, tag->GetComment().c_str());
      viz_track_set_lyrics(track, tag->GetLyrics().c_str());
      viz_track_set_tracknum(track, tag->GetTrackNumber());
      viz_track_set_discnum(track, tag->GetDiscNumber());
      viz_track_set_duration(track, tag->GetDuration());
      viz_track_set_year(track, tag->GetYear());
      viz_track_set_rating(track, tag->GetRating());

      bool result = m_pStruct->OnAction(action, track);
      viz_release(track);
      return result;
    }
    return m_pStruct->OnAction(action, NULL);
  }
  return false;
}

void CVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  CSingleLock lock (m_critSection);
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
  CSingleLock lock (m_critSection);
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
    try
    {
      m_pStruct->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, m_fFreq, AUDIO_BUFFER_SIZE);
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
      m_pStruct->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, NULL, 0);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception in Visualisation::AudioData()");
    }
  }
  return ;
}

void CVisualisation::CreateBuffers()
{
  CSingleLock lock (m_critSection);
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

bool CVisualisation::UpdateTrack()
{
  bool handled;
  if (m_initialized)
  {
    // get the current album art filename
    m_AlbumThumb = g_infoManager.GetImage(MUSICPLAYER_COVER, WINDOW_INVALID);

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

bool CVisualisation::GetPresets()
{
  m_presets.clear();
  viz_preset_list_t presets = NULL;
  presets = m_pStruct->GetPresets();
  int c = viz_preset_list_get_count(presets);
  if (presets)
  {
    for (int i=0; i < c; i++)
    {
      viz_preset_t preset = NULL;
      preset = viz_preset_list_get_item(presets, i);
      if (preset)
      {
        m_presets.push_back(viz_preset_name(preset));
      }
      viz_release(preset);
    }
    viz_release(presets);
  }
  return (!m_presets.empty());
}

void CVisualisation::GetCurrentPreset(char **pPreset, bool *locked)
{
  if (!m_initialized)
    return;

  if (pPreset && locked && m_pStruct->GetCurrentPreset)
  {
    char **presets = NULL;
    int currentPreset = 0;
    int numPresets = 0;
    *locked = false;
    viz_preset_list_t list = NULL;
    list = m_pStruct->GetPresets();
    if (presets && currentPreset < numPresets)
      *pPreset = presets[currentPreset];
  }
}

bool CVisualisation::IsLocked()
{
  char *preset;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return locked;
}

char *CVisualisation::GetPreset()
{
  char *preset = NULL;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return preset;
}

