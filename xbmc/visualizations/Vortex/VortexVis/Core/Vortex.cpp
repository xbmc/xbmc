/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <windows.h>
#include <io.h>

#include "Vortex.h"
#include "Renderer.h"
#include <math.h>
#include "fft.h"
#include "Preset.h"
#include "Texture.h"
#include "Mesh.h"
#include "Shader.h"
#include "XmlDocument.h"

#include <string>
using namespace std;

#include "angelscript.h"
#include "../../angelscript/add_on/scriptstring/scriptstring.h"

#include "DebugConsole.h"
#include "../../../../addons/include/xbmc_vis_types.h"

// Effects
#include "Map.h"
#include "VoicePrint.h"
#include "Tunnel.h"

#define NUM_FREQUENCIES (512)
#define TEXTURE_FRAMEBUFFER (1)
#define TEXTURE_NEXTPRESET  (2)
#define TEXTURE_CURRPRESET  (3)
#define TEXTURE_ALBUMART    (4)

// Announcements currently disabled due to fonts no longer loading in the latest version of XBMC
// No idea why yet, they work fine in stand alone mode and previous versions of XBMC
#define ENABLE_ANNOUNCE (0)

class TrackInfo
{
public:
	TrackInfo()
	{
		Title = "None";
		TrackNumber = DiscNumber = Duration = Year = 0;
		Rating = 0;
	}

	void SetFromVisTrack(VisTrack* pVisTrack)
	{
		Title = pVisTrack->title;
		Artist = pVisTrack->artist;
		Album = pVisTrack->album;
		AlbumArtist = pVisTrack->albumArtist;
		Genre = pVisTrack->genre;
		Comment = pVisTrack->comment;
		Lyrics = pVisTrack->lyrics;
		TrackNumber = pVisTrack->trackNumber;
		DiscNumber = pVisTrack->discNumber;
		Duration = pVisTrack->duration;
		Year = pVisTrack->year;
		Rating = pVisTrack->rating;
	}

	string Title;
	string Artist;
	string Album;
	string AlbumArtist;
	string Genre;
	string Comment;
	string Lyrics;

	int        TrackNumber;
	int        DiscNumber;
	int        Duration;
	int        Year;
	char       Rating;
};

char g_TexturePath[ 512 ] = "special://xbmc//addons/visualization.vortex//Textures//";
char g_PresetPath[ 512 ] = "special://xbmc//addons/visualization.vortex//Presets//";
char g_TransitionPath[ 512 ] = "special://xbmc//addons/visualization.vortex//Transitions//";
char g_AnnouncePath[ 512 ] = "special://xbmc//addons/visualization.vortex//Announcements//";

class FileHolder
{
public:
	FileHolder() :
		m_FilenameAddresses( NULL ),
		m_AllFilenames( NULL )
	{
	
	}

	~FileHolder()
	{
		if ( m_AllFilenames )
		{
			delete[] m_AllFilenames;
		}

		if ( m_FilenameAddresses )
		{
			delete m_FilenameAddresses;
		}
	}

	void GetFiles( const string fileDir, const string fileExt )
	{
		if ( m_AllFilenames )
		{
			delete[] m_AllFilenames;
			m_AllFilenames = NULL;
		}

		if ( m_FilenameAddresses )
		{
			delete m_FilenameAddresses;
			m_FilenameAddresses = NULL;
		}

		m_Filenames.clear();
		string path = fileDir + "*." + fileExt;

		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile( path.c_str(), &findData );
		if ( hFind == INVALID_HANDLE_VALUE )
		{
			return;
		}

		do 
		{
			m_Filenames.push_back( findData.cFileName);
		} while ( FindNextFile( hFind, &findData ) != FALSE );

		FindClose( hFind );
	}

	const char* GetFilename(unsigned int index)
	{
		if ( index < 0 || index > m_Filenames.size() )
		{
			return "Invalid";
		}
		else
		{
			return m_Filenames[ index ].c_str();
		}
	}

	int NumFiles() { return m_Filenames.size(); }

	char** GetAllFilenames()
	{
		if ( m_AllFilenames == NULL )
		{
			int totalFilenameLength = 0;
			for( int i = 0; i < NumFiles(); i++ )
			{
				totalFilenameLength += m_Filenames[ i ].length() - 3;
			}
			totalFilenameLength += 1;
			m_AllFilenames = new char[ totalFilenameLength ];
			m_FilenameAddresses = new char*[ NumFiles() ];

			int currentOffset = 0;
			for( int i = 0; i < NumFiles(); i++ )
			{
				strncpy_s( &m_AllFilenames[ currentOffset ], totalFilenameLength - currentOffset, m_Filenames[ i ].c_str(), m_Filenames[ i ].length() - 4 );
				m_FilenameAddresses[ i ] = &m_AllFilenames[ currentOffset ];
				currentOffset += m_Filenames[ i ].length() - 3;
			}
			m_AllFilenames[ currentOffset ] = '\0';

		}

		return m_FilenameAddresses;
	}

private:
	vector<string>	m_Filenames;
	char*			m_AllFilenames;
	char**			m_FilenameAddresses;
};

namespace
{
	// Presets
	Preset g_preset1;
	Preset g_preset2;
	Preset g_transition;
	Preset g_AnnouncePreset;
	Preset* g_presets[2] = {&g_preset1, &g_preset2};

