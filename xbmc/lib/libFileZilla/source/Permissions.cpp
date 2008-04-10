// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// Permissions.cpp: Implementierung der Klasse CPermissions.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "misc\md5.h"
#include "Permissions.h"
#include "misc\MarkupSTL.h"
#include "options.h"
#include "GUISettings.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPermissionsHelperWindow

class CPermissionsHelperWindow
{
public:
	CPermissionsHelperWindow(CPermissions *pPermissions)
	{
		ASSERT(pPermissions);
		m_pPermissions=pPermissions;
		
		//Create window
		WNDCLASSEX wndclass; 
		wndclass.cbSize=sizeof wndclass; 
		wndclass.style=0; 
		wndclass.lpfnWndProc=WindowProc; 
		wndclass.cbClsExtra=0; 
		wndclass.cbWndExtra=0; 
		wndclass.hInstance=GetModuleHandle(0); 
		wndclass.hIcon=0; 
		wndclass.hCursor=0; 
		wndclass.hbrBackground=0; 
		wndclass.lpszMenuName=0; 
		wndclass.lpszClassName=_T("CPermissions Helper Window"); 
		wndclass.hIconSm=0; 
	
		RegisterClassEx(&wndclass);
	
		m_hWnd=CreateWindow(_T("CPermissions Helper Window"), _T("CPermissions Helper Window"), 0, 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(0));
		ASSERT(m_hWnd);
		SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
	};

	virtual ~CPermissionsHelperWindow()
	{
		//Destroy window
		if (m_hWnd)
		{
			DestroyWindow(m_hWnd);
			m_hWnd=0;
		}
	}

	HWND GetHwnd()
	{
		return m_hWnd;
	}

protected:
	static LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		if (message==WM_USER)
		{
			CPermissionsHelperWindow *pWnd=(CPermissionsHelperWindow *)GetWindowLong(hWnd, GWL_USERDATA);
			if (!pWnd)
				return 0;
			ASSERT(pWnd);
			ASSERT(pWnd->m_pPermissions);
			
			pWnd->m_pPermissions->m_sync.Lock();
			
			pWnd->m_pPermissions->m_GroupsList.clear();
			for (CPermissions::t_GroupsList::iterator groupiter=pWnd->m_pPermissions->m_sGroupsList.begin(); groupiter!=pWnd->m_pPermissions->m_sGroupsList.end(); groupiter++)
				pWnd->m_pPermissions->m_GroupsList.push_back(*groupiter);
			
			pWnd->m_pPermissions->m_UsersList.clear();
			for (CPermissions::t_UsersList::iterator iter=pWnd->m_pPermissions->m_sUsersList.begin(); iter!=pWnd->m_pPermissions->m_sUsersList.end(); iter++)
			{
				CUser user = *iter;
				user.pOwner = NULL;
				if (user.group != _T(""))
				{
					for (CPermissions::t_GroupsList::iterator groupiter=pWnd->m_pPermissions->m_GroupsList.begin(); groupiter!=pWnd->m_pPermissions->m_GroupsList.end(); groupiter++)
						if (groupiter->group == user.group)
						{
							user.pOwner = &(*groupiter);
							break;
						}
				}
				pWnd->m_pPermissions->m_UsersList.push_back(user);
			}
			
			pWnd->m_pPermissions->m_sync.Unlock();
		}
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

protected:
	CPermissions *m_pPermissions;

private:
	HWND m_hWnd;
};

/////////////////////////////////////////////////////////////////////////////
// CPermissions

CCriticalSectionWrapper CPermissions::m_sync;
CPermissions::t_UsersList CPermissions::m_sUsersList;
CPermissions::t_GroupsList CPermissions::m_sGroupsList;
std::list<CPermissions *> CPermissions::m_sInstanceList;

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CPermissions::CPermissions()
{
	Init();

	m_pPermissionsHelperWindow=new CPermissionsHelperWindow(this);
}

CPermissions::~CPermissions()
{
	m_sync.Lock();
	std::list<CPermissions *>::iterator instanceIter;
	for (instanceIter=m_sInstanceList.begin(); instanceIter!=m_sInstanceList.end(); instanceIter++)
		if (*instanceIter==this)
			break;
	ASSERT(instanceIter!=m_sInstanceList.end());
	m_sInstanceList.erase(instanceIter);
	m_sync.Unlock();
	if (m_pPermissionsHelperWindow)
		delete m_pPermissionsHelperWindow;
}

