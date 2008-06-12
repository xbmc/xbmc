
BOOL DisplaySystemVersion(LPTSTR buffer)
{
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;
	
	LPTSTR tmp=buffer;
	
	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	// If that fails, try using the OSVERSIONINFO structure.
	
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
	bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);
	if( !bOsVersionInfoEx )
	{
		// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
			return FALSE;
	}
	
	switch (osvi.dwPlatformId)
	{
		// Tests for Windows NT product family.
	case VER_PLATFORM_WIN32_NT:
		
		// Test for the product.
		
		if ( osvi.dwMajorVersion <= 4 )
			tmp+=_stprintf( tmp, _T("Microsoft Windows NT ") );
		
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			tmp+=_stprintf( tmp, _T("Microsoft Windows 2000 ") );
		
		
		if( bOsVersionInfoEx )  // Use information from GetVersionEx.
		{ 
			// Test for the workstation type.
			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
					tmp+=_stprintf ( tmp, _T("Microsoft Windows XP ") );
				
				if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					tmp+=_stprintf ( tmp, _T("Home Edition ") );
				else
					tmp+=_stprintf ( tmp, _T("Professional ") );
			}
			
			// Test for the server type.
			else if ( osvi.wProductType == VER_NT_SERVER )
			{
				if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
					tmp+=_stprintf ( tmp, _T("Microsoft Windows .NET ") );
				
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					tmp+=_stprintf ( tmp, _T("DataCenter Server ") );
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					if( osvi.dwMajorVersion == 4 )
						tmp+=_stprintf ( tmp, _T("Advanced Server ") );
					else
						tmp+=_stprintf ( tmp, _T("Enterprise Server ") );
				else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
					tmp+=_stprintf ( tmp, _T("Web Server ") );
				else
					tmp+=_stprintf ( tmp, _T("Server ") );
			}
		}
		else   // Use the registry on early versions of Windows NT.
		{
			HKEY hKey;
			TCHAR szProductType[80];
			DWORD dwBufLen=80*sizeof(TCHAR);
			
			RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				_T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
				0, KEY_QUERY_VALUE, &hKey );
			RegQueryValueEx( hKey, _T("ProductType"), NULL, NULL,
				(LPBYTE) szProductType, &dwBufLen);
			RegCloseKey( hKey );
			if ( lstrcmpi( _T("WINNT"), szProductType) == 0 )
				tmp+=_stprintf( tmp, _T("Professional ") );
			if ( lstrcmpi( _T("LANMANNT"), szProductType) == 0 )
				tmp+=_stprintf( tmp, _T("Server ") );
			if ( lstrcmpi( _T("SERVERNT"), szProductType) == 0 )
				tmp+=_stprintf( tmp, _T("Advanced Server ") );
		}
		
		// Display version, service pack (if any), and build number.
		
		if ( osvi.dwMajorVersion <= 4 )
		{
			tmp+=_stprintf ( tmp, _T("version %d.%d %s (Build %d)"),
				osvi.dwMajorVersion,
				osvi.dwMinorVersion,
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
		}
		else
		{ 
			tmp+=_stprintf ( tmp, _T("%s (Build %d)"),
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
		}
		break;
		
		// Test for the Windows 95 product family.
	case VER_PLATFORM_WIN32_WINDOWS:
		
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			tmp+=_stprintf ( tmp, _T("Microsoft Windows 95 ") );
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
				tmp+=_stprintf( tmp, _T("OSR2 ") );
		} 
		
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			tmp+=_stprintf ( tmp, _T("Microsoft Windows 98 ") );
			if ( osvi.szCSDVersion[1] == 'A' )
				tmp+=_stprintf( tmp, _T("SE ") );
		} 
		
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
		{
			tmp+=_stprintf ( tmp, _T("Microsoft Windows Millennium Edition ") );
		} 
		break;
	}
	if (tmp==buffer)
		tmp+=_stprintf( tmp, _T("%d.%d build %d"), osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	return TRUE; 
}
