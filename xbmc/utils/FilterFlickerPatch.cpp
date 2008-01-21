// ******************************************************
// XBE Flicker Filter Patch 26.08.2006 GeminiServer
// ******************************************************
// Based on: hargle and VooD []
//           Win32 Flicker Filter XBE Patcher
//           http://xbox.nugnugnug.com/FlickerFucker/
//
// ******************************************************
#include "stdafx.h"
#include "FilterFlickerPatch.h"
#include "../FileSystem/File.h"
#include "../Util.h"
#include "../GUISettings.h"
#include "log.h"

using namespace XFILE;

int patchCount=0;
UINT patches[50][2];

bool CGFFPatch::FFPatch(CStdString m_FFPatchFilePath, CStdString &strNEW_FFPatchFilePath)  // apply the patch
{
  CFile xbe;
  CStdString tmp;
	UINT location, startPos;
	BOOL results=FALSE;
 
  CLog::Log(LOGDEBUG, __FUNCTION__" - Opening file: %s",m_FFPatchFilePath.c_str());
  if (!xbe.Open(m_FFPatchFilePath))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - ERROR: Opening file: %s",m_FFPatchFilePath.c_str());
    return false;
  }
  
  xbe.Open(m_FFPatchFilePath);
  int size = (int)xbe.GetLength();
	tmp.Format("%d", (size/1024) );
	
  CLog::Log(LOGDEBUG, __FUNCTION__" - File size is:%s kb",tmp.c_str());
	
  BYTE* pbuffer = new BYTE[size+1];
	if ( pbuffer == NULL )
	{
		CLog::Log(LOGDEBUG, __FUNCTION__" - Unable to allocate enough memory to load file!");
		xbe.Close();
		return false;
	}

  xbe.Read(&pbuffer[0],size);
  CLog::Log(LOGDEBUG, __FUNCTION__" - XBE File loaded successfully into memory");
	CLog::Log(LOGDEBUG, __FUNCTION__" - Examining file for potential patch locations...");
	startPos = 0;

	for (int i=0; i<50; i++)
	{
		patches[i][0] = 0;
		patches[i][1] = 0;
	}
	patchCount=0;

	while (1)
	{
		location = searchData(&pbuffer[0], startPos, size);
		if ( (location == 0) && (patchCount > 0) )  // looked through the whole file?
			break;

		if ( (location == 0) && (patchCount == 0) )
		{
			CLog::Log(LOGDEBUG, __FUNCTION__" - Unable to find key bytes to apply patch.");
			CLog::Log(LOGDEBUG, __FUNCTION__" - Could be a previos patched, homebrew or a non patchable xbe!");
			CLog::Log(LOGDEBUG, __FUNCTION__" - File not patched, exiting");
			return false;
		}
		
    patches[patchCount][0] = location;
		patches[patchCount][1] = examinePatch(&pbuffer[0],location);
		if (patches[patchCount][1] > 0)
		{
			switch (patches[patchCount][1])
			{
				case 1:
					CLog::Log(LOGDEBUG, __FUNCTION__" - Patch type 1 found.");
					break;
				case 2:
					CLog::Log(LOGDEBUG, __FUNCTION__" - Patch type 2 found.");
					break;
				case 3:
					CLog::Log(LOGDEBUG, __FUNCTION__" - Patch type 3 found.");
					break;
				case 4:
					CLog::Log(LOGDEBUG, __FUNCTION__" - Patch type 4 found.");
					break;
			}
			patchCount++;
		}
		startPos = location+4;		// keep searching...
	}

  xbe.Close();
  if (patchCount > 0)
	{
		results = applyPatches(&pbuffer[0], patchCount);
		if (results)
		{
      // Create a new file with the patch!
      CFile file;
      CStdString strNewFFPFile;
      strNewFFPFile.Format("%s_ffp.xbe",m_FFPatchFilePath.Left(m_FFPatchFilePath.GetLength()-4));
      if(file.OpenForWrite(strNewFFPFile.c_str(),true,true))
      {
        file.Write(&pbuffer[0],size);
        file.Close();
        CLog::Log(LOGDEBUG, __FUNCTION__" - File written and closed.");
        strNEW_FFPatchFilePath = strNewFFPFile;
      }else return false;
		}
	}
  delete []pbuffer;
  return true;
}


BOOL CGFFPatch::applyPatches(BYTE* pbuffer, int patchCount)
{
	BOOL results = FALSE;

		for (int i=0; i<patchCount; i++)
		{
			 
      bool bDoPatch = true; //can be setting controlled
      if (bDoPatch) // PatchMethod: 1, 2, 3, 4
			{
				patches[i][1] &= 0x7f;
				switch (patches[i][1])
				{
				case 1:
					results |= Patch1(&pbuffer[0], patches[i][0]);
					break;
				case 2:
					results |= Patch2(&pbuffer[0], patches[i][0]);
					break;
				case 3:
					results |= Patch3(&pbuffer[0], patches[i][0]);
					break;
				case 4:
					results |= Patch4(&pbuffer[0], patches[i][0]);
					break;
				}
			}
		}
	return results;
}

BOOL CGFFPatch::Patch1(BYTE* pbuffer, UINT location)
{
	int flickerVal = g_guiSettings.GetInt("videoscreen.flickerfilter");
	UINT tmp=location;
	replaceConditionalJump(&pbuffer[0], tmp, 0x20);
	pbuffer[location-1] = flickerVal;
  CLog::Log(LOGDEBUG, __FUNCTION__" - Patching type 1, finished.");	
	return TRUE;
}