int CPermissions::GetDirectoryListing(LPCTSTR user, CStdString dir, t_dirlisting *&pResult)
{
	unsigned int index;
	for (index=0; index<m_UsersList.size(); index++)
	{
		if (!m_UsersList[index].user.CompareNoCase(user))
			break;
	}
	if (index==m_UsersList.size())
		return PERMISSION_DENIED;
	BOOL bRelative = m_UsersList[index].UseRelativePaths();
	
	t_directory directory;
	BOOL bTruematch;
	int res = GetRealDirectory(dir, index, directory, bTruematch);
	CStdString sFileSpec = "*.*";
	if (res==PERMISSION_FILENOTDIR || res==PERMISSION_NOTFOUND) // Try listing using a direct wildcard filespec instead?
	{	// The PERMISSION_NOTFOUND case above can be removed to not allow wildcards.
		int i = dir.find_last_of("\\/");
		if (i < 0)
			return res;
		sFileSpec = dir.Mid(i+1);
		dir = dir.Left(i+1);
		res = GetRealDirectory(dir, index, directory, bTruematch);
	}
	if (res)
		return res;
	if (!directory.bDirList)
		return PERMISSION_DENIED;
	if (!bTruematch && !directory.bDirSubdirs)
		return PERMISSION_DENIED;
	
	WIN32_FIND_DATA FindFileData;
	WIN32_FIND_DATA NextFindFileData;
	HANDLE hFind;
	TIME_ZONE_INFORMATION tzInfo;
	int tzRes = GetTimeZoneInformation(&tzInfo);
	_int64 offset = tzInfo.Bias+((tzRes==TIME_ZONE_ID_DAYLIGHT)?tzInfo.DaylightBias:tzInfo.StandardBias);
	offset *= 60 * 10000000;

	t_dirlisting *pDir = new t_dirlisting;
	pDir->len = 0;
	pDir->pNext = NULL;
	pResult = pDir;

#ifdef _XBOX
	XSetFileCacheSize(256*1024);
#endif
		
	BOOL bIncludeLinks;
	// If NOT searching for all files, exclude links in the first run
	if (sFileSpec == "*.*" || sFileSpec == "*")
		bIncludeLinks = TRUE;
	else
		bIncludeLinks = FALSE;
	hFind = FindFirstFile(directory.dir+"\\" + sFileSpec, &NextFindFileData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		FindFileData=NextFindFileData;
		if (!FindNextFile(hFind, &NextFindFileData))
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}

		if (!_tcscmp(FindFileData.cFileName, _T(".")) || !_tcscmp(FindFileData.cFileName, _T("..")))
			continue;
		
		// This wastes some memory but keeps the whole thing fast
		if ((8192 - pDir->len) < (60 + 2 * MAX_PATH))
		{
			pDir->pNext = new t_dirlisting;
			pDir = pDir->pNext;
			pDir->len = 0;
			pDir->pNext = NULL;
		}
		
		CStdString fn = FindFileData.cFileName;
		CStdString fn2 = fn.Right(4);
		fn2.MakeLower();
		
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			memcpy(pDir->buffer + pDir->len, "drwxr-xr-x", 10);
			pDir->len += 10;
		}
		else if (fn2==".lnk" && m_UsersList[index].ResolveLinks())
		{
			if (!bIncludeLinks)
				continue;
			if (bRelative)
			{
				fn=fn.Left(fn.GetLength()-4);
				t_directory directory;
				BOOL truematch;
				if (GetRealDirectory(dir+"/"+fn,index,directory, truematch))
					continue;
				if (!directory.bDirList)
					continue;
				if (!truematch && !directory.bDirSubdirs)
					continue;
				memcpy(pDir->buffer + pDir->len, "drwxr-xr-x", 10);
				pDir->len += 10;
			}
			else
			{
				CStdString lnkpath = GetShortcutTarget(directory.dir+"\\"+FindFileData.cFileName);
				lnkpath.Replace(":u", m_UsersList[index].user);
				lnkpath.Replace(":U", m_UsersList[index].user);
				if (lnkpath == "")
					continue;
				lnkpath.Replace("\\", "/");
				lnkpath.TrimRight("/");
				fn = fn.Left(fn.GetLength()-4);
				
				t_directory directory;
				BOOL truematch;
				if (GetRealDirectory("/"+lnkpath,index,directory,truematch))
					continue;
				if (!directory.bDirList)
					continue;
				if (!truematch && !directory.bDirSubdirs)
					continue;

				memcpy(pDir->buffer + pDir->len, "lrwxr-xr-x", 10);
				pDir->len += 10;
				directory.dir.Replace("\\","/");
				directory.dir.TrimRight("/");
				fn = fn + " -> "+"/" + directory.dir;
			}
		}
		else
		{
			pDir->buffer[pDir->len++] = '-';
			pDir->buffer[pDir->len++] = directory.bFileRead ? 'r' : '-';
			pDir->buffer[pDir->len++] = directory.bFileWrite ? 'w' : '-';
			
			BOOL isexe = FALSE;
			CStdString ext = fn.Right(4);
			ext.MakeLower();
			if (ext.ReverseFind('.')!=-1)
			{
				if (ext == ".exe")
					isexe = TRUE;
				else if (ext == ".bat")
					isexe = TRUE;
				else if (ext == ".com")
					isexe = TRUE;
			}
			pDir->buffer[pDir->len++] = isexe ? 'x' : '-';
			pDir->buffer[pDir->len++] = directory.bFileRead ? 'r' : '-';
			pDir->buffer[pDir->len++] = '-';
			pDir->buffer[pDir->len++] = isexe ? 'x' : '-';
			pDir->buffer[pDir->len++] = directory.bFileRead ? 'r' : '-';
			pDir->buffer[pDir->len++] = '-';
			pDir->buffer[pDir->len++] = isexe ? 'x' : '-';
		}
		memcpy(pDir->buffer + pDir->len, "   1 ftp      ftp", 17);
		pDir->len += 17;

		CStdString size;
		_int64 size64 = FindFileData.nFileSizeLow + ((_int64)FindFileData.nFileSizeHigh<<32);
		pDir->len += sprintf(pDir->buffer + pDir->len, "% 14I64d", size64);
			
		SYSTEMTIME sLocalTime;
		GetLocalTime(&sLocalTime);
		FILETIME fTime;
		VERIFY(SystemTimeToFileTime(&sLocalTime, &fTime));

		SYSTEMTIME sFileTime;
		_int64 t1 = ((_int64)FindFileData.ftLastWriteTime.dwHighDateTime<<32) + FindFileData.ftLastWriteTime.dwLowDateTime;
		t1-=offset;
		FindFileData.ftLastWriteTime.dwHighDateTime = (DWORD)(t1>>32);
		FindFileData.ftLastWriteTime.dwLowDateTime = (DWORD)(t1%0xFFFFFFFF);
													
		FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &sFileTime);
				
		_int64 t2 = ((_int64)fTime.dwHighDateTime<<32) + fTime.dwLowDateTime;
		const char months[][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		pDir->len += sprintf(pDir->buffer + pDir->len, " %s %02d ", months[sFileTime.wMonth-1], sFileTime.wDay);
#if !defined(_XBOX)
		// bug: wrong (future) year causes some ftp clients (e.g. flashfxp, cuteftp) to not display directory entries
		if (t1 > t2 || (t2-t1) > ((_int64)1000000*60*60*24*350))
			pDir->len += sprintf(pDir->buffer + pDir->len, "%d  ", sFileTime.wYear);
		else
#endif
			pDir->len += sprintf(pDir->buffer + pDir->len, "%02d:%02d ", sFileTime.wHour, sFileTime.wMinute);

		int len = fn.GetLength();
		memcpy(pDir->buffer + pDir->len, fn.c_str(), len);
		pDir->len += len;
		pDir->buffer[pDir->len++] = '\r';
		pDir->buffer[pDir->len++] = '\n';
	}
	if (bIncludeLinks || !m_UsersList[index].ResolveLinks())
	{
#ifdef _XBOX
		XSetFileCacheSize(64*1024);
#endif
		return 0;
	}

	// Now repeat the search with .lnk added
	hFind = FindFirstFile(directory.dir+"\\" + sFileSpec + ".lnk", &NextFindFileData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		FindFileData=NextFindFileData;
		if (!FindNextFile(hFind, &NextFindFileData))
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
			
		if (!_tcscmp(FindFileData.cFileName, _T(".")) || !_tcscmp(FindFileData.cFileName, _T("..")))
			continue;
	
		// This wastes some memory but keeps the whole thing fast
		if ((8192 - pDir->len) < (60 + MAX_PATH))
		{
			pDir->pNext = new t_dirlisting;
			pDir = pDir->pNext;
			pDir->len = 0;
			pDir->pNext = NULL;
		}
		
		CStdString fn = FindFileData.cFileName;
		
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		
		if (bRelative)
		{
			fn = fn.Left(fn.GetLength()-4);
			t_directory directory;
			BOOL truematch;
			if (GetRealDirectory(dir+"/"+fn,index,directory,truematch))
				continue;
			if (!directory.bDirList)
				continue;
			if (!truematch && !directory.bDirSubdirs)
				continue;
			memcpy(pDir->buffer + pDir->len, "drwxr-xr-x", 10);
			pDir->len += 10;
		}
		else
		{
			CStdString lnkpath = GetShortcutTarget(directory.dir+"\\"+FindFileData.cFileName);
			lnkpath.Replace(":u", m_UsersList[index].user);
			lnkpath.Replace(":U", m_UsersList[index].user);
			if (lnkpath=="")
				continue;
			lnkpath.Replace("\\","/");
			lnkpath.TrimRight("/");
			fn=fn.Left(fn.GetLength()-4);
			
			t_directory directory;
			BOOL truematch;
			if (GetRealDirectory("/"+lnkpath,index,directory,truematch))
				continue;
			if (!directory.bDirList)
				continue;
			if (!truematch && !directory.bDirSubdirs)
				continue;
			memcpy(pDir->buffer + pDir->len, "lrwxr-xr-x", 10);
			pDir->len += 10;
			directory.dir.Replace("\\","/");
			directory.dir.TrimRight("/");
			fn=fn+" -> "+"/"+directory.dir;
		}
		
		memcpy(pDir->buffer + pDir->len, "   1 ftp      ftp", 17);
		pDir->len += 17;
		
		CStdString size;
		_int64 size64 = FindFileData.nFileSizeLow + ((_int64)FindFileData.nFileSizeHigh<<32);
		pDir->len += sprintf(pDir->buffer + pDir->len, "% 14I64d", size64);
		
		SYSTEMTIME sLocalTime;
		GetLocalTime(&sLocalTime);
		FILETIME fTime;
		VERIFY(SystemTimeToFileTime(&sLocalTime, &fTime));
		
		SYSTEMTIME sFileTime;
		_int64 t1 = ((_int64)FindFileData.ftLastWriteTime.dwHighDateTime<<32) + FindFileData.ftLastWriteTime.dwLowDateTime;
		t1-=offset;
		FindFileData.ftLastWriteTime.dwHighDateTime = (DWORD)(t1>>32);
		FindFileData.ftLastWriteTime.dwLowDateTime = (DWORD)(t1%0xFFFFFFFF);
		
		FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &sFileTime);
		
		_int64 t2 = ((_int64)fTime.dwHighDateTime<<32) + fTime.dwLowDateTime;
		const char months[][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		pDir->len += sprintf(pDir->buffer + pDir->len, " %s %02d ", months[sFileTime.wMonth-1], sFileTime.wDay);
#if !defined(_XBOX)
		// bug: wrong (future) year causes some ftp clients (e.g. flashfxp, cuteftp) to not display directory entries
		if (t1 > t2 || (t2-t1) > ((_int64)1000000*60*60*24*350))
			pDir->len += sprintf(pDir->buffer + pDir->len, "%d  ", sFileTime.wYear);
		else
#endif
			pDir->len += sprintf(pDir->buffer + pDir->len, "%02d:%02d ", sFileTime.wHour, sFileTime.wMinute);
		
		int len = fn.GetLength();
		memcpy(pDir->buffer + pDir->len, fn.c_str(), len);
		pDir->len += len;
		pDir->buffer[pDir->len++] = '\r';
		pDir->buffer[pDir->len++] = '\n';
	}

#ifdef _XBOX
	XSetFileCacheSize(64*1024);
#endif

	return 0;
}

int CPermissions::GetDirName(LPCTSTR user, CStdString dirname, CStdString currentdir, int op, CStdString &physical, CStdString &logical)
{
	//Reformat the directory
	dirname.Replace("\\", "/");
	while(dirname.Replace("//", "/"));
	dirname.TrimRight("/");
	if (dirname == "")
		return PERMISSION_NOTFOUND;
	else
	{
		//Reassamble the path, parse the dots
		std::list<CStdString> piecelist;
		int pos;
		if (dirname.Left(1)!="/")
		{ //New relative path
		  //The current dir has to be added to the split into pieces
			CStdString tmp=currentdir;
			tmp.TrimRight("/");
			tmp.TrimLeft("/");
			while((pos=tmp.Find("/"))!=-1)
			{
				piecelist.push_back(tmp.Left(pos));
				tmp=tmp.Mid(pos+1);
			}
			if (tmp!="")
				piecelist.push_back(tmp);	
		}
		//Split the new path into pieces and add it to the piecelist
		dirname.TrimLeft("/");
		while((pos=dirname.Find("/"))!=-1 || dirname.size())
		{
      if(pos<0) pos = dirname.size();
      CStdString tmp = dirname.Left(pos);

      if (g_guiSettings.GetBool("servers.ftpautofatx"))
			{
        if(tmp.length() > 42)
          tmp = tmp.Left(42);
        tmp.TrimRight(" \\");
				if( tmp.length() && tmp[tmp.length()-1] != ':') // avoid fuckups with F: etc
					CUtil::RemoveIllegalChars(tmp);
      }

      if (tmp!="")
			  piecelist.push_back(tmp);
			dirname=dirname.Mid(pos+1);
		}

    int remove=0; //Number of pieces that will be removed due to dots
		for (std::list<CStdString>::reverse_iterator iter=piecelist.rbegin(); iter!=piecelist.rend(); iter++)
		{
			CStdString tmp=*iter;
			if (tmp == "..")
			{ //Oh, double dots found! Remove one piece
				remove++;
				tmp=tmp.Mid(1);
			}
			if (tmp!=".") //Skips single dots
			{
				if (remove) 
					remove--; //Don't add the piece
				else
					dirname="/"+tmp+dirname;
			}			
		}
		if (remove)
			return PERMISSION_DENIED;
		if (dirname == "")
			return PERMISSION_NOTFOUND;
	}

	CStdString dir;
	int pos = dirname.ReverseFind('/');
	if (pos == -1)
		return PERMISSION_NOTFOUND;
  logical = dirname;
  dir=dirname.Left(pos);
	if (dir == "")
		dir = "/";
	dirname = dirname.Mid(pos+1);

	//Get userindex based on user string
	unsigned int index;
	for (index=0; index<m_UsersList.size(); index++)
	{
		if (!m_UsersList[index].user.CompareNoCase(user))
			break;
	}
	if (index==m_UsersList.size())
		return PERMISSION_DENIED; //No user found
	
  CStdString realdir, realdirname;
  CStdString dir2(dir), dirname2(dirname);
	//Get the physical path, only of dir to get the right permissions
	t_directory directory;
	BOOL truematch;
	int res;
	do
	{
		res = GetRealDirectory(dir, index, directory, truematch);
		if (res&PERMISSION_NOTFOUND && op==DOP_CREATE) 
		{ //that path could not be found. Maybe more than one directory level has to be created, check that
			int pos = dir.ReverseFind('/');
			if (pos == -1)
				return res;
			if (dir=="/") //We are already at the topmost level!
				return res;
			dirname=dir.Mid(pos+1)+"/"+dirname;
			dir=dir.Left(pos);
			if (dir=="")
				dir = "/";
			continue;
		}
		else if (res) 
			return res;
		realdir = directory.dir;
    realdirname = dirname;
		if (!directory.bDirDelete && op&DOP_DELETE)
			res |= PERMISSION_DENIED;
		if (!directory.bDirCreate && op&DOP_CREATE)
			res |= PERMISSION_DENIED;
		if (!truematch && !directory.bDirSubdirs)
			res |= PERMISSION_DENIED;
		break;
	} while (TRUE);

  //realdir and realdirname should now be complete, (realdir contains the existing part, realdirname the other
  realdirname.Replace("/", "\\");
  physical = realdir + "\\" + realdirname;

  //Restore what we actually want to create
  dir = dir2;
  dirname = dirname2;

	//Check if dir+dirname is a valid path
	int res2 = GetRealDirectory(dir+"/"+dirname, index, directory, truematch);
	if (!res2 && op&DOP_CREATE)
		res |= PERMISSION_DOESALREADYEXIST;
	else if (!(res2 & PERMISSION_NOTFOUND))
		return res | res2;
		
	//dir+dirname could no be found
	DWORD nAttributes = GetFileAttributes(physical);
	if (nAttributes==0xFFFFFFFF && !(op&DOP_CREATE))
		res |= PERMISSION_NOTFOUND;
	else if (!(nAttributes&FILE_ATTRIBUTE_DIRECTORY))
		res |= PERMISSION_FILENOTDIR;
	
	//Finally, a valid path+dirname!
	return res;
}

