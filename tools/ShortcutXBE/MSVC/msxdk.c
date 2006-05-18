#include "xtl.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "msxdk.h"

/* XDK starting point, map to openxdk starting point */
extern void XBoxStartup();
int main(int argc,char* argv[])
{
  XBoxStartup();
}

typedef enum _RETURN_FIRMWARE
{
	ReturnFirmwareHalt          = 0x00,
	ReturnFirmwareReboot        = 0x01,
	ReturnFirmwareQuickReboot   = 0x02,
	ReturnFirmwareHard          = 0x03,
	ReturnFirmwareFatal         = 0x04,
	ReturnFirmwareAll           = 0x05
}
RETURN_FIRMWARE, *LPRETURN_FIRMWARE;

typedef unsigned long DWORD;
typedef unsigned char UCHAR;

// ******************************************************************
// * LAUNCH_DATA_HEADER
// ******************************************************************
typedef struct _LAUNCH_DATA_HEADER
{
	DWORD   dwLaunchDataType;
	DWORD   dwTitleId;
	char    szLaunchPath[520];
	DWORD   dwFlags;
}
LAUNCH_DATA_HEADER, *PLAUNCH_DATA_HEADER;

// ******************************************************************
// * LAUNCH_DATA_PAGE
// ******************************************************************
typedef struct _LAUNCH_DATA_PAGE
{
	LAUNCH_DATA_HEADER  Header;
	UCHAR               Pad[492];
	UCHAR               LaunchData[3072];
}
LAUNCH_DATA_PAGE, *PLAUNCH_DATA_PAGE;

/* oddly enough, this is a pointer to a pointer */
extern PLAUNCH_DATA_PAGE *LaunchDataPage;

long __stdcall IoCreateSymbolicLink(PANSI_STRING SymbolicLinkName, PANSI_STRING DeviceName);
long __stdcall IoDeleteSymbolicLink(PANSI_STRING SymbolicLinkName);
void __stdcall HalWriteSMBusValue(BYTE, BYTE, BOOL, BYTE);
void __stdcall MmPersistContiguousMemory(PVOID BaseAddress, ULONG NumberOfBytes,BOOLEAN Persist);
void* __stdcall MmAllocateContiguousMemory(ULONG NumberOfBytes);
void __stdcall HalReturnToFirmware(RETURN_FIRMWARE Routine);

#define STATUS_SUCCESS 0x00000000

typedef struct _DRIVEMAPPING
{
  char szDriveLetter;
  char* szDevice;
  int iPartition;
} DRIVEMAPPING;

DRIVEMAPPING g_driveMapping[] =
  {
    { 'C', "Harddisk0\\Partition2", 2},
    { 'D', "Cdrom0", -1},
    { 'E', "Harddisk0\\Partition1", 1},
    { 'F', "Harddisk0\\Partition6", 6},
    { 'X', "Harddisk0\\Partition3", 3},
    { 'Y', "Harddisk0\\Partition4", 4},
    { 'Z', "Harddisk0\\Partition5", 5},
    { 'G', "Harddisk0\\Partition7", 7},
  };

#define NUM_OF_DRIVES ( sizeof( g_driveMapping) / sizeof( g_driveMapping[0] ) )

int  XGetTickCount() { return GetTickCount(); }
void XSleep(int milliseconds) { Sleep(milliseconds); }

long XUnmount(const char* szDrive)
{
  char szDestinationDrive[16];
  ANSI_STRING LinkName;

  sprintf(szDestinationDrive, "\\??\\%s", szDrive);
  LinkName.Length = strlen(szDestinationDrive);
  LinkName.MaximumLength = sizeof(szDestinationDrive);
  LinkName.Buffer = szDestinationDrive;

  

  return( IoDeleteSymbolicLink(&LinkName) == STATUS_SUCCESS );
}


