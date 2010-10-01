/*
 *      Copyright (C) 2010 Team XBMC
 *      http://xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Vortex.h"

#include <assert.h>
#include <stdio.h>
#include <io.h>
#include <math.h>


#include "..\angelscript\sdk\angelscript\include\angelscript.h"
#include "VortexString.h"
#include "Preset.h"
#include "Renderer.h"
#include "Texture.h"
#include "FFT.h"
#include "XmlDocument.h"

#include "Effects\Metaballs.h"
#include "Effects\Tunnel.h"
#include "Effects\Map.h"
#include "Effects\VoicePrint.h"


float g_bass;
float g_middle;
float g_treble;

#define CREDITS


namespace
{
	FLOAT fSecsPerTick;
	LARGE_INTEGER qwTime, qwLastTime, qwElapsedTime, qwAppTime, qwElapsedAppTime;
	FLOAT fTime, fElapsedTime, fAppTime, fElapsedAppTime;

	VOID InitTime()
	{
		// Get the frequency of the timer
		LARGE_INTEGER qwTicksPerSec;
		QueryPerformanceFrequency( &qwTicksPerSec );
		fSecsPerTick = 1.0f / (FLOAT)qwTicksPerSec.QuadPart;

		// Save the start time
		QueryPerformanceCounter( &qwTime );
		qwLastTime.QuadPart = qwTime.QuadPart;

		qwAppTime.QuadPart        = 0;
		qwElapsedTime.QuadPart    = 0;
		qwElapsedAppTime.QuadPart = 0;
	}

	VOID UpdateTime()
	{

		QueryPerformanceCounter( &qwTime );
		qwElapsedTime.QuadPart = qwTime.QuadPart - qwLastTime.QuadPart;
		qwLastTime.QuadPart    = qwTime.QuadPart;
		qwElapsedAppTime.QuadPart = qwElapsedTime.QuadPart;
		qwAppTime.QuadPart    += qwElapsedAppTime.QuadPart;

		// Store the current time values as floating point
		fTime           = fSecsPerTick * ((FLOAT)(qwTime.QuadPart));
		fElapsedTime    = fSecsPerTick * ((FLOAT)(qwElapsedTime.QuadPart));
		fAppTime        = fSecsPerTick * ((FLOAT)(qwAppTime.QuadPart));
		fElapsedAppTime = fSecsPerTick * ((FLOAT)(qwElapsedAppTime.QuadPart));
	}

	// the settings vector
	vector<VisSetting> m_vecSettings;



}

class asCOutputStream : public asIOutputStream
{
public:
	void Write(const char *text) { OutputDebugString(text); }
};

#define NUM_FREQUENCIES (512)

struct SoundData_t
{
	float   imm[2][3];                // bass, mids, treble, no damping, for each channel (long-term average is 1)
	float   avg[2][3];               // bass, mids, treble, some damping, for each channel (long-term average is 1)
	float   med_avg[2][3];          // bass, mids, treble, more damping, for each channel (long-term average is 1)
	//    float   long_avg[2][3];        // bass, mids, treble, heavy damping, for each channel (long-term average is 1)
	float   fWaveform[2][576];             // Not all 576 are valid! - only NUM_WAVEFORM_SAMPLES samples are valid for each channel (note: NUM_WAVEFORM_SAMPLES is declared in shell_defines.h)
	float   fSpectrum[2][NUM_FREQUENCIES]; // NUM_FREQUENCIES samples for each channel (note: NUM_FREQUENCIES is declared in shell_defines.h)

	float specImm[32];
	float specAvg[32];
	float specMedAvg[32];

	float bigSpecImm[512];
	float leftBigSpecAvg[512];
	float rightBigSpecAvg[512];
};

#define TEXTURE_FRAMEBUFFER (1)
#define TEXTURE_NEXTPRESET  (2)
#define TEXTURE_CURRPRESET  (3)
#define TEXTURE_ALBUMART    (4)

//-----------------------------------------------------------------------------
//                            V A R I A B L E S
//-----------------------------------------------------------------------------
namespace
{
	// AngelScript
	asIScriptEngine* g_scriptEngine = NULL;
	asCOutputStream g_outputStream;

	// Presets
	Preset_c g_preset1;
	Preset_c g_preset2;
	Preset_c g_transition;
	Preset_c* g_presets[2] = {&g_preset1, &g_preset2};

#ifdef STAND_ALONE
	const char PRESET_DIR[] = "d:\\Presets\\";
#else
	const char PRESET_DIR[] = "q:\\Visualisations\\Vortex\\Presets\\";
#endif

	LPDIRECT3DTEXTURE8 g_currPresetTex;
	LPDIRECT3DTEXTURE8 g_newPresetTex;
	LPDIRECT3DTEXTURE8 g_albumArt = NULL;
	LPDIRECT3DTEXTURE8 g_vortexLogo = NULL;
#ifdef CREDITS
	LPDIRECT3DTEXTURE8 g_creditsTexture = NULL;
#endif

	SoundData_t g_sound;
	FFT g_fftobj;

	float g_timePass = 1.0f/50.0f;
	bool   g_finished = false;

	int g_currPresetId = 0;
	int g_transitionId = 0;

	bool g_showBeatDetection = false;
	bool g_lockPreset = false;
	int g_renderTarget = 0;
	bool g_randPresets = true;
	bool g_useAlbumArt = true;

	bool g_enableTransitions = true;

	float g_mainCounter = 7.0f;
	bool g_inTransition = false;
	int g_loadPreset = -1;

	int* g_randPresetList;

	float g_timeBetweenPresets = 5.0f;
	float g_timeBetweenPresetsRand = 5.0f;

	int g_audioCalled = 0;

#ifdef CREDITS
	bool g_drawCredits = 0;
	float g_creditsOffset = -2.25;

#endif

	enum State_e {STATE_RENDER_PRESET, STATE_TRANSITION};
	State_e g_currState = STATE_RENDER_PRESET;

	char* g_xmlFile;

};


//-- Init ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool Vortex_c::Init(IDirect3DDevice8* device, int iXpos, int iYPos, int iWidth, int iHeight, float pixelRatio, char* xmlFile)
{
	g_xmlFile = xmlFile;
	srand(::GetTickCount());

	g_currState = STATE_RENDER_PRESET;

	InitTime();

	Renderer_c::Init(device, iXpos, iYPos, iWidth, iHeight, pixelRatio);

	g_audioCalled = 0;

	g_albumArt = NULL;
	g_vortexLogo = Renderer_c::LoadTexture("q:\\Visualisations\\Vortex\\Textures\\Vortex.jpg");

	g_currPresetTex =  Renderer_c::CreateTexture(512, 512);
	g_newPresetTex =  Renderer_c::CreateTexture(512, 512);

	if (!InitAngelScript())
		return false;

	g_fftobj.Init(576, NUM_FREQUENCIES);

	m_presets		= new char[16384];
	m_sizeOfPresetList =      16384;
	m_transitions		= new char[16384];
	m_sizeOfTransitionList =      16384;
	m_presetAddr = NULL;
	m_transitionAddr = NULL;

	GetPresets(PRESET_DIR);
	g_randPresetList = new int [m_numPresets];

	for (int i = 0; i < m_numPresets; i++)
	{
		g_randPresetList[i] = i;
	}

	// Mix em up a bit
	for (int i = 0; i < 100; i++)
	{
		int a = rand() % m_numPresets;
		int b = rand() % m_numPresets;
		int old = g_randPresetList[a];
		g_randPresetList[a] = g_randPresetList[b];
		g_randPresetList[b] = old;
	}

	g_preset1.Init(g_scriptEngine, "PRESET1");
	g_preset2.Init(g_scriptEngine, "PRESET2");
	g_transition.Init(g_scriptEngine, "TRANSITION");

#ifdef STAND_ALONE

	if (m_numPresets > 0)
	{
		char filename[256];
		g_currPresetId = rand() % m_numPresets;
		sprintf(filename, "d:\\Presets\\%s.vtx", m_presetAddr[g_currPresetId]);

		g_presets[0]->Begin(filename);
	}

#else

	if (m_numPresets > 0)
	{
		if (g_lockPreset)
		{
			if (g_currPresetId >= m_numPresets || g_currPresetId < 0)
			{
				g_currPresetId = rand() % m_numPresets;
			}
		}
		else if (g_randPresets)
		{
			g_currPresetId = rand() % m_numPresets;
		}

		char filename[256];
		sprintf(filename, "q:\\Visualisations\\Vortex\\Presets\\%s.vtx", m_presetAddr[g_currPresetId]);
		g_preset1.Begin(filename);
	}

#endif

	m_vecSettings.clear();

	//  VisSetting setting1(VisSetting::CHECK, "Show Beat Detection");
	//  setting1.current = GetBeatDetection() != 0;
	//  m_vecSettings.push_back(setting1);

	VisSetting setting1(VisSetting::CHECK, "Enable Transitions");
	setting1.current = g_enableTransitions != 0;
	m_vecSettings.push_back(setting1);

	VisSetting setting5(VisSetting::CHECK, "Use Album Art");
	setting5.current = GetUseAlbumArt() != 0;
	m_vecSettings.push_back(setting5);

	VisSetting setting2(VisSetting::CHECK, "Random Presets");
	setting2.current = GetRandomPresets() != 0;
	m_vecSettings.push_back(setting2);

	VisSetting setting4(VisSetting::SPIN, "Time Between Presets");
	for (int i=0; i < 55; i++)
	{
		char temp[10];
		sprintf(temp, "%i secs", i + 5);
		setting4.AddEntry(temp);
	}
	setting4.current = (int)(GetTimeBetweenPresets() - 5);
	m_vecSettings.push_back(setting4);

	VisSetting setting3(VisSetting::SPIN, "Additional Random Time");
	for (int i=0; i < 60; i++)
	{
		char temp[10];
		sprintf(temp, "%i secs", i);
		setting3.AddEntry(temp);
	}
	setting3.current = (int)(GetTimeBetweenPresetsRand());
	m_vecSettings.push_back(setting3);

	VisSetting setting6(VisSetting::CHECK, "Credits");
	setting6.current = g_drawCredits != 0;
	m_vecSettings.push_back(setting6);


	return true;

} // Init

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Vortex_c::Stop()
{
	delete[] g_randPresetList;
	delete[] m_transitionAddr;
	delete[] m_presetAddr;
	delete[] m_presets;
	delete[] m_transitions;

	g_preset1.End();
	g_preset2.End();
	g_transition.End();

	g_currPresetTex->Release();
	g_newPresetTex->Release();
	if (g_albumArt)
		Renderer_c::ReleaseTexture(g_albumArt);
	if (g_vortexLogo)
		Renderer_c::ReleaseTexture(g_vortexLogo);
#ifdef CREDITS
	if (g_creditsTexture)
		Renderer_c::ReleaseTexture(g_creditsTexture);
#endif
	Renderer_c::Exit();

	g_scriptEngine->Release();

} // Stop

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Vortex_c::Render()
{
	if (m_numPresets == 0)
	{
		Renderer_c::Clear(0);
		return;
	}

	UpdateTime();

	g_timePass = fElapsedAppTime;

#ifdef STAND_ALONE
	short wav[512];
	static int offset = 0;
	for (int i=0; i<512; i+=2)
	{
		wav[i] = rand() % 32768 - (32768 / 2);
		wav[i+1] = rand() % 32768 - (32768 / 2);
	}
	offset += 1;

	AudioData(wav, 512, NULL, 0);

	float avgMul = 1.4f;
	float fade = 0.97f;
	g_bass *= fade;
	g_treble *= fade;
	g_middle *= fade;

	if (rand() % 100 > 90)
		g_bass = 1;
	if (rand() % 100 > 90)
		g_middle = 1;
	if (rand() % 100 > 90)
		g_treble = 1;

#else

	if (g_audioCalled == 0)
	{
		// If audio update is not being called, clear out the data
		short zero[2] = {0, 0};
		AudioData(zero, 2, NULL, 0);
		g_bass = 0;
		g_middle = 0;
		g_treble = 0;
		g_audioCalled = 0;
	}
	else
	{
		g_audioCalled--;
	}


#endif

	Render2();

} // Render


//-- Render2 ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Vortex_c::Render2()
{
	static bool doOnce = true;
	//---------------------------------------------------------------------------
	// Do rendering

	//OutputDebugString("STATE RENDER\n");
	switch (g_currState)
	{
	case STATE_RENDER_PRESET:
		{
			//     OutputDebugString("STATE = STATE_RENDER_PRESET\n");
			g_renderTarget = TEXTURE_FRAMEBUFFER;
			Renderer_c::SetRenderTargetBackBuffer();
			Renderer_c::SetDefaults();
			Renderer_c::Clear(0);
			g_presets[0]->Render();
			doOnce = true;
			break;
		}

	case STATE_TRANSITION:
		{
			//      OutputDebugString("STATE = STATE_TRANSITION\n");
			//      if (doOnce)
			{
				Renderer_c::SetDefaults();
				Renderer_c::SetRenderTarget(g_currPresetTex);
				g_renderTarget = TEXTURE_CURRPRESET;
				Renderer_c::Clear(0);
				g_presets[0]->Render();
				doOnce = false;
			}

			Renderer_c::SetDefaults();
			Renderer_c::SetRenderTarget(g_newPresetTex);
			g_renderTarget = TEXTURE_NEXTPRESET;
			Renderer_c::Clear(0);
			g_presets[1]->Render();

			Renderer_c::SetDefaults();
			Renderer_c::SetRenderTargetBackBuffer();
			g_renderTarget = TEXTURE_FRAMEBUFFER;
			Renderer_c::Clear(0);
			g_transition.Render();
			break;
		}
	}

	// Restore state here
	Renderer_c::SetRenderTargetBackBuffer();
	Renderer_c::SetDefaults();


	//---------------------------------------------------------------------------
	// Do updating

	//  OutputDebugString("STATE UPDATE\n");

	switch (g_currState)
	{
	case STATE_RENDER_PRESET:
		{
			//      OutputDebugString("STATE = STATE_RENDER_PRESET\n");
			if (!g_lockPreset)
				g_mainCounter -= g_timePass;

			if (g_mainCounter <= 0 || g_loadPreset != -1)
			{
				char filename[255];

				if (g_mainCounter <= 0)
				{
					// Not in a transition, preset not locked and time for a new preset
					if (g_randPresets)
					{
						int nextPreset = rand() % m_numPresets;
						if (m_numPresets > 5)
						{
							int index = rand() % (min(5, m_numPresets));
							nextPreset = g_randPresetList[index];

							for (int i = index; i < m_numPresets-1; i++)
							{
								g_randPresetList[i] = g_randPresetList[i+1];
							}
							g_randPresetList[i] = nextPreset;
						}

						if (nextPreset == g_currPresetId)
							g_currPresetId = (g_currPresetId+1) % m_numPresets;
						else
							g_currPresetId = nextPreset;
					}
					else
					{
						g_currPresetId = (g_currPresetId+1) % m_numPresets;
					}
				}
				else
				{
					// New preset requested
					g_currPresetId = g_loadPreset;
					g_loadPreset = -1;
				}

				g_finished = true;

				sprintf(filename, "%s%s.vtx", PRESET_DIR, m_presetAddr[g_currPresetId]);

				// Load preset
				if (g_presets[1]->Begin(filename) == true)
				{
					// Load and begin transition
					if (m_numTransitions != 0 && g_enableTransitions)
					{
						g_transitionId = (g_transitionId+1) % m_numTransitions;
						sprintf(filename, "%s%s", PRESET_DIR, m_transitionAddr[g_transitionId]);
						if (g_transition.Begin(filename) == true)
						{
							// Transition loading succeeded
							g_finished = false;
						}
					}
				}


				g_currState = STATE_TRANSITION;

			}

			break;
		}

	case STATE_TRANSITION:
		{
			//      OutputDebugString("STATE = STATE_TRANSITION\n");
			if (g_finished)
			{
				g_mainCounter = g_timeBetweenPresets + ((rand() % 100) / 100.0f) * g_timeBetweenPresetsRand;
				g_presets[0]->End();
				g_transition.End();
				Preset_c* temp = g_presets[0];
				g_presets[0] = g_presets[1];
				g_presets[1] = temp;
				g_finished = false;

				g_currState = STATE_RENDER_PRESET;

			}
			break;
		}
	}


	if (g_showBeatDetection)
	{
		Renderer_c::SetDrawMode2d();

		Renderer_c::Colour(1, 0, 0, 1);
		Renderer_c::Rect(-0.5f, 0, -0.25f, -g_bass*0.5f);
		Renderer_c::Colour(1, 1, 1, 1);
		Renderer_c::Rect(-0.5f, 1, -0.25f, 1-g_sound.med_avg[0][0]*0.07f);
		Renderer_c::Colour(1, 0, 0, 1);
		Renderer_c::Rect(-0.4f, 1, -0.35f, 1-g_sound.avg[0][0]*0.07f);

		Renderer_c::Colour(0, 1, 0, 1);
		Renderer_c::Rect(-0.125f, 0, 0.125f, -g_middle*0.5f);
		Renderer_c::Colour(1, 1, 1, 1);
		Renderer_c::Rect(-0.125f, 1, 0.125f, 1-g_sound.med_avg[0][1]*0.07f);
		Renderer_c::Colour(0, 1, 0, 1);
		Renderer_c::Rect(-0.025f, 1, 0.025f, 1-g_sound.avg[0][1]*0.07f);

		Renderer_c::Colour(0, 0, 1, 1);
		Renderer_c::Rect(0.25f, 0, 0.5f, -g_treble*0.5f);
		Renderer_c::Colour(1, 1, 1, 1);
		Renderer_c::Rect(0.25f, 1, 0.5f, 1-g_sound.med_avg[0][2]*0.07f);
		Renderer_c::Colour(0, 0, 1, 1);
		Renderer_c::Rect(0.35f, 1, 0.4f, 1-g_sound.avg[0][2]*0.07f);

		Renderer_c::Colour(1, 1, 1, 1);

	}

#ifdef CREDITS
	if (g_drawCredits)
	{
		Renderer_c::Translate(0, 0, 2.0f);
		Renderer_c::SetAspect(1);

		Renderer_c::SetBlendMode(BLEND_MOD);
		Renderer_c::SetTexture(g_creditsTexture);
		Renderer_c::TexRect(-0.75f, 1.25 + g_creditsOffset, 0.75f, -1.25+ g_creditsOffset);

		g_creditsOffset += 0.12f * g_timePass;

		if (g_creditsOffset >= 2.25)
			g_creditsOffset = -2.25;

	}
#endif
}


float AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps)
{
	// returns the equivalent per-frame decay rate at actual_fps

	// basically, do all your testing at fps1 and get a good decay rate;
	// then, in the real application, adjust that rate by the actual fps each time you use it.

	float per_second_decay_rate_at_fps1 = powf(per_frame_decay_rate_at_fps1, fps1);
	float per_frame_decay_rate_at_fps2 = powf(per_second_decay_rate_at_fps1, 1.0f/actual_fps);

	return per_frame_decay_rate_at_fps2;
}

void Vortex_c::AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	// Audio analysis based on code from Milkdrop

	g_audioCalled = 5;

	int m_fps = 50;

	float temp_wave[2][576];

	int ipos=0;
	int oldI = 0;
	while (ipos < 576)
	{
		for (int i=0; i < iAudioDataLength; i+=2)
		{
			g_sound.fWaveform[0][ipos] = float(((((pAudioData[i] / 32768.0f) * 255.0f))));
			g_sound.fWaveform[1][ipos] = float(((((pAudioData[i+1] / 32768.0f) * 255.0f))));


			// damp the input into the FFT a bit, to reduce high-frequency noise:
			temp_wave[0][ipos] = 0.5f*(g_sound.fWaveform[0][ipos] + g_sound.fWaveform[0][oldI]);
			temp_wave[1][ipos] = 0.5f*(g_sound.fWaveform[1][ipos] + g_sound.fWaveform[1][oldI]);
			oldI = ipos;
			ipos++;
			if (ipos >= 576) break;
		}
	}

	// we get 576 samples in from winamp.
	// the output of the fft has 'num_frequencies' samples,
	//   and represents the frequency range 0 hz - 22,050 hz.
	// usually, plugins only use half of this output (the range 0 hz - 11,025 hz),
	//   since >10 khz doesn't usually contribute much.

	g_fftobj.time_to_frequency_domain(temp_wave[0], g_sound.fSpectrum[0]);
	g_fftobj.time_to_frequency_domain(temp_wave[1], g_sound.fSpectrum[1]);


	// sum (left channel) spectrum up into 3 bands
	// [note: the new ranges do it so that the 3 bands are equally spaced, pitch-wise]
	float min_freq = 200.0f;
	float max_freq = 11025.0f;
	float net_octaves = (logf(max_freq/min_freq) / logf(2.0f));     // 5.7846348455575205777914165223593
	float octaves_per_band = net_octaves / 3.0f;                    // 1.9282116151858401925971388407864
	float mult = powf(2.0f, octaves_per_band); // each band's highest freq. divided by its lowest freq.; 3.805831305510122517035102576162
	// [to verify: min_freq * mult * mult * mult should equal max_freq.]
	//    for (int ch=0; ch<2; ch++)
	{
		for (int i=0; i<3; i++)
		{
			// old guesswork code for this:
			//   float exp = 2.1f;
			//   int start = (int)(NUM_FREQUENCIES*0.5f*powf(i/3.0f, exp));
			//   int end   = (int)(NUM_FREQUENCIES*0.5f*powf((i+1)/3.0f, exp));
			// results:
			//          old range:      new range (ideal):
			//   bass:  0-1097          200-761
			//   mids:  1097-4705       761-2897
			//   treb:  4705-11025      2897-11025
			int start = (int)(NUM_FREQUENCIES * min_freq*powf(mult, (float)i  )/11025.0f);
			int end   = (int)(NUM_FREQUENCIES * min_freq*powf(mult, (float)i+1)/11025.0f);
			if (start < 0) start = 0;
			if (end > NUM_FREQUENCIES) end = NUM_FREQUENCIES;

			g_sound.imm[0][i] = 0;
			for (int j=start; j<end; j++)
			{
				g_sound.imm[0][i] += g_sound.fSpectrum[0][j];
				g_sound.imm[0][i] += g_sound.fSpectrum[1][j];
			}
			g_sound.imm[0][i] /= (float)(end-start)*2;
		}
	}

	//MRC
	octaves_per_band = net_octaves / 32.0f;                    // 1.9282116151858401925971388407864
	mult = powf(2.0f, octaves_per_band); // each band's highest freq. divided by its lowest freq.; 3.805831305510122517035102576162
	for (int i=0; i<32; i++)
	{
		// old guesswork code for this:
		//   float exp = 2.1f;
		//   int start = (int)(NUM_FREQUENCIES*0.5f*powf(i/3.0f, exp));
		//   int end   = (int)(NUM_FREQUENCIES*0.5f*powf((i+1)/3.0f, exp));
		// results:
		//          old range:      new range (ideal):
		//   bass:  0-1097          200-761
		//   mids:  1097-4705       761-2897
		//   treb:  4705-11025      2897-11025
		int start = (int)(NUM_FREQUENCIES * min_freq*powf(mult, (float)i  )/11025.0f);
		int end   = (int)(NUM_FREQUENCIES * min_freq*powf(mult, (float)i+1)/11025.0f);
		if (start < 0) start = 0;
		if (end > NUM_FREQUENCIES) end = NUM_FREQUENCIES;

		g_sound.specImm[i] = 0;
		for (int j=start; j<end; j++)
			g_sound.specImm[i] += g_sound.fSpectrum[0][j];
		g_sound.specImm[i] /= (float)(end-start);
		g_sound.specImm[i] /= 0.3808f;

		float avg_mix;
		if (g_sound.specImm[i] > g_sound.specAvg[i])
			avg_mix = AdjustRateToFPS(0.2f, 14.0f, (float)m_fps);
		else
			avg_mix = AdjustRateToFPS(0.5f, 14.0f, (float)m_fps);
		g_sound.specAvg[i] = g_sound.specAvg[i]*avg_mix + g_sound.specImm[i]*(1-avg_mix);

		float med_mix  = 0.91f;//0.800f + 0.11f*powf(t, 0.4f);    // primarily used for velocity_damping
		med_mix  = AdjustRateToFPS( med_mix, 14.0f, (float)m_fps);
		g_sound.specMedAvg[i]  =  g_sound.specMedAvg[i]*(med_mix ) + g_sound.specImm[i]*(1-med_mix);
	}

	for (int i=0; i<512; i++)
	{
		g_sound.bigSpecImm[i] = g_sound.fSpectrum[0][i];
		g_sound.bigSpecImm[i] *= 0.2408f;

		float avg_mix;
		if (g_sound.bigSpecImm[i] > g_sound.leftBigSpecAvg[i])
			avg_mix = AdjustRateToFPS(0.2f, 14.0f, (float)m_fps);
		else
			avg_mix = AdjustRateToFPS(0.5f, 14.0f, (float)m_fps);

		g_sound.leftBigSpecAvg[i] = g_sound.leftBigSpecAvg[i]*avg_mix + g_sound.bigSpecImm[i]*(1-avg_mix);
	}

	for (int i=0; i<512; i++)
	{
		g_sound.bigSpecImm[i] = g_sound.fSpectrum[1][i];
		g_sound.bigSpecImm[i] *= 0.2408f;

		float avg_mix;
		if (g_sound.bigSpecImm[i] > g_sound.rightBigSpecAvg[i])
			avg_mix = AdjustRateToFPS(0.2f, 14.0f, (float)m_fps);
		else
			avg_mix = AdjustRateToFPS(0.5f, 14.0f, (float)m_fps);

		g_sound.rightBigSpecAvg[i] = g_sound.rightBigSpecAvg[i]*avg_mix + g_sound.bigSpecImm[i]*(1-avg_mix);
	}


	// multiply by long-term, empirically-determined inverse averages:
	// (for a trial of 244 songs, 10 seconds each, somewhere in the 2nd or 3rd minute,
	//  the average levels were: 0.326781557	0.38087377	0.199888934
	for (int ch=0; ch<2; ch++)
	{
		g_sound.imm[ch][0] /= 0.326781557f;//0.270f;   
		g_sound.imm[ch][1] /= 0.380873770f;//0.343f;   
		g_sound.imm[ch][2] /= 0.199888934f;//0.295f;   
	}

	// do temporal blending to create attenuated and super-attenuated versions
	for (ch=0; ch<2; ch++)
	{
		for (int i=0; i<3; i++)
		{
			// g_sound.avg[i]
			{
				float avg_mix;
				if (g_sound.imm[ch][i] > g_sound.avg[ch][i])
					avg_mix = AdjustRateToFPS(0.2f, 14.0f, (float)m_fps);
				else
					avg_mix = AdjustRateToFPS(0.5f, 14.0f, (float)m_fps);
				//                if (g_sound.imm[ch][i] > g_sound.avg[ch][i])
				//                  avg_mix = 0.5f;
				//                else 
				//                  avg_mix = 0.8f;
				g_sound.avg[ch][i] = g_sound.avg[ch][i]*avg_mix + g_sound.imm[ch][i]*(1-avg_mix);
			}

			{
				float med_mix  = 0.91f;//0.800f + 0.11f*powf(t, 0.4f);    // primarily used for velocity_damping
				float long_mix = 0.96f;//0.800f + 0.16f*powf(t, 0.2f);    // primarily used for smoke plumes
				med_mix  = AdjustRateToFPS( med_mix, 14.0f, (float)m_fps);
				long_mix = AdjustRateToFPS(long_mix, 14.0f, (float)m_fps);
				g_sound.med_avg[ch][i]  =  g_sound.med_avg[ch][i]*(med_mix ) + g_sound.imm[ch][i]*(1-med_mix );
				//                g_sound.long_avg[ch][i] = g_sound.long_avg[ch][i]*(long_mix) + g_sound.imm[ch][i]*(1-long_mix);
			}
		}
	}

	float newBass = ((g_sound.avg[0][0] - g_sound.med_avg[0][0]) / g_sound.med_avg[0][0]) * 2;
	float newMiddle = ((g_sound.avg[0][1] - g_sound.med_avg[0][1]) / g_sound.med_avg[0][1]) * 2;
	float newTreble = ((g_sound.avg[0][2] - g_sound.med_avg[0][2]) / g_sound.med_avg[0][2]) * 2;
	newBass = max(min(newBass, 1.0f), -1.0f);
	newMiddle = max(min(newMiddle, 1.0f), -1.0f);
	newTreble = max(min(newTreble, 1.0f), -1.0f);


	float avg_mix = 0.5f;
//	if (newTreble > g_treble)
//		avg_mix = 0.5f;
//	else 
//		avg_mix = 0.5f;
	g_bass = g_bass*avg_mix + newBass*(1-avg_mix);
	g_middle = g_middle*avg_mix + newMiddle*(1-avg_mix);
	g_treble = g_treble*avg_mix + newTreble*(1-avg_mix);

	/*
	if (g_bass < 0)
	g_bass *= 1.5f;
	if (g_middle < 0)
	g_middle *= 1.5f;
	if (g_treble < 0)
	g_treble *= 1.5f;*/


	g_bass =   max(min(g_bass,   1.0f), -1.0f);
	g_middle = max(min(g_middle, 1.0f), -1.0f);
	g_treble = max(min(g_treble, 1.0f), -1.0f);

}

