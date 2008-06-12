//---------------------------------------------------------------------------
#ifndef CRegistryH
#define CRegistryH
//---------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>
//#include <string.h>
//#include <memory.h>

class CRegistry 
{
public:
			CRegistry();
			~CRegistry();

	BOOL	Open(HKEY hKey, char *SubKey);
	BOOL	OpenCreate(HKEY hKey, char *SubKey);
	void	Close();
	void	DeleteVal(char *SubKey);
	void	DeleteKey(char *SubKey);

	void	SetBool(char *keyStr , BOOL val);
	void	SetByte(char *keyStr , BYTE val);
	void	SetWord(char *keyStr , WORD val);
	void	SetDword(char *keyStr , DWORD val);
	void	SetFloat(char *keyStr , float val);
	void	SetStr(char *keyStr , char *valStr);
	void	SetValN(char *keyStr , BYTE *addr,  DWORD size);

	BOOL	GetSetBool(char *keyStr, BOOL var);
	BYTE	GetSetByte(char *keyStr, BYTE var);
	WORD	GetSetWord(char *keyStr, WORD var);
	DWORD	GetSetDword(char *keyStr, DWORD var);
	float	GetSetFloat(char *keyStr, float var);
	int		GetSetStr(char *keyStr, char *tempString, char *dest, int maxLen);
	int		GetSetValN(char *keyStr, BYTE *tempAddr, BYTE *addr, DWORD size);

	HKEY	regKey;
	char	*path;
};

#endif