#include "stdafx.h"
#include "accounts.h"

t_directory::t_directory()
{
	bAutoCreate = FALSE;
}

t_group::t_group()
{
	pOwner = NULL;
	nDownloadSpeedLimitType = 0;
	nUploadSpeedLimitType = 0;
	nDownloadSpeedLimit = 10;
	nUploadSpeedLimit = 10;

	nBypassServerDownloadSpeedLimit = 0;
	nBypassServerUploadSpeedLimit = 0;
}

t_group& t_group::operator=(const t_group &a)
{
	group = a.group;
	nLnk = a.nLnk;
	nRelative = a.nRelative;
	nBypassUserLimit = a.nBypassUserLimit;
	nUserLimit = a.nUserLimit;
	nIpLimit = a.nIpLimit;
	permissions = a.permissions;
	nUploadSpeedLimit = a.nUploadSpeedLimit;
	nDownloadSpeedLimit = a.nDownloadSpeedLimit;
	nUploadSpeedLimitType = a.nUploadSpeedLimitType;
	nDownloadSpeedLimitType = a.nDownloadSpeedLimitType;
	DownloadSpeedLimits = a.DownloadSpeedLimits;
	UploadSpeedLimits = a.UploadSpeedLimits;
	nBypassServerDownloadSpeedLimit = a.nBypassServerDownloadSpeedLimit;
	nBypassServerUploadSpeedLimit = a.nBypassServerUploadSpeedLimit;
	return *this;
}

bool t_group::UseRelativePaths() const
{
	if (!nRelative)
		return false;
	if (nRelative == 2 && pOwner)
		return pOwner->UseRelativePaths();
	return true;
}

bool t_group::ResolveLinks() const
{
	if (!nLnk)
		return false;
	if (nLnk == 2 && pOwner)
		return pOwner->ResolveLinks();
	return true;
}

bool t_group::BypassUserLimit() const
{
	if (!nBypassUserLimit)
		return false;
	if (nBypassUserLimit == 2 && pOwner)
		return pOwner->BypassUserLimit();
	return true;
}

int t_group::GetIpLimit() const
{
	if (nIpLimit)
		return nIpLimit;
	if (pOwner)
		return pOwner->GetIpLimit();
	return 0;
}

int t_group::GetUserLimit() const
{
	if (nUserLimit)
		return nUserLimit;
	if (pOwner)
		return pOwner->GetUserLimit();
	return 0;
}

t_user::t_user()
{
}

t_user& t_user::operator=(const t_user &a)
{
	group = a.group;
	pOwner = a.pOwner;
	user=a.user;
	password=a.password;
	nLnk = a.nLnk;
	nRelative = a.nRelative;
	nBypassUserLimit = a.nBypassUserLimit;
	nUserLimit = a.nUserLimit;
	nIpLimit = a.nIpLimit;
	permissions = a.permissions;	
	nUploadSpeedLimit = a.nUploadSpeedLimit;
	nDownloadSpeedLimit = a.nDownloadSpeedLimit;
	nUploadSpeedLimitType = a.nUploadSpeedLimitType;
	nDownloadSpeedLimitType = a.nDownloadSpeedLimitType;
	DownloadSpeedLimits = a.DownloadSpeedLimits;
	UploadSpeedLimits = a.UploadSpeedLimits;
	nBypassServerDownloadSpeedLimit = a.nBypassServerDownloadSpeedLimit;
	nBypassServerUploadSpeedLimit = a.nBypassServerUploadSpeedLimit;
	return *this;
}