int CPermissions::GetFileName(LPCTSTR user, CStdString filename, CStdString currentdir, int op, CStdString &result)
{
	//Get userindex based on user string
	unsigned int index;
	for (index=0; index<m_UsersList.size(); index++)
	{
		if (!m_UsersList[index].user.CompareNoCase(user))
			break;
	}
	if (index==m_UsersList.size())
		return PERMISSION_DENIED; //No user found

	//If links are resolved, don't allow any operation on lnk files
	BOOL bLnk = m_UsersList[index].ResolveLinks();
	if (filename.Right(4)==".lnk" && bLnk)
		return PERMISSION_DENIED;

	//Reformat the directory
	filename.Replace("\\","/");
	while(filename.Replace("//","/"));
	filename.TrimRight("/");
	if (filename=="")
		return PERMISSION_NOTFOUND;
	else
	{
		//Reassamble the path, parse the dots
		std::list<CStdString> piecelist;
		int pos;
		if (filename.Left(1)!="/")
		{ //New relative path
		  //The current dir has to be added to the split into pieces
			CStdString tmp=currentdir;
			tmp.TrimRight("/");
			tmp.TrimLeft("/");
			while((pos=tmp.Find("/"))!=-1)
			{
				piecelist.push_back(tmp.Left(pos));
				tmp=tmp.Mid(pos+1);
			}
			if (tmp!="")
				piecelist.push_back(tmp);	
		}
		//Split the new path into pieces and add it to the piecelist
		filename.TrimLeft("/");
		while((pos=filename.Find("/"))!=-1)
		{
			piecelist.push_back(filename.Left(pos));
			filename=filename.Mid(pos+1);
		}
		if (filename!="")
			piecelist.push_back(filename);
		filename="";
		int remove=0; //Number of pieces that will be removed due to dots
		for (std::list<CStdString>::reverse_iterator iter=piecelist.rbegin(); iter!=piecelist.rend(); iter++)
		{
			CStdString tmp=*iter;
			while (tmp == "..")
			{ //Oh, double dots found! Remove one piece
				remove++;
				tmp=tmp.Mid(1);
			}
			if (tmp!=".") //Skips single dots
			{
				if (remove) 
					remove--; //Don't add the piece
				else
					filename="/"+tmp+filename;
			}			
		}
		if (remove)
			return PERMISSION_DENIED;
		if (filename=="")
			return PERMISSION_NOTFOUND;
	}
	CStdString dir;
	int pos=filename.ReverseFind('/');
	if (pos==-1)
		return PERMISSION_NOTFOUND;
	dir=filename.Left(pos);
	if (dir=="")
		dir="/";
	filename=filename.Mid(pos+1);
	dir.MakeLower();
	//dir now is the absolute path (logical server path of course)
	//while filename is the filename

	//Get the physical path
	t_directory directory;
	BOOL truematch;
	int res=GetRealDirectory(dir,index,directory,truematch);
	
	dir = directory.dir;
	result = dir+"\\"+filename;
	if (res)
		return res;
	if (!directory.bFileRead && op&FOP_READ)
		res |= PERMISSION_DENIED;
	if (!directory.bFileDelete && op&FOP_DELETE)
		res |= PERMISSION_DENIED;
	if (!directory.bFileWrite && op&(FOP_CREATENEW|FOP_WRITE|FOP_APPEND))
		res |= PERMISSION_DENIED;
	if (!truematch && !directory.bDirSubdirs)
		res |= PERMISSION_DENIED;
	dir.TrimRight("\\");
	DWORD nAttributes = GetFileAttributes(dir+"\\"+filename);
	if (nAttributes==0xFFFFFFFF)
	{
		if (!(op&(FOP_WRITE|FOP_APPEND|FOP_CREATENEW)))
			res |= PERMISSION_NOTFOUND;
	}
	else
	{
		if (nAttributes&FILE_ATTRIBUTE_DIRECTORY)
			res |= PERMISSION_DIRNOTFILE;
		if (!directory.bFileAppend && op&FOP_APPEND)
			res |= PERMISSION_DENIED;
		if (!directory.bFileDelete && op&FOP_WRITE)
			res |= PERMISSION_DENIED;
		if (op & FOP_CREATENEW)
			res |= PERMISSION_DOESALREADYEXIST;
	}
	
	//If res is 0 we finally have a valid path+filename!
	return res;
}

CStdString CPermissions::GetHomeDir(const CUser &user, BOOL bRealPath /*=FALSE*/) const
{
	BOOL bRelative = user.UseRelativePaths();

	CStdString path;
	if (bRealPath || !bRelative)
	{
		if (user.homedir == "")
			return "";
		path = user.homedir;
	}
	
	if (bRealPath)
	{
		path.Replace(":u", user.user);
		path.Replace(":U", user.user);
		return path;
	}
	

	if (!bRelative)
	{
		path.Replace("\\","/");
		path.TrimRight("/");
		path="/"+path;
		path.Replace(":u", user.user);
		path.Replace(":U", user.user);
		return path;
	}
	return "/";
}

CStdString CPermissions::GetHomeDir(LPCTSTR username, BOOL bRealPath /*=FALSE*/) const
{
	CUser user;
	if (!GetUser(username, user))
		return "";

	return GetHomeDir(user, bRealPath);
}

CStdString CPermissions::GetHomeDir(unsigned int index, BOOL bRealPath /*=FALSE*/) const
{
	if (index<0 || index>=m_UsersList.size())
		return "";

	return GetHomeDir(m_UsersList[index], bRealPath);
}

