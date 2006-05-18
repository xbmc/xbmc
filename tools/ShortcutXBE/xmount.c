#include "xmount.h"

#include <xboxkrnl/xboxkrnl.h>
#include <xboxkrnl/types.h>
#define MAX_PATH 256

#include <string.h>
#include <stdio.h>

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
