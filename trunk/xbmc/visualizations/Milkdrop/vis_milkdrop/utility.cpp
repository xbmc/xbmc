/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "utility.h"
#include <stdio.h>
#include <math.h>
#include <windows.h>
//#include <xtl.h>
#include <d3d9.h>

float PowCosineInterp(float x, float pow)
{
    // input (x) & output should be in range 0..1.
    // pow > 0: tends to push things toward 0 and 1
    // pow < 0: tends to push things toward 0.5.

    if (x<0) 
        return 0;
    if (x>1)
        return 1;

    int bneg = (pow < 0) ? 1 : 0;
    if (bneg)
        pow = -pow;

    if (pow>1000) pow=1000;

    int its = (int)pow;
    for (int i=0; i<its; i++)
    {
        if (bneg)
            x = InvCosineInterp(x);
        else
            x = CosineInterp(x);
    }
    float x2 = (bneg) ? InvCosineInterp(x) : CosineInterp(x);
    float dx = pow - its;
    return ((1-dx)*x + (dx)*x2);
}

float AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps)
{
    // returns the equivalent per-frame decay rate at actual_fps
    
    // basically, do all your testing at fps1 and get a good decay rate;
    // then, in the real application, adjust that rate by the actual fps each time you use it.
    
    float per_second_decay_rate_at_fps1 = powf(per_frame_decay_rate_at_fps1, fps1);
    float per_frame_decay_rate_at_fps2 = powf(per_second_decay_rate_at_fps1, 1.0f/actual_fps);

    return per_frame_decay_rate_at_fps2;
}

#if 0

#define INITEMPBUFFERSIZE	(1024*8)
static char		INITempBuffer[INITEMPBUFFERSIZE];
static char		INIfsp[MAX_PATH];
static char		INIsection[256];

static void GetFromINIArray(char *mem, char *key, char *def, char *buffer, int buffersize)
{
	char		*p;
	char		realkey[256];

	strcpy(realkey, key);				//make a copy incase key==buffer
	memcpy(buffer, def, buffersize);
	while(mem[0]!=0 || mem[1]!=0)
	{
		if (strnicmp(realkey, mem, strlen(realkey))==0)
		{
			p = strchr(mem, '=');
			if (p)
			{
				p++;
				while(*p==32)	p++;

				memcpy(buffer, p, buffersize);
				return;
			}
		}
		mem+= strlen(mem)+1;
	}
}

static  void InternalGetPrivateProfileSectionData(FILE* Fh, int fsize, char *ReturnBuffer,int size)  
{
	char		chr;
	int		strcount;
	char	*RB = ReturnBuffer;

	while((fsize>0)&&(size>4))
	{
		fread(&chr,sizeof(char),1,Fh);
		fsize--;

		if( (chr > ' ') && (chr <='z') )
		{
			if(chr=='[') break;

			strcount = 0;

			ReturnBuffer[0] = chr;
			ReturnBuffer++;
			size--;
			strcount++;

			while((fsize>0)&&(size>4))
			{
				fread(&chr,sizeof(char),1,Fh);
				fsize--;

				if((chr==0x0a)||(chr==0x0d))
				{
					if(strcount)
					{
						while (ReturnBuffer[-1]==32)
						{
							ReturnBuffer--;
							size++;
						}

						ReturnBuffer[0] = 0;
						ReturnBuffer++;
						size--;
					}
					break;
				}
				else
				{
					if (chr==0x09)	chr=32;
					ReturnBuffer[0] = chr;
					ReturnBuffer++;
					size--;
					strcount++;
				}
			}
		}
	}

	while (ReturnBuffer[-1]==32)
	{
		ReturnBuffer--;
		size++;
	}

	ReturnBuffer[0] = 0;
	ReturnBuffer[1] = 0;
}

int InternalGetPrivateProfileSection(char *Group,char *ReturnBuffer,int size,char *filename)
{
	FILE* Fh;
	int     fsize;
	char      chr;
	int     count;
	int     strcount;
	char    tmpbuf[256];

	count = 0;

	Fh = fopen(filename,"rb");
	if(Fh==0) return(0);

	fseek(Fh, 0, SEEK_END);
	fsize = ftell(Fh);
	fseek(Fh,0, SEEK_SET);

	while((fsize>0)&&(size>4))
	{
		fread(&chr,sizeof(char),1,Fh);
		fsize--;

		if(chr=='[')
		{
			strcount = 0;

			while((fsize>0)&&(size>4))
			{
				fread(&chr,sizeof(char),1,Fh);
				fsize--;

				if(chr==']')
				{
					if(strcount)
					{
						tmpbuf[strcount] = 0;

						// string found, check against 'group'

						if(stricmp(Group,tmpbuf)==0)
						{
							// got it!

							InternalGetPrivateProfileSectionData(Fh,fsize,ReturnBuffer,size);
							fclose(Fh);
							return(0);
						}
					}
					break;
				}
				else
				{
					tmpbuf[strcount] = chr;
					strcount++;
				}
			}
		}
	}

	ReturnBuffer[0] = 0;
	ReturnBuffer[1] = 0;

	fclose(Fh);
	return(0);
}

bool validIniFile = false;
char* iniFile = NULL;
char currIniFilename[MAX_PATH];
char currIniSection[256];
int iniFileSize = 0;


int	InternalGetPrivateProfileString(char *szSectionName, char *szKeyName, char *szDefault, char *buffer, int size, char *szIniFile)
{
	bool newFile = false;

	if (stricmp(currIniFilename, szIniFile))
	{
		// Load new ini file
		newFile = true;

		FILE* handle;

		handle = fopen(szIniFile, "rb");
		if (handle == 0)
		{
			validIniFile = false;
			GetFromSectionCache(szKeyName, szDefault, buffer, size);
			return (strlen(buffer));
		}

		fseek(Fh, 0, SEEK_END);
		iniFileSize = ftell(Fh);
		fseek(Fh,0, SEEK_SET);

		iniFile = new char[iniFileSize];
		fread(iniFile, iniFileSize, 1, handle);
		fclose(iniFile);
	}

	if (newFile || stricmp(currIniSection, szSectionName))
	{
		
	}



/*

	if (stricmp(INIfsp, szIniFile)!=0 || stricmp(INIsection, szSectionName)!=0)
	{
		InternalGetPrivateProfileSection(szSectionName, INITempBuffer, INITEMPBUFFERSIZE, szIniFile);
		strcpy(INIfsp, szIniFile);
		strcpy(INIsection, szSectionName);
	}

	GetFromINIArray(INITempBuffer, szKeyName, szDefault, buffer, size);
	return(strlen(buffer));
*/
}

#endif

#define SECTION_CACHE_SIZE (1024 * 40)
bool validIni = false;
char currIniFilename[MAX_PATH];
char currIniSection[256];
char sectionCache[SECTION_CACHE_SIZE];