int CPermissions::GetRealDirectory(CStdString directory, int user, t_directory &ret, BOOL &truematch)
{
	BOOL bRelative = m_UsersList[user].UseRelativePaths();
	BOOL bLnk = m_UsersList[user].ResolveLinks();
	CStdString realpath;
	directory.TrimLeft("/");
	directory.TrimRight("/");
	if (!bRelative)
	{
		directory.Replace("/","\\");
		while (directory.Replace("\\\\", "\\"));

		directory.TrimRight("\\");

		//Split dir into pieces
		std::list<CStdString> PathPieces;
		int pos;
		
		int remove=0;
		while((pos=directory.ReverseFind('\\'))!=-1 || directory!="")
		{
			CStdString piece=directory.Mid(pos+1);
			
			BOOL bRemoveThis=FALSE;
#ifdef _XBOX
			while (piece.Left(3)=="../")
			{ //Oh, double dots found! Remove one piece
				bRemoveThis=TRUE;
				remove++;
				piece=piece.Mid(2);
			}
#else
			while (piece.Left(2)=="..")
			{ //Oh, double dots found! Remove one piece
				bRemoveThis=TRUE;
				remove++;
				piece=piece.Mid(1);
			}
#endif
			if (!bRemoveThis && piece!=".") //Skips single dots
			{
				if (remove) 
					remove--; //Don't add the piece
				else
					PathPieces.push_front(piece);
			}
			if (directory.GetLength()>pos)
				directory=directory.Left(pos);
			else
				directory="";
		}
		if (remove) //Deny access above home
			return PERMISSION_DENIED;

		if (PathPieces.empty())
			return PERMISSION_DENIED;

		CStdString path = PathPieces.front();
		PathPieces.pop_front();

#ifdef _XBOX
	    if (1 /*g_stSettings.m_bFTPSingleCharDrives*/)
	    {
			// modified to be consistent with other xbox ftp behavior: drive 
			// name is a single character without the ':' at the end
			if (path.size() == 1)
				path += ':';
	    }
#endif

		for (std::list<CStdString>::iterator iter = PathPieces.begin(); iter!=PathPieces.end(); iter++)
		{
			CStdString piece=*iter;
			DWORD nAttributes=GetFileAttributes(path+"\\"+piece);
			if (nAttributes!=0xFFFFFFFF)
			{
				if (!(nAttributes&FILE_ATTRIBUTE_DIRECTORY))
					return PERMISSION_DIRNOTFILE;
				path+="\\"+piece;
			}
			else if (m_UsersList[user].ResolveLinks() && (nAttributes=GetFileAttributes(path+"\\"+piece+".lnk"))!=0xFFFFFFFF )
			{
				if (nAttributes&FILE_ATTRIBUTE_DIRECTORY)
					return PERMISSION_DIRNOTFILE;
			
				CStdString target=GetShortcutTarget(path+"\\"+piece+".lnk");
				target.Replace(":u", m_UsersList[user].user);
				target.Replace(":U", m_UsersList[user].user);
				if (target=="")
					return PERMISSION_NOTFOUND;
				if (target.Right(2)!=":\\")
				{
					nAttributes=GetFileAttributes(target);
					if (nAttributes==0xFFFFFFFF)
						return PERMISSION_NOTFOUND;
					else if (!(nAttributes&FILE_ATTRIBUTE_DIRECTORY))
						return PERMISSION_FILENOTDIR;
				}
				path=target;
			}
			else
				return PERMISSION_NOTFOUND;
		}
		realpath = path;
	}
	else
	{
		//First get the home dir
		CStdString homepath = GetHomeDir(user, TRUE);
		if (homepath == "") //No homedir found
			return PERMISSION_DENIED;
		
		homepath.TrimRight("\\");

		//Split dir into pieces
		std::list<CStdString> PathPieces;
		int pos;
	
		int remove=0;
		while((pos=directory.ReverseFind('/'))!=-1 || directory!="")
		{
			CStdString piece=directory.Mid(pos+1);

			BOOL bRemoveThis=FALSE;
#ifdef _XBOX
			while (piece.Left(3)=="../")
			{ //Oh, double dots found! Remove one piece
				bRemoveThis=TRUE;
				remove++;
				piece=piece.Mid(2);
			}
#else
			while (piece.Left(2)=="..")
			{ //Oh, double dots found! Remove one piece
				bRemoveThis=TRUE;
				remove++;
				piece=piece.Mid(1);
			}
#endif
			if (!bRemoveThis && piece!=".") //Skips single dots
			{
				if (remove) 
					remove--; //Don't add the piece
				else
					PathPieces.push_front(piece);
			}
			if (directory.GetLength()>pos)
				directory=directory.Left(pos);
			else
				directory="";
		}
		if (remove) //Deny access above home
			return PERMISSION_DENIED;
      
		CStdString path = homepath;

		for (std::list<CStdString>::iterator iter=PathPieces.begin(); iter!=PathPieces.end(); iter++)
		{
			CStdString piece=*iter;
			DWORD nAttributes=GetFileAttributes(path+"\\"+piece);
			if (nAttributes!=0xFFFFFFFF)
			{
				if (!(nAttributes&FILE_ATTRIBUTE_DIRECTORY))
					return PERMISSION_FILENOTDIR;
				path+="\\"+piece;
			}
			else if (m_UsersList[user].ResolveLinks() && (nAttributes=GetFileAttributes(path+"\\"+piece+".lnk"))!=0xFFFFFFFF )
			{
				if (nAttributes&FILE_ATTRIBUTE_DIRECTORY)
					return PERMISSION_NOTFOUND;
			
				CStdString target=GetShortcutTarget(path+"\\"+piece+".lnk");
				target.Replace(":u", m_UsersList[user].user);
				target.Replace(":U", m_UsersList[user].user);
				if (target=="")
					return PERMISSION_NOTFOUND;
				if (target.Right(2)!=":\\")
				{
					nAttributes=GetFileAttributes(target);
					if (nAttributes==0xFFFFFFFF)
						return PERMISSION_NOTFOUND;
					else if (!(nAttributes&FILE_ATTRIBUTE_DIRECTORY))
						return PERMISSION_FILENOTDIR;
				}
				path = target;
			}
			else
				return PERMISSION_NOTFOUND;
		}
		realpath = path;
	}
	//We found a valid local path! Now find an matching path within the permissions
	truematch = FALSE;
	CStdString path = realpath;
	while (path!="")
	{
		BOOL bFoundMatch = FALSE;
		unsigned int i;
		CStdString path2=path;
		path2.TrimRight("\\");
		path2.MakeLower();
		for (i=0; i<m_UsersList[user].permissions.size(); i++)
		{
			CStdString path3=m_UsersList[user].permissions[i].dir;
			path3.TrimRight("\\");
			path3.MakeLower();
			path3.Replace(":u", m_UsersList[user].user);
			path3.Replace(":U", m_UsersList[user].user);
			if (path3==path2)
			{
				if (path==realpath)
					truematch = TRUE;
				bFoundMatch = TRUE;
				ret = m_UsersList[user].permissions[i];
				break;
			}
		}
		if (!bFoundMatch && m_UsersList[user].pOwner)
			for (i=0; i<m_UsersList[user].pOwner->permissions.size(); i++)
			{
				CStdString path3=m_UsersList[user].pOwner->permissions[i].dir;
				path3.TrimRight("\\");
				path3.MakeLower();
				path3.Replace(":u", m_UsersList[user].user);
				path3.Replace(":U", m_UsersList[user].user);
				if (path3==path2)
				{
					if (path==realpath)
						truematch = TRUE;
					bFoundMatch = TRUE;
					ret = m_UsersList[user].pOwner->permissions[i];
					break;
				}
			}

		if (!bFoundMatch)
		{
			int pos = path.ReverseFind('\\');
			if (pos!=-1)
				path=path.Left(pos);
			else
			{
				return PERMISSION_DENIED;
			}
			continue;
		}
		realpath.TrimRight('\\');
		ret.dir = realpath;
		return 0;
	}
	return PERMISSION_NOTFOUND;
}

int CPermissions::ChangeCurrentDir(LPCTSTR user, CStdString &currentdir, CStdString &dir)
{
    //Get userindex based on user string
	unsigned int index;
    for (index=0; index<m_UsersList.size(); index++)
    {
        if (!m_UsersList[index].user.CompareNoCase(user))
            break;
    }
    if (index==m_UsersList.size())
        return PERMISSION_DENIED; //No user found
    BOOL bRelative=m_UsersList[index].UseRelativePaths();

    //Reformat the directory
	dir.Replace("\\","/");
	while(dir.Replace("//","/"));
	dir.TrimRight("/");
	if (dir=="")
	{
		if (!bRelative)
		{
			//dir was / - We need to prepend with the drive letter
			dir=currentdir.Mid(1, 2) + '/';
		}
		else
		{
			dir="/";
		}
	}
	else
	{
		//Reassamble the path, remove the dots
		std::list<CStdString> piecelist;
		int pos;
		if (!bRelative && dir.GetLength()>=2)
		{
			//Make sure / is in front of a dir starting with a drive letter
			if (isalpha(dir[0]) && dir[1]==':')
				dir="/"+dir;
		}
		if (dir.Left(1)!="/")
		{ //New relative path
		  //The current dir has to be added to the split into pieces
			CStdString tmp=currentdir;
			tmp.TrimRight("/");
			tmp.TrimLeft("/");
			while((pos=tmp.Find("/"))!=-1)
			{
				piecelist.push_back(tmp.Left(pos));
				tmp=tmp.Mid(pos+1);
			}
			if (tmp!="")
				piecelist.push_back(tmp);	
		}
		else
		{
			if (!bRelative)
			{
				//dir starts with a / - Does that include the driver letter?
				if (dir.GetLength()<3 || !isalpha(dir[1]) || (dir[2] != ':'))
				{
#ifdef _XBOX
					if (1 /*g_stSettings.m_bFTPSingleCharDrives*/ &&
					  (isalpha(dir[1]) && ((dir.GetLength() == 2) || (dir[2] == '/'))))
					{
					// modified to be consistent with other xbox ftp behavior: drive 
					// name is a single character without the ':' at the end
					// this is the drive letter specified without the ':' character 
					// at the end - correct it
					CString drive = dir.substr(0, 2);
					drive.ToUpper();
					if (dir.GetLength() > 3)
					  dir = drive + ":/" + dir.Mid(3);
					else
					  dir = drive + ":/";
					}
					else
					{
#endif
				  //We were given an absolute path without a drive letter we need to put one in the dir list
				  piecelist.push_back(currentdir.Mid(1, 2));
#ifdef _XBOX				  
					}
#endif
				}
			}
		}

		//Split the new path into pieces and add it to the piecelist
		dir.TrimLeft("/");
		while((pos=dir.Find("/"))!=-1)
		{
			piecelist.push_back(dir.Left(pos));
			dir=dir.Mid(pos+1);
		}
		if (dir!="")
			piecelist.push_back(dir);
		dir="";
		int remove=0; //Number of pieces that will be removed due to dots
		for (std::list<CStdString>::reverse_iterator iter=piecelist.rbegin(); iter!=piecelist.rend(); iter++)
		{
			CStdString tmp=*iter;
			if (tmp == "..")
			{ //Oh, double dots found! Remove one piece
				remove++;
				tmp=tmp.Mid(1);
			}
			if (tmp!=".") //Skips single dots
			{
				if (remove) 
					remove--; //Don't add the piece
				else
					dir="/"+tmp+dir;
			}			
		}
		if (dir=="")
			dir="/";
	}
	//dir now is the absolute vpath
#if defined(_XBOX)
	//in case of xbox => if the user has a homedir of "/"
	//make it currentdir without any checks for real file permissions
	if((dir=="/") && (bRelative == FALSE) && (GetHomeDir(user)=="/")) {
		currentdir=dir;
		return 0;
	}
#endif
	//Get the physical path
	t_directory directory;
	BOOL truematch;
	int res=GetRealDirectory(dir,index,directory,truematch);
	if (res)
		return res;
	if (!directory.bDirList)
	{
		if (!directory.bFileRead && !directory.bFileWrite)
			return PERMISSION_DENIED;
	}
	if (!truematch && !directory.bDirSubdirs)
		return PERMISSION_DENIED;
	//Finally, a valid path!
	if (bRelative)
		currentdir=dir; //Server paths are relative, so we can use the logical server path
	else
	{
		//No relative server path, we have to convert the physical path into a logical server path.
		directory.dir.Replace("\\","/");
		directory.dir.TrimRight("/");
		currentdir="/"+directory.dir;
	}
#ifdef _XBOX
	if ((currentdir.size() == 3) && (currentdir[0]=='/') && (isalpha(currentdir[1])) && (currentdir[2]==':'))
		currentdir.MakeUpper();
#endif
	return 0;
}

CStdString CPermissions::GetShortcutTarget(LPCTSTR filename)
{
#if !defined(_XBOX)
	CoInitialize(0);
	
	CStdString target;
	HRESULT hres;
	IShellLink *psl;
	char szGotPath [MAX_PATH];
	WIN32_FIND_DATA wfd;

	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance (CLSID_ShellLink, NULL,
	CLSCTX_INPROC_SERVER,
	IID_IShellLink, (void **)&psl);
	if (SUCCEEDED (hres))
	{
		IPersistFile *ppf;

		// Get a pointer to the IPersistFile interface.
		hres = psl->QueryInterface (IID_IPersistFile, (void
			**)&ppf);

		if (SUCCEEDED (hres))
		{
			WORD wsz [MAX_PATH]; // buffer for Unicode string

			// Ensure that the string consists of Unicode characters.
			MultiByteToWideChar (CP_ACP, 0, filename, -1, wsz,
			MAX_PATH);
			
			// Load the shortcut.
			hres = ppf->Load (wsz, STGM_READ);
			
			if (SUCCEEDED (hres))
			{
				// Resolve the shortcut.
				hres = psl->Resolve (0, SLR_ANY_MATCH|SLR_NO_UI);
				if (SUCCEEDED (hres))
				{
					strcpy (szGotPath, filename);
					// Get the path to the shortcut target.
					hres = psl->GetPath (szGotPath, MAX_PATH,
					(WIN32_FIND_DATA *)&wfd, 0);
					target=szGotPath;
				}
			}
			// Release the pointer to IPersistFile.
			ppf->Release ();
		}
		// Release the pointer to IShellLink.
		psl->Release ();
	}
	CoUninitialize();
	if (SUCCEEDED(hres))
		return target;
	else
		return "";
#else
  return "";
#endif
}