int mystrcmpi(char *s1, char *s2)
{
	// returns  1 if s1 comes before s2
	// returns  0 if equal
	// returns -1 if s1 comes after s2
	// treats all characters/symbols by their ASCII values, 
	//    except that it DOES ignore case.

	int i=0;

	//	while (LC2UC[s1[i]] == LC2UC[s2[i]] && s1[i] != 0)
	while (s1[i] == s2[i] && s1[i] != 0)
		i++;

	//FIX THIS!

	if (s1[i]==0 && s2[i]==0)
		return 0;
	else if (s1[i]==0)
		return -1;
	else if (s2[i]==0)
		return 1;
	else 
		return s1[i] < s2[i] ? -1 : 1;
	//		return (LC2UC[s1[i]] < LC2UC[s2[i]]) ? -1 : 1;
}

// Swiped from Milkdrop
void Vortex_c::MergeSortPresets(int left, int right)
{
	// note: left..right range is inclusive
	int nItems = right-left+1;

	if (nItems > 2)
	{
		// recurse to sort 2 halves (but don't actually recurse on a half if it only has 1 element)
		int mid = (left+right)/2;
		/*if (mid   != left) */ MergeSortPresets(left, mid);
		/*if (mid+1 != right)*/ MergeSortPresets(mid+1, right);

		// then merge results
		int a = left;
		int b = mid + 1;
		while (a <= mid && b <= right)
		{
			bool bSwap;

			// merge the sorted arrays; give preference to strings that start with a '*' character
			int nSpecial = 0;
			if (m_presetAddr[a][0] == '*') nSpecial++;
			if (m_presetAddr[b][0] == '*') nSpecial++;

			if (nSpecial == 1)
			{
				bSwap = (m_presetAddr[b][0] == '*');
			}
			else
			{
				bSwap = (mystrcmpi(m_presetAddr[a], m_presetAddr[b]) > 0);
			}

			if (bSwap)
			{
				char* temp = m_presetAddr[b];
				for (int k=b; k>a; k--)
					m_presetAddr[k] = m_presetAddr[k-1];
				m_presetAddr[a] = temp;
				mid++;
				b++;
			}
			a++;
		}
	}
	else if (nItems == 2)
	{
		// sort 2 items; give preference to 'special' strings that start with a '*' character
		int nSpecial = 0;
		if (m_presetAddr[left][0] == '*') nSpecial++;
		if (m_presetAddr[right][0] == '*') nSpecial++;

		if (nSpecial == 1)
		{
			if (m_presetAddr[right][0] == '*')
			{
				char* temp = m_presetAddr[left];
				m_presetAddr[left] = m_presetAddr[right];
				m_presetAddr[right] = temp;
			}
		}
		else if (mystrcmpi(m_presetAddr[left], m_presetAddr[right]) > 0)
		{
			char* temp = m_presetAddr[left];
			m_presetAddr[left] = m_presetAddr[right];
			m_presetAddr[right] = temp;
		}
	}
}