	LPDIRECT3DTEXTURE9 g_currPresetTex;
	LPDIRECT3DTEXTURE9 g_newPresetTex;
	LPDIRECT3DTEXTURE9 g_albumArt = NULL;
	LPDIRECT3DTEXTURE9 g_vortexLogo = NULL;

	int g_renderTarget = 0;
	int g_currPresetId = 0;
	int g_transitionId = 0;
	int g_loadPreset = -1;
	int* g_randomPresetIndices;
	float g_mainCounter = 7.0f;

	// User config settings
	UserSettings g_Settings;

	enum EState {STATE_RENDER_PRESET, STATE_TRANSITION};
	EState g_currentState = STATE_RENDER_PRESET;

	TrackInfo g_TrackInfo;

	FileHolder	g_PresetFiles;
	FileHolder	g_TransitionFiles;
	FileHolder	g_AnnouncementFiles;

	FFT g_fftobj;

	FLOAT fSecsPerTick;
	LARGE_INTEGER qwTime, qwLastTime, qwElapsedTime, qwAppTime, qwElapsedAppTime;
	FLOAT fTime, fElapsedTime, fAppTime, fElapsedAppTime;
	FLOAT fLastTime = 0;
	INT iFrames = 0;
	FLOAT fFPS = 0;
}

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
	srand(qwTime.QuadPart);

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

	// Keep track of the frame count

	iFrames++;

	// Update the scene stats once per second
	if( fAppTime - fLastTime > 1.0f )
	{
		fFPS = ( float )( iFrames / ( fAppTime - fLastTime ) );
		fLastTime = fAppTime;
		iFrames = 0;
	}

}

int GetRandomPreset()
{
	// Pick a random preset from the first 5 and then move that index to the end of the random list
	// This will bias the randomness so you don't see the same preset again until its back in the top 5
	int randomIndex = rand() % ( min( 5, g_PresetFiles.NumFiles() ) );
	int nextPreset = g_randomPresetIndices[ randomIndex ];

	for ( int i = randomIndex; i < g_PresetFiles.NumFiles() - 1; i++ )
	{
		g_randomPresetIndices[ i ] = g_randomPresetIndices[ i + 1 ];
	}
	g_randomPresetIndices[ g_PresetFiles.NumFiles() - 1 ] = nextPreset;

	return nextPreset;
}

void Vortex::Init( LPDIRECT3DDEVICE9 pD3DDevice, int iPosX, int iPosY, int iWidth, int iHeight, float fPixelRatio )
{
	InitTime();

	DebugConsole::Init();
	Renderer::Init( pD3DDevice, iPosX, iPosY, iWidth, iHeight, fPixelRatio );
	InitAngelScript();
	g_fftobj.Init(576, NUM_FREQUENCIES);

	char fullname[ 512 ];
	sprintf_s(fullname, 512, "%s%s", g_TexturePath, "Vortex-v.jpg" );
	g_vortexLogo = Renderer::LoadTexture(fullname);
	g_currPresetTex =  Renderer::CreateTexture(1024, 768);
	g_newPresetTex =  Renderer::CreateTexture(1024, 768);

	g_PresetFiles.GetFiles( g_PresetPath, "vtx" );

	if ( g_PresetFiles.NumFiles() == 0 )
	{
		return;
	}

	g_TransitionFiles.GetFiles( g_TransitionPath, "tra" );
	g_AnnouncementFiles.GetFiles( g_AnnouncePath, "ann" );

	g_randomPresetIndices = new int[ g_PresetFiles.NumFiles() ];

	for ( int i = 0; i < g_PresetFiles.NumFiles(); i++ )
	{
		g_randomPresetIndices[ i ] = i;
	}

	// Mix em up a bit
	for ( int i = 0; i < 100; i++ )
	{
		int a = rand() % g_PresetFiles.NumFiles();
		int b = rand() % g_PresetFiles.NumFiles();
		int old = g_randomPresetIndices[ a ];
		g_randomPresetIndices[ a ] = g_randomPresetIndices[ b ];
		g_randomPresetIndices[ b ] = old;
	}

	g_preset1.Init(m_pScriptEngine, "PRESET1");
	g_preset1.m_presetId = 0;
	g_preset2.Init(m_pScriptEngine, "PRESET2");
	g_preset2.m_presetId = 1;
	g_transition.Init(m_pScriptEngine, "TRANSITION");
	g_transition.m_presetId = 2;

	g_AnnouncePreset.Init(m_pScriptEngine, "ANNOUNCE");
	g_AnnouncePreset.m_presetId = 3;

	if ( g_Settings.PresetLocked )
	{
		// Check that the preset locked in the settings file is still valid
		if ( g_currPresetId >= g_PresetFiles.NumFiles() || g_currPresetId < 0 )
		{
			g_currPresetId = GetRandomPreset();
		}
	}
	else if ( g_Settings.RandomPresetsEnabled )
	{
		g_currPresetId = GetRandomPreset();
	}

	if ( g_currPresetId < 0 || g_currPresetId >= g_PresetFiles.NumFiles() )
	{
		g_currPresetId = 0;
	}
	char filename[ 256 ];
	sprintf( filename, "%s%s", g_PresetPath, g_PresetFiles.GetFilename( g_currPresetId ) );
	g_presets[ 0 ]->Begin( filename );
	g_mainCounter = g_Settings.TimeBetweenPresets + ((rand() % 100) / 100.0f) * g_Settings.TimeBetweenPresetsRand;
	g_currentState = STATE_RENDER_PRESET;
}