void GetFromCache(char *cache, char *key, char *szDefault, char *buffer, int buffersize)
{
	char *p;

	memcpy(buffer, szDefault, buffersize); // Copy default

	if (!validIni)
		return;

	int keyLength = strlen(key);

	while (cache[0] != 0 || cache[1] != 0)
	{
		if (strnicmp(key, cache, keyLength)==0)
		{
			if (cache[keyLength] == '=')
			{
				p = strchr(cache, '=');
				if (p)
				{
					p++;
					while(*p==32)
						p++;

					memcpy(buffer, p, buffersize);
					return;
				}
			}
		}
		cache += strlen(cache)+1;
	}

}

void InternalGetPrivateProfileSection(char *szSectionName, char *buffer, int size, char *szIniFile)
{
	FILE* handle;
	int fileSize;
	char tempSectionName[256];
	int strIndex;
	char* fileData;
	char* filePtr;
	char c;

	validIni = false;

	handle = fopen(szIniFile, "rb");
	
	if (handle == 0)
	{
		return;
	}

	fseek(handle, 0, SEEK_END);
	fileSize = ftell(handle);
	fseek(handle,0, SEEK_SET);
	fileData = new char[fileSize];
	fread(fileData, fileSize, 1, handle);
	fclose(handle);

	filePtr = fileData;

	while (fileSize > 0)
	{
		c = *filePtr++;
		fileSize--;

		if (c == '[')
		{
			// Start of section name
			strIndex = 0;

			while (fileSize > 0)
			{
				c = *filePtr++;
				fileSize--;

				if (c == ']')
				{
					// End of section name, check if its the one we want
					tempSectionName[strIndex] = 0;

					if (stricmp(szSectionName, tempSectionName) == 0)
					{
						while (fileSize > 0)
						{
							c = *filePtr++;
							fileSize--;

							if ((c > ' ') && (c <='z'))
							{
								if (c == '[')
									break; // end of section

								strIndex = 0;

								buffer[0] = c;
								buffer++;
								size--;
								strIndex++;

								if (size == 0)
								{
									OutputDebugString("Section cache size too small.\n");
									delete[] fileData;
									return;
								}

								while (fileSize > 0)
								{
									c = *filePtr++;
									fileSize--;

									if ((c == 0x0a) || (c == 0x0d))
									{
										if (strIndex)
										{
											while (buffer[-1] == 32)
											{
												buffer--;
												size++;
											}

											buffer[0] = 0;
											buffer++;
											size--;
										}
										break;
									}
									else
									{
//										if (c == 0x09)
//											c = 32;
										if (c != 0x09 && c != 32)
										{
											buffer[0] = c;
											buffer++;
											size--;
											strIndex++;

											if (size == 0)
											{
												OutputDebugString("Section cache size too small.\n");
												delete[] fileData;
												return;
											}
										}
									}
								}
							}
						}

						while (buffer[-1] == 32)
						{
							buffer--;
							size++;
						}

						buffer[0] = 0;
						buffer[1] = 0;

						delete[] fileData;
						validIni = true;
						return;
					}
					break;
				}
				else
				{
					tempSectionName[strIndex] = c;
					strIndex++;
				}
			}
		}
	}

	delete[] fileData;

}

int	InternalGetPrivateProfileString(char *szSectionName, char *szKeyName, char *szDefault, char *buffer, int size, char *szIniFile)
{
	if (stricmp(currIniFilename, szIniFile) || stricmp(currIniSection, szSectionName))
	{
		InternalGetPrivateProfileSection(szSectionName, sectionCache, SECTION_CACHE_SIZE, szIniFile);

		strcpy(currIniFilename, szIniFile);
		strcpy(currIniSection, szSectionName);
	}

	GetFromCache(sectionCache, szKeyName, szDefault, buffer, size);

	return (strlen(buffer));
}

int InternalGetPrivateProfileInt(char *szSectionName, char *szKeyName, int iDefault, char *szIniFile)
{
	char string[64];
	char szDefault[64];
	int ret = iDefault;

	sprintf(szDefault, "%d", iDefault);
	if (InternalGetPrivateProfileString(szSectionName, szKeyName, szDefault, string, 64, szIniFile) > 0)
	{
		sscanf(string, "%d", &ret);
	}

	return ret;
}

float InternalGetPrivateProfileFloat(char *szSectionName, char *szKeyName, float fDefault, char *szIniFile)
{
    char string[64];
    char szDefault[64];
    float ret = fDefault;

    sprintf(szDefault, "%f", fDefault);

    if (InternalGetPrivateProfileString(szSectionName, szKeyName, szDefault, string, 64, szIniFile) > 0)
    {
        sscanf(string, "%f", &ret);
    }

    return ret;
}

bool WritePrivateProfileFloat(float f, char *szKeyName, char *szIniFile, char *szSectionName)
{
//    char szValue[32];
//    sprintf(szValue, "%f", f);
//    return (WritePrivateProfileString(szSectionName, szKeyName, szValue, szIniFile) != 0);
	return true;
}

bool WritePrivateProfileInt(int d, char *szKeyName, char *szIniFile, char *szSectionName)
{
//    char szValue[32];
//    sprintf(szValue, "%d", d);
//    return (WritePrivateProfileString(szSectionName, szKeyName, szValue, szIniFile) != 0);
	return true;

}

void SetScrollLock(int bNewState)
{
/*
    if (bNewState != (GetKeyState(VK_SCROLL) & 1))
    {
        // Simulate a key press
        keybd_event( VK_SCROLL,
                      0x45,
                      KEYEVENTF_EXTENDEDKEY | 0,
                      0 );

        // Simulate a key release
        keybd_event( VK_SCROLL,
                      0x45,
                      KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                      0);
    }
*/
}

void RemoveExtension(char *str)
{
    char *p = strrchr(str, '.');
    if (p) *p = 0;
}

void RemoveSingleAmpersands(char *str)
{
    int len = strlen(str);
    int deleted = 0;

    for (int i=0; i<len; i++)
    {
        if (str[i] == '&')
        {
            if (str[i+1] == '&')
            {
                // two in a row: replace with single ampersand, move on
                deleted++;
                i++;
            }
            else
            {
                // remove single ampersand, hold place
                deleted++;                
            }
        }
        else if (deleted > 0)
            str[i-deleted] = str[i];
    }
    str[len-deleted] = 0;
}

void TextToGuid(char *str, GUID *pGUID)
{
    if (!str) return;
    if (!pGUID) return;

    DWORD d[11];
    
    sscanf(str, "%u %u %u %u %u %u %u %u %u %u %u", 
        &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10]);

    pGUID->Data1 = (DWORD)d[0];
    pGUID->Data2 = (WORD)d[1];
    pGUID->Data3 = (WORD)d[2];
    pGUID->Data4[0] = (BYTE)d[3];
    pGUID->Data4[1] = (BYTE)d[4];
    pGUID->Data4[2] = (BYTE)d[5];
    pGUID->Data4[3] = (BYTE)d[6];
    pGUID->Data4[4] = (BYTE)d[7];
    pGUID->Data4[5] = (BYTE)d[8];
    pGUID->Data4[6] = (BYTE)d[9];
    pGUID->Data4[7] = (BYTE)d[10];
}