void Vortex_c::GetPresets(const char* presetDir)
{
	struct _finddata_t c_file;
	long hFile;

	char szMask[512];
	char szPath[512];

	strcpy(szPath, presetDir);
	int len = strlen(szPath);
	if (len>0 && szPath[len-1] != '\\') 
	{
		strcat(szPath, "\\");
	}
	strcpy(szMask, szPath);
	strcat(szMask, "*.*");

	WIN32_FIND_DATA ffd;
	ZeroMemory(&ffd, sizeof(ffd));

	for (int i=0; i<2; i++)		// usually RETURNs at end of first loop
	{
		m_numPresets = 0;
		m_numTransitions = 0;

		// find first file
		if( (hFile = _findfirst(szMask, &c_file )) != -1L )		// note: returns filename -without- path
		{
			char *p = m_presets;
			int  bytes_left = m_sizeOfPresetList - 1;		// save space for extra null-termination of last string
			char *tran = m_transitions;
			int  tranBytesLeft = m_sizeOfTransitionList - 1;		// save space for extra null-termination of last string

			do
			{
				bool bSkip = false;

				char szFilename[512];
				strcpy(szFilename, c_file.name);
				len = strlen(szFilename);

				if (len >= 4)
				{
					if (strcmpi(c_file.name + len - 4, ".vtx") == 0)
					{
						len = len - 4;
						// Preset
						bytes_left -= len+1;

						m_numPresets++;
						if (bytes_left >= 0)
						{
							strncpy(p, szFilename, len);
							p[len] = 0;
							p += len+1;
						}
					}
					else if (strcmpi(c_file.name + len - 4, ".tra") == 0)
					{
						// Transition
						tranBytesLeft -= len+1;

						m_numTransitions++;
						if (tranBytesLeft >= 0)
						{
							strcpy(tran, szFilename);
							tran += len+1;
						}
					}
				}
			}
			while(_findnext(hFile,&c_file) == 0);

			_findclose( hFile );

			if (bytes_left >= 0 && tranBytesLeft >= 0) 
			{
				// success!  double-null-terminate the last string.
				*p = 0;
				*tran = 0;

				// also fill each slot of m_presetAddr with the address of each string
				// but do it in ALPHABETICAL ORDER
				if (m_presetAddr)
					delete m_presetAddr;
				m_presetAddr = new char*[m_numPresets];

				if (m_transitionAddr)
					delete m_transitionAddr;
				m_transitionAddr = new char*[m_numTransitions];

				// the unsorted version:
				p = m_presets;
				for (int k = 0; k < m_numPresets; k++)
				{
					m_presetAddr[k] = p;
					while (*p) p++;
					p++;
				}

				MergeSortPresets(0, m_numPresets - 1);

				tran = m_transitions;
				for (int k = 0; k < m_numTransitions; k++)
				{
					m_transitionAddr[k] = tran;
					while (*tran) tran++;
					tran++;
				}

				return;
			}
			else
			{
				// reallocate and do it again
				//dumpmsg("too many presets -> reallocating list...");
				int new_size = (-bytes_left + m_sizeOfPresetList)*11/10;	// overallocate a bit
				delete m_presets;
				m_presets = new char[new_size];
				m_sizeOfPresetList = new_size;

				new_size = (-tranBytesLeft + m_sizeOfTransitionList)*11/10;	// overallocate a bit
				delete m_transitions;
				m_transitions = new char[new_size];
				m_sizeOfTransitionList = new_size;

			}
		}
	}

	// should never get here
	m_numPresets = 0;
	m_numTransitions = 0;
}


