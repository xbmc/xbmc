#ifndef PROCESSORINFO_H
#define PROCESSORINFO_H

class CProcessorInfo
{
protected:

	SYSTEM_INFO m_sysInfo;

public:
   CProcessorInfo(void)
   {
      ::GetSystemInfo(&m_sysInfo);
   }

   virtual ~CProcessorInfo(void)
   {
   }

   CStdString GetProcessorName(void)
   {
      CStdString sRC;

      CStdString sSpeed;
      CStdString sVendor;

   	// Get the processor speed info.
   	HKEY hKey;
   	LONG result = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"), 0, KEY_QUERY_VALUE, &hKey);

	   // Check if the function has succeeded.
      if (result == ERROR_SUCCESS) 
      {
      	DWORD data;
		   DWORD dataSize = sizeof(data);
         result = ::RegQueryValueEx (hKey, _T("~MHz"), NULL, NULL, (LPBYTE)&data, &dataSize);
		
         if (result == ERROR_SUCCESS)
         {
            sSpeed.Format ( _T("Speed: %dMHz "), data);
         }
         else
         {
            sSpeed = _T("Speed: Unknown ");
         }

      	TCHAR vendorData [64];
		   dataSize = sizeof (vendorData);

		   result = ::RegQueryValueEx (hKey, _T("VendorIdentifier"), NULL, NULL, (LPBYTE)vendorData, &dataSize);

         if (result == ERROR_SUCCESS)
         {
            sVendor.Format ( _T("Vendor: %s "), vendorData);
         }
         else
         {
            sVendor = _T("Vendor: Unknown ");
         }
	   }

	   // Make sure to close the reg key
      RegCloseKey (hKey);

      CStdString sType;
	   switch (m_sysInfo.dwProcessorType)
      {
      case PROCESSOR_INTEL_386:
         sType = _T("Type: Intel 386 ");
         break;
      case PROCESSOR_INTEL_486:
         sType = _T("Type: Intel 486 ");
         break;
      case PROCESSOR_INTEL_PENTIUM:
         sType = _T("Type: Intel Pentium compatible");
         break;
      case PROCESSOR_MIPS_R4000:
         sType = _T("Type: MIPS ");
         break;
      case PROCESSOR_ALPHA_21064:
         sType = _T("Type: Alpha ");
         break;
      default:
         sType = _T("Type: Unknown ");
         break;
	   }
      
      CStdString sProcessors;
      sProcessors.Format( _T("Number Of Processors: %lu "), m_sysInfo.dwNumberOfProcessors);

      CStdString sArchitecture;
      CStdString sProcessorLevel;
      CStdString sStepping;

      switch(m_sysInfo.wProcessorArchitecture)
      {
      case PROCESSOR_ARCHITECTURE_INTEL:
         sArchitecture = _T("Architecture: Intel ");
   		switch (m_sysInfo.wProcessorLevel) 
         {
			case 3:
            sProcessorLevel = _T("Level: 80386");
            {
               int iSteppingLevel = m_sysInfo.wProcessorRevision / 100;
               int iStepping = m_sysInfo.wProcessorRevision % 100;
               sStepping.Format( _T("Stepping: %c%u "), iSteppingLevel, iStepping);
            }
				break;
			case 4:
            sProcessorLevel = _T("Level: 80486");
            {
               int iSteppingLevel = m_sysInfo.wProcessorRevision / 100;
               int iStepping = m_sysInfo.wProcessorRevision % 100;
               sStepping.Format( _T("Stepping: %c%u "), iSteppingLevel, iStepping);
            }
				break;
			case 5:
            sProcessorLevel = _T("Level: Pentium");
            {
               typedef BOOL (*PIPFP)(DWORD);
				   PIPFP lpfn = (PIPFP)::GetProcAddress(GetModuleHandle( _T("kernel32.dll") ), "IsProcessorFeaturePresentA");
				   if (lpfn)
				   {
				      if ((lpfn)(PF_MMX_INSTRUCTIONS_AVAILABLE)) 
					   {
					      sProcessorLevel += _T (" MMX");
                  }
				   }

               int iModel = m_sysInfo.wProcessorRevision / 100;
               int iStepping = m_sysInfo.wProcessorRevision % 100;
               sStepping.Format( _T("Stepping: %u-%u "), iModel, iStepping);
            }
				break;
         case 6:
            sProcessorLevel = _T("Level: Pentium II/Pro");
            {
               int iModel = m_sysInfo.wProcessorRevision / 100;
               int iStepping = m_sysInfo.wProcessorRevision % 100;
               sStepping.Format( _T("Stepping: %u-%u "), iModel, iStepping);
            }
				break;
         default:
            sProcessorLevel.Format( _T("Level: Unknown %u "), m_sysInfo.wProcessorLevel);
            {
               int iModel = m_sysInfo.wProcessorRevision / 100;
               int iStepping = m_sysInfo.wProcessorRevision % 100;
               sStepping.Format( _T("Stepping: %u-%u "), iModel, iStepping);
            }
            break;
         }
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         sArchitecture = "Architecture: MIPS ";
         switch(m_sysInfo.wProcessorLevel)
         {
         case 0004:
            sProcessorLevel = "Level: R4000 ";
				break;
         default:
            sProcessorLevel.Format( _T("Level: Unknown %u "), m_sysInfo.wProcessorLevel);
            break;
         }
         sStepping.Format( _T("Stepping: 00%u"), m_sysInfo.wProcessorRevision);
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         sArchitecture = "Architecture: Alpha ";
         sProcessorLevel.Format( _T("Level: %u "), m_sysInfo.wProcessorLevel);
         {
            int iModel = m_sysInfo.wProcessorRevision / 100;
            int iStepping = m_sysInfo.wProcessorRevision % 100;
            sStepping.Format( _T("Stepping: %c%u "), iModel, iStepping);
         }
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         sArchitecture = _T("Architecture: PowerPC ");
         switch(m_sysInfo.wProcessorLevel)
         {
			case 1:
            sProcessorLevel = _T("Level: 601 ");
				break;
			case 3:
            sProcessorLevel = _T("Level: 603 ");
				break;
			case 4:
            sProcessorLevel = _T("Level: 604 ");
				break;
			case 6:
            sProcessorLevel = _T("Level: 603+ ");
				break;
			case 9:
            sProcessorLevel = _T("Level: 604+ ");
				break;
			case 20:
            sProcessorLevel = _T("Level: 620 ");
				break;
         default:
            sProcessorLevel.Format( _T("Level: Unknown %u "), m_sysInfo.wProcessorLevel);
            break;
         }
         {
            int iModel = m_sysInfo.wProcessorRevision / 100;
            int iStepping = m_sysInfo.wProcessorRevision % 100;
            sStepping.Format( _T("Stepping: %u.%u "), iModel, iStepping);
         }
         break;
      case PROCESSOR_ARCHITECTURE_UNKNOWN:
         sArchitecture = "Architecture: Unknown ";
         sProcessorLevel.Format( _T("Level: Unknown %u "), m_sysInfo.wProcessorLevel);
         {
            int iModel = m_sysInfo.wProcessorRevision / 100;
            int iStepping = m_sysInfo.wProcessorRevision % 100;
            sStepping.Format( _T("Stepping: %u-%u "), iModel, iStepping);
         }
         break;
      default:
         sArchitecture.Format( _T("Architecture: Unknown %u "), m_sysInfo.wProcessorArchitecture);
         sProcessorLevel.Format( _T("Level: Unknown %u "), m_sysInfo.wProcessorLevel);
         {
            int iModel = m_sysInfo.wProcessorRevision / 100;
            int iStepping = m_sysInfo.wProcessorRevision % 100;
            sStepping.Format( _T("Stepping: %u-%u "), iModel, iStepping);
         }
         break;
      }

      sRC = sVendor + "," + sSpeed + "," + sType + "," + sProcessors + "," + sArchitecture + "," + sProcessorLevel + "," + sStepping;

      return sRC;
   }
};

class CMemoryInfo
{
protected:

public:

   CMemoryInfo(void)
   {
   }

   CStdString GetMemoryInfo(void)
   {
      CStdString sRC;

	   MEMORYSTATUS memoryStatus;

	   memset (&memoryStatus, sizeof (MEMORYSTATUS), 0);
	   memoryStatus.dwLength = sizeof (MEMORYSTATUS);
	   GlobalMemoryStatus (&memoryStatus);

      DWORD dwMinWSSize;
      DWORD dwMaxWSSize;

      ::GetProcessWorkingSetSize(GetCurrentProcess(), &dwMinWSSize, &dwMaxWSSize);

      sRC.Format( _T("Memory Used %lu%%, Total Physical Memory %luKB, Physical Memory Available %luKB, Total Virtual Memory %luKB, Available Virtual Memory %luKB, Working Set Min : %luKB Max : %luKB .\r\n"), memoryStatus.dwMemoryLoad, memoryStatus.dwTotalPhys / 1024, memoryStatus.dwAvailPhys / 1024, memoryStatus.dwTotalVirtual / 1024, memoryStatus.dwAvailVirtual / 1024, dwMinWSSize/1024, dwMaxWSSize/1024);

      return sRC;
   }

};

#endif