void GuidToText(GUID *pGUID, char *str, int nStrLen)
{
    // note: nStrLen should be set to sizeof(str).
    if (!str) return;
    if (!nStrLen) return;
    str[0] = 0;
    if (!pGUID) return;
    
    DWORD d[11];
    d[0]  = (DWORD)pGUID->Data1;
    d[1]  = (DWORD)pGUID->Data2;
    d[2]  = (DWORD)pGUID->Data3;
    d[3]  = (DWORD)pGUID->Data4[0];
    d[4]  = (DWORD)pGUID->Data4[1];
    d[5]  = (DWORD)pGUID->Data4[2];
    d[6]  = (DWORD)pGUID->Data4[3];
    d[7]  = (DWORD)pGUID->Data4[4];
    d[8]  = (DWORD)pGUID->Data4[5];
    d[9]  = (DWORD)pGUID->Data4[6];
    d[10] = (DWORD)pGUID->Data4[7];

    sprintf(str, "%u %u %u %u %u %u %u %u %u %u %u", 
        d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);
}

/*
int GetPentiumTimeRaw(unsigned __int64 *cpu_timestamp)
{
    // returns 0 on failure, 1 on success 
    // warning: watch out for wraparound!
    
    // note: it's probably better to use QueryPerformanceFrequency 
    // and QueryPerformanceCounter()!

    // get high-precision time:
    __try
    {
        unsigned __int64 *dest = (unsigned __int64 *)cpu_timestamp;
        __asm 
        {
            _emit 0xf        // these two bytes form the 'rdtsc' asm instruction,
            _emit 0x31       //  available on Pentium I and later.
            mov esi, dest
            mov [esi  ], eax    // lower 32 bits of tsc
            mov [esi+4], edx    // upper 32 bits of tsc
        }
        return 1;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return 0;
}
        
double GetPentiumTimeAsDouble(unsigned __int64 frequency)
{
    // returns < 0 on failure; otherwise, returns current cpu time, in seconds.
    // warning: watch out for wraparound!

    // note: it's probably better to use QueryPerformanceFrequency 
    // and QueryPerformanceCounter()!

    if (frequency==0)
        return -1.0;

    // get high-precision time:
    __try
    {
        unsigned __int64 high_perf_time;
        unsigned __int64 *dest = &high_perf_time;
        __asm 
        {
            _emit 0xf        // these two bytes form the 'rdtsc' asm instruction,
            _emit 0x31       //  available on Pentium I and later.
            mov esi, dest
            mov [esi  ], eax    // lower 32 bits of tsc
            mov [esi+4], edx    // upper 32 bits of tsc
        }
        __int64 time_s     = (__int64)(high_perf_time / frequency);  // unsigned->sign conversion should be safe here
        __int64 time_fract = (__int64)(high_perf_time % frequency);  // unsigned->sign conversion should be safe here
        // note: here, we wrap the timer more frequently (once per week) 
        // than it otherwise would (VERY RARELY - once every 585 years on
        // a 1 GHz), to alleviate floating-point precision errors that start 
        // to occur when you get to very high counter values.  
        double ret = (time_s % (60*60*24*7)) + (double)time_fract/(double)((__int64)frequency);
        return ret;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return -1.0;
    }

    return -1.0;
}
*/