float Rand()
{
	return (rand() % 100 ) / 100.0f;
}

float Mag(float a, float b)
{
	return sqrtf(a*a + b*b);
}

float FloatClamp(float val, float min, float max)
{
	if (val < min)
		val = min;
	else if (val > max)
		val = max;

	return val;
}

int IntClamp(int val, int min, int max)
{
	if (val < min)
		val = min;
	else if (val > max)
		val = max;

	return val;
}

float WaveLeft(int index)
{
	return g_sound.fWaveform[0][index] * (1.0f / 255.0f);
}

float WaveRight(int index)
{
	return g_sound.fWaveform[1][index] * (1.0f / 255.0f);
}

float GetSpec(int index)
{
	index = min(index, 32);
	index = max(index, 0);
	if (g_sound.specMedAvg[index] == 0)
		return 0;

	return (g_sound.specAvg[index] / g_sound.specMedAvg[index]);
}

float GetSpecLeft(int index)
{
	index = min(index, 511);
	index = max(index, 0);

	index = (int)((index / 511.0f) * 350.0f);

	return g_sound.leftBigSpecAvg[index];
}

float GetSpecRight(int index)
{
	index = min(index, 511);
	index = max(index, 0);
	index = (int)((index / 511.0f) * 350.0f);

	return g_sound.rightBigSpecAvg[index];
}


