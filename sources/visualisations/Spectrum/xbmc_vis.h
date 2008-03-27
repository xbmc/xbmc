#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#include <vector>
#include <xtl.h>

using namespace std;

int htoi(const char *str) /* Convert hex string to integer */
{
	unsigned int digit, number = 0;
	while (*str)
	{
		if (isdigit(*str))
			digit = *str - '0';
		else
			digit = tolower(*str)-'a'+10;
		number<<=4;
		number+=digit;
		str++;
	}
	return number;  
}

//#define NEW_STRING(str, ch) { str = new char[strlen(ch) + 1]; strcpy(str, ch); };

#pragma comment (lib, "lib/xbox_dx8.lib" )

extern "C"
{
  // exports for d3d hacks
  void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
  void d3dSetRenderState(DWORD dwY, DWORD dwZ);

  // The VIS_INFO structure to tell XBMC what data you need.
	struct VIS_INFO
  {
		bool bWantsFreq;
		int iSyncDelay;
		//		int iAudioDataLength;
		//		int iFreqDataLength;
	};

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
  // the settings vector
  vector<VisSetting> m_vecSettings;

  // the action commands
  #define VIS_ACTION_NEXT_PRESET    1
  #define VIS_ACTION_PREV_PRESET    2
  #define VIS_ACTION_LOAD_PRESET    3
  #define VIS_ACTION_RANDOM_PRESET  4
  #define VIS_ACTION_LOCK_PRESET    5

  // Functions that your visualisation must implement
  void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  bool OnAction(long action, void *param);
  void GetInfo(VIS_INFO* pInfo);
  void GetSettings(vector<VisSetting> **vecSettings);
  void UpdateSetting(int num);
  void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);

  // Structure to transfer the above functions to XBMC
  struct Visualisation
  {
    void (__cdecl *Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName);
    void (__cdecl *Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl *AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl *Render)();
    void (__cdecl *Stop)();
    void (__cdecl *GetInfo)(VIS_INFO* pInfo);
    bool (__cdecl *OnAction)(long action, void *param);
    void (__cdecl *GetSettings)(vector<VisSetting> **vecSettings);
    void (__cdecl *UpdateSetting)(int num);
    void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  };

  // function to export the above structure to XBMC
	void __declspec(dllexport) get_module(struct Visualisation* pVisz)
	{
		pVisz->Create = Create;
		pVisz->Start = Start;
		pVisz->AudioData = AudioData;
		pVisz->Render = Render;
		pVisz->Stop = Stop;
		pVisz->GetInfo = GetInfo;
    pVisz->OnAction = OnAction;
    pVisz->GetSettings = GetSettings;
    pVisz->UpdateSetting = UpdateSetting;
    pVisz->GetPresets = GetPresets;
	};
};

#endif