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

#include "UPnPMimeSettings.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_UPNP_MIME "upnpmime"

using namespace std;
using namespace XFILE;

CUPnPMimeSettings::CUPnPMimeSettings()
{
	Clear();
}

CUPnPMimeSettings::~CUPnPMimeSettings()
{
	Clear();
}

CUPnPMimeSettings& CUPnPMimeSettings::Get()
{
	static CUPnPMimeSettings sUPnPMimeSettings;
	return sUPnPMimeSettings;
}

void CUPnPMimeSettings::OnSettingsUnloaded()
{
	Clear();
}

bool CUPnPMimeSettings::Load(const std::string &file)
{
	CSingleLock lock(m_critical);

	Clear();

	if (!CFile::Exists(file))
		return false;

	CXBMCTinyXML doc;
	if (!doc.LoadFile(file))
	{
		CLog::Log(LOGERROR, "CUPnPMimeSettings: error loading %s, Line %d\n%s", file.c_str(), doc.ErrorRow(), doc.ErrorDesc());
		return false;
	}

	TiXmlElement *pRootElement = doc.RootElement();
	if (pRootElement == NULL || !StringUtils::EqualsNoCase(pRootElement->Value(), XML_UPNP_MIME))
	{
		CLog::Log(LOGERROR, "CUPnPMimeSettings: error loading %s, no <upnpmime> node", file.c_str());
		return false;
	}

	// load settings
	CUPnPMimeProfile *pMimeProfile;
	TiXmlElement *pProfile = pRootElement->FirstChildElement("profile");
	if (pProfile == NULL) return false;
	while (pProfile)
	{
		pMimeProfile = new CUPnPMimeProfile();

		pMimeProfile->m_id = XMLUtils::GetAttribute(pProfile, "id");
		pMimeProfile->m_profileType = GetProfileType(XMLUtils::GetAttribute(pProfile, "type"));
		if (pMimeProfile->m_profileType == UPNP_PROFILE_UNKNOWN || pMimeProfile->m_id == "")
		{
			CLog::Log(LOGERROR, "CUPnPMimeSettings: unknown profile type or id");
			delete pMimeProfile;
			Clear();
			return false;
		}

		TiXmlElement* pMime = pProfile->FirstChildElement("mime");
		std::string ext = "";
		std::string mimetype = "";
		while (pMime)
		{
			ext = XMLUtils::GetAttribute(pMime, "ext");
			mimetype = XMLUtils::GetAttribute(pMime, "mimetype");

			if (ext == "" || mimetype == "")
			{
				CLog::Log(LOGERROR, "CUPnPMimeSettings: unknown extension or mime type");
				delete pMimeProfile;
				Clear();
				return false;
			}

			pMimeProfile->m_mimeTypes[ext] = mimetype;
			pMime = pMime->NextSiblingElement("mime");
		}

		m_profiles.push_back(pMimeProfile);
		pProfile = pProfile->NextSiblingElement("profile");
	}

	return true;
}

bool CUPnPMimeSettings::Save(const std::string &file) const
{
	CLog::Log(LOGINFO, "CUPnPMimeSettings: saving not implemented");
	return false;
}

void CUPnPMimeSettings::Clear()
{
	CSingleLock lock(m_critical);

	std::vector<CUPnPMimeProfile *>::iterator iter = m_profiles.begin();
	for (iter = m_profiles.begin(); iter != m_profiles.end(); ++iter)
	{
		delete *iter;
	}
	m_profiles.clear();

	m_deviceName = "";
	m_deviceUUID = "";
}

ProfileType CUPnPMimeSettings::GetProfileType(std::string profileType)
{
	if (profileType == "name") return UPNP_PROFILE_NAME;
	else if (profileType == "uuid") return UPNP_PROFILE_UUID;
	else return UPNP_PROFILE_UNKNOWN;
}

const char* CUPnPMimeSettings::GetMimeTypeForExtension(const NPT_String& extension)
{
	std::vector<CUPnPMimeProfile *>::iterator iter = m_profiles.begin();
	for (iter = m_profiles.begin(); iter != m_profiles.end(); ++iter)
	{
		ProfileType profileType = (*iter)->m_profileType;
		std::string deviceID;

		if (profileType == UPNP_PROFILE_NAME) deviceID = m_deviceName;
		else if (profileType == UPNP_PROFILE_UUID) deviceID = m_deviceUUID;
		else return NULL;

		if (deviceID != (*iter)->m_id) continue;

		std::map<std::string, std::string>::iterator iter_map = (*iter)->m_mimeTypes.find(extension.GetChars());
		if (iter_map != (*iter)->m_mimeTypes.end())
		{
			CLog::Log(LOGINFO, "using type from user list for extension: %s", (*iter_map).second.c_str());
			return (*iter_map).second.c_str();
		}
	}

	return NULL;
}

void CUPnPMimeSettings::SetUPnPDevice(const PLT_DeviceData *device)
{
	m_deviceName = device->GetFriendlyName();
	m_deviceUUID = device->GetUUID();
}