BOOL CPermissions::GetUser(CStdString user, CUser &userdata) const
{
	for (unsigned int i=0; i<m_UsersList.size(); i++)
	{
		if (!user.CompareNoCase(m_UsersList[i].user))
		{
			userdata = m_UsersList[i];
			return TRUE;
		}
	}
	return FALSE;
}

int CPermissions::GetShortDirectoryListing(LPCTSTR user, CStdString currentDir, CStdString dirToDisplay, t_dirlisting *&pResult)
{
	unsigned int index;
	for (index=0; index<m_UsersList.size(); index++)
	{
		if (!m_UsersList[index].user.CompareNoCase(user))
			break;
	}
	if (index == m_UsersList.size())
		return PERMISSION_DENIED;
	BOOL bRelative = m_UsersList[index].UseRelativePaths();
	
	CStdString dir;
	if (dirToDisplay[0] != '/')
		dir = currentDir + "/" + dirToDisplay;
	else
		dir = dirToDisplay;
	

	t_directory directory;
	BOOL bTruematch;
	int res = GetRealDirectory(dir, index, directory, bTruematch);
	CStdString sFileSpec = "*.*";
	if (res==PERMISSION_FILENOTDIR || res==PERMISSION_NOTFOUND) // Try listing using a direct wildcard filespec instead?
	{	// The PERMISSION_NOTFOUND case above can be removed to not allow wildcards.
		int i = dir.find_last_of("\\/");
		if (i < 0)
			return res;
		sFileSpec = dir.Mid(i+1);
		dir = dir.Left(i+1);

		i = dirToDisplay.find_last_of("/");
		if (i >= 0)			
			dirToDisplay = dirToDisplay.Left(i+1);
		else
			dirToDisplay = "";

		res = GetRealDirectory(dir, index, directory, bTruematch);
	}

	if (res)
		return res;
	if (!directory.bDirList)
		return PERMISSION_DENIED;
	if (!bTruematch && !directory.bDirSubdirs)
		return PERMISSION_DENIED;

	if (dirToDisplay != "")
		dirToDisplay += "/";

	t_dirlisting *pDir = new t_dirlisting;
	pDir->len = 0;
	pDir->pNext = NULL;
	pResult = pDir;
	
	BOOL bIncludeLinks;
	// If NOT searching for all files, exclude links in the first run
	if (sFileSpec == "*.*" || sFileSpec == "*")
		bIncludeLinks = TRUE;
	else
		bIncludeLinks = FALSE;

	WIN32_FIND_DATA FindFileData;
	WIN32_FIND_DATA NextFindFileData;
	HANDLE hFind;
	hFind = FindFirstFile(directory.dir + "\\" + sFileSpec, &NextFindFileData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		FindFileData=NextFindFileData;
		if (!FindNextFile(hFind, &NextFindFileData))
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
		
		if (!_tcscmp(FindFileData.cFileName, _T(".")) || !_tcscmp(FindFileData.cFileName, _T("..")))
			continue;
		
		// This wastes some memory but keeps the whole thing fast
		if ((8192 - pDir->len) < (10 + 2 * MAX_PATH))
		{
			pDir->pNext = new t_dirlisting;
			pDir = pDir->pNext;
			pDir->len = 0;
			pDir->pNext = NULL;
		}
		
		CStdString fn = FindFileData.cFileName;
		CStdString fn2 = fn.Right(4);
		fn2.MakeLower();
		
		if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fn2==".lnk" && m_UsersList[index].ResolveLinks())
		{
			if (!bIncludeLinks)
				continue;
			if (bRelative)
			{
				fn = fn.Left(fn.GetLength()-4);
				t_directory directory;
				BOOL truematch;
				if (GetRealDirectory(dir+"/"+fn, index, directory ,truematch))
					continue;
				if (!directory.bDirList)
					continue;
				if (!truematch && !directory.bDirSubdirs)
					continue;
			}
			else
			{
				CStdString lnkpath = GetShortcutTarget(directory.dir+"\\"+FindFileData.cFileName);
				if (lnkpath=="")
					continue;
				lnkpath.Replace(":u", m_UsersList[index].user);
				lnkpath.Replace(":U", m_UsersList[index].user);
				lnkpath.Replace("\\","/");
				lnkpath.TrimRight("/");
				fn = fn.Left(fn.GetLength()-4);
				
				t_directory directory;
				BOOL truematch;
				if (GetRealDirectory("/"+lnkpath,index,directory,truematch))
					continue;
				if (!directory.bDirList)
					continue;
				if (!truematch && !directory.bDirSubdirs)
					continue;
				directory.dir.Replace("\\","/");
				directory.dir.TrimRight("/");
			}
		}
		int len = dirToDisplay.GetLength();
		memcpy(pDir->buffer + pDir->len, dirToDisplay.c_str(), len);
		pDir->len += len;
		len = fn.GetLength();
		memcpy(pDir->buffer + pDir->len, fn.c_str(), len);
		pDir->len += len;
		pDir->buffer[pDir->len++] = '\r';
		pDir->buffer[pDir->len++] = '\n';

	}

	if (bIncludeLinks || !m_UsersList[index].ResolveLinks())
		return 0;

	// Now repeat the search with .lnk added
	hFind = FindFirstFile(directory.dir + "\\" + sFileSpec + ".lnk", &NextFindFileData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		FindFileData=NextFindFileData;
		if (!FindNextFile(hFind, &NextFindFileData))
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
		
		if (!_tcscmp(FindFileData.cFileName, _T(".")) || !_tcscmp(FindFileData.cFileName, _T("..")))
			continue;
		
		// This wastes some memory but keeps the whole thing fast
		if ((8192 - pDir->len) < (10 + 2 * MAX_PATH))
		{
			pDir->pNext = new t_dirlisting;
			pDir = pDir->pNext;
			pDir->len = 0;
			pDir->pNext = NULL;
		}
		
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		CStdString fn = FindFileData.cFileName;
		
		if (bRelative)
		{
			fn = fn.Left(fn.GetLength()-4);
			t_directory directory;
			BOOL truematch;
			if (GetRealDirectory(dir+"/"+fn, index, directory ,truematch))
				continue;
			if (!directory.bDirList)
				continue;
			if (!truematch && !directory.bDirSubdirs)
				continue;
		}
		else
		{
			CStdString lnkpath = GetShortcutTarget(directory.dir+"\\"+FindFileData.cFileName);
			if (lnkpath=="")
				continue;
			lnkpath.Replace(":u", m_UsersList[index].user);
			lnkpath.Replace(":U", m_UsersList[index].user);
			lnkpath.Replace("\\","/");
			lnkpath.TrimRight("/");
			fn = fn.Left(fn.GetLength()-4);
			
			t_directory directory;
			BOOL truematch;
			if (GetRealDirectory("/"+lnkpath,index,directory,truematch))
				continue;
			if (!directory.bDirList)
				continue;
			if (!truematch && !directory.bDirSubdirs)
				continue;
			directory.dir.Replace("\\","/");
			directory.dir.TrimRight("/");
		}
		
		int len = fn.GetLength();
		memcpy(pDir->buffer + pDir->len, fn.c_str(), len);
		pDir->len += len;
		pDir->buffer[pDir->len++] = '\r';
		pDir->buffer[pDir->len++] = '\n';

	}
	return 0;
}