long XMount(const char* szDrive, char* szDevice)
{
  char szSourceDevice[MAX_PATH];
  char szDestinationDrive[16];
  ANSI_STRING DeviceName, LinkName;

  sprintf(szSourceDevice, "%s", szDevice);
  sprintf(szDestinationDrive, "\\??\\%s", szDrive);

  DeviceName.Length = strlen(szSourceDevice);
  DeviceName.MaximumLength = sizeof(szSourceDevice);
  DeviceName.Buffer = szSourceDevice;

  LinkName.Length = strlen(szDestinationDrive);
  LinkName.MaximumLength = sizeof(szDestinationDrive);
  LinkName.Buffer = szDestinationDrive;

  return( IoCreateSymbolicLink(&LinkName, &DeviceName) == STATUS_SUCCESS );
}


void XLaunchXBE(const char* fullpath)
{
  char* xbename;
  char devicepath[MAX_PATH];
  char launchpath[MAX_PATH];
  char* lastslash;

  OutputDebugString(__FUNCTION__" - Path: ");
  OutputDebugString(fullpath);
  OutputDebugString("\n");

  xbename = strrchr(fullpath, '\\');
  if( xbename == NULL )
  {
    OutputDebugString(__FUNCTION__" - Unable to find xbename\n");
    return;
  }

  xbename++;

  if( fullpath[1] == ':' )
  {
    char drive = toupper(fullpath[0]);
    int i;
    
    /* lookup partition path for given drive */
    for(i = 0; i < NUM_OF_DRIVES; i++)
    {
      if( drive == g_driveMapping[i].szDriveLetter )
      {
        strcpy(devicepath, "\\Device\\");
        strcat(devicepath, g_driveMapping[i].szDevice);
        break;
      }
    }
    /* add the rest of the path */
    strcat(devicepath, fullpath+2);
  }
  else
    strcpy(devicepath, fullpath);

  OutputDebugString(__FUNCTION__" - Device Path: ");
  OutputDebugString(devicepath);
  OutputDebugString("\n");

  /* find the xbe name */
  lastslash = strrchr(devicepath, '\\');
  if( lastslash == NULL )
  {
    OutputDebugString(__FUNCTION__" - Unable to find xbe name\n");
    return;
  }
  
  /* get rid of xbe name from devicepath */
  *lastslash = '\0';

  /* mount this path to drive D: */
  XUnmount("D:");
  if( !XMount("D:", devicepath ) )
  {
    OutputDebugString(__FUNCTION__" - Unable to mount drive\n");
    return;
  }
  
  strcpy(launchpath, "D:\\");
  strcat(launchpath, xbename);

  { /* check so file exists*/
    struct stat buffer;
    if( stat(launchpath, &buffer) != 0 )
    {
      OutputDebugString(__FUNCTION__" - File not found\n");
      return;
    }
  }

  /* sometimes xdk doesn't allocate this area properly */
  /* let's do it manually, and launch manually so we can keep any extra parameters given */

  if ( (*LaunchDataPage) == NULL)
  {
    (*LaunchDataPage) = MmAllocateContiguousMemory(0x1000);
    MmPersistContiguousMemory((*LaunchDataPage), 0x1000, TRUE);
    memset((void*)(*LaunchDataPage), 0, 0x1000);

    /* set some launch info */
    (*LaunchDataPage)->Header.dwLaunchDataType = LDT_FROM_DASHBOARD;
    (*LaunchDataPage)->Header.dwTitleId = 0;
  }	

  /* these flags seem to tell the debug bios to return to debug dash */
  /* reseting these for now, untill we know of anything needing to have that set*/
  (*LaunchDataPage)->Header.dwFlags = 0;

  strcpy((*LaunchDataPage)->Header.szLaunchPath, devicepath);
  strcat((*LaunchDataPage)->Header.szLaunchPath, ";");
  strcat((*LaunchDataPage)->Header.szLaunchPath, xbename);

  /* now tell bios to startup */
  HalReturnToFirmware(ReturnFirmwareQuickReboot);

  /* crap function returned, something failed */
  OutputDebugString(__FUNCTION__" - Failed");
}

void XReboot()
{
  HalReturnToFirmware(ReturnFirmwareHard);  
}


void debugPrint(const char* format, ...)
{
  char buffer[1024];

  va_list va;
  va_start(va, format);
  vsprintf(buffer, format, va);
  va_end(va);

  OutputDebugString(buffer);
}
