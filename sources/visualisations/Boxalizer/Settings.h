#ifndef XBMC_SETTINGS_H_
#define XBMC_SETTINGS_H_
#include "XmlDocument.h"
#include "xbmc_vis.h"

#define MIN_FREQUENCY 1				// allowable frequency range
#define MAX_FREQUENCY 24000
#define MAX_BARS 256				// number of bars in the Spectrum
#define MAX_PRESETS 256

extern VIS_INFO										vInfo;

class CSettings
{
public:	
  void LoadSettings();
  void SaveSettings();
  void LoadPreset(int nPreset);
  void SetDefaults();

	struct stSettings
	{
	public:
		int		m_iSyncDelay;
		int		m_iBars;				// number of bars to draw
		bool	m_bLogScale;			// true if our frequency is on a log scale
		bool	m_bAverageLevels;		// show average levels?
		float	m_fMinFreq;				// wanted frequency range
		float	m_fMaxFreq;
		bool	m_bMixChannels;			// Mix channels, or stereo?
		int		m_iLingerTime;			// time in msecs to wait on each row
		float	m_fBarDepth;			// Depth of each bar/cube/row/
		char	m_szTextureFile[256];	// Filename of the texture to use
		bool	m_bCamStatic;			// Draw one row and leave the camera static
		float	m_fCamX;				// Camera X value
		float	m_fCamY;				// Camera Y value
		float	m_fCamLookX;			// Camera X Lookat
		float	m_fCamLookY;			// Camera Y Lookat
	};
  char m_szVisName[1024];
    // presets stuff
  char      m_szPresetsDir[256];  // as in the xml
  char      m_szPresetsPath[512]; // fully qualified path
  char*     m_szPresets[256];
  int       m_nPresets;
  int       m_nCurrentPreset;
};
extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
#endif XBMC_SETTINGS_H_