float MyFabs(float a)
{
	return fabs(a);
}

int MyAbs(int a)
{
	return abs(a);
}

float MyAtan2(float a, float b)
{
	return atan2f(a, b);
}

void SetTexture(int textureId)
{
	if (textureId == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer_c::SetTexture(g_currPresetTex);
		return;
	}
	else if (textureId == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer_c::SetTexture(g_newPresetTex);
		return;
	}
	else if (textureId == TEXTURE_ALBUMART)
	{
		if (g_albumArt && g_useAlbumArt)
			Renderer_c::SetTexture(g_albumArt);
		else
			Renderer_c::SetTexture(g_vortexLogo);

		return;
	}
	Renderer_c::SetTexture(NULL);

} // SetTexture

void SetEnvTexture(int textureId)
{
	if (textureId == TEXTURE_FRAMEBUFFER)
	{
		// Current frame buffer
		Renderer_c::CopyToScratch();
		Renderer_c::SetEnvTexture(SCRATCH_TEXTURE);
	}
	else if (textureId == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer_c::SetEnvTexture(g_currPresetTex);
		return;
	}
	else if (textureId == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer_c::SetEnvTexture(g_newPresetTex);
		return;
	}
	else if (textureId == TEXTURE_ALBUMART)
	{
		if (g_albumArt && g_useAlbumArt)
			Renderer_c::SetTexture(g_albumArt);
		else
			Renderer_c::SetTexture(g_vortexLogo);
		return;
	}

	Renderer_c::SetEnvTexture(NULL);

} // SetTexture


