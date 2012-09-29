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
#include <vector>
#include "Vortex.h"

#include "../../../addons/include/xbmc_vis_dll.h"
#include "../../../addons/include/xbmc_addon_cpp_dll.h"


Vortex* g_Vortex = NULL;

// settings vector
StructSetting** g_structSettings;

extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
	if (!props)
		return ADDON_STATUS_UNKNOWN;

	VIS_PROPS* visprops = (VIS_PROPS*)props;

	g_Vortex = new Vortex;
	g_Vortex->Init( ( LPDIRECT3DDEVICE9 )visprops->device, visprops->x, visprops->y, visprops->width, visprops->height, visprops->pixelRatio );

	return ADDON_STATUS_NEED_SETTINGS;
}

extern "C" void Start( int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName )
{
	g_Vortex->Start( iChannels, iSamplesPerSec, iBitsPerSample, szSongName );
}

extern "C" void ADDON_Stop()
{
	if ( g_Vortex )
	{
		g_Vortex->Shutdown();
		delete g_Vortex;
		g_Vortex = NULL;
	}
}

extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	g_Vortex->AudioData( pAudioData, iAudioDataLength, pFreqData, iFreqDataLength );
}

extern "C" void Render()
{
	g_Vortex->Render();
}

extern "C" void GetInfo(VIS_INFO* pInfo)
{
	pInfo->bWantsFreq = false;
	pInfo->iSyncDelay = 0;
}

extern "C"   bool OnAction(long action, const void *param)
{
	bool handled = true;
	if( action == VIS_ACTION_UPDATE_TRACK )
	{
		VisTrack* visTrack = (VisTrack*) param;
		g_Vortex->UpdateTrack( visTrack );
	}
	else if( action == VIS_ACTION_UPDATE_ALBUMART )
	{
		g_Vortex->UpdateAlbumArt( ( char* ) param );
	}
	else if (action == VIS_ACTION_NEXT_PRESET)
	{
		g_Vortex->LoadNextPreset();
	}
	else if (action == VIS_ACTION_PREV_PRESET)
	{
		g_Vortex->LoadPreviousPreset();
	}
	else if (action == VIS_ACTION_LOAD_PRESET && param)
	{
		g_Vortex->LoadPreset( (*(int *)param) );
	}
	else if (action == VIS_ACTION_LOCK_PRESET)
	{
		g_Vortex->GetUserSettings().PresetLocked = !g_Vortex->GetUserSettings().PresetLocked;
	}
	else if (action == VIS_ACTION_RANDOM_PRESET)
	{
		g_Vortex->LoadRandomPreset();
	}
	else
	{
		handled = false;
	}

	return handled;
}

extern "C" unsigned int GetPresets(char ***presets)
{
	if( g_Vortex == NULL )
	{
		return 0;
	}
	return g_Vortex->GetPresets( presets );
}
//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
	if ( g_Vortex)
		return g_Vortex->GetCurrentPresetIndex();
	return 0;
}


//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
	if ( g_Vortex )
		return g_Vortex->GetUserSettings().PresetLocked;
	else
		return false;
}

//-- Destroy-------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
	Stop();
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
	return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
	return ADDON_STATUS_OK;
}

extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting*** sSet)
{
	return 0;
}

extern "C" void ADDON_FreeSettings()
{

}

extern "C" ADDON_STATUS ADDON_SetSetting(const char* id, const void* value)
{
	if ( !id || !value || g_Vortex == NULL )
		return ADDON_STATUS_UNKNOWN;

	UserSettings& userSettings = g_Vortex->GetUserSettings();

	if (strcmpi(id, "Use Preset") == 0)
	{
		OnAction(34, &value);
	}
	else if (strcmpi(id, "RandomPresets") == 0)
	{
		userSettings.RandomPresetsEnabled = *(bool*)value == 1;
	}
	else if (strcmpi(id, "TimeBetweenPresets") == 0)
	{
		userSettings.TimeBetweenPresets = (float)(*(int*)value * 5 + 5);
	}
	else if (strcmpi(id, "AdditionalRandomTime") == 0)
	{
		userSettings.TimeBetweenPresetsRand = (float)(*(int*)value * 5 );
	}
	else if (strcmpi(id, "EnableTransitions") == 0)
	{
		userSettings.TransitionsEnabled = *(bool*)value == 1;
	}
	else if (strcmpi(id, "StopFirstPreset") == 0)
	{
		userSettings.StopFirstPreset = *(bool*)value == 1;
	}
	else if (strcmpi(id, "ShowFPS") == 0)
	{
		userSettings.ShowFPS = *(bool*)value == 1;
	}
	else if (strcmpi(id, "ShowDebugConsole") == 0)
	{
		userSettings.ShowDebugConsole = *(bool*)value == 1;
	}
	else if (strcmpi(id, "ShowAudioAnalysis") == 0)
	{
		userSettings.ShowAudioAnalysis = *(bool*)value == 1;
	}
 	else
 		return ADDON_STATUS_UNKNOWN;

	return ADDON_STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C"   unsigned int GetSubModules(char ***presets)
{
  return 0; // this vis supports 0 sub modules
}