unsigned char * t_group::ParseBuffer(unsigned char *pBuffer, int length)
{
	unsigned char *p = pBuffer;
	
	if ((p-pBuffer+2)>length)
		return NULL;
	int len = *p * 256 + p[1];
	p+=2;
	
	if ((p-pBuffer+len)>length)
		return NULL;
	char *pStr = group.GetBuffer(len);
	if (!pStr)
		return NULL;
	memcpy(pStr, p, len);
	group.ReleaseBuffer(len);
	p+=len;
	
	if ((p-pBuffer+11)>length)
		return NULL;
	
	memcpy(&nIpLimit, p, 4);
	p+=4;
	memcpy(&nUserLimit, p, 4);
	p+=4;
	
	int options = *p++;
	
	nBypassUserLimit	= options & 0x03;
	nLnk				= (options >> 2) & 0x03;
	nRelative			= (options >> 4) & 0x03;
	
	int dircount = *p * 256 + p[1];
	p += 2;

	BOOL bGotHome = FALSE;
	
	for (int j=0; j<dircount; j++)
	{
		t_directory dir;
		if ((p-pBuffer+2)>length)
			return NULL;
		
		len = *p * 256 + p[1];
		p+=2;
		if ((p-pBuffer+len)>length)
			return NULL;
		char *pStr = dir.dir.GetBuffer(len);
		if (!pStr)
			return NULL;
		memcpy(pStr, p, len);
		dir.dir.ReleaseBuffer(len);
		p+=len;
		
		if ((p-pBuffer+2)>length)
			return NULL;
		int rights = *p * 256 + p[1];
		p+=2;
		
		dir.bDirCreate	= rights & 0x0001 ? 1:0;
		dir.bDirDelete	= rights & 0x0002 ? 1:0;
		dir.bDirList	= rights & 0x0004 ? 1:0;
		dir.bDirSubdirs	= rights & 0x0008 ? 1:0;
		dir.bFileAppend	= rights & 0x0010 ? 1:0;
		dir.bFileDelete	= rights & 0x0020 ? 1:0;
		dir.bFileRead	= rights & 0x0040 ? 1:0;
		dir.bFileWrite	= rights & 0x0080 ? 1:0;
		dir.bIsHome		= rights & 0x0100 ? 1:0;
		dir.bAutoCreate	= rights & 0x0200 ? 1:0;

		//Avoid multiple home dirs
		if (dir.bIsHome)
			if (!bGotHome)
				bGotHome = TRUE;
			else
				dir.bIsHome = FALSE;
		
		permissions.push_back(dir);
	}

	if ((p-pBuffer+7)>length)
		return NULL;
	nDownloadSpeedLimitType = *p % 4;
	nUploadSpeedLimitType = (*p >> 2) % 4;
	nBypassServerDownloadSpeedLimit = (*p >> 4) % 4;
	nBypassServerUploadSpeedLimit = (*p++ >> 6) % 4;
	nDownloadSpeedLimit = *p++ << 8;
	nDownloadSpeedLimit |= *p++;
	nUploadSpeedLimit = *p++ << 8;
	nUploadSpeedLimit |= *p++;

	if (!nDownloadSpeedLimit)
		nDownloadSpeedLimit = 10;
	if (!nUploadSpeedLimit)
		nUploadSpeedLimit = 10;
	
	int num = *p++ << 8;
	num |= *p++;
	while (num--)
	{
		CSpeedLimit sl;
		p = sl.ParseBuffer(p, length-(p-pBuffer));
		if (!p)
			return NULL;
		DownloadSpeedLimits.push_back(sl);
	}

	if ((p-pBuffer+2)>length)
		return NULL;
	num = *p++ << 8;
	num |= *p++;
	while (num--)
	{
		CSpeedLimit sl;
		p = sl.ParseBuffer(p, length-(p-pBuffer));
		if (!p)
			return NULL;
		UploadSpeedLimits.push_back(sl);
	}

	return p;
}

char * t_group::FillBuffer(char *p) const
{
	*p++ = group.GetLength() / 256;
	*p++ = group.GetLength() % 256;
	memcpy(p, group, group.GetLength());
	p += group.GetLength();
	
	memcpy(p, &nIpLimit, 4);
	p+=4;
	memcpy(p, &nUserLimit, 4);
	p+=4;
	
	int options = 0;
	options |= nBypassUserLimit % 4;
	options |= (nLnk % 4) << 2;
	options |= (nRelative % 4) << 4;
	
	*p++=options%256;
	
	*p++ = permissions.size() / 256;
	*p++ = permissions.size() % 256;
	for (std::vector<t_directory>::const_iterator permissioniter = permissions.begin(); permissioniter!=permissions.end(); permissioniter++)
	{
		*p++ = permissioniter->dir.GetLength() / 256;
		*p++ = permissioniter->dir.GetLength() % 256;
		memcpy(p, permissioniter->dir, permissioniter->dir.GetLength());
		p += permissioniter->dir.GetLength();
		
		int rights = 0;
		rights |= permissioniter->bDirCreate	? 0x0001:0;
		rights |= permissioniter->bDirDelete	? 0x0002:0;
		rights |= permissioniter->bDirList		? 0x0004:0;
		rights |= permissioniter->bDirSubdirs	? 0x0008:0;
		rights |= permissioniter->bFileAppend	? 0x0010:0;
		rights |= permissioniter->bFileDelete	? 0x0020:0;
		rights |= permissioniter->bFileRead		? 0x0040:0;
		rights |= permissioniter->bFileWrite	? 0x0080:0;
		rights |= permissioniter->bIsHome		? 0x0100:0;
		rights |= permissioniter->bAutoCreate	? 0x0200:0;
		*p++ = rights / 256;
		*p++ = rights % 256;
	}

	*p++ = (nDownloadSpeedLimitType%4) + ((nUploadSpeedLimitType%4)<<2) +
		   ((nBypassServerDownloadSpeedLimit%4)<<4) + ((nBypassServerUploadSpeedLimit%4)<<6);
	*p++ = nDownloadSpeedLimit >> 8;
	*p++ = nDownloadSpeedLimit % 256;
	*p++ = nUploadSpeedLimit >> 8;
	*p++ = nUploadSpeedLimit % 256;
	SPEEDLIMITSLIST::const_iterator iter;
	*p++ = DownloadSpeedLimits.size() >> 8;
	*p++ = DownloadSpeedLimits.size() % 256;
	for (iter = DownloadSpeedLimits.begin(); (iter != DownloadSpeedLimits.end()) && p; iter++)
		p = iter->FillBuffer(p);
	if (!p)
		return NULL;

	*p++ = UploadSpeedLimits.size() >> 8;
	*p++ = UploadSpeedLimits.size() % 256;
	for (iter = UploadSpeedLimits.begin(); (iter != UploadSpeedLimits.end()) && p; iter++)
		p = iter->FillBuffer(p);
	if (!p)
		return FALSE;

	return p;
}