BOOL CGFFPatch::Patch2(BYTE* pbuffer, UINT location)
{
	int flickerVal = g_guiSettings.GetInt("videoscreen.flickerfilter");
	UINT tmp=location;
	replaceConditionalJump(&pbuffer[0], tmp, 0x20);
	pbuffer[tmp] = 0x6a;
	pbuffer[tmp+1] = 00;
	pbuffer[location-2] = flickerVal;
	pbuffer[location-1] = 0x90;
	replaceConditionalJump(&pbuffer[0], tmp, 10);	// see if there's a 2nd conditional jump we need to worry about.
	CLog::Log(LOGDEBUG, __FUNCTION__" - Patching type 2, finished.");
	return TRUE;
}

BOOL CGFFPatch::Patch3(BYTE* pbuffer, UINT location)
{
	int flickerVal = g_guiSettings.GetInt("videoscreen.flickerfilter");
	UINT tmp=location;
	replaceConditionalJump(&pbuffer[0], tmp, 0x20);
	pbuffer[tmp] = pbuffer[location-2];		// move the solitary push
	pbuffer[location-2] = 0x6a;				// replace current byte with 2byte sequence 
	pbuffer[location-1] = flickerVal;
	replaceConditionalJump(&pbuffer[0], tmp, 10);	// see if there's a 2nd conditional jump we need to worry about.
	CLog::Log(LOGDEBUG, __FUNCTION__" - Patching type 3, finished.");		
	return TRUE;
}

BOOL CGFFPatch::Patch4(BYTE* pbuffer, UINT location)
{
	int flickerVal = g_guiSettings.GetInt("videoscreen.flickerfilter");
	UINT tmp=location;
	replaceConditionalJump(&pbuffer[0], tmp, 0x20);
	pbuffer[tmp] = pbuffer[location-4];		// move the 2 bytes before the pushes start.  Prolly an "xor reg,reg"
	pbuffer[tmp+1] = pbuffer[location-3];		
	pbuffer[location-4] = pbuffer[location-2];	// move the push statement up 2 bytes
	pbuffer[location-3] = 0x6a;					// add in push [flickerval]
  pbuffer[location-2] = flickerVal;
	pbuffer[location-1] = 0x90;					// NOP
	replaceConditionalJump(&pbuffer[0], tmp, 10);
  CLog::Log(LOGDEBUG, __FUNCTION__" - Patching type 4, finished.");
	return TRUE;
}
void CGFFPatch::replaceConditionalJump(BYTE* pbuffer, UINT &location, UINT range)
{
	if (findConditionalJump(&pbuffer[0], location, range) )
	{
		pbuffer[location]=0x90;
		pbuffer[location+1]=0x90;
		CLog::Log(LOGDEBUG, __FUNCTION__" - Conditional JMP replaced.");
	}
}

BOOL CGFFPatch::findConditionalJump(BYTE* pbuffer, UINT &location, UINT range)
{
	CLog::Log(LOGDEBUG, __FUNCTION__" - Locate conditional jump...");
	CStdString tmp;
	for (UINT i=location; i>(location-range); i--)		// should be within ~20 bytes
	{
    //if ( (pbuffer[i] == 0x74) && (pbuffer[i+1] < range+0x10) )
    if ( (pbuffer[i] == 0x74) && (pbuffer[i+1] < range+0x10) && (pbuffer[i-3] != 0x8b) )
    {
			location = i;
				tmp.Format("%x", i);
				CLog::Log(LOGDEBUG, __FUNCTION__" - found at:0x",tmp.c_str());
			return TRUE;
		}
	}
	CLog::Log(LOGDEBUG, __FUNCTION__" - Not Found.  Skipping");
	return FALSE;
}
int CGFFPatch::examinePatch(BYTE* pbuffer, UINT location)
{
	UINT tmp=location;
	if (findConditionalJump(&pbuffer[0], tmp, 0x20) == FALSE)
		return 0;
	if ( (pbuffer[location-2] == 0x6a) && 
		 (pbuffer[location-1] >= 0 ) && 
		 (pbuffer[location-1] < 6 ) &&
		 ( (pbuffer[location-3] == 0) && (pbuffer[location-4] == 0x6a) || ((pbuffer[location-3] & 0x50) == 0x50) ) )
		return 1;
	if ( ((pbuffer[location-1] & 0x50) == 0x50) && 
		 (pbuffer[location-2] == 0) && 
		 (pbuffer[location-3] == 0x6a) )
		return 2;

  if ( ((pbuffer[location-1] & 0x50) == 0x50) && 
		 ((pbuffer[location-2] & 0x50) == 0x50) &&
		  (pbuffer[location-3] == 0x00) )			
		return 3;

  if ( ((pbuffer[location-1] & 0x50) == 0x50) && 
		 ((pbuffer[location-2] & 0x50) == 0x50) &&
		  (pbuffer[location-5] == 0x00) )
		return 4;
	return 0; //Default
}

UINT CGFFPatch::searchData(BYTE* pBuffer, UINT startPos, UINT size)
{
	for (UINT i=startPos; i<size; i++)
	{
		if ( (pBuffer[i] == 0x6a) && 
			 (pBuffer[i+1] == 0x0b) && 
			 ((pBuffer[i+2] & 0x50) == 0x50) &&
			 (pBuffer[i+3] == 0xff) )
			return i;

	}
	return 0;  // no (more) matches
}