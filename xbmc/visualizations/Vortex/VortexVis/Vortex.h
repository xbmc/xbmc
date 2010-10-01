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

#ifndef VORTEX_H
#include <vector>
#include <XTL.h>

using namespace std;

// the action commands
enum VIS_ACTION { VIS_ACTION_NONE = 0,
VIS_ACTION_NEXT_PRESET,
VIS_ACTION_PREV_PRESET,
VIS_ACTION_LOAD_PRESET,
VIS_ACTION_RANDOM_PRESET,
VIS_ACTION_LOCK_PRESET,
VIS_ACTION_RATE_PRESET_PLUS,
VIS_ACTION_RATE_PRESET_MINUS,
VIS_ACTION_UPDATE_ALBUMART};


// The VisSetting class for GUI settings for vis.
class VisSetting
{
public:
	enum SETTING_TYPE { NONE=0, CHECK, SPIN };
	VisSetting(SETTING_TYPE t, const char *label)
	{
		name = NULL;
		if (label)
		{
			name = new char[strlen(label)+1];
			strcpy(name, label);
		}
		current = 0;
		type = t;
	};
	VisSetting(const VisSetting &rhs) // copy constructor
	{
		name = NULL;
		if (rhs.name)
		{
			name = new char[strlen(rhs.name)+1];
			strcpy(name, rhs.name);
		}
		current = rhs.current;
		type = rhs.type;
		for (unsigned int i = 0; i < rhs.entry.size(); i++)
		{
			char *lab = new char[strlen(rhs.entry[i]) + 1];
			strcpy(lab, rhs.entry[i]);
			entry.push_back(lab);
		}
	}
	~VisSetting()
	{
		if (name)
			delete[] name;
		for (unsigned int i=0; i < entry.size(); i++)
			delete[] entry[i];
	}
	void AddEntry(const char *label)
	{
		if (!label || type != SPIN) return;
		char *lab = new char[strlen(label) + 1];
		strcpy(lab, label);
		entry.push_back(lab);
	}
	SETTING_TYPE type;
	char *name;
	int  current;
	vector<const char *> entry;
};

class Vortex_c
{
public:
	bool Init(LPDIRECT3DDEVICE8 pd3dDevice, int iXpos, int yPos, int iWidth, int iHeight, float pixelRatio, char* xmlFile);
	void Stop();
	void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	void Render();
	void Render2();

	// Settings
	void ShowBeatDetection(bool show);
	void RandomPresets(bool random);
	void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
	void LoadNextPreset();
	void LoadPreviousPreset();
	void LoadPreset(int id);
	void LoadRandomPreset();
	void ToggleLockPreset();
	void UpdateAlbumArt(char* artFilename);

	void LoadSettings(char* filename);
	void SaveSettings(char* filename);

	float GetTimeBetweenPresets();
	float GetTimeBetweenPresetsRand();
	void SetTimeBetweenPresets(float time);
	void SetTimeBetweenPresetsRand(float time);
	bool GetBeatDetection();
	bool GetRandomPresets();
	bool GetUseAlbumArt();
	void SetUseAlbumArt(bool use);
	vector<VisSetting> *GetSettings();
	void UpdateSettings(int num);
	bool OnAction(long flags, void *param);

private:
	void GetPresets(const char* presetDir);
	void MergeSortPresets(int left, int right);
	bool InitAngelScript();

	char* m_presets;
	char** m_presetAddr;
	int	m_sizeOfPresetList;
	int m_numPresets;

	char* m_transitions;
	char** m_transitionAddr;
	int m_sizeOfTransitionList;
	int m_numTransitions;

};

#endif