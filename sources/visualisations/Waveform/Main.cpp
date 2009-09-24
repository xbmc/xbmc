// Waveform.vis
// A simple visualisation example by MrC

#include <xtl.h>
#include "xbmc_vis.h"

char g_visName[512];
LPDIRECT3DDEVICE8 g_device;
float g_fWaveform[2][512];
D3DVIEWPORT8  g_viewport;

struct Vertex_t
{
  float x, y, z, w;
  D3DCOLOR  col;
};

#define VERTEX_FORMAT     (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)


//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName, float fPixelRatio)
{
  strcpy(g_visName, szVisualisationName);
  g_device = pd3dDevice;
  g_viewport.X = iPosX;
  g_viewport.Y = iPosY;
  g_viewport.Width = iWidth;
  g_viewport.Height = iHeight;
  g_viewport.MinZ = 0;
  g_viewport.MaxZ = 1;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void Stop()
{

}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{

  // Convert the audio data into a floating -1 to +1 range
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      g_fWaveform[0][ipos] = pAudioData[i] / 32768.0f;    // left channel
      g_fWaveform[1][ipos] = pAudioData[i+1] / 32768.0f;  // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  Vertex_t  verts[512];

  g_device->SetVertexShader(VERTEX_FORMAT);
  g_device->SetPixelShader(NULL);

  // Left channel
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    verts[i].y = g_viewport.Y + g_viewport.Height * 0.33f + (g_fWaveform[0][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1;
    verts[i].w = 1;
  }

  g_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 255, verts, sizeof(Vertex_t));

  // Right channel
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    verts[i].y = g_viewport.Y + g_viewport.Height * 0.66f + (g_fWaveform[1][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1;
    verts[i].w = 1;
  }

  g_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 255, verts, sizeof(Vertex_t));

}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, void *param)
{
  bool ret = false;
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{

}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
  return;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num)
{

}
