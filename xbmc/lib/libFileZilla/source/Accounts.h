#ifndef ACCOUNTS_H_INCLUDED
#define ACCOUNTS_H_INCLUDED

#ifndef _AFX
#define CString CStdString
#endif //_AFX

#include "SpeedLimit.h"

class t_directory
{
public:
	t_directory();
	CString dir;
	BOOL bFileRead, bFileWrite, bFileDelete, bFileAppend;
	BOOL bDirCreate, bDirDelete, bDirList, bDirSubdirs, bIsHome;
	BOOL bAutoCreate;
};

class t_group
{
public:
	t_group();

	virtual int GetRequiredBufferLen() const;
	virtual char * FillBuffer(char *p) const;
	virtual unsigned char * ParseBuffer(unsigned char *pBuffer, int length);

	virtual bool UseRelativePaths() const;
	virtual bool ResolveLinks() const;
	virtual bool BypassUserLimit() const;
	virtual int GetUserLimit() const;
	virtual int GetIpLimit() const;

	virtual int GetCurrentDownloadSpeedLimit() const;
	virtual int GetCurrentUploadSpeedLimit() const;
	virtual bool BypassServerDownloadSpeedLimit() const;
	virtual bool BypassServerUploadSpeedLimit() const;

	virtual t_group& t_group::operator=(const t_group &a);
	
	CString group;
	std::vector<t_directory> permissions;
	int nLnk, nRelative, nBypassUserLimit;
	int nUserLimit, nIpLimit;

	int nDownloadSpeedLimitType;
	int nDownloadSpeedLimit;
	int nUploadSpeedLimitType;
	int nUploadSpeedLimit;
	SPEEDLIMITSLIST DownloadSpeedLimits, UploadSpeedLimits;
	int nBypassServerDownloadSpeedLimit;
	int nBypassServerUploadSpeedLimit;
	
	t_group *pOwner;
};

class t_user : public t_group
{
public:
	t_user();
	
	virtual int GetRequiredBufferLen() const;
	virtual char * FillBuffer(char *p) const;
	virtual unsigned char * ParseBuffer(unsigned char *pBuffer, int length);
	
	virtual t_user& t_user::operator=(const t_user &a);
	
	CString user;
	CString password;
};

#endif //#define ACCOUNTS_H_INCLUDED