int t_group::GetRequiredBufferLen() const
{
	int len = 9;
	len += group.GetLength() + 2;
	
	len += 2;
	for (std::vector<t_directory>::const_iterator permissioniter = permissions.begin(); permissioniter!=permissions.end(); permissioniter++)
	{
		t_directory directory = *permissioniter;
		len += 2;
		len += directory.dir.GetLength() + 2;
	}

	//Speed limits
	len += 5; //Basic limits
	len += 4; //Number of rules
	SPEEDLIMITSLIST::const_iterator iter;
	for (iter = DownloadSpeedLimits.begin(); iter != DownloadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();
	for (iter = UploadSpeedLimits.begin(); iter != UploadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();
	return len;
}

int t_group::GetCurrentDownloadSpeedLimit() const
{
	switch (nDownloadSpeedLimitType)
	{
	case 0:
		if (pOwner)
			return pOwner->GetCurrentDownloadSpeedLimit();
		else
			return 0;
	case 1:
		return 0;
	case 2:
		return nDownloadSpeedLimit;
	case 3:
		{
			SYSTEMTIME st;
			GetSystemTime(&st);
			for (SPEEDLIMITSLIST::const_iterator iter = DownloadSpeedLimits.begin(); iter != DownloadSpeedLimits.end(); iter++)
				if (iter->IsItActive(st))
					return iter->m_Speed;
		}
		if (pOwner)
			return pOwner->GetCurrentDownloadSpeedLimit();
		else
			return 0;
	}
	return 0;
}

int t_group::GetCurrentUploadSpeedLimit() const
{
	switch (nUploadSpeedLimitType)
	{
	case 0:
		if (pOwner)
			return pOwner->GetCurrentUploadSpeedLimit();
		else
			return 0;
	case 1:
		return 0;
	case 2:
		return nUploadSpeedLimit;
	case 3:
		{
			SYSTEMTIME st;
			GetSystemTime(&st);
			for (SPEEDLIMITSLIST::const_iterator iter = UploadSpeedLimits.begin(); iter != UploadSpeedLimits.end(); iter++)
				if (iter->IsItActive(st))
					return iter->m_Speed;
		}
		if (pOwner)
			return pOwner->GetCurrentUploadSpeedLimit();
		else
			return 0;
	}
	return 0;
}

bool t_group::BypassServerDownloadSpeedLimit() const
{
	if (nBypassServerDownloadSpeedLimit == 1)
		return true;
	else if (!nBypassServerDownloadSpeedLimit)
		return false;
	else if (pOwner)
		return pOwner->BypassServerDownloadSpeedLimit();
	else
		return false;
}

bool t_group::BypassServerUploadSpeedLimit() const
{
	if (nBypassServerUploadSpeedLimit == 1)
		return true;
	else if (!nBypassServerUploadSpeedLimit)
		return false;
	else if (pOwner)
		return pOwner->BypassServerUploadSpeedLimit();
	else
		return false;
}

unsigned char * t_user::ParseBuffer(unsigned char *pBuffer, int length)
{
	unsigned char *p = pBuffer;

	p = t_group::ParseBuffer(p, length);
	if (!p)
		return NULL;

	if ((p-pBuffer+2)>length)
		return NULL;
	int len = *p * 256 + p[1];
	p+=2;
	if ((p-pBuffer+len)>length)
		return NULL;
	char *pStr = user.GetBuffer(len);
	if (!pStr)
		return NULL;
	memcpy(pStr, p, len);
	user.ReleaseBuffer(len);
	p+=len;
	
	if ((p-pBuffer+2)>length)
		return NULL;
	len = *p * 256 + p[1];
	p+=2;
	if ((p-pBuffer+len)>length)
		return NULL;
	pStr = password.GetBuffer(len);
	if (!pStr)
		return NULL;
	memcpy(pStr, p, len);
	password.ReleaseBuffer(len);
	p+=len;

	return p;
}

char * t_user::FillBuffer(char *p) const
{
	p = t_group::FillBuffer(p);
	if (!p)
		return NULL;

	*p++ = user.GetLength() / 256;
	*p++ = user.GetLength() % 256;
	memcpy(p, user, user.GetLength());
	p += user.GetLength();
	
	*p++ = password.GetLength() / 256;
	*p++ = password.GetLength() % 256;
	memcpy(p, password, password.GetLength());
	p += password.GetLength();

	return p;
}

int t_user::GetRequiredBufferLen() const
{
	int len = t_group::GetRequiredBufferLen();
	len += user.GetLength() + 2;
	len += password.GetLength() + 2;
	return len;
}