#ifdef _DEBUG
    void OutputDebugMessage(char *szStartText, HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
    {
        // note: this function does NOT log WM_MOUSEMOVE, WM_NCHITTEST, or WM_SETCURSOR
        //        messages, since they are so frequent.
        // note: these identifiers were pulled from winuser.h

        //if (msg == WM_MOUSEMOVE || msg == WM_NCHITTEST || msg == WM_SETCURSOR)
        //    return;

        #ifdef _DEBUG
            char buf[64];
            int matched = 1;

            sprintf(buf, "WM_");
    
            switch(msg)
            {
            case 0x0001: strcat(buf, "CREATE"); break;                      
            case 0x0002: strcat(buf, "DESTROY"); break;
            case 0x0003: strcat(buf, "MOVE"); break;
            case 0x0005: strcat(buf, "SIZE"); break;
            case 0x0006: strcat(buf, "ACTIVATE"); break;
            case 0x0007: strcat(buf, "SETFOCUS"); break;
            case 0x0008: strcat(buf, "KILLFOCUS"); break;
            case 0x000A: strcat(buf, "ENABLE"); break;
            case 0x000B: strcat(buf, "SETREDRAW"); break;
            case 0x000C: strcat(buf, "SETTEXT"); break;
            case 0x000D: strcat(buf, "GETTEXT"); break;
            case 0x000E: strcat(buf, "GETTEXTLENGTH"); break;
            case 0x000F: strcat(buf, "PAINT"); break;
            case 0x0010: strcat(buf, "CLOSE"); break;
            case 0x0011: strcat(buf, "QUERYENDSESSION"); break;
            case 0x0012: strcat(buf, "QUIT"); break;
            case 0x0013: strcat(buf, "QUERYOPEN"); break;
            case 0x0014: strcat(buf, "ERASEBKGND"); break;
            case 0x0015: strcat(buf, "SYSCOLORCHANGE"); break;
            case 0x0016: strcat(buf, "ENDSESSION"); break;
            case 0x0018: strcat(buf, "SHOWWINDOW"); break;
            case 0x001A: strcat(buf, "WININICHANGE"); break;
            case 0x001B: strcat(buf, "DEVMODECHANGE"); break;
            case 0x001C: strcat(buf, "ACTIVATEAPP"); break;
            case 0x001D: strcat(buf, "FONTCHANGE"); break;
            case 0x001E: strcat(buf, "TIMECHANGE"); break;
            case 0x001F: strcat(buf, "CANCELMODE"); break;
            case 0x0020: strcat(buf, "SETCURSOR"); break;
            case 0x0021: strcat(buf, "MOUSEACTIVATE"); break;
            case 0x0022: strcat(buf, "CHILDACTIVATE"); break;
            case 0x0023: strcat(buf, "QUEUESYNC"); break;
            case 0x0024: strcat(buf, "GETMINMAXINFO"); break;
            case 0x0026: strcat(buf, "PAINTICON"); break;
            case 0x0027: strcat(buf, "ICONERASEBKGND"); break;
            case 0x0028: strcat(buf, "NEXTDLGCTL"); break;
            case 0x002A: strcat(buf, "SPOOLERSTATUS"); break;
            case 0x002B: strcat(buf, "DRAWITEM"); break;
            case 0x002C: strcat(buf, "MEASUREITEM"); break;
            case 0x002D: strcat(buf, "DELETEITEM"); break;
            case 0x002E: strcat(buf, "VKEYTOITEM"); break;
            case 0x002F: strcat(buf, "CHARTOITEM"); break;
            case 0x0030: strcat(buf, "SETFONT"); break;
            case 0x0031: strcat(buf, "GETFONT"); break;
            case 0x0032: strcat(buf, "SETHOTKEY"); break;
            case 0x0033: strcat(buf, "GETHOTKEY"); break;
            case 0x0037: strcat(buf, "QUERYDRAGICON"); break;
            case 0x0039: strcat(buf, "COMPAREITEM"); break;
            case 0x0041: strcat(buf, "COMPACTING"); break;
            case 0x0044: strcat(buf, "COMMNOTIFY"); break;
            case 0x0046: strcat(buf, "WINDOWPOSCHANGING"); break;
            case 0x0047: strcat(buf, "WINDOWPOSCHANGED"); break;
            case 0x0048: strcat(buf, "POWER"); break;
            case 0x004A: strcat(buf, "COPYDATA"); break;
            case 0x004B: strcat(buf, "CANCELJOURNAL"); break;

            #if(WINVER >= 0x0400)
            case 0x004E: strcat(buf, "NOTIFY"); break;
            case 0x0050: strcat(buf, "INPUTLANGCHANGEREQUEST"); break;
            case 0x0051: strcat(buf, "INPUTLANGCHANGE"); break;
            case 0x0052: strcat(buf, "TCARD"); break;
            case 0x0053: strcat(buf, "HELP"); break;
            case 0x0054: strcat(buf, "USERCHANGED"); break;
            case 0x0055: strcat(buf, "NOTIFYFORMAT"); break;
            case 0x007B: strcat(buf, "CONTEXTMENU"); break;
            case 0x007C: strcat(buf, "STYLECHANGING"); break;
            case 0x007D: strcat(buf, "STYLECHANGED"); break;
            case 0x007E: strcat(buf, "DISPLAYCHANGE"); break;
            case 0x007F: strcat(buf, "GETICON"); break;
            case 0x0080: strcat(buf, "SETICON"); break;
            #endif 

            case 0x0081: strcat(buf, "NCCREATE"); break;
            case 0x0082: strcat(buf, "NCDESTROY"); break;
            case 0x0083: strcat(buf, "NCCALCSIZE"); break;
            case 0x0084: strcat(buf, "NCHITTEST"); break;
            case 0x0085: strcat(buf, "NCPAINT"); break;
            case 0x0086: strcat(buf, "NCACTIVATE"); break;
            case 0x0087: strcat(buf, "GETDLGCODE"); break;
            case 0x0088: strcat(buf, "SYNCPAINT"); break;
            case 0x00A0: strcat(buf, "NCMOUSEMOVE"); break;
            case 0x00A1: strcat(buf, "NCLBUTTONDOWN"); break;
            case 0x00A2: strcat(buf, "NCLBUTTONUP"); break;
            case 0x00A3: strcat(buf, "NCLBUTTONDBLCLK"); break;
            case 0x00A4: strcat(buf, "NCRBUTTONDOWN"); break;
            case 0x00A5: strcat(buf, "NCRBUTTONUP"); break;
            case 0x00A6: strcat(buf, "NCRBUTTONDBLCLK"); break;
            case 0x00A7: strcat(buf, "NCMBUTTONDOWN"); break;
            case 0x00A8: strcat(buf, "NCMBUTTONUP"); break;
            case 0x00A9: strcat(buf, "NCMBUTTONDBLCLK"); break;
            case 0x0100: strcat(buf, "KEYDOWN"); break;
            case 0x0101: strcat(buf, "KEYUP"); break;
            case 0x0102: strcat(buf, "CHAR"); break;
            case 0x0103: strcat(buf, "DEADCHAR"); break;
            case 0x0104: strcat(buf, "SYSKEYDOWN"); break;
            case 0x0105: strcat(buf, "SYSKEYUP"); break;
            case 0x0106: strcat(buf, "SYSCHAR"); break;
            case 0x0107: strcat(buf, "SYSDEADCHAR"); break;
            case 0x0108: strcat(buf, "KEYLAST"); break;

            #if(WINVER >= 0x0400)
            case 0x010D: strcat(buf, "IME_STARTCOMPOSITION"); break;
            case 0x010E: strcat(buf, "IME_ENDCOMPOSITION"); break;
            case 0x010F: strcat(buf, "IME_COMPOSITION"); break;
            //case 0x010F: strcat(buf, "IME_KEYLAST"); break;
            #endif 

            case 0x0110: strcat(buf, "INITDIALOG"); break;
            case 0x0111: strcat(buf, "COMMAND"); break;
            case 0x0112: strcat(buf, "SYSCOMMAND"); break;
            case 0x0113: strcat(buf, "TIMER"); break;
            case 0x0114: strcat(buf, "HSCROLL"); break;
            case 0x0115: strcat(buf, "VSCROLL"); break;
            case 0x0116: strcat(buf, "INITMENU"); break;
            case 0x0117: strcat(buf, "INITMENUPOPUP"); break;
            case 0x011F: strcat(buf, "MENUSELECT"); break;
            case 0x0120: strcat(buf, "MENUCHAR"); break;
            case 0x0121: strcat(buf, "ENTERIDLE"); break;
            #if(WINVER >= 0x0500)
            case 0x0122: strcat(buf, "MENURBUTTONUP"); break;
            case 0x0123: strcat(buf, "MENUDRAG"); break;
            case 0x0124: strcat(buf, "MENUGETOBJECT"); break;
            case 0x0125: strcat(buf, "UNINITMENUPOPUP"); break;
            case 0x0126: strcat(buf, "MENUCOMMAND"); break;
            #endif 

            case 0x0132: strcat(buf, "CTLCOLORMSGBOX"); break;
            case 0x0133: strcat(buf, "CTLCOLOREDIT"); break;
            case 0x0134: strcat(buf, "CTLCOLORLISTBOX"); break;
            case 0x0135: strcat(buf, "CTLCOLORBTN"); break;
            case 0x0136: strcat(buf, "CTLCOLORDLG"); break;
            case 0x0137: strcat(buf, "CTLCOLORSCROLLBAR"); break;
            case 0x0138: strcat(buf, "CTLCOLORSTATIC"); break;

            //case 0x0200: strcat(buf, "MOUSEFIRST"); break;
            case 0x0200: strcat(buf, "MOUSEMOVE"); break;
            case 0x0201: strcat(buf, "LBUTTONDOWN"); break;
            case 0x0202: strcat(buf, "LBUTTONUP"); break;
            case 0x0203: strcat(buf, "LBUTTONDBLCLK"); break;
            case 0x0204: strcat(buf, "RBUTTONDOWN"); break;
            case 0x0205: strcat(buf, "RBUTTONUP"); break;
            case 0x0206: strcat(buf, "RBUTTONDBLCLK"); break;
            case 0x0207: strcat(buf, "MBUTTONDOWN"); break;
            case 0x0208: strcat(buf, "MBUTTONUP"); break;
            case 0x0209: strcat(buf, "MBUTTONDBLCLK"); break;

            #if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
            case 0x020A: strcat(buf, "MOUSEWHEEL"); break;
            //case 0x020A: strcat(buf, "MOUSELAST"); break;
            #else
            //case 0x0209: strcat(buf, "MOUSELAST"); break;
            #endif 

            case 0x0210: strcat(buf, "PARENTNOTIFY"); break;
            case 0x0211: strcat(buf, "ENTERMENULOOP"); break;
            case 0x0212: strcat(buf, "EXITMENULOOP"); break;

            #if(WINVER >= 0x0400)
            case 0x0213: strcat(buf, "NEXTMENU"); break;
            case 0x0214: strcat(buf, "SIZING"); break;
            case 0x0215: strcat(buf, "CAPTURECHANGED"); break;
            case 0x0216: strcat(buf, "MOVING"); break;
            case 0x0218: strcat(buf, "POWERBROADCAST"); break;
            case 0x0219: strcat(buf, "DEVICECHANGE"); break;
            #endif 

            /*
            case 0x0220: strcat(buf, "MDICREATE"); break;
            case 0x0221: strcat(buf, "MDIDESTROY"); break;
            case 0x0222: strcat(buf, "MDIACTIVATE"); break;
            case 0x0223: strcat(buf, "MDIRESTORE"); break;
            case 0x0224: strcat(buf, "MDINEXT"); break;
            case 0x0225: strcat(buf, "MDIMAXIMIZE"); break;
            case 0x0226: strcat(buf, "MDITILE"); break;
            case 0x0227: strcat(buf, "MDICASCADE"); break;
            case 0x0228: strcat(buf, "MDIICONARRANGE"); break;
            case 0x0229: strcat(buf, "MDIGETACTIVE"); break;
            */

            case 0x0230: strcat(buf, "MDISETMENU"); break;
            case 0x0231: strcat(buf, "ENTERSIZEMOVE"); break;
            case 0x0232: strcat(buf, "EXITSIZEMOVE"); break;
            case 0x0233: strcat(buf, "DROPFILES"); break;
            case 0x0234: strcat(buf, "MDIREFRESHMENU"); break;


            /*
            #if(WINVER >= 0x0400)
            case 0x0281: strcat(buf, "IME_SETCONTEXT"); break;
            case 0x0282: strcat(buf, "IME_NOTIFY"); break;
            case 0x0283: strcat(buf, "IME_CONTROL"); break;
            case 0x0284: strcat(buf, "IME_COMPOSITIONFULL"); break;
            case 0x0285: strcat(buf, "IME_SELECT"); break;
            case 0x0286: strcat(buf, "IME_CHAR"); break;
            #endif 
            #if(WINVER >= 0x0500)
            case 0x0288: strcat(buf, "IME_REQUEST"); break;
            #endif 
            #if(WINVER >= 0x0400)
            case 0x0290: strcat(buf, "IME_KEYDOWN"); break;
            case 0x0291: strcat(buf, "IME_KEYUP"); break;
            #endif 
            */

            #if(_WIN32_WINNT >= 0x0400)
            case 0x02A1: strcat(buf, "MOUSEHOVER"); break;
            case 0x02A3: strcat(buf, "MOUSELEAVE"); break;
            #endif 

            case 0x0300: strcat(buf, "CUT"); break;
            case 0x0301: strcat(buf, "COPY"); break;
            case 0x0302: strcat(buf, "PASTE"); break;
            case 0x0303: strcat(buf, "CLEAR"); break;
            case 0x0304: strcat(buf, "UNDO"); break;
            case 0x0305: strcat(buf, "RENDERFORMAT"); break;
            case 0x0306: strcat(buf, "RENDERALLFORMATS"); break;
            case 0x0307: strcat(buf, "DESTROYCLIPBOARD"); break;
            case 0x0308: strcat(buf, "DRAWCLIPBOARD"); break;
            case 0x0309: strcat(buf, "PAINTCLIPBOARD"); break;
            case 0x030A: strcat(buf, "VSCROLLCLIPBOARD"); break;
            case 0x030B: strcat(buf, "SIZECLIPBOARD"); break;
            case 0x030C: strcat(buf, "ASKCBFORMATNAME"); break;
            case 0x030D: strcat(buf, "CHANGECBCHAIN"); break;
            case 0x030E: strcat(buf, "HSCROLLCLIPBOARD"); break;
            case 0x030F: strcat(buf, "QUERYNEWPALETTE"); break;
            case 0x0310: strcat(buf, "PALETTEISCHANGING"); break;
            case 0x0311: strcat(buf, "PALETTECHANGED"); break;
            case 0x0312: strcat(buf, "HOTKEY"); break;

            #if(WINVER >= 0x0400)
            case 0x0317: strcat(buf, "PRINT"); break;
            case 0x0318: strcat(buf, "PRINTCLIENT"); break;

            case 0x0358: strcat(buf, "HANDHELDFIRST"); break;
            case 0x035F: strcat(buf, "HANDHELDLAST"); break;

            case 0x0360: strcat(buf, "AFXFIRST"); break;
            case 0x037F: strcat(buf, "AFXLAST"); break;
            #endif 

            case 0x0380: strcat(buf, "PENWINFIRST"); break;
            case 0x038F: strcat(buf, "PENWINLAST"); break;

            default: 
                sprintf(buf, "unknown"); 
                matched = 0;
                break;
            }

            int n = strlen(buf);
            int desired_len = 24;
            int spaces_to_append = desired_len-n;
            if (spaces_to_append>0)
            {
                for (int i=0; i<spaces_to_append; i++)
                    buf[n+i] = ' ';
                buf[desired_len] = 0;
            }      

            char buf2[256];
            if (matched)
                sprintf(buf2, "%shwnd=%08x, msg=%s, w=%08x, l=%08x\n", szStartText, hwnd, buf, wParam, lParam);
            else
                sprintf(buf2, "%shwnd=%08x, msg=unknown/0x%08x, w=%08x, l=%08x\n", szStartText, hwnd, msg, wParam, lParam);
            OutputDebugString(buf2);
        #endif
    }
#endif

void DownloadDirectX(HWND hwnd)
{
/*
    char szUrl[] = "http://www.microsoft.com/windows/directx/";
    int ret = (int)ShellExecute(NULL, "open", szUrl, "", "", SW_SHOWNORMAL);
    if (ret <= 32)
    {
        char buf[1024];
        switch(ret)
        {
        case SE_ERR_FNF:
        case SE_ERR_PNF:
            sprintf(buf, "the URL\r\r\t%s\r\rcould not be opened.", szUrl);
            break;
        case SE_ERR_ACCESSDENIED:
        case SE_ERR_SHARE:
            sprintf(buf, "access to the URL\r\r\t%s\r\rwas denied.", szUrl);
            break;
        case SE_ERR_NOASSOC:
            sprintf(buf, "the URL\r\r\t%s\r\rcould not be accessed because there is no application\rassociated with documents of this type.", szUrl);
            break;
        default:
            sprintf(buf, "the URL\r\r\t%s\r\rcould not be accessed (error code: %d)", szUrl, ret);
            break;
        }
        MessageBox(hwnd, buf, "error opening URL", MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
    }
*/
}

void MissingDirectX(HWND hwnd)
{
/*
    // DIRECTX MISSING OR CORRUPT -> PROMPT TO GO TO WEB.
    int ret = MessageBox(hwnd, 
        #ifndef D3D_SDK_VERSION
            --- error; you need to #include <d3d8.h> ---
        #endif
        #if (D3D_SDK_VERSION==120)
            // plugin was *built* using the DirectX 8.0 sdk, therefore, 
            // the dx8.0 runtime is missing or corrupt
            "Failed to initialize DirectX 8.0 or later; it is either missing or corrupt.\r"
            "DirectX 8.0 or later must be installed before you can run this plugin!\r"
            "\r"
            "Would you like to be taken to http://www.microsoft.com/windows/directx,\r"
            "where you can download the latest version of DirectX?\r"
            "\r"
            "Please click YES to open this page in your default web browser,\r"
            "or NO to simply return to Winamp."
        #else
            // plugin was *built* using some other version of the DirectX8 sdk, such as 
            // 8.1b; therefore, we don't know exactly what version to tell them they need
            // to install; so we ask them to go get the *latest* version.
            "Failed to initialize DirectX; it is either missing or corrupt.\r"
            "The latest version of DirectX must be installed before you can run this plugin!\r"
            "\r"
            "Would you like to be taken to http://www.microsoft.com/windows/directx,\r"
            "where you can download the latest version of DirectX?\r"
            "\r"
            "Please click YES to open this page in your default web browser,\r"
            "or NO to simply return to Winamp."
        #endif
        ,"DirectX Missing or Corrupt",MB_YESNO|MB_SETFOREGROUND|MB_TOPMOST|MB_TASKMODAL);
        
    if (ret==IDYES)
        DownloadDirectX(hwnd);
*/
}

bool CheckForMMX()
{
    DWORD bMMX = 0;
    DWORD *pbMMX = &bMMX;
    __try {
        __asm {
            mov eax, 1
            cpuid
            mov edi, pbMMX
            mov dword ptr [edi], edx
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bMMX = 0;
    }

    if (bMMX & 0x00800000)  // check bit 23
		return true;

	return false;
}

bool CheckForSSE()
{
    /*
    The SSE instruction set was introduced with the Pentium III and features:
        * Additional MMX instructions such as min/max
        * Prefetch and write-through instructions for optimizing data movement 
            from and to the L2/L3 caches and main memory
        * 8 New 128 bit XMM registers (xmm0..xmm7) and corresponding 32 bit floating point 
            (single precision) instructions
    */

	DWORD bSSE = 0;
	DWORD *pbSSE = &bSSE;
    __try {
	    __asm
	    {
		    mov eax, 1
		    cpuid
            mov edi, pbSSE
            mov dword ptr [edi], edx
	    }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bSSE = 0;
    }

	if (bSSE & 0x02000000)  // check bit 25
		return true;

	return false;
}

void memset_MMX(void *pDest, unsigned char value, int nBytes)
{
	// NOTE #1: YOU MUST MAKE SURE MMX IS SUPPORTED BEFORE CALLING THIS ROUTINE
	// NOTE #2: YOU MUST ENSURE THAT THE POINTERS ARE ALIGNED TO 8-BYTE BOUNDARIES
	
	int offset  = ((unsigned long)pDest) % 8;

    if (nBytes < 32)
	{
		memset(pDest, value, nBytes);
	}
	else
	{
		if (offset != 0)
		{
			int prebytes = 8-offset;
			memset(pDest, value, prebytes);
			pDest = (void*)((unsigned long)pDest + prebytes);
			nBytes -= prebytes;
		}

		int nLoops = nBytes/64;
        unsigned __int64 value8x = value;
        value8x = value8x | (value8x<<8);
        value8x = value8x | (value8x<<16);
        value8x = value8x | (value8x<<32);

		if (nLoops > 0)
		__asm
		{
			mov ecx, nLoops
			mov edi, pDest

            movq mm0, qword ptr value8x
            movq mm1, qword ptr value8x
            movq mm2, qword ptr value8x
            movq mm3, qword ptr value8x
            movq mm4, qword ptr value8x
            movq mm5, qword ptr value8x
            movq mm6, qword ptr value8x
            movq mm7, qword ptr value8x

			ALIGN 16
			MMX_memset_loop:

                MOVQ       [edi], mm0       
                MOVQ       [edi+8], mm1
                MOVQ       [edi+16], mm2
                MOVQ       [edi+24], mm3
                MOVQ       [edi+32], mm4
                MOVQ       [edi+40], mm5
                MOVQ       [edi+48], mm6
                MOVQ       [edi+56], mm7
                
                ADD        edi, 64

				dec    ecx
				jnz    MMX_memset_loop

			EMMS
		}

		int nBytesDone = nLoops*64;
		int nBytesLeft = nBytes - nBytesDone;
		if (nBytesLeft > 0)
			memset((unsigned char *)pDest + nBytesDone, value, nBytesLeft);
	}
}

void memcpy_MMX(void *pDest, void *pSrc, int nBytes)
{
	// NOTE #1: YOU MUST MAKE SURE MMX IS SUPPORTED BEFORE CALLING THIS ROUTINE
	// NOTE #2: YOU MUST ENSURE THAT THE POINTERS ARE ALIGNED TO 8-BYTE BOUNDARIES
	
	int offset  = ((unsigned long)pSrc) % 8;
	int offset2 = ((unsigned long)pDest) % 8;

	if (offset != offset2 || nBytes < 32)
	{
		// don't use MMX, since data is not aligned
		memcpy(pDest, pSrc, nBytes);
	}
	else
	{
		if (offset != 0)
		{
			int prebytes = 8-offset;
			memcpy(pDest, pSrc, prebytes);
			pSrc = (void*)((unsigned long)pSrc + prebytes);
			pDest = (void*)((unsigned long)pDest + prebytes);
			nBytes -= prebytes;
			//assert((((unsigned long)pSrc) % 8)==0);
			//assert((((unsigned long)pDest) % 8)==0);
		}

		int nLoops = nBytes/64;

		if (nLoops > 0)
		__asm
		{
			mov ecx, nLoops
			mov eax, pSrc
			mov edi, pDest

			ALIGN 16
			MMX_memcpy_loop:

                MOVQ       mm0, [eax]   
                MOVQ       mm1, [eax+8] 
                MOVQ       mm2, [eax+16]
                MOVQ       mm3, [eax+24]
                MOVQ       mm4, [eax+32]
                MOVQ       mm5, [eax+40]
                MOVQ       mm6, [eax+48]
                MOVQ       mm7, [eax+56]

                MOVQ       [edi], mm0       
                MOVQ       [edi+8], mm1
                MOVQ       [edi+16], mm2
                MOVQ       [edi+24], mm3
                MOVQ       [edi+32], mm4
                MOVQ       [edi+40], mm5
                MOVQ       [edi+48], mm6
                MOVQ       [edi+56], mm7
                
                ADD        eax, 64
                ADD        edi, 64

				dec    ecx
				jnz    MMX_memcpy_loop

			EMMS
		}

		int nBytesDone = nLoops*64;
		int nBytesLeft = nBytes - nBytesDone;
		if (nBytesLeft > 0)
		{
			memcpy((unsigned char *)pDest + nBytesDone, (unsigned char *)pSrc + nBytesDone, nBytesLeft);
		}
	}
}

void GetDesktopFolder(char *szDesktopFolder) // should be MAX_PATH len.
{
    // returns the path to the desktop folder, WITHOUT a trailing backslash.

    szDesktopFolder[0] = 0;
/*
    ITEMIDLIST pidl;
    ZeroMemory(&pidl, sizeof(pidl));
    if (!SHGetPathFromIDList(&pidl, szDesktopFolder))
        szDesktopFolder[0] = 0;
*/
}
/*
void ExecutePidl(LPITEMIDLIST pidl, char *szPathAndFile, char *szWorkingDirectory, HWND hWnd)
{

    // This function was based on code by Jeff Prosise.

    // Note: for some reason, ShellExecuteEx fails when executing
    // *shortcuts* (.lnk files) from the desktop, using their PIDLs.  
    // So, if that fails, we try again w/the plain old text filename 
    // (szPathAndFile).

    char szVerb[] = "open";
    char szFilename2[MAX_PATH];

    sprintf(szFilename2, "%s.lnk", szPathAndFile);

    // -without the "no-verb" pass,
    //   certain icons still don't work (like shortcuts
    //   to IE, VTune...)
    // -without the "context menu" pass,
    //   certain others STILL don't work (Netscape...) 
    // -without the 'ntry' pass, shortcuts (to folders/files)
    //   don't work
    for (int verb_pass=0; verb_pass<2; verb_pass++)
    {
        for (int ntry=0; ntry<3; ntry++)
        {
            for (int context_pass=0; context_pass<2; context_pass++)
            {
                SHELLEXECUTEINFO sei = { sizeof(sei) };
                sei.hwnd = hWnd;
                sei.fMask = SEE_MASK_FLAG_NO_UI;
                if (context_pass==1)
                    sei.fMask |= SEE_MASK_INVOKEIDLIST;
                sei.lpVerb = (verb_pass) ? NULL : szVerb;
                sei.lpDirectory = szWorkingDirectory;
                sei.nShow = SW_SHOWNORMAL;
        
                if (ntry==0)
                {
                    // this case works for most non-shortcuts
                    sei.fMask |= SEE_MASK_IDLIST;
                    sei.lpIDList = pidl;
                }
                else if (ntry==1)
                {
                    // this case is required for *shortcuts to folders* to work
                    sei.lpFile = szPathAndFile;
                }
                else if (ntry==2)
                {
                    // this case is required for *shortcuts to files* to work
                    sei.lpFile = szFilename2;
                }

                if (ShellExecuteEx(&sei))
                    return;
            }
        }
    }

}
*/

//WNDPROC        g_pOldWndProc;
//LPCONTEXTMENU2 g_pIContext2or3;

LRESULT CALLBACK HookWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
#if 0
   //UINT uItem;
   //TCHAR szBuf[MAX_PATH];

   switch (msg) 
   { 
   case WM_DRAWITEM:
   case WM_MEASUREITEM:
      if(wp) break; // not menu related
   case WM_INITMENUPOPUP:
      g_pIContext2or3->HandleMenuMsg(msg, wp, lp);
      return (msg==WM_INITMENUPOPUP ? 0 : TRUE); // handled

   /*case WM_MENUSELECT:
      // if this is a shell item, get its descriptive text
      uItem = (UINT) LOWORD(wp);   
      if(0 == (MF_POPUP & HIWORD(wp)) && uItem >= 1 && uItem <= 0x7fff) 
      {
         g_pIContext2or3->GetCommandString(uItem-1, GCS_HELPTEXT,
            NULL, szBuf, sizeof(szBuf)/sizeof(szBuf[0]) );

         // set the status bar text
         ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText(szBuf);
         return 0;
      }
      break;*/

   default:
      break;
   }

   // for all untreated messages, call the original wndproc
   return ::CallWindowProc(g_pOldWndProc, hWnd, msg, wp, lp);
#endif
   return 0;
}

#if 0
BOOL DoExplorerMenu (HWND hwnd, LPITEMIDLIST pidlMain, POINT point)
{
    LPMALLOC pMalloc;
    LPSHELLFOLDER psfFolder, psfNextFolder;
    LPITEMIDLIST pidlItem, pidlNextItem, *ppidl;
    LPCONTEXTMENU pContextMenu;
    CMINVOKECOMMANDINFO ici;
    UINT nCount, nCmd;
    BOOL bResult;
    HMENU hMenu;

    //
    // Get pointers to the shell's IMalloc interface and the desktop's
    // IShellFolder interface.
    //
    bResult = FALSE;

    if (!SUCCEEDED (SHGetMalloc (&pMalloc)))
        return bResult;

    if (!SUCCEEDED (SHGetDesktopFolder (&psfFolder))) {
        pMalloc->Release();
        return bResult;
    }

    if (nCount = GetItemCount (pidlMain)) // nCount must be > 0
    {
        //
        // Initialize psfFolder with a pointer to the IShellFolder
        // interface of the folder that contains the item whose context
        // menu we're after, and initialize pidlItem with a pointer to
        // the item's item ID. If nCount > 1, this requires us to walk
        // the list of item IDs stored in pidlMain and bind to each
        // subfolder referenced in the list.
        //
        pidlItem = pidlMain;

        while (--nCount) {
            //
            // Create a 1-item item ID list for the next item in pidlMain.
            //
            pidlNextItem = DuplicateItem (pMalloc, pidlItem);
            if (pidlNextItem == NULL) {
                psfFolder->Release();
                pMalloc->Release();
                return bResult;
            }

            //
            // Bind to the folder specified in the new item ID list.
            //
            if (!SUCCEEDED (psfFolder->BindToObject(pidlNextItem, NULL, IID_IShellFolder, (void**)&psfNextFolder)))  // modified by RG
            {
                pMalloc->Free(pidlNextItem);
                psfFolder->Release();
                pMalloc->Release();
                return bResult;
            }

            //
            // Release the IShellFolder pointer to the parent folder
            // and set psfFolder equal to the IShellFolder pointer for
            // the current folder.
            //
            psfFolder->Release();
            psfFolder = psfNextFolder;

            //
            // Release the storage for the 1-item item ID list we created
            // just a moment ago and initialize pidlItem so that it points
            // to the next item in pidlMain.
            //
            pMalloc->Free(pidlNextItem);
            pidlItem = GetNextItem (pidlItem);
        }

        //
        // Get a pointer to the item's IContextMenu interface and call
        // IContextMenu::QueryContextMenu to initialize a context menu.
        //
        ppidl = &pidlItem;
        if (SUCCEEDED (psfFolder->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)ppidl, IID_IContextMenu, NULL, (void**)&pContextMenu)))   // modified by RG
        {
            // try to see if we can upgrade to an IContextMenu3 
            // or IContextMenu2 interface pointer:
            int level = 1;
            void *pCM = NULL;
            if (pContextMenu->QueryInterface(IID_IContextMenu3, &pCM) == NOERROR)
            {
                pContextMenu->Release();
                pContextMenu = (LPCONTEXTMENU)pCM;
                level = 3;
            }
            else if (pContextMenu->QueryInterface(IID_IContextMenu2, &pCM) == NOERROR)
            {
                pContextMenu->Release();
                pContextMenu = (LPCONTEXTMENU)pCM;
                level = 2;
            }

            hMenu = CreatePopupMenu ();
            if (SUCCEEDED (pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE))) 
            {
                ClientToScreen (hwnd, &point);

                // install the subclassing "hook", for versions 2 or 3
                if (level >= 2) 
                {
                    g_pOldWndProc   = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (DWORD)HookWndProc);
                    g_pIContext2or3 = (LPCONTEXTMENU2)pContextMenu; // cast ok for ICMv3
                }
                else 
                {
                    g_pOldWndProc   = NULL;
                    g_pIContext2or3 = NULL;
                }

                //
                // Display the context menu.
                //
                nCmd = TrackPopupMenu (hMenu, TPM_LEFTALIGN |
                    TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                    point.x, point.y, 0, hwnd, NULL);

                // restore old wndProc
                if (g_pOldWndProc) 
                {
                    SetWindowLong(hwnd, GWL_WNDPROC, (DWORD)g_pOldWndProc);
				}

				//
				// If a command was selected from the menu, execute it.
				//
				if (nCmd >= 1 && nCmd <= 0x7fff) 
				{
					ZeroMemory(&ici, sizeof(ici));
					ici.cbSize          = sizeof (CMINVOKECOMMANDINFO);
					//ici.fMask           = 0;
					ici.hwnd            = hwnd;
					ici.lpVerb          = MAKEINTRESOURCE (nCmd - 1);
					//ici.lpParameters    = NULL;
					//ici.lpDirectory     = NULL;
					ici.nShow           = SW_SHOWNORMAL;
					//ici.dwHotKey        = 0;
					//ici.hIcon           = NULL;

					if (SUCCEEDED ( pContextMenu->InvokeCommand (&ici)))
						bResult = TRUE;
				}
				/*else if (nCmd) 
				{
				PostMessage(hwnd, WM_COMMAND, nCmd, NULL); // our command
				}*/

			}
			DestroyMenu (hMenu);
			pContextMenu->Release();
		}
	}

	//
	// Clean up and return.
	//
	psfFolder->Release();
	pMalloc->Release();

	return bResult;
	return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Note: a special thanks goes out to Jeff Prosise for writing & publishing 