BOOL CPermissions::Lookup(LPCTSTR user, LPCTSTR pass, CUser &userdata, int noPasswordCheck)
{
	const char *tmp = pass;
	MD5 md5;
	md5.update((unsigned char *)tmp, _tcslen(pass));
	md5.finalize();
	char *res = md5.hex_digest();
	CStdString hash=res;
	delete [] res;
	for (unsigned int i=0; i<m_UsersList.size(); i++)
	{
		CStdString curUser=m_UsersList[i].user;
		curUser.MakeLower();
		if (curUser==user)
		{
			if (noPasswordCheck || m_UsersList[i].password==hash || m_UsersList[i].password=="")
			{
				userdata=m_UsersList[i];
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return FALSE;
}

void CPermissions::UpdateInstances()
{
	m_sync.Lock();
	for (std::list<CPermissions *>::iterator iter=m_sInstanceList.begin(); iter!=m_sInstanceList.end(); iter++)
	{
		if (*iter!=this)
		{
			ASSERT((*iter)->m_pPermissionsHelperWindow);
			::PostMessage((*iter)->m_pPermissionsHelperWindow->GetHwnd(), WM_USER, 0, 0);
		}
	}
	m_sync.Unlock();
}

void CPermissions::SetKey(CMarkupSTL *pXML, LPCTSTR name, LPCTSTR value)
{
	ASSERT(pXML);
	pXML->AddChildElem(_T("Option"), value);
	pXML->AddChildAttrib(_T("Name"), name);
}

void CPermissions::SavePermissions(CMarkupSTL *pXML, const t_group &user)
{
	pXML->AddChildElem(_T("Permissions"));
	pXML->IntoElem();
	for (unsigned int i=0; i<user.permissions.size(); i++)
	{
		pXML->AddChildElem(_T("Permission"));
		pXML->AddChildAttrib(_T("Dir"), user.permissions[i].dir);
		pXML->IntoElem();
		SetKey(pXML, "FileRead", user.permissions[i].bFileRead ? "1":"0");
		SetKey(pXML, "FileWrite", user.permissions[i].bFileWrite ? "1":"0");
		SetKey(pXML, "FileDelete", user.permissions[i].bFileDelete ?"1":"0");
		SetKey(pXML, "FileAppend", user.permissions[i].bFileAppend ? "1":"0");
		SetKey(pXML, "DirCreate", user.permissions[i].bDirCreate ? "1":"0");
		SetKey(pXML, "DirDelete", user.permissions[i].bDirDelete ? "1":"0");
		SetKey(pXML, "DirList", user.permissions[i].bDirList ? "1":"0");
		SetKey(pXML, "DirSubdirs", user.permissions[i].bDirSubdirs ? "1":"0");	
		SetKey(pXML, "IsHome", user.permissions[i].bIsHome ? "1":"0");
		SetKey(pXML, "AutoCreate", user.permissions[i].bAutoCreate ? "1":"0");
		pXML->OutOfElem();
	}
	pXML->OutOfElem();
}

BOOL CPermissions::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
{
	if (!pBuffer)
		return FALSE;

	DWORD len = 4;
	m_sync.Lock();
	t_GroupsList::iterator groupiter;
	for (groupiter=m_sGroupsList.begin(); groupiter!=m_sGroupsList.end(); groupiter++)
		len += groupiter->GetRequiredBufferLen();

	t_UsersList::iterator iter;
	for (iter=m_sUsersList.begin(); iter!=m_sUsersList.end(); iter++)
		len += iter->GetRequiredBufferLen();

	*pBuffer=new char[len];
	char *p=*pBuffer;
	
	*p++ = m_sGroupsList.size()/256;
	*p++ = m_sGroupsList.size()%256;
	for (groupiter=m_sGroupsList.begin(); groupiter!=m_sGroupsList.end(); groupiter++)
	{
		p = groupiter->FillBuffer(p);
		if (!p)
		{
			delete [] *pBuffer;
			*pBuffer = NULL;
			return FALSE;
		}
	}
	
	*p++ = m_sUsersList.size()/256;
	*p++ = m_sUsersList.size()%256;
	for (iter=m_sUsersList.begin(); iter!=m_sUsersList.end(); iter++)
	{
		p = iter->FillBuffer(p);
		if (!p)
		{
			delete [] *pBuffer;
			*pBuffer = NULL;
			return FALSE;
		}
	}
	m_sync.Unlock();
	*nBufferLength = len;

	return TRUE;
}

BOOL CPermissions::ParseUsersCommand(unsigned char *pData, DWORD dwDataLength)
{
	m_GroupsList.clear();
	m_UsersList.clear();
	unsigned char *p=pData;
	
	if (dwDataLength < 2)
		return FALSE;
	int num = *p * 256 + p[1];
	p+=2;

	int i;
	for (i=0; i<num; i++)
	{
		t_group group;
		p = group.ParseBuffer(p, dwDataLength - (p-pData));
		if (!p)
			return FALSE;
		
		if (group.group != _T(""))
		{
			//Set a home dir if no home dir could be read
			BOOL bGotHome = FALSE;
			for (unsigned int dir = 0; dir<group.permissions.size(); dir++)
				if (group.permissions[dir].bIsHome)
				{
					bGotHome = TRUE;
					break;
				}

			if (!bGotHome && !group.permissions.empty())
				group.permissions.begin()->bIsHome = TRUE;

			m_GroupsList.push_back(group);
		}
	}

	if (static_cast<unsigned int>(p-pData+2)>dwDataLength)
		return FALSE;

	num = *p * 256 + p[1];
	p+=2;
	for (i=0; i<num; i++)
	{
		CUser user;
		
		p = user.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
			return FALSE;
		
		if (user.user != _T(""))
		{
			user.pOwner = NULL;
			if (user.group != _T(""))
			{
				for (t_GroupsList::iterator groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
					if (groupiter->group == user.group)
					{
						user.pOwner = &(*groupiter);
						break;
					}
				if (!user.pOwner)
					user.group = "";
			}

			if (!user.pOwner)
			{
				//Set a home dir if no home dir could be read
				BOOL bGotHome = FALSE;
				for (unsigned int dir = 0; dir<user.permissions.size(); dir++)
					if (user.permissions[dir].bIsHome)
					{
						bGotHome = TRUE;
						break;
					}

				if (!bGotHome && !user.permissions.empty())
					user.permissions.begin()->bIsHome = TRUE;
			}

			std::vector<t_directory>::iterator iter;
			for (iter = user.permissions.begin(); iter != user.permissions.end(); iter++)
			{
				if (iter->bIsHome)
				{
					user.homedir = iter->dir;
					break;
				}
			}
			if (user.homedir=="" && user.pOwner)
			{
				for (iter = user.pOwner->permissions.begin(); iter != user.pOwner->permissions.end(); iter++)
				{
					if (iter->bIsHome)
					{
						user.homedir = iter->dir;
						break;
					}
				}
			}

			m_UsersList.push_back(user);
		}
	}

	//Update the account list
	m_sync.Lock();
	
	m_sGroupsList.clear();
	for (t_GroupsList::const_iterator groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
		m_sGroupsList.push_back(*groupiter);

	m_sUsersList.clear();
	for (t_UsersList::const_iterator iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
		m_sUsersList.push_back(*iter);

	UpdateInstances();
	
	m_sync.Unlock();
	
	CMarkupSTL *pXML=COptions::GetXML();
	if (pXML)
	{
		while(pXML->FindChildElem(_T("Groups")))
			pXML->RemoveChildElem();
		pXML->AddChildElem(_T("Groups"));
		pXML->IntoElem();
		
		//Save the changed user details
		for (t_GroupsList::const_iterator groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
		{
			pXML->AddChildElem(_T("Group"));
			pXML->AddChildAttrib(_T("Name"), groupiter->group);
			pXML->IntoElem();
			
			CStdString str;
			str.Format(_T("%d"), groupiter->nLnk);
			SetKey(pXML, "Resolve Shortcuts", str);
			str.Format(_T("%d"), groupiter->nRelative);
			SetKey(pXML, "Relative", str);
			str.Format(_T("%d"), groupiter->nBypassUserLimit);
			SetKey(pXML, "Bypass server userlimit", str);
			str.Format(_T("%d"), groupiter->nUserLimit);
			SetKey(pXML, "User Limit", str);
			str.Format(_T("%d"), groupiter->nIpLimit);
			SetKey(pXML, "IP Limit", str);
			
			SavePermissions(pXML, *groupiter);
			SaveSpeedLimits(pXML, *groupiter);

			pXML->OutOfElem();
		}
		pXML->OutOfElem();
		pXML->ResetChildPos();

		while(pXML->FindChildElem(_T("Users")))
			pXML->RemoveChildElem();
		pXML->AddChildElem(_T("Users"));
		pXML->IntoElem();
		
		//Save the changed user details
		for (t_UsersList::const_iterator iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
		{
			pXML->AddChildElem(_T("User"));
			pXML->AddChildAttrib(_T("Name"), iter->user);
			pXML->IntoElem();
			
			CStdString str;
			SetKey(pXML, "Pass", iter->password);
			SetKey(pXML, "Group", iter->group);
			str.Format(_T("%d"), iter->nLnk);
			SetKey(pXML, "Resolve Shortcuts", str);
			str.Format(_T("%d"), iter->nRelative);
			SetKey(pXML, "Relative", str);
			str.Format(_T("%d"), iter->nBypassUserLimit);
			SetKey(pXML, "Bypass server userlimit", str);
			str.Format(_T("%d"), iter->nUserLimit);
			SetKey(pXML, "User Limit", str);
			str.Format(_T("%d"), iter->nIpLimit);
			SetKey(pXML, "IP Limit", str);
			
			SavePermissions(pXML, *iter);
			SaveSpeedLimits(pXML, *iter);

			pXML->OutOfElem();
		}
		if (!COptions::FreeXML(pXML))
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

BOOL CPermissions::Init()
{
	m_sync.Lock();
	if (m_sInstanceList.empty() && m_sUsersList.empty())
	{
		CMarkupSTL *pXML=COptions::GetXML();
		if (pXML)
		{
			if (!pXML->FindChildElem(_T("Groups")))
				pXML->AddChildElem(_T("Groups"));
			pXML->IntoElem();
			while (pXML->FindChildElem(_T("Group")))
			{
				t_group group;
				group.nIpLimit = group.nIpLimit = group.nUserLimit = 0;
				group.nBypassUserLimit = group.nLnk = group.nRelative = 2;
				group.group = pXML->GetChildAttrib(_T("Name"));
				if (group.group!="")
				{
					pXML->IntoElem();
					
					while (pXML->FindChildElem(_T("Option")))
					{
						CStdString name = pXML->GetChildAttrib(_T("Name"));
						CStdString value = pXML->GetChildData();
						if (name==_T("Resolve Shortcuts"))
							group.nLnk = _ttoi(value);
						else if (name == _T("Relative"))
							group.nRelative = _ttoi(value);
						else if (name == _T("Bypass server userlimit"))
							group.nBypassUserLimit = _ttoi(value);
						else if (name == _T("User Limit"))
							group.nUserLimit = _ttoi(value);
						else if (name == _T("IP Limit"))
							group.nIpLimit = _ttoi(value);
						
						if (group.nUserLimit<0 || group.nUserLimit>999999999)
							group.nUserLimit=0;
						if (group.nIpLimit<0 || group.nIpLimit>999999999)
							group.nIpLimit=0;
					}
					
					BOOL bGotHome = FALSE;
					ReadPermissions(pXML, group, bGotHome);
					//Set a home dir if no home dir could be read
					if (!bGotHome && !group.permissions.empty())
						group.permissions.begin()->bIsHome = TRUE;

					ReadSpeedLimits(pXML, group);
					
					m_sGroupsList.push_back(group);
					pXML->OutOfElem();
				}
			}
			pXML->OutOfElem();
			pXML->ResetChildPos();
				
			if (!pXML->FindChildElem(_T("Users")))
				pXML->AddChildElem(_T("Users"));
			pXML->IntoElem();

			while (pXML->FindChildElem(_T("User")))
			{
				CUser user;
				user.nIpLimit = user.nIpLimit = user.nUserLimit = 0;
				user.nBypassUserLimit = user.nLnk = user.nRelative = 2;
				user.user=pXML->GetChildAttrib(_T("Name"));
				if (user.user!="")
				{
					pXML->IntoElem();

					while (pXML->FindChildElem(_T("Option")))
					{
						CStdString name = pXML->GetChildAttrib(_T("Name"));
						CStdString value = pXML->GetChildData();
						if (name == _T("Pass"))
							user.password = value;
						else if (name==_T("Resolve Shortcuts"))
							user.nLnk = _ttoi(value);
						else if (name == _T("Relative"))
							user.nRelative = _ttoi(value);
						else if (name == _T("Bypass server userlimit"))
							user.nBypassUserLimit = _ttoi(value);
						else if (name == _T("User Limit"))
							user.nUserLimit = _ttoi(value);
						else if (name == _T("IP Limit"))
							user.nIpLimit = _ttoi(value);
						else if (name == _T("Group"))
							user.group = value;

						if (user.nUserLimit<0 || user.nUserLimit>999999999)
							user.nUserLimit=0;
						if (user.nIpLimit<0 || user.nIpLimit>999999999)
							user.nIpLimit=0;
					}

					if (user.group != _T(""))
					{
						for (t_GroupsList::iterator groupiter = m_sGroupsList.begin(); groupiter != m_sGroupsList.end(); groupiter++)
							if (groupiter->group == user.group)
							{
								user.pOwner = &(*groupiter);
								break;
							}
						
						if (!user.pOwner)
							user.group = "";
					}
					
					BOOL bGotHome = FALSE;
					ReadPermissions(pXML, user, bGotHome);
					
					//Set a home dir if no home dir could be read
					if (!bGotHome && !user.pOwner)
					{
						if (!user.permissions.empty())
							user.permissions.begin()->bIsHome = TRUE;
					}
					
					std::vector<t_directory>::iterator iter;
					for (iter = user.permissions.begin(); iter != user.permissions.end(); iter++)
					{
						if (iter->bIsHome)
						{
							user.homedir = iter->dir;
							break;
						}
					}
					if (user.homedir=="" && user.pOwner)
					{
						for (iter = user.pOwner->permissions.begin(); iter != user.pOwner->permissions.end(); iter++)
						{
							if (iter->bIsHome)
							{
								user.homedir = iter->dir;
								break;
							}
						}
					}
					
					ReadSpeedLimits(pXML, user);

					m_sUsersList.push_back(user);
					pXML->OutOfElem();
				}
			}
			COptions::FreeXML(pXML);
		}
	}
	m_GroupsList.clear();
	for (t_GroupsList::iterator groupiter=m_sGroupsList.begin(); groupiter!=m_sGroupsList.end(); groupiter++)
		m_GroupsList.push_back(*groupiter);

	m_UsersList.clear();
	for (t_UsersList::iterator iter=m_sUsersList.begin(); iter!=m_sUsersList.end(); iter++)
	{
		CUser user = *iter;
		user.pOwner = NULL;
		if (user.group != _T(""))
		{
			for (t_GroupsList::iterator groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
				if (groupiter->group == user.group)
				{
					user.pOwner = &(*groupiter);
					break;
				}
		}
		m_UsersList.push_back(user);
	}

	std::list<CPermissions *>::iterator instanceIter;
	for (instanceIter=m_sInstanceList.begin(); instanceIter!=m_sInstanceList.end(); instanceIter++)
		if (*instanceIter==this)
			break;
	if (instanceIter == m_sInstanceList.end())
		m_sInstanceList.push_back(this);
	m_sync.Unlock();

	return TRUE;
}

void CPermissions::ReadPermissions(CMarkupSTL *pXML, t_group &user, BOOL &bGotHome)
{
	bGotHome = FALSE;
	pXML->ResetChildPos();
	while (pXML->FindChildElem(_T("Permissions")))
	{
		pXML->IntoElem();
		while (pXML->FindChildElem(_T("Permission")))
		{
			t_directory dir;
			dir.dir=pXML->GetChildAttrib(_T("Dir"));
			if (dir.dir!=_T(""))
			{
				pXML->IntoElem();
				while (pXML->FindChildElem(_T("Option")))
				{
					CStdString name=pXML->GetChildAttrib(_T("Name"));
					CStdString value=pXML->GetChildData();
					if (name==_T("FileRead"))
						dir.bFileRead=value==_T("1");	
					else if (name==_T("FileWrite"))
						dir.bFileWrite=value==_T("1");	
					else if (name==_T("FileDelete"))
						dir.bFileDelete=value==_T("1");	
					else if (name==_T("FileAppend"))
						dir.bFileAppend=value==_T("1");	
					else if (name==_T("DirCreate"))
						dir.bDirCreate=value==_T("1");	
					else if (name==_T("DirDelete"))
						dir.bDirDelete=value==_T("1");	
					else if (name==_T("DirList"))
						dir.bDirList=value==_T("1");	
					else if (name==_T("DirSubdirs"))
						dir.bDirSubdirs=value==_T("1");	
					else if (name==_T("IsHome"))
						dir.bIsHome=value==_T("1");	
					else if (name==_T("AutoCreate"))
						dir.bAutoCreate=value==_T("1");	
				}
				
				//Avoid multiple home dirs
				if (dir.bIsHome)
					if (!bGotHome)
						bGotHome = TRUE;
					else
						dir.bIsHome = FALSE;
					user.permissions.push_back(dir);
					pXML->OutOfElem();
			}								
		}
		pXML->OutOfElem();
	}
}

void CPermissions::AutoCreateDirs(const char *username)
{
	CUser user;
	if (!GetUser(username, user))
		return;
	for (std::vector<t_directory>::iterator permissioniter = user.permissions.begin(); permissioniter!=user.permissions.end(); permissioniter++)
		if (permissioniter->bAutoCreate)
		{
			CStdString dir = permissioniter->dir;
			dir.Replace(":u", user.user);
			dir.Replace(":U", user.user);
			CreateDirectory(dir, NULL);
		}
	if (user.pOwner)
		for (std::vector<t_directory>::iterator permissioniter = user.pOwner->permissions.begin(); permissioniter!=user.pOwner->permissions.end(); permissioniter++)
			if (permissioniter->bAutoCreate)
			{
				CStdString dir = permissioniter->dir;
				dir.Replace(":u", user.user);
				dir.Replace(":U", user.user);
				CreateDirectory(dir, NULL);
			}
	
}

void CPermissions::ReadSpeedLimits(CMarkupSTL *pXML, t_group &group)
{
	pXML->ResetChildPos();
				
	while (pXML->FindChildElem(_T("SpeedLimits")))
	{
		CStdString str;
		int n;

		str = pXML->GetChildAttrib("DlType");
		n = _ttoi(str);
		if (n >= 0 && n < 4)
			group.nDownloadSpeedLimitType = n;
		str = pXML->GetChildAttrib("DlLimit");
		n = _ttoi(str);
		if (n > 0 && n < 65536)
			group.nDownloadSpeedLimit = n;
		str = pXML->GetChildAttrib("UlType");
		n = _ttoi(str);
		if (n >= 0 && n < 4)
			group.nUploadSpeedLimitType = n;
		str = pXML->GetChildAttrib("UlLimit");
		n = _ttoi(str);
		if (n > 0 && n < 65536)
			group.nUploadSpeedLimit = n;

		str = pXML->GetChildAttrib("ServerDlLimitBypass");
		n = _ttoi(str);
		if (n >= 0 && n < 4)
			group.nBypassServerDownloadSpeedLimit = n;

		str = pXML->GetChildAttrib("ServerUlLimitBypass");
		n = _ttoi(str);
		if (n >= 0 && n < 4)
			group.nBypassServerUploadSpeedLimit = n;

		pXML->IntoElem();

		while (pXML->FindChildElem(_T("Download")))
		{
			pXML->IntoElem();

			while (pXML->FindChildElem(_T("Rule")))
			{
				CSpeedLimit limit;
				str = pXML->GetChildAttrib("Speed");
				n = _ttoi(str);
				if (n < 0 || n > 65535)
					n = 10;
				limit.m_Speed = n;
				
				pXML->IntoElem();
				
				if (pXML->FindChildElem("Days"))
				{
					str = pXML->GetChildData();
					if (str != "")
						n = _ttoi(str);
					else
						n = 0x7F;
					limit.m_Day = n & 0x7F;
				}
				pXML->ResetChildPos();
				
				limit.m_DateCheck = FALSE;
				if (pXML->FindChildElem("Date"))
				{
					limit.m_DateCheck = TRUE;
					str = pXML->GetChildAttrib("Year");
					n = _ttoi(str);
					if (n < 1900 || n > 3000)
						n = 2003;
					limit.m_Date.y = n;
					str = pXML->GetChildAttrib("Month");
					n = _ttoi(str);
					if (n < 1 || n > 12)
						n = 1;
					limit.m_Date.m = n;
					str = pXML->GetChildAttrib("Day");
					n = _ttoi(str);
					if (n < 1 || n > 31)
						n = 1;
					limit.m_Date.d = n;
				}
				pXML->ResetChildPos();
				
				limit.m_FromCheck = FALSE;
				if (pXML->FindChildElem("From"))
				{
					limit.m_FromCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_FromTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.s = n;
				}
				pXML->ResetChildPos();
				
				limit.m_ToCheck = FALSE;
				if (pXML->FindChildElem("To"))
				{
					limit.m_ToCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_ToTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.s = n;
				}
				pXML->ResetChildPos();
				
				pXML->OutOfElem();

				group.DownloadSpeedLimits.push_back(limit);
			}
			pXML->OutOfElem();
		}
		pXML->ResetChildPos();

		while (pXML->FindChildElem(_T("Upload")))
		{
			pXML->IntoElem();

			while (pXML->FindChildElem(_T("Rule")))
			{
				CSpeedLimit limit;
				str = pXML->GetChildAttrib("Speed");
				n = _ttoi(str);
				if (n < 0 || n > 65535)
					n = 10;
				limit.m_Speed = n;
				
				pXML->IntoElem();
				
				if (pXML->FindChildElem("Days"))
				{
					str = pXML->GetChildData();
					if (str != "")
						n = _ttoi(str);
					else
						n = 0x7F;
					limit.m_Day = n & 0x7F;
				}
				pXML->ResetChildPos();
				
				limit.m_DateCheck = FALSE;
				if (pXML->FindChildElem("Date"))
				{
					limit.m_DateCheck = TRUE;
					str = pXML->GetChildAttrib("Year");
					n = _ttoi(str);
					if (n < 1900 || n > 3000)
						n = 2003;
					limit.m_Date.y = n;
					str = pXML->GetChildAttrib("Month");
					n = _ttoi(str);
					if (n < 1 || n > 12)
						n = 1;
					limit.m_Date.m = n;
					str = pXML->GetChildAttrib("Day");
					n = _ttoi(str);
					if (n < 1 || n > 31)
						n = 1;
					limit.m_Date.d = n;
				}
				pXML->ResetChildPos();
				
				limit.m_FromCheck = FALSE;
				if (pXML->FindChildElem("From"))
				{
					limit.m_FromCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_FromTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.s = n;
				}
				pXML->ResetChildPos();
				
				limit.m_ToCheck = FALSE;
				if (pXML->FindChildElem("To"))
				{
					limit.m_ToCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_ToTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.s = n;
				}
				pXML->ResetChildPos();
				
				pXML->OutOfElem();

				group.UploadSpeedLimits.push_back(limit);
			}
			pXML->OutOfElem();
		}

		pXML->OutOfElem();
	}
}

void CPermissions::SaveSpeedLimits(CMarkupSTL *pXML, const t_group &group)
{
	pXML->AddChildElem(_T("SpeedLimits"));

	CStdString str;

	pXML->SetChildAttrib("DlType", group.nDownloadSpeedLimitType);
	pXML->SetChildAttrib("DlLimit", group.nDownloadSpeedLimit);
	pXML->SetChildAttrib("UlType", group.nUploadSpeedLimitType);
	pXML->SetChildAttrib("UlLimit", group.nUploadSpeedLimit);

	pXML->SetChildAttrib("ServerDlLimitBypass", group.nBypassServerDownloadSpeedLimit);
	pXML->SetChildAttrib("ServerUlLimitBypass", group.nBypassServerUploadSpeedLimit);
		
	pXML->IntoElem();

	unsigned int i;

	pXML->AddChildElem("Download");
	pXML->IntoElem();
	for (i=0; i<group.DownloadSpeedLimits.size(); i++)
	{
		CSpeedLimit limit = group.DownloadSpeedLimits[i];
		pXML->AddChildElem(_T("Rule"));

		pXML->SetChildAttrib(_T("Speed"), limit.m_Speed);

		pXML->IntoElem();

		str.Format("%d", limit.m_Day);
		pXML->AddChildElem(_T("Days"), str);

		if (limit.m_DateCheck)
		{
			pXML->AddChildElem(_T("Date"));
			pXML->SetChildAttrib(_T("Year"), limit.m_Date.y);
			pXML->SetChildAttrib(_T("Month"), limit.m_Date.m);
			pXML->SetChildAttrib(_T("Day"), limit.m_Date.d);
		}

		if (limit.m_FromCheck)
		{
			pXML->AddChildElem(_T("From"));
			pXML->SetChildAttrib(_T("Hour"), limit.m_FromTime.h);
			pXML->SetChildAttrib(_T("Minute"), limit.m_FromTime.m);
			pXML->SetChildAttrib(_T("Second"), limit.m_FromTime.s);
		}
		
		if (limit.m_ToCheck)
		{
			pXML->AddChildElem(_T("To"));
			pXML->SetChildAttrib(_T("Hour"), limit.m_ToTime.h);
			pXML->SetChildAttrib(_T("Minute"), limit.m_ToTime.m);
			pXML->SetChildAttrib(_T("Second"), limit.m_ToTime.s);
		}
	
		pXML->OutOfElem();
	}
	pXML->OutOfElem();

	pXML->AddChildElem("Upload");
	pXML->IntoElem();
	for (i=0; i<group.UploadSpeedLimits.size(); i++)
	{
		CSpeedLimit limit = group.UploadSpeedLimits[i];
		pXML->AddChildElem(_T("Rule"));

		pXML->SetChildAttrib(_T("Speed"), limit.m_Speed);

		pXML->IntoElem();

		str.Format("%d", limit.m_Day);
		pXML->AddChildElem(_T("Days"), str);

		if (limit.m_DateCheck)
		{
			pXML->AddChildElem(_T("Date"));
			pXML->SetChildAttrib(_T("Year"), limit.m_Date.y);
			pXML->SetChildAttrib(_T("Month"), limit.m_Date.m);
			pXML->SetChildAttrib(_T("Day"), limit.m_Date.d);
		}

		if (limit.m_FromCheck)
		{
			pXML->AddChildElem(_T("From"));
			pXML->SetChildAttrib(_T("Hour"), limit.m_FromTime.h);
			pXML->SetChildAttrib(_T("Minute"), limit.m_FromTime.m);
			pXML->SetChildAttrib(_T("Second"), limit.m_FromTime.s);
		}
		
		if (limit.m_ToCheck)
		{
			pXML->AddChildElem(_T("To"));
			pXML->SetChildAttrib(_T("Hour"), limit.m_ToTime.h);
			pXML->SetChildAttrib(_T("Minute"), limit.m_ToTime.m);
			pXML->SetChildAttrib(_T("Second"), limit.m_ToTime.s);
		}
	
		pXML->OutOfElem();
	}
	pXML->OutOfElem();
	
	pXML->OutOfElem();
}

void CPermissions::ReloadConfig()
{
	m_UsersList.clear();
	m_GroupsList.clear();

	CMarkupSTL *pXML = COptions::GetXML();
	if (pXML)
	{
		if (!pXML->FindChildElem(_T("Groups")))
			pXML->AddChildElem(_T("Groups"));
		pXML->IntoElem();
		while (pXML->FindChildElem(_T("Group")))
		{
			t_group group;
			group.nIpLimit = group.nIpLimit = group.nUserLimit = 0;
			group.nBypassUserLimit = group.nLnk = group.nRelative = 2;
			group.group = pXML->GetChildAttrib(_T("Name"));
			if (group.group!="")
			{
				pXML->IntoElem();
				
				while (pXML->FindChildElem(_T("Option")))
				{
					CStdString name = pXML->GetChildAttrib(_T("Name"));
					CStdString value = pXML->GetChildData();
					if (name==_T("Resolve Shortcuts"))
						group.nLnk = _ttoi(value);
					else if (name == _T("Relative"))
						group.nRelative = _ttoi(value);
					else if (name == _T("Bypass server userlimit"))
						group.nBypassUserLimit = _ttoi(value);
					else if (name == _T("User Limit"))
						group.nUserLimit = _ttoi(value);
					else if (name == _T("IP Limit"))
						group.nIpLimit = _ttoi(value);
						
					if (group.nUserLimit<0 || group.nUserLimit>999999999)
						group.nUserLimit=0;
					if (group.nIpLimit<0 || group.nIpLimit>999999999)
						group.nIpLimit=0;
				}
					
				BOOL bGotHome = FALSE;
				ReadPermissions(pXML, group, bGotHome);
				//Set a home dir if no home dir could be read
				if (!bGotHome && !group.permissions.empty())
					group.permissions.begin()->bIsHome = TRUE;

				ReadSpeedLimits(pXML, group);
					
				m_GroupsList.push_back(group);
				pXML->OutOfElem();
			}
		}
		pXML->OutOfElem();
		pXML->ResetChildPos();
			
		if (!pXML->FindChildElem(_T("Users")))
			pXML->AddChildElem(_T("Users"));
		pXML->IntoElem();

		while (pXML->FindChildElem(_T("User")))
		{
			CUser user;
			user.nIpLimit = user.nIpLimit = user.nUserLimit = 0;
			user.nBypassUserLimit = user.nLnk = user.nRelative = 2;
			user.user=pXML->GetChildAttrib(_T("Name"));
			if (user.user!="")
			{
				pXML->IntoElem();

				while (pXML->FindChildElem(_T("Option")))
				{
					CStdString name = pXML->GetChildAttrib(_T("Name"));
					CStdString value = pXML->GetChildData();
					if (name == _T("Pass"))
						user.password = value;
					else if (name==_T("Resolve Shortcuts"))
						user.nLnk = _ttoi(value);
					else if (name == _T("Relative"))
						user.nRelative = _ttoi(value);
					else if (name == _T("Bypass server userlimit"))
						user.nBypassUserLimit = _ttoi(value);
					else if (name == _T("User Limit"))
						user.nUserLimit = _ttoi(value);
					else if (name == _T("IP Limit"))
						user.nIpLimit = _ttoi(value);
					else if (name == _T("Group"))
						user.group = value;

					if (user.nUserLimit<0 || user.nUserLimit>999999999)
						user.nUserLimit=0;
					if (user.nIpLimit<0 || user.nIpLimit>999999999)
						user.nIpLimit=0;
				}

				if (user.group != _T(""))
				{
					for (t_GroupsList::iterator groupiter = m_GroupsList.begin(); groupiter != m_GroupsList.end(); groupiter++)
						if (groupiter->group == user.group)
						{
							user.pOwner = &(*groupiter);
							break;
						}
					
					if (!user.pOwner)
						user.group = "";
				}
					
				BOOL bGotHome = FALSE;
				ReadPermissions(pXML, user, bGotHome);
					
				//Set a home dir if no home dir could be read
				if (!bGotHome && !user.pOwner)
				{
					if (!user.permissions.empty())
						user.permissions.begin()->bIsHome = TRUE;
				}
				
				std::vector<t_directory>::iterator iter;
				for (iter = user.permissions.begin(); iter != user.permissions.end(); iter++)
				{
					if (iter->bIsHome)
					{
						user.homedir = iter->dir;
						break;
					}
				}
				if (user.homedir=="" && user.pOwner)
				{
					for (iter = user.pOwner->permissions.begin(); iter != user.pOwner->permissions.end(); iter++)
					{
						if (iter->bIsHome)
						{
							user.homedir = iter->dir;
							break;
						}
					}
				}
					
				ReadSpeedLimits(pXML, user);

				m_UsersList.push_back(user);
				pXML->OutOfElem();
			}
		}
		COptions::FreeXML(pXML);
	}

	m_sync.Lock();

	m_sGroupsList.clear();
	for (t_GroupsList::iterator groupiter = m_GroupsList.begin(); groupiter != m_GroupsList.end(); groupiter++)
		m_sGroupsList.push_back(*groupiter);

	m_sUsersList.clear();
	for (t_UsersList::iterator iter = m_UsersList.begin(); iter != m_UsersList.end(); iter++)
	{
		CUser user = *iter;
		user.pOwner = NULL;
		if (user.group != _T(""))
		{
			for (t_GroupsList::iterator groupiter = m_GroupsList.begin(); groupiter != m_GroupsList.end(); groupiter++)
				if (groupiter->group == user.group)
				{
					user.pOwner = &(*groupiter);
					break;
				}
		}
		m_sUsersList.push_back(user);
	}

	UpdateInstances();

	m_sync.Unlock();
	
	return;
}