void SetUserTexture(Texture_c& texture)
{
	Renderer_c::SetTexture(texture.m_texture);

	texture.Release();
}


void SetMapTexture(Map_c& map)
{
	Renderer_c::SetTexture(map.GetTexture());

} // SetMapTexture

void SetVoicePrintTexture(VoicePrint_c& vp)
{
	Renderer_c::SetTexture(vp.GetTexture());

} // SetVoicePrintTexture


void SetUserEnvTexture(Texture_c& texture)
{
	Renderer_c::SetEnvTexture(texture.m_texture);

	texture.Release();
}

void SetRenderTarget(int target)
{
	if (g_renderTarget== TEXTURE_FRAMEBUFFER)
	{
		// Current frame buffer
		Renderer_c::SetRenderTargetBackBuffer();
	}
	else if (g_renderTarget == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer_c::SetRenderTarget(g_currPresetTex);
	}
	else if (g_renderTarget == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer_c::SetRenderTarget(g_newPresetTex);
	}
}

void SetUserRenderTarget(Texture_c& texture)
{
	if (texture.m_renderTarget && texture.m_texture)
	{
		Renderer_c::SetRenderTarget(texture.m_texture);
	}
	else
	{
		Renderer_c::SetRenderTargetBackBuffer();
	}

	texture.Release();
}

void SetMapRenderTarget(Map_c& map)
{
	Renderer_c::SetRenderTarget(map.GetTexture());
}