//        the following code!
//  
//  FUNCTION:       GetItemCount
//
//  DESCRIPTION:    Computes the number of item IDs in an item ID list.
//
//  INPUT:          pidl = Pointer to an item ID list.
//
//  RETURNS:        Number of item IDs in the list.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

UINT GetItemCount (LPITEMIDLIST pidl)
{
	USHORT nLen;
	UINT nCount;

	nCount = 0;
	while ((nLen = pidl->mkid.cb) != 0) {
		pidl = GetNextItem (pidl);
		nCount++;
	}
	return nCount;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Note: a special thanks goes out to Jeff Prosise for writing & publishing 
//        the following code!
//  
//  FUNCTION:       GetNextItem
//
//  DESCRIPTION:    Finds the next item in an item ID list.
//
//  INPUT:          pidl = Pointer to an item ID list.
//
//  RETURNS:        Pointer to the next item.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LPITEMIDLIST GetNextItem (LPITEMIDLIST pidl)
{
	USHORT nLen;

	if ((nLen = pidl->mkid.cb) == 0)
		return NULL;

	return (LPITEMIDLIST) (((LPBYTE) pidl) + nLen);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Note: a special thanks goes out to Jeff Prosise for writing & publishing 
//        the following code!
//  
//  FUNCTION:       DuplicateItem
//
//  DESCRIPTION:    Makes a copy of the next item in an item ID list.
//
//  INPUT:          pMalloc = Pointer to an IMalloc interface.
//                  pidl    = Pointer to an item ID list.
//
//  RETURNS:        Pointer to an ITEMIDLIST containing the copied item ID.
//
//  NOTES:          It is the caller's responsibility to free the memory
//                  allocated by this function when the item ID is no longer
//                  needed. Example:
//
//                    pidlItem = DuplicateItem (pMalloc, pidl);
//                      .
//                      .
//                      .
//                    pMalloc->lpVtbl->Free (pMalloc, pidlItem);
//
//                  Failure to free the ITEMIDLIST will result in memory
//                  leaks.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LPITEMIDLIST DuplicateItem (LPMALLOC pMalloc, LPITEMIDLIST pidl)
{
	USHORT nLen;
	LPITEMIDLIST pidlNew;

	nLen = pidl->mkid.cb;
	if (nLen == 0)
		return NULL;

	pidlNew = (LPITEMIDLIST) pMalloc->Alloc (
		nLen + sizeof (USHORT));
	if (pidlNew == NULL)
		return NULL;

	CopyMemory (pidlNew, pidl, nLen);
	*((USHORT*) (((LPBYTE) pidlNew) + nLen)) = 0;

	return pidlNew;
}

//----------------------------------------------------------------------
// A special thanks goes out to Jeroen-bart Engelen (Yeep) for providing
// his source code for getting the position & label information for all 
// the icons on the desktop, as found below.  See his article at 
// http://www.digiwar.com/scripts/renderpage.php?section=2&subsection=2
//----------------------------------------------------------------------

void FindDesktopWindows(HWND *desktop_progman, HWND *desktopview_wnd, HWND *listview_wnd)
{
	*desktop_progman = NULL;
	*desktopview_wnd = NULL;
	*listview_wnd = NULL;

	*desktop_progman = FindWindow(NULL, ("Program Manager"));
	if(*desktop_progman == NULL)
	{
		//MessageBox(NULL, "Unable to get the handle to the Program Manager.", "Fatal error", MB_OK|MB_ICONERROR);
		return;
	}

	*desktopview_wnd = FindWindowEx(*desktop_progman, NULL, "SHELLDLL_DefView", NULL);
	if(*desktopview_wnd == NULL)
	{
		//MessageBox(NULL, "Unable to get the handle to the desktopview.", "Fatal error", MB_OK|MB_ICONERROR);
		return;
	}

	// Thanks ef_ef_ef@yahoo.com for pointing out this works in NT 4 and not the way I did it originally.
	*listview_wnd = FindWindowEx(*desktopview_wnd, NULL, "SysListView32", NULL);
	if(*listview_wnd == NULL)
	{
		//MessageBox(NULL, "Unable to get the handle to the folderview.", "Fatal error", MB_OK|MB_ICONERROR);
		return;
	}
}

//----------------------------------------------------------------------

/*
int DetectWin2kOrLater()
{
// returns 0 for win95, NT, 98, me, or error
// returns 1 for win2k, xp...

OSVERSIONINFO osvi = { sizeof(osvi) };
if (GetVersionEx(&osvi))
{
//                Win95  Win98  WinNT 3.51  WinNT 4.0  Win 2000  WinXP
//dwPlatformID    1      1      2           2          2         2
//dwMajorVersion  4      4      3           4          5         5
//dwMinorVersion  0      10     51          0          0         1

if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion >= 5)
return 1;
else
return 0;
}

// error
return 0;
}
*/

//----------------------------------------------------------------------

int GetDesktopIconSize()
{
	int ret = 32;

	// reads the key: HKEY_CURRENT_USER\Control Panel, Desktop\WindowMetrics\Shell Icon Size
	unsigned char buf[64];
	unsigned long len = sizeof(buf);
	DWORD type;
	HKEY key;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop\\WindowMetrics", 0, KEY_READ, &key))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(key, "Shell Icon Size", NULL, &type, (unsigned char*)buf, &len) &&
			type == REG_SZ)
		{
			int x = atoi((char*)buf);
			if (x>0 && x<=128)
				ret = x;
		}

		RegCloseKey(key);
	}

	return ret;
}
#endif