void Vortex::Start( int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName )
{
}

void Vortex::UpdateTrack( VisTrack* pVisTrack )
{
	g_TrackInfo.SetFromVisTrack( pVisTrack );

	// TODO: if song change announcements are enabled...
#if ENABLE_ANNOUNCE
	{
		char filename[ 512 ];
		sprintf(filename, "%s%s.ann", g_AnnouncePath, "Announce");
		g_AnnouncePreset.Begin( filename );
	}
#endif
}

void Vortex::Shutdown()
{
	g_preset1.End();
	g_preset2.End();
	g_transition.End();
	g_AnnouncePreset.End();

	delete[] g_randomPresetIndices;

	if ( m_pScriptEngine )
	{
		m_pScriptEngine->Release();
		m_pScriptEngine = NULL;
	}

	if ( g_vortexLogo )
	{
		Renderer::ReleaseTexture( g_vortexLogo );
	}

	if ( g_albumArt )
	{
		g_albumArt->Release();
		g_albumArt = NULL;
	}
	if ( g_currPresetTex )
	{
		g_currPresetTex->Release();
		g_currPresetTex = NULL;
	}
	if ( g_newPresetTex )
	{
		g_newPresetTex->Release();
		g_newPresetTex = NULL;
	}

	Renderer::Exit();
	g_fftobj.CleanUp();
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

struct SoundData
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

SoundData g_sound;
float g_bass;
float g_treble;
float g_middle;
float g_timePass;
bool g_finished;

void Vortex::AudioData( const short* pAudioData, int iAudioDataLength, float* pFreq, int iFreqDataLength )
{
	float tempWave[2][576];

	int iPos = 0;
	int iOld = 0;
	//const float SCALE = (1.0f / 32768.0f ) * 255.0f;
	while ( iPos < 576 )
	{
		for ( int i = 0; i < iAudioDataLength; i += 2 )
		{
			g_sound.fWaveform[ 0 ][ iPos ] = float( ( pAudioData[ i ] / 32768.0f ) * 255.0f );
			g_sound.fWaveform[ 1 ][ iPos ] = float( ( pAudioData[ i+1 ] / 32768.0f) * 255.0f );

			// damp the input into the FFT a bit, to reduce high-frequency noise:
			tempWave[ 0 ][ iPos ] = 0.5f * ( g_sound.fWaveform[ 0 ][ iPos ] + g_sound.fWaveform[ 0 ][ iOld ] );
			tempWave[ 1 ][ iPos ] = 0.5f * ( g_sound.fWaveform[ 1 ][ iPos ] + g_sound.fWaveform[ 1 ][ iOld ] );
			iOld = iPos;
			iPos++;
			if ( iPos >= 576 )
				break;
		}
	}

	g_fftobj.time_to_frequency_domain( tempWave[ 0 ], g_sound.fSpectrum[ 0 ] );
	g_fftobj.time_to_frequency_domain( tempWave[ 1 ], g_sound.fSpectrum[ 1 ] );
}

void AnalyzeSound()
{
	// Some bits of this were pinched from Milkdrop...
	int m_fps = 60;

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
	for (int ch=0; ch<2; ch++)
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


	float avg_mix;
	if (newTreble > g_treble)
		avg_mix = 0.5f;
	else 
		avg_mix = 0.5f;
	g_bass = g_bass*avg_mix + newBass*(1-avg_mix);
	g_middle = g_middle*avg_mix + newMiddle*(1-avg_mix);
	g_treble = g_treble*avg_mix + newTreble*(1-avg_mix);

	g_bass =   max(min(g_bass,   1.0f), -1.0f);
	g_middle = max(min(g_middle, 1.0f), -1.0f);
	g_treble = max(min(g_treble, 1.0f), -1.0f);
}

namespace
{
	float vl[256];
	float tm[256];
	float mx[256];
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

void UpdateSpectrum()
{
	int x;
	for (x=0;x<256;x=x+1)
		vl[x] = 0;
	for (x=0;x<512;x=x+1)
		vl[x/2] = vl[x/2] + GetSpecLeft(x)+GetSpecRight(x);

	for (int x = 0; x < 256; x++)
	{
		vl[x] = vl[x]/8;
		if (vl[x]>1.0)
			vl[x]=1.0;
	}
}

float GetSpec( int x )
{
	if ( x < 0 )
		x = 0;
	if ( x > 511 )
		x = 511;
	return vl[ x ];
}

void SwapPresets()
{
	g_presets[0]->End();
	g_transition.End();
	Preset* temp = g_presets[0];
	g_presets[0] = g_presets[1];
	g_presets[1] = temp;
}


void Vortex::Render()
{
	if ( g_PresetFiles.NumFiles() == 0)
	{
		return;
	}

	Renderer::GetBackBuffer();

	UpdateTime();

	g_timePass = 1.0f / 60.f;//fElapsedAppTime;
	g_timePass = fElapsedAppTime;

	AnalyzeSound();
	UpdateSpectrum();

	static bool doOnce = true;
	//---------------------------------------------------------------------------
	// Do rendering

	switch ( g_currentState )
	{
	case STATE_RENDER_PRESET:
		{
			g_renderTarget = TEXTURE_FRAMEBUFFER;
			Renderer::SetRenderTargetBackBuffer();
			Renderer::SetDefaults();
			Renderer::Clear(0);
			g_presets[0]->Render();
			break;
		}

	case STATE_TRANSITION:
		{
			if (doOnce)
			{
				Renderer::SetDefaults();
				Renderer::SetRenderTarget(g_currPresetTex);
				g_renderTarget = TEXTURE_CURRPRESET;
				Renderer::Clear(0);
				g_presets[0]->Render();
				doOnce = false;
			}

			Renderer::SetDefaults();
			Renderer::SetRenderTarget(g_newPresetTex);
			g_renderTarget = TEXTURE_NEXTPRESET;
			Renderer::Clear(0);
			g_presets[1]->Render();

			Renderer::SetDefaults();
			Renderer::SetRenderTargetBackBuffer();
			g_renderTarget = TEXTURE_FRAMEBUFFER;
			Renderer::Clear(0);
			g_transition.Render();
			break;
		}
	}

	// Restore state
	Renderer::SetRenderTargetBackBuffer();
	Renderer::SetDefaults();

	//---------------------------------------------------------------------------
	// Do state updating

	switch ( g_currentState )
	{
	case STATE_RENDER_PRESET:
		{
			//      OutputDebugString("STATE = STATE_RENDER_PRESET\n");
			if ( !g_Settings.PresetLocked )
			{
				g_mainCounter -= g_timePass;
			}

			if ( g_mainCounter <= 0 || g_loadPreset != -1 )
			{
				char filename[ 255 ];

				if ( g_mainCounter <= 0 )
				{
					// Not in a transition, preset not locked and time for a new preset
					if ( g_Settings.RandomPresetsEnabled )
					{
						int nextPreset = GetRandomPreset();	
						if ( nextPreset == g_currPresetId )
							g_currPresetId = (g_currPresetId+1) % g_PresetFiles.NumFiles();
						else
							g_currPresetId = nextPreset;
					}
					else
					{
						g_currPresetId = (g_currPresetId+1) % g_PresetFiles.NumFiles();
					}
				}
				
				if ( g_loadPreset != -1 )
				{
					// New preset requested
					g_currPresetId = g_loadPreset;
					g_loadPreset = -1;
				}

				g_finished = true;
				g_mainCounter = g_Settings.TimeBetweenPresets + ((rand() % 100) / 100.0f) * g_Settings.TimeBetweenPresetsRand;

				// Load preset
				sprintf(filename, "%s%s", g_PresetPath, g_PresetFiles.GetFilename( g_currPresetId ) );
				if ( g_presets[ 1 ]->Begin( filename ) == true )
				{
					// Load and begin transition
					if ( g_Settings.TransitionsEnabled && g_TransitionFiles.NumFiles() != 0 )
					{
						g_transitionId = ( g_transitionId + 1 ) % g_TransitionFiles.NumFiles();
						sprintf( filename, "%s%s", g_TransitionPath, g_TransitionFiles.GetFilename( g_transitionId ) );
						if ( g_transition.Begin( filename ) == true )
						{
							// Transition loading succeeded
							g_finished = false;
							doOnce = true;
							g_currentState = STATE_TRANSITION;
						}
						else
						{
							SwapPresets();
						}
					}
					else
					{
						SwapPresets();
					}
				}
			}
			break;
		}

	case STATE_TRANSITION:
		{
			//      OutputDebugString("STATE = STATE_TRANSITION\n");
			if (g_finished)
			{
				g_mainCounter = g_Settings.TimeBetweenPresets + ((rand() % 100) / 100.0f) * g_Settings.TimeBetweenPresetsRand;
				SwapPresets();
				g_finished = false;

				g_currentState = STATE_RENDER_PRESET;
			}
			break;
		}
	}

	//----------------------------------------------------------------------------
	// Render announcement overlay if there is one
	if ( g_AnnouncePreset.IsValid() )
	{
		g_finished = false;
		Renderer::ClearDepth();
		g_AnnouncePreset.Render();
		if ( g_finished )
			g_AnnouncePreset.End();
	}

	Renderer::SetDefaults();
	Renderer::SetDrawMode2d();

//	Renderer::Clear( 0 );
// 	Renderer::SetDefaults();
// 	Preset1.Render();

//	Renderer::SetDefaults();
//	Renderer::SetDrawMode2d();
//	Renderer::Rect( -1.0, -1.0, 1.0, 1.0, 0xff000000 );
/*

	if ( g_Settings.ShowAudioAnalysis )
	{
		FLOAT BAR_WIDTH = 1.0f / 128;
		/ *
		for(int i = 0; i < 128; i++)
		{
		/ *if(bar_heights[i] > 4)
		bar_heights[i] -= 4;
		else
		bar_heights[i] = 0;* /
		//		Renderer::Rect( ( i * (WIDTH / NUM_BANDS) ) / 512.0f,
		//						 ( HEIGHT - 1 - bar_heights[i] ) / 512.0f,
		//						 ( i * (WIDTH / NUM_BANDS ) + (WIDTH / NUM_BANDS) - 1 ) / 512.0f,
		//						 bar_heights[i] / 512.0f, 0xff0000ff);

		//		Renderer::Rect( -1 + (i * BAR_WIDTH), 1, (-1 + (i+1)) * BAR_WIDTH, 1 - (bar_heights[ i ] / 200.0f), 0xff0000ff);
		Renderer::Rect( -0.5f + (i * BAR_WIDTH), 0, -0.5f + ((i+1) * BAR_WIDTH), 0 - (bars[ i ] / 100.0f), 0xff0000ff);

		//		Renderer::Rect( -0.5f + (i * BAR_WIDTH), 1, -0.5f + ((i+1) * BAR_WIDTH), 1 - (SpecAvgLeft[ i ] / 100.0f), 0xffff00ff);

		//		Renderer::Rect( -0.5 + (i * BAR_WIDTH), 1 - (SpecAvgLeft[ i ] / 100.0f), -0.5 + ((i+1) * BAR_WIDTH), 1 - (SpecAvgLeft[ i ] / 100.0f), 0xffffffff);

		// 		Renderer::Line(-0.5f + (i * BAR_WIDTH),
		// 						1 - (SpecMedAvgLeft[ i ] / 100.0f),
		// 						-0.5f + ((i+1) * BAR_WIDTH),
		// 						1 - (SpecMedAvgLeft[ i ] / 100.0f),
		// 						0xffffffff );
		}
		* /

		//	Renderer::Colour(1, 0, 0, 1);
		Renderer::Rect(-0.5f, 0, -0.25f, -g_bass*0.5f, 0xffff0000);
		//	Renderer::Colour(1, 1, 1, 1);
		Renderer::Rect(-0.5f, 1, -0.25f, 1-g_sound.med_avg[0][0]*0.07f, 0xffffffff);
		//	Renderer::Colour(1, 0, 0, 1);
		Renderer::Rect(-0.4f, 1, -0.35f, 1-g_sound.avg[0][0]*0.07f, 0xffff0000);

		// 	Renderer::Colour(0, 1, 0, 1);
		Renderer::Rect(-0.125f, 0, 0.125f, -g_middle*0.5f, 0xff00ff00);
		// 	Renderer::Colour(1, 1, 1, 1);
		Renderer::Rect(-0.125f, 1, 0.125f, 1-g_sound.med_avg[0][1]*0.07f, 0xffffffff);
		//	Renderer::Colour(0, 1, 0, 1);
		Renderer::Rect(-0.025f, 1, 0.025f, 1-g_sound.avg[0][1]*0.07f, 0xff00ff00);

		// 	Renderer::Colour(0, 0, 1, 1);
		Renderer::Rect(0.25f, 0, 0.5f, -g_treble*0.5f, 0xff0000ff);
		// 	Renderer::Colour(1, 1, 1, 1);
		Renderer::Rect(0.25f, 1, 0.5f, 1-g_sound.med_avg[0][2]*0.07f, 0xffffffff);
		//	Renderer::Colour(0, 0, 1, 1);
		Renderer::Rect(0.35f, 1, 0.4f, 1-g_sound.avg[0][2]*0.07f, 0xff0000ff);

		{
			for (int x = 0; x < 256; x++)
			{
				// 			vl[x] = vl[x]/8;
				// 			if (vl[x]>1.0)
				// 				vl[x]=1.0;

				float xPos = x / 128.0f;

				Renderer::Begin( D3DPT_TRIANGLEFAN );

				Renderer::Colour(0,0,1,1);
				Renderer::Vertex(-1+xPos+(1/128.0f),1,0);
				Renderer::Vertex(-1+xPos,1,0);
				Renderer::Colour(0,vl[x],1, 1);
				Renderer::Vertex(-1+xPos,1-vl[x]*0.4f,0);      
				Renderer::Vertex(-1+xPos+(1/128.0f),1-vl[x]*0.4f,0);
				/ *
				gfxColour(1,1,1,1);
				gfxVertex(xPos+(1/128.0f),mx[x]*0.4,0);
				gfxVertex(xPos,mx[x]*0.4,0);
				gfxVertex(xPos,(mx[x]-0.01)*0.4,0);
				gfxVertex(xPos+(1/128.0f),(mx[x]-0.01)*0.4,0);
				* /
				Renderer::End();
			}
		}
	}

*/
	if( g_Settings.ShowDebugConsole )
	{
		DebugConsole::Render();
	}

	if( g_Settings.ShowFPS )
	{
		char FrameRate[256];
		sprintf_s(FrameRate, 256, "FPS = %0.02f\n", fFPS );
		Renderer::DrawText(0, 0, FrameRate, 0xffffffff );
	}

	Renderer::ReleaseBackbuffer();
}

// Function implementation with native calling convention
void PrintString(string &str)
{
	DebugConsole::Log( str.c_str() );
}

// Function implementation with generic script interface
void PrintString_Generic(asIScriptGeneric *gen)
{
	string *str = (string*)gen->GetArgAddress(0);
	DebugConsole::Log( str->c_str() );
}

// Function wrapper is needed when native calling conventions are not supported
void timeGetTime_Generic(asIScriptGeneric *gen)
{
	gen->SetReturnDWord(timeGetTime());
}

#define assert(x) (x)

void ConfigureEngine( asIScriptEngine* engine )
{
	int r;

	// Register the script string type
	// Look at the implementation for this function for more information  
	// on how to register a custom string type, and other object types.
	// The implementation is in "/add_on/scriptstring/scriptstring.cpp"
	RegisterScriptString(engine);

	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Register the functions that the scripts will be allowed to use.
		// Note how the return code is validated with an assert(). This helps
		// us discover where a problem occurs, and doesn't pollute the code
		// with a lot of if's. If an error occurs in release mode it will
		// be caught when a script is being built, so it is not necessary
		// to do the verification here as well.
		r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterGlobalFunction("uint GetSystemTime()", asFUNCTION(timeGetTime), asCALL_STDCALL); assert( r >= 0 );
	}
	else
	{
		// Notice how the registration is almost identical to the above. 
		r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterGlobalFunction("uint GetSystemTime()", asFUNCTION(timeGetTime_Generic), asCALL_GENERIC); assert( r >= 0 );
	}


	// It is possible to register the functions, properties, and types in 
	// configuration groups as well. When compiling the scripts it then
	// be defined which configuration groups should be available for that
	// script. If necessary a configuration group can also be removed from
	// the engine, so that the engine configuration could be changed 
	// without having to recompile all the scripts.
}

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	char txt[512];
	sprintf_s(txt, 512, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);

	if ( msg->type == asMSGTYPE_INFORMATION )
	{
		DebugConsole::Log( txt );
	}
	else if ( msg->type == asMSGTYPE_WARNING )
	{
		DebugConsole::Warn( txt );
	}
	else
	{
		DebugConsole::Error( txt );
	}
}

float GetWaveLeft(int index)
{
	if ( index < 0 )
		index = 0;
	else if ( index > 575 )
		index = 575;
	return g_sound.fWaveform[0][index] * (1.0f / 255.0f);
}

float GetWaveRight(int index)
{
	if ( index < 0 )
		index = 0;
	else if ( index > 575 )
		index = 575;
	return g_sound.fWaveform[1][index] * (1.0f / 255.0f);
}

void SetUserEnvTexture(Texture& texture)
{
	Renderer::SetEnvTexture(texture.m_pTexture);

	texture.Release();
}

void SetUserTexture(Texture& texture)
{
	Renderer::SetTexture(texture.m_pTexture);

	texture.Release();
}

float Mag(float a, float b)
{
	return sqrtf(a*a + b*b);
}

float Rand()
{
	return (rand() % 100 ) / 100.0f;
}

void SetUserRenderTarget(Texture& texture)
{
	if (texture.m_renderTarget && texture.m_pTexture)
	{
		Renderer::SetRenderTarget(texture.m_pTexture);
	}
	else
	{
		Renderer::SetRenderTargetBackBuffer();
	}

	texture.Release();
}

void SetRenderTarget(int target)
{
	if (g_renderTarget== TEXTURE_FRAMEBUFFER)
	{
		// Current frame buffer
		Renderer::SetRenderTargetBackBuffer();
	}
	else if (g_renderTarget == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer::SetRenderTarget(g_currPresetTex);
	}
	else if (g_renderTarget == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer::SetRenderTarget(g_newPresetTex);
	}
}

void SetTexture(int textureId)
{
	if (textureId == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer::SetTexture(g_currPresetTex);
	}
	else if (textureId == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer::SetTexture(g_newPresetTex);
	}
	else if (textureId == TEXTURE_ALBUMART)
	{
		if ( g_albumArt )
			Renderer::SetTexture( g_albumArt );
		else
			Renderer::SetTexture( g_vortexLogo );
	}
	else
	{
		Renderer::SetTexture(NULL);
	}

} // SetTexture

void SetEnvTexture(int textureId)
{
	if (textureId == TEXTURE_CURRPRESET)
	{
		// Current preset
		Renderer::SetEnvTexture(g_currPresetTex);
	}
	else if (textureId == TEXTURE_NEXTPRESET)
	{
		// Next preset
		Renderer::SetEnvTexture(g_newPresetTex);
	}
	else if (textureId == TEXTURE_ALBUMART)
	{
		if ( g_albumArt /*&& g_useAlbumArt*/ )
			Renderer::SetTexture( g_albumArt );
		else
			//			Renderer_c::SetTexture(g_vortexLogo);
			Renderer::SetEnvTexture( NULL );
	}
	else
	{
		Renderer::SetEnvTexture(NULL);
	}

} // SetTexture



int MyAbs( int a )
{
	return  a < 0 ? -a : a;
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

void SetEffectTexture( EffectBase& rEffect )
{
	Renderer::SetTexture( rEffect.GetTexture() );
	rEffect.Release();
}

void SetEffectRenderTarget( EffectBase& rEffect )
{
	Renderer::SetRenderTarget( rEffect.GetRenderTarget() );

	rEffect.Release();
}

void DrawMesh( Mesh& pMesh )
{
	Renderer::DrawMesh( pMesh.GetMesh() );
	pMesh.Release();
}

void NULLFunction()
{

}

bool Vortex::InitAngelScript()
{
	// Create the script engine
	m_pScriptEngine = asCreateScriptEngine( ANGELSCRIPT_VERSION );
	if ( m_pScriptEngine == 0 )
	{
		DebugConsole::Error( "AngelScript: Failed to create script engine.\n" );
		return false;
	}

	// The script compiler will write any compiler messages to the callback.
	m_pScriptEngine->SetMessageCallback( asFUNCTION( MessageCallback ), 0, asCALL_CDECL );

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine( m_pScriptEngine );

	int r;

	//---------------------------------------------------------------------------
	// Math functions
	r = m_pScriptEngine->RegisterGlobalFunction("float Rand()", asFUNCTION(Rand), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Mag(float, float)", asFUNCTION(Mag), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Clamp(float, float, float)", asFUNCTION(FloatClamp), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("int Clamp(int, int, int)", asFUNCTION(IntClamp), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Fabs(float)", asFUNCTION(fabsf), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("int Abs(int)", asFUNCTION(MyAbs), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Atan2(float, float)", asFUNCTION(atan2f), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTION(powf), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTION(sqrtf), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float Atan(float)", asFUNCTION(atanf), asCALL_CDECL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Global vars
  	r = m_pScriptEngine->RegisterGlobalProperty("const float TIMEPASS", &g_timePass); assert(r >= 0);
  	r = m_pScriptEngine->RegisterGlobalProperty("bool FINISHED", &g_finished); assert(r >= 0);

  	r = m_pScriptEngine->RegisterGlobalProperty("const float BASS", &g_bass); assert(r >= 0);
  	r = m_pScriptEngine->RegisterGlobalProperty("const float MIDDLE", &g_middle); assert(r >= 0);
  	r = m_pScriptEngine->RegisterGlobalProperty("const float TREBLE", &g_treble); assert(r >= 0);

	r = m_pScriptEngine->RegisterGlobalFunction("float WaveLeft(int)", asFUNCTION(GetWaveLeft), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("float WaveRight(int)", asFUNCTION(GetWaveRight), asCALL_CDECL); assert(r >= 0);
// 	r = g_scriptEngine->RegisterGlobalFunction("float GetSpec(int)", asFUNCTION(GetSpec), asCALL_CDECL); assert(r >= 0);
 	r = m_pScriptEngine->RegisterGlobalFunction("float GetSpecLeft(int)", asFUNCTION(GetSpecLeft), asCALL_CDECL); assert(r >= 0);
 	r = m_pScriptEngine->RegisterGlobalFunction("float GetSpecRight(int)", asFUNCTION(GetSpecRight), asCALL_CDECL); assert(r >= 0);

	r = m_pScriptEngine->RegisterGlobalFunction("float GetSpec(int)", asFUNCTION(GetSpec), asCALL_CDECL); assert(r >= 0);

	//----------------------------------------------------------------------------
	// Register Track Info
	r = m_pScriptEngine->RegisterObjectType("TrackInfo", sizeof(TrackInfo), asOBJ_VALUE | asOBJ_POD); assert( r >= 0 );

	// Register the object properties
	r = m_pScriptEngine->RegisterObjectProperty("const TrackInfo", "string Title", offsetof(TrackInfo, Title)); assert( r >= 0 );
	r = m_pScriptEngine->RegisterObjectProperty("const TrackInfo", "int TrackNumber", offsetof(TrackInfo, Title)); assert( r >= 0 );

	r = m_pScriptEngine->RegisterGlobalProperty("const TrackInfo CurrentTrackInfo", &g_TrackInfo); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Renderer functions
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxBegin(uint)", asFUNCTION(Renderer::Begin), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxEnd()", asFUNCTION(Renderer::End), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxClear(uint)", asFUNCTION(Renderer::Clear), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxClear(float r, float g, float b)", asFUNCTION(Renderer::ClearFloat), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxColour(float r, float g, float b, float a)", asFUNCTION(Renderer::Colour), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxNormal(float nx, float ny, float nz)", asFUNCTION(Renderer::Normal), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxTexCoord(float u, float v)", asFUNCTION(Renderer::TexCoord), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxVertex(float x, float y, float z)", asFUNCTION(Renderer::Vertex), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxCube(float nx, float ny, float nz, float x, float y, float z)", asFUNCTION(Renderer::Cube), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSphere(float size)", asFUNCTION(Renderer::SimpleSphere), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxPushMatrix()", asFUNCTION(Renderer::PushMatrix), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxPopMatrix()", asFUNCTION(Renderer::PopMatrix), asCALL_CDECL); assert(r >= 0);
//	r = m_pScriptEngine->RegisterGlobalFunction("void gfxRotate(float x, float y, float z)", asFUNCTION(Renderer::Rotate), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxRotate(float angle, float x, float y, float z)", asFUNCTION(Renderer::RotateAxis), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxTranslate(float x, float y, float z)", asFUNCTION(Renderer::Translate), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxScale(float x, float y, float z)", asFUNCTION(Renderer::Scale), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)", asFUNCTION(Renderer::LookAt), asCALL_CDECL); assert(r >= 0);

//	r = m_pScriptEngine->RegisterGlobalFunction("void gfxRect(float x1, float y1, float x2, float y2)", asFUNCTION(Renderer::Rect), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxTexRect(float x1, float y1, float x2, float y2)", asFUNCTION(Renderer::TexRect), asCALL_CDECL); assert(r >= 0);
//	//  r = g_scriptEngine->RegisterGlobalFunction("void gfxSetDrawMode2d()", asFUNCTION(Renderer::SetDrawMode2d), asCALL_CDECL); assert(r >= 0);
//	//  r = g_scriptEngine->RegisterGlobalFunction("void gfxSetDrawMode3d()", asFUNCTION(Renderer::SetDrawMode3d), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetBlendMode(int)", asFUNCTION(Renderer::SetBlendMode), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetAspect(float)", asFUNCTION(Renderer::SetAspect), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetFillMode(int)", asFUNCTION(Renderer::SetFillMode), asCALL_CDECL); assert(r >= 0);

	// Not supported functions from Xbox Vortex
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetLineWidth(float)", asFUNCTION(NULLFunction), asCALL_CDECL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Texture Object
	// Registering the reference type
	r = m_pScriptEngine->RegisterObjectType("Texture", 0, asOBJ_REF); assert( r >= 0 );
	// Registering the factory behaviour
	r = m_pScriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_FACTORY, "Texture@ f()", asFUNCTION(Texture_Factory), asCALL_CDECL); assert( r >= 0 );
	// Registering the addref/release behaviours
 	r = m_pScriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_ADDREF, "void f()", asMETHOD(Texture,AddRef), asCALL_THISCALL); assert( r >= 0 );
 	r = m_pScriptEngine->RegisterObjectBehaviour("Texture", asBEHAVE_RELEASE, "void f()", asMETHOD(Texture,Release), asCALL_THISCALL); assert( r >= 0 );
 	r = m_pScriptEngine->RegisterObjectMethod("Texture", "void CreateTexture()", asMETHOD(Texture, CreateTexture), asCALL_THISCALL); assert(r >= 0);
 	r = m_pScriptEngine->RegisterObjectMethod("Texture", "void LoadTexture(string&)", asMETHOD(Texture, LoadTexture), asCALL_THISCALL); assert(r >= 0);

	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetEnvTexture(uint)", asFUNCTION(SetEnvTexture), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetEnvTexture(Texture@)", asFUNCTION(SetUserEnvTexture), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetTexture(Texture@)", asFUNCTION(SetUserTexture), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetTexture(uint)", asFUNCTION(SetTexture), asCALL_CDECL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Mesh Object
	// Registering the reference type
	r = m_pScriptEngine->RegisterObjectType("Mesh", 0, asOBJ_REF); assert( r >= 0 );
	// Registering the factory behaviour
	r = m_pScriptEngine->RegisterObjectBehaviour("Mesh", asBEHAVE_FACTORY, "Mesh@ f()", asFUNCTION(Mesh_Factory), asCALL_CDECL); assert( r >= 0 );
	// Registering the addref/release behaviours
	r = m_pScriptEngine->RegisterObjectBehaviour("Mesh", asBEHAVE_ADDREF, "void f()", asMETHOD(Mesh,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = m_pScriptEngine->RegisterObjectBehaviour("Mesh", asBEHAVE_RELEASE, "void f()", asMETHOD(Mesh,Release), asCALL_THISCALL); assert( r >= 0 );
	r = m_pScriptEngine->RegisterObjectMethod("Mesh", "void CreateTextMesh(string&, bool bCentered)", asMETHOD(Mesh, CreateTextMesh), asCALL_THISCALL); assert(r >= 0);

	r = m_pScriptEngine->RegisterGlobalFunction("void gfxDrawMesh(Mesh@)", asFUNCTION(DrawMesh), asCALL_CDECL); assert(r >= 0);


	//---------------------------------------------------------------------------
	// Register the different effects
	EffectBase::RegisterScriptInterface( m_pScriptEngine );
	Map::RegisterScriptInterface( m_pScriptEngine );
	VoicePrint::RegisterScriptInterface( m_pScriptEngine );
	Tunnel::RegisterScriptInterface( m_pScriptEngine );

	//---------------------------------------------------------------------------
	// Texture Functions
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(Texture@)", asFUNCTION(SetUserRenderTarget), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(int)", asFUNCTION(SetRenderTarget), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetRenderTarget(EffectBase@)", asFUNCTION(SetEffectRenderTarget), asCALL_CDECL); assert(r >= 0);
	r = m_pScriptEngine->RegisterGlobalFunction("void gfxSetTexture(EffectBase@)", asFUNCTION(SetEffectTexture), asCALL_CDECL); assert(r >= 0);

	return true;
}

void Vortex::UpdateAlbumArt( char* artFilename )
{
	Renderer::ReleaseTexture( g_albumArt );
	g_albumArt = Renderer::LoadTexture( artFilename );
}

int Vortex::GetCurrentPresetIndex()
{
	return g_currPresetId;
}

UserSettings& Vortex::GetUserSettings()
{
	return g_Settings;
}

int Vortex::GetPresets( char*** Presets )
{
	*Presets = g_PresetFiles.GetAllFilenames();
	return g_PresetFiles.NumFiles();
}

void Vortex::LoadNextPreset()
{
	g_loadPreset = ( g_currPresetId + 1 ) % g_PresetFiles.NumFiles();
}

void Vortex::LoadPreviousPreset()
{
	g_loadPreset = g_currPresetId - 1;
	if ( g_loadPreset < 0 )
	{
		g_loadPreset = g_PresetFiles.NumFiles() - 1;
	}
}

void Vortex::LoadRandomPreset()
{
	g_loadPreset = GetRandomPreset();
}

void Vortex::LoadPreset( int PresetId )
{
	g_loadPreset = PresetId;
}