//-- InitAngelScript ----------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool Vortex_c::InitAngelScript()
{
	//---------------------------------------------------------------------------
	// Create the scripting engine
	g_scriptEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	g_scriptEngine->SetCommonMessageStream(&g_outputStream);


	//---------------------------------------------------------------------------
	// Register functions and variables

	int r;

	//---------------------------------------------------------------------------
	// String
	RegisterVortexScriptString(g_scriptEngine);

	//---------------------------------------------------------------------------
	// Math functions
	r = g_scriptEngine->RegisterGlobalFunction("float Rand()", asFUNCTION(Rand), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Mag(float, float)", asFUNCTION(Mag), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Clamp(float, float, float)", asFUNCTION(FloatClamp), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("int Clamp(int, int, int)", asFUNCTION(IntClamp), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Fabs(float)", asFUNCTION(MyFabs), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("int Abs(int)", asFUNCTION(MyAbs), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Atan2(float, float)", asFUNCTION(MyAtan2), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTION(powf), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTION(sqrtf), asCALL_CDECL); assert(r >= 0);

	//---------------------------------------------------------------------------
	// Renderer functions
	r = g_scriptEngine->RegisterGlobalFunction("void gfxBegin(uint)", asFUNCTION(Renderer_c::Begin), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxEnd()", asFUNCTION(Renderer_c::End), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxClear(uint)", asFUNCTION(Renderer_c::Clear), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxClear(float r, float g, float b)", asFUNCTION(Renderer_c::ClearFloat), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxColour(float r, float g, float b, float a)", asFUNCTION(Renderer_c::Colour), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxNormal(float nx, float ny, float nz)", asFUNCTION(Renderer_c::Normal), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxTexCoord(float u, float v)", asFUNCTION(Renderer_c::TexCoord), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxVertex(float x, float y, float z)", asFUNCTION(Renderer_c::Vertex), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxCube(float nx, float ny, float nz, float x, float y, float z)", asFUNCTION(Renderer_c::Cube), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxPushMatrix()", asFUNCTION(Renderer_c::PushMatrix), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxPopMatrix()", asFUNCTION(Renderer_c::PopMatrix), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxRotate(float x, float y, float z)", asFUNCTION(Renderer_c::Rotate), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxRotate(float angle, float x, float y, float z)", asFUNCTION(Renderer_c::RotateAxis), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxTranslate(float x, float y, float z)", asFUNCTION(Renderer_c::Translate), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxScale(float x, float y, float z)", asFUNCTION(Renderer_c::Scale), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)", asFUNCTION(Renderer_c::LookAt), asCALL_CDECL); assert(r >= 0);

	r = g_scriptEngine->RegisterGlobalFunction("void gfxRect(float x1, float y1, float x2, float y2)", asFUNCTION(Renderer_c::Rect), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxTexRect(float x1, float y1, float x2, float y2)", asFUNCTION(Renderer_c::TexRect), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetBlendMode(uint)", asFUNCTION(Renderer_c::SetBlendMode), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetLineWidth(float)", asFUNCTION(Renderer_c::SetLineWidth), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetAspect(float)", asFUNCTION(Renderer_c::SetAspect), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetFillMode(int)", asFUNCTION(Renderer_c::SetFillMode), asCALL_CDECL); assert(r >= 0);

	//---------------------------------------------------------------------------
	// Global vars
	r = g_scriptEngine->RegisterGlobalProperty("const float TIMEPASS;", &g_timePass); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalProperty("bool FINISHED;", &g_finished); assert(r >= 0);

	r = g_scriptEngine->RegisterGlobalProperty("const float BASS;", &g_bass); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalProperty("const float MIDDLE;", &g_middle); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalProperty("const float TREBLE;", &g_treble); assert(r >= 0);

	r = g_scriptEngine->RegisterGlobalFunction("float WaveLeft(int)", asFUNCTION(WaveLeft), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float WaveRight(int)", asFUNCTION(WaveRight), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float GetSpec(int)", asFUNCTION(GetSpec), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float GetSpecLeft(int)", asFUNCTION(GetSpecLeft), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("float GetSpecRight(int)", asFUNCTION(GetSpecRight), asCALL_CDECL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Tunnel
	r = g_scriptEngine->RegisterObjectType("Tunnel", sizeof(Tunnel_c), asOBJ_CLASS_CD);	 assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Tunnel", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Tunnel_c::TunnelConstruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Tunnel", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Tunnel_c::TunnelDestruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Tunnel", "void Render(int, float, float, float)", asMETHOD(Tunnel_c, RenderLayer), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Tunnel", "void Update(float, float)", asMETHOD(Tunnel_c, Update), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Tunnel", "void Render()", asMETHOD(Tunnel_c, Render), asCALL_THISCALL); assert(r >= 0);

	//---------------------------------------------------------------------------
	// Map
	r = g_scriptEngine->RegisterObjectType("Map", sizeof(Map_c), asOBJ_CLASS_CD);	 assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Map_c::MapConstruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Map_c::MapDestruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Map", "void Render()", asMETHOD(Map_c, Render), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Map", "void SetValues(int, int, float, float, float, float, float)", asMETHOD(Map_c, SetValues), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Map", "void SetTimed()", asMETHOD(Map_c, SetTimed), asCALL_THISCALL); assert(r >= 0);

	//---------------------------------------------------------------------------
	// VoiceMap
	r = g_scriptEngine->RegisterObjectType("VoicePrint", sizeof(VoicePrint_c), asOBJ_CLASS_CD);	 assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(VoicePrint_c::Construct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(VoicePrint_c::Destruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("VoicePrint", "void Render()", asMETHOD(VoicePrint_c, Render), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("VoicePrint", "void LoadColourMap(string& in)", asMETHOD(VoicePrint_c, LoadColourMap), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("VoicePrint", "void SetSpeed(float)", asMETHOD(VoicePrint_c, SetSpeed), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("VoicePrint", "void SetRect(float, float, float, float)", asMETHOD(VoicePrint_c, SetRect), asCALL_THISCALL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Texture Object
	r = g_scriptEngine->RegisterObjectType("Texture", sizeof(Texture_c), asOBJ_CLASS_CDA);
	r = g_scriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Texture_c::TextureConstruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_ADDREF, "void f()", asMETHOD(Texture_c, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_RELEASE, "void f()", asMETHOD(Texture_c, Release), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_ASSIGNMENT, "Texture &f(Texture &in)", asMETHOD(Texture_c, operator=), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Texture", "void CreateTexture()", asMETHOD(Texture_c, CreateTexture), asCALL_THISCALL); assert(r >= 0);
	r = g_scriptEngine->RegisterObjectMethod("Texture", "void LoadTexture(string& in)", asMETHOD(Texture_c, LoadTexture), asCALL_THISCALL); assert(r >= 0);

	//---------------------------------------------------------------------------
	// Texture Functions
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetTexture(uint)", asFUNCTION(SetTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetTexture(Texture@)", asFUNCTION(SetUserTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetTexture(Map&)", asFUNCTION(SetMapTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetTexture(VoicePrint&)", asFUNCTION(SetVoicePrintTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetEnvTexture(uint)", asFUNCTION(SetEnvTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetEnvTexture(Texture@)", asFUNCTION(SetUserEnvTexture), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(uint)", asFUNCTION(SetRenderTarget), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(Texture@)", asFUNCTION(SetUserRenderTarget), asCALL_CDECL); assert(r >= 0);
	r = g_scriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(Map&)", asFUNCTION(SetMapRenderTarget), asCALL_CDECL); assert(r >= 0);


	return true;

} // InitAngelScript

void Vortex_c::ShowBeatDetection(bool show)
{
	g_showBeatDetection = show;
}
bool Vortex_c::GetBeatDetection()
{
	return g_showBeatDetection ;
}
bool Vortex_c::GetRandomPresets()
{
	return g_randPresets ;
}

void Vortex_c::RandomPresets(bool random)
{
	g_randPresets = random;
}

bool Vortex_c::GetUseAlbumArt()
{
	return g_useAlbumArt;
}

void Vortex_c::SetUseAlbumArt(bool use)
{
	g_useAlbumArt = use;
}

void Vortex_c::GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
	if (!pPresets || !numPresets || !currentPreset || !locked) return;
	*pPresets = m_presetAddr;
	*numPresets = m_numPresets;
	*currentPreset = g_currPresetId;
	*locked = g_lockPreset;
}

void Vortex_c::LoadNextPreset()
{
	g_loadPreset = (g_currPresetId+1) % m_numPresets;

}

void Vortex_c::LoadPreviousPreset()
{
	g_loadPreset = (g_currPresetId-1);
	if (g_loadPreset < 0)
		g_loadPreset = m_numPresets-1;

}

void Vortex_c::LoadPreset(int id)
{
	g_loadPreset = id;
}

void Vortex_c::LoadRandomPreset()
{
	g_loadPreset = rand() % m_numPresets;
	if (g_loadPreset == g_currPresetId)
		g_loadPreset = (g_currPresetId+1) % m_numPresets;

}

void Vortex_c::ToggleLockPreset()
{
	g_lockPreset = !g_lockPreset;
	if (!g_lockPreset)
		g_mainCounter = 0;  // Force to load a new preset when leaving lock
}

void Vortex_c::UpdateAlbumArt(char* artFilename)
{
	if (g_albumArt != NULL)
		g_albumArt->Release();

	g_albumArt = Renderer_c::LoadTexture(artFilename);
}

float Vortex_c::GetTimeBetweenPresets()
{
	return g_timeBetweenPresets;
}
float Vortex_c::GetTimeBetweenPresetsRand()
{
	return g_timeBetweenPresetsRand;

}
void Vortex_c::SetTimeBetweenPresets(float time)
{
	g_timeBetweenPresets = time;
}

void Vortex_c::SetTimeBetweenPresetsRand(float time)
{
	g_timeBetweenPresetsRand = time;
}


void Vortex_c::LoadSettings(char* filename)
{
	XmlNode node;
	CXmlDocument doc;

	// Load the config file
	if (doc.Load(filename) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		while(node > 0)
		{
			if (!strcmpi(doc.GetNodeTag(node),"TimeBetweenPresets"))
			{
				g_timeBetweenPresets = (float)atof(doc.GetNodeText(node));
			}
			else if (!strcmpi(doc.GetNodeTag(node),"TimeBetweenPresetsRand"))
			{
				g_timeBetweenPresetsRand = (float)atof(doc.GetNodeText(node));
			}
			else if (!strcmpi(doc.GetNodeTag(node),"HoldPreset"))
			{
				g_lockPreset = atoi(doc.GetNodeText(node)) == 1;
			}
			else if (!strcmpi(doc.GetNodeTag(node),"CurrentPreset"))
			{
				g_currPresetId = atoi(doc.GetNodeText(node));
			}
			else if (!strcmpi(doc.GetNodeTag(node),"ShowBeatDetection"))
			{
				g_showBeatDetection = !strcmpi(doc.GetNodeText(node),"true");
			}
			else if (!strcmpi(doc.GetNodeTag(node),"RandomPresets"))
			{
				g_randPresets = !strcmpi(doc.GetNodeText(node),"true");
			}
			else if (!strcmpi(doc.GetNodeTag(node),"UseAlbumArt"))
			{
				g_useAlbumArt = !strcmpi(doc.GetNodeText(node),"true");
			}
			else if (!strcmpi(doc.GetNodeTag(node),"EnableTransitions"))
			{
				g_enableTransitions = !strcmpi(doc.GetNodeText(node),"true");
			}
			node = doc.GetNextNode(node);
		}

		doc.Close();
	}
}

void Vortex_c::SaveSettings(char* filename)
{

	WriteXML doc;
	if (!doc.Open(filename, "visualisation"))
	{
		OutputDebugString("Vortex ERROR: Failed to open xml\n");
		return;
	}

	doc.WriteTag("TimeBetweenPresets", g_timeBetweenPresets);
	doc.WriteTag("TimeBetweenPresetsRand", g_timeBetweenPresetsRand);
	doc.WriteTag("HoldPreset", g_lockPreset ? 1 : 0);
	doc.WriteTag("CurrentPreset", g_currPresetId);
	//  doc.WriteTag("ShowBeatDetection", g_showBeatDetection ? true : false);
	doc.WriteTag("RandomPresets", g_randPresets ? true : false);
	doc.WriteTag("UseAlbumArt", g_useAlbumArt ? true : false);
	doc.WriteTag("EnableTransitions", g_enableTransitions ? true : false);

	doc.Close();
}

vector<VisSetting> * Vortex_c::GetSettings()
{
	return &m_vecSettings;
}

void Vortex_c::UpdateSettings(int num)
{
	VisSetting &setting = m_vecSettings[num];
	if (strcmpi(setting.name, "Use Preset") == 0)
		OnAction(34, (void *)&setting.current);
	else if (strcmpi(setting.name, "Show Beat Detection") == 0)
		ShowBeatDetection(setting.current == 1);
	else if (strcmpi(setting.name, "Random Presets") == 0)
		RandomPresets(setting.current == 1);
	else if (strcmpi(setting.name, "Time Between Presets") == 0)
		SetTimeBetweenPresets((float)(setting.current + 5));
	else if (strcmpi(setting.name, "Additional Random Time") == 0)
		SetTimeBetweenPresetsRand((float)(setting.current));
	else if (strcmpi(setting.name, "Use Album Art") == 0)
		SetUseAlbumArt((setting.current == 1));
	else if (strcmpi(setting.name, "Enable Transitions") == 0)
		g_enableTransitions = setting.current == 1;
#ifdef CREDITS
	else if (strcmpi(setting.name, "Credits") == 0)
	{
		g_drawCredits = setting.current == 1;

		if (g_drawCredits)
		{
			g_creditsTexture = Renderer_c::LoadTexture("q:\\Visualisations\\Vortex\\Textures\\Vortex_credits.png");
			g_creditsOffset = -2.25f;
		}
		else
		{
			if (g_creditsTexture)
				g_creditsTexture->Release();

			g_creditsTexture = NULL;
		}
	}
#endif
}

bool Vortex_c::OnAction(long flags, void *param)
{
	bool ret = false;
	if (flags == VIS_ACTION_NEXT_PRESET)
	{
		LoadNextPreset();
		ret = true;
	}
	else if (flags == VIS_ACTION_PREV_PRESET)
	{
		LoadPreviousPreset();
		ret = true;
	}
	else if (flags == VIS_ACTION_LOAD_PRESET && param)
	{
		LoadPreset(*(int *)param);
		ret = true;
	}
	else if (flags == VIS_ACTION_LOCK_PRESET)
	{
		ToggleLockPreset();
		ret = true;
	}
	else if (flags == VIS_ACTION_RANDOM_PRESET)
	{
		LoadRandomPreset();
		ret = true;
	}
	else if (flags == VIS_ACTION_UPDATE_ALBUMART)
	{
		UpdateAlbumArt((char*)param);
		ret = true;
	}
	if (ret)
		SaveSettings(g_xmlFile);

	return ret;
}