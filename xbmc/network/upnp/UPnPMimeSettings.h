/*
*      Copyright (C) 2013 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/
#pragma once
#include <string>

#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"
#include "Neptune/Source/Core/NptStrings.h"
#include "Platinum/Source/Platinum/Platinum.h"


enum ProfileType
{
	UPNP_PROFILE_NAME,
	UPNP_PROFILE_UUID,
	UPNP_PROFILE_UNKNOWN
};

struct CUPnPMimeProfile
{
	std::string m_id;
	ProfileType m_profileType;
	std::map <std::string, std::string> m_mimeTypes;
};

class CUPnPMimeSettings : public ISettingsHandler
{
public:
	static CUPnPMimeSettings& Get();

	virtual void OnSettingsUnloaded();

	bool Load(const std::string &file);
	bool Save(const std::string &file) const;
	void Clear();
	const char* GetMimeTypeForExtension(const NPT_String& extension);
	void SetUPnPDevice(const PLT_DeviceData *device);

protected:
	CUPnPMimeSettings();
	CUPnPMimeSettings(const CUPnPMimeSettings&);
	CUPnPMimeSettings const& operator=(CUPnPMimeSettings const&);
	virtual ~CUPnPMimeSettings();

private:
	std::vector <CUPnPMimeProfile *> m_profiles;
	CCriticalSection m_critical;
	std::string m_deviceUUID;
	std::string m_deviceName;

	ProfileType GetProfileType(std::string profileType);
};
