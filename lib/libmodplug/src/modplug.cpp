/*
 * This source code is public domain.
 *
 * Authors: Kenton Varda <temporal@gauge3d.org> (C interface wrapper)
 */

#include "modplug.h"
#include "stdafx.h"
#include "sndfile.h"

struct _ModPlugFile
{
	CSoundFile mSoundFile;
};

namespace ModPlug
{
	ModPlug_Settings gSettings =
	{
		MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION,

		2,
		16,
		44100,
		MODPLUG_RESAMPLE_LINEAR,

		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	int gSampleSize;

	void UpdateSettings(bool updateBasicConfig)
	{
		if(gSettings.mFlags & MODPLUG_ENABLE_REVERB)
		{
			CSoundFile::SetReverbParameters(gSettings.mReverbDepth,
			                                gSettings.mReverbDelay);
		}

		if(gSettings.mFlags & MODPLUG_ENABLE_MEGABASS)
		{
			CSoundFile::SetXBassParameters(gSettings.mBassAmount,
			                               gSettings.mBassRange);
		}
		else // modplug seems to ignore the SetWaveConfigEx() setting for bass boost
			CSoundFile::SetXBassParameters(0, 0);

		if(gSettings.mFlags & MODPLUG_ENABLE_SURROUND)
		{
			CSoundFile::SetSurroundParameters(gSettings.mSurroundDepth,
			                                  gSettings.mSurroundDelay);
		}

		if(updateBasicConfig)
		{
			CSoundFile::SetWaveConfig(gSettings.mFrequency,
			                          gSettings.mBits,
			                          gSettings.mChannels);

			gSampleSize = gSettings.mBits / 8 * gSettings.mChannels;
		}

		CSoundFile::SetWaveConfigEx(gSettings.mFlags & MODPLUG_ENABLE_SURROUND,
		                            !(gSettings.mFlags & MODPLUG_ENABLE_OVERSAMPLING),
		                            gSettings.mFlags & MODPLUG_ENABLE_REVERB,
		                            true,
		                            gSettings.mFlags & MODPLUG_ENABLE_MEGABASS,
		                            gSettings.mFlags & MODPLUG_ENABLE_NOISE_REDUCTION,
		                            false);
		CSoundFile::SetResamplingMode(gSettings.mResamplingMode);
	}
}

ModPlugFile* ModPlug_Load(const void* data, int size)
{
	ModPlugFile* result = new ModPlugFile;
	ModPlug::UpdateSettings(true);
	if(result->mSoundFile.Create((const BYTE*)data, size))
	{
		result->mSoundFile.SetRepeatCount(ModPlug::gSettings.mLoopCount);
		return result;
	}
	else
	{
		delete result;
		return NULL;
	}
}

void ModPlug_Unload(ModPlugFile* file)
{
	file->mSoundFile.Destroy();
	delete file;
}

int ModPlug_Read(ModPlugFile* file, void* buffer, int size)
{
	return file->mSoundFile.Read(buffer, size) * ModPlug::gSampleSize;
}

const char* ModPlug_GetName(ModPlugFile* file)
{
	return file->mSoundFile.GetTitle();
}

int ModPlug_GetLength(ModPlugFile* file)
{
	return file->mSoundFile.GetSongTime() * 1000;
}

void ModPlug_InitMixerCallback(ModPlugFile* file,ModPlugMixerProc proc)
{
	file->mSoundFile.gpSndMixHook = (LPSNDMIXHOOKPROC)proc ;
	return;
}

void ModPlug_UnloadMixerCallback(ModPlugFile* file)
{
	file->mSoundFile.gpSndMixHook = NULL;
	return ;
}

unsigned int ModPlug_GetMasterVolume(ModPlugFile* file)
{
	return (unsigned int)file->mSoundFile.m_nMasterVolume;
}

void ModPlug_SetMasterVolume(ModPlugFile* file,unsigned int cvol)
{
	(void)file->mSoundFile.SetMasterVolume( (UINT)cvol,
						FALSE );
	return ;
}

int ModPlug_GetCurrentSpeed(ModPlugFile* file)
{
	return file->mSoundFile.m_nMusicSpeed;
}

int ModPlug_GetCurrentTempo(ModPlugFile* file)
{
	return file->mSoundFile.m_nMusicTempo;
}

int ModPlug_GetCurrentOrder(ModPlugFile* file)
{
	return file->mSoundFile.GetCurrentOrder();
}

int ModPlug_GetCurrentPattern(ModPlugFile* file)
{
	return file->mSoundFile.GetCurrentPattern();
}

int ModPlug_GetCurrentRow(ModPlugFile* file)
{
	return file->mSoundFile.m_nRow;
}

int ModPlug_GetPlayingChannels(ModPlugFile* file)
{
	return ( file->mSoundFile.m_nMixChannels < file->mSoundFile.m_nMaxMixChannels ? file->mSoundFile.m_nMixChannels : file->mSoundFile.m_nMaxMixChannels );
}

void ModPlug_SeekOrder(ModPlugFile* file,int order)
{
	file->mSoundFile.SetCurrentOrder(order);
}

int ModPlug_GetModuleType(ModPlugFile* file)
{
	return file->mSoundFile.m_nType;
}

char* ModPlug_GetMessage(ModPlugFile* file)
{
	return file->mSoundFile.m_lpszSongComments;
}

#ifndef MODPLUG_NO_FILESAVE
char ModPlug_ExportS3M(ModPlugFile* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveS3M(filepath,0);
}

char ModPlug_ExportXM(ModPlugFile* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveXM(filepath,0);
}

char ModPlug_ExportMOD(ModPlugFile* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveMod(filepath,0);
}

char ModPlug_ExportIT(ModPlugFile* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveIT(filepath,0);
}
#endif // MODPLUG_NO_FILESAVE

unsigned int ModPlug_NumInstruments(ModPlugFile* file)
{
	return file->mSoundFile.m_nInstruments;
}

unsigned int ModPlug_NumSamples(ModPlugFile* file)
{
	return file->mSoundFile.m_nSamples;
}

unsigned int ModPlug_NumPatterns(ModPlugFile* file)
{
	return file->mSoundFile.GetNumPatterns();
}

unsigned int ModPlug_NumChannels(ModPlugFile* file)
{
	return file->mSoundFile.GetNumChannels();
}

unsigned int ModPlug_SampleName(ModPlugFile* file,unsigned int qual,char* buff)
{
	return file->mSoundFile.GetSampleName(qual,buff);
}

unsigned int ModPlug_InstrumentName(ModPlugFile* file,unsigned int qual,char* buff)
{
	return file->mSoundFile.GetInstrumentName(qual,buff);
}

ModPlugNote* ModPlug_GetPattern(ModPlugFile* file,int pattern,unsigned int* numrows) {
	if ( pattern<MAX_PATTERNS ) {
		if (file->mSoundFile.Patterns[pattern]) {
			if (numrows) *numrows=(unsigned int)file->mSoundFile.PatternSize[pattern];
			return (ModPlugNote*)file->mSoundFile.Patterns[pattern];
		}
	}
	return NULL;
}

void ModPlug_Seek(ModPlugFile* file, int millisecond)
{
	int maxpos;
	int maxtime = file->mSoundFile.GetSongTime() * 1000;
	float postime;

	if(millisecond > maxtime)
		millisecond = maxtime;
	maxpos = file->mSoundFile.GetMaxPosition();
	postime = (float)maxpos / (float)maxtime;

	file->mSoundFile.SetCurrentPos((int)(millisecond * postime));
}

void ModPlug_GetSettings(ModPlug_Settings* settings)
{
	memcpy(settings, &ModPlug::gSettings, sizeof(ModPlug_Settings));
}

void ModPlug_SetSettings(const ModPlug_Settings* settings)
{
	memcpy(&ModPlug::gSettings, settings, sizeof(ModPlug_Settings));
	ModPlug::UpdateSettings(false); // do not update basic config.
}
