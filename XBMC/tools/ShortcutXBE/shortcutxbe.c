
#ifdef _MSC_VER

#include "MSVC/msxdk.h"

#else

#include <hal/xbox.h>
#include <hal/fileio.h>
#include <openxdk/debug.h>
#include <xboxkrnl/xboxkrnl.h>
#include <xboxkrnl/types.h>

#define MAX_PATH 256
#include "xmount.h"

#endif

#include <string.h>
#include <stdio.h>

static FILE* logfile = NULL;

void initlog()
{
  char devicepath[MAX_PATH];
  char filename[MAX_PATH];
  char *temp;
  #ifdef _MSC_VER
  PANSI_STRING imageFileName = (PANSI_STRING)XeImageFileName;
  #else
  PANSI_STRING imageFileName = (PANSI_STRING)&XeImageFileName;
  #endif
  
  strncpy(devicepath, imageFileName->Buffer, imageFileName->Length);
  devicepath[imageFileName->Length] = '\0';

  temp = strrchr(devicepath, '\\');
  if( temp == NULL ) return;

  strcpy(filename, "D:");
  strcat(filename, temp);

  /* let the device path only include path and not filename */
  *temp = '\0';

  /* extension */
  temp = strstr(filename, ".xbe");
  if( temp == NULL ) return;
  
  /* switch extension */
  temp[1] = 'l';
  temp[2] = 'o';
  temp[3] = 'g';

  /* mount up a drive to use for debug logging */
  XUnmount("D:");
  XMount("D:", devicepath);

  logfile = fopen(filename, "w+t");
}

void debuglog(const char* format, ...)
{
  char buffer[1024];

  va_list va;
  va_start(va, format);
  vsprintf(buffer, format, va);
  va_end(va);

  strcat(buffer, "\n");

  if( logfile )
  {
    fputs(buffer, logfile);
    fflush(logfile);
  }

#ifdef _MSC_VER
  OutputDebugString(buffer);
#endif
}


void ErrorHandler(char *xbepath)
{
    /* attempting to find something to launch wich hopefully have ftp */
    if (xbepath && 0 != strcmpi("C:\\avalaunch.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (c:\\avalaunch.xbe)");
      XLaunchXBE("C:\\avalaunch.xbe");
    }
    if (xbepath && 0 != strcmpi("C:\\unleashx.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\unleashx.xbe)");
      XLaunchXBE("C:\\unleashx.xbe");
    }
    if (xbepath && 0 != strcmpi("C:\\xbmc.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\xbmc.xbe)");
      XLaunchXBE("C:\\xbmc.xbe");
    }
    
    /* evox fails on debug bios oddly enough */
    if (xbepath && 0 != strcmpi("C:\\evoxdash.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\evoxdash.xbe)");
      XLaunchXBE("C:\\evoxdash.xbe");
    }
 
    /* nothing found, try to launch standard dash */
    if (xbepath && 0 != strcmpi("C:\\xboxdash.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\xboxdash.xbe)");
      XLaunchXBE("C:\\xboxdash.xbe");
    }
    if (xbepath && 0 != strcmpi("C:\\default.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\default.xbe)");
      XLaunchXBE("C:\\default.xbe");
    }
    if (xbepath && 0 != strcmpi("C:\\msdash.xbe", xbepath))
    {
      debuglog("Attempting to launch backup FTP server (C:\\msdash.xbe)");
      XLaunchXBE("C:\\msdash.xbe");
    }
    debuglog("All failed :( - trying to reboot");
    XReboot();
}

int LaunchShortcut(char* filename)
{
  FILE* file;
  char target[MAX_PATH];
  unsigned int length;

  /* attempt to open file */
  debuglog("INFO - openening cutfile: %s", filename);
  
  
  file = fopen(filename, "r+t");

  if(file == NULL)
  {
    debuglog("ERROR - Unable to open cutfile");
    return -1;
  }

  /* read first line untill linebreak */
  length = 0;
  while(1)
  {
    target[length] = fgetc(file);
    if( target[length] == EOF || target[length] == '\n' || target[length] == '\r' )
      break;
    length++;
  }
  fclose(file);

  if(length <= 0)
  {
    debuglog("ERROR - Unable to read cutfile");    
    return -1;
  }

  /* null terminate string, and chop of any trailing blanks*/
  target[length] = '\0';

  /* launching xbe */
  debuglog("INFO - Launching xbe: %s", target);
  XLaunchXBE(target);

  /* if we get here something went wrong */
  return -1;
  
}

/* initial starting point of program */
void XBoxStartup()
{
  char devicepath[MAX_PATH];
  char xbepath[MAX_PATH];
  char shortcut[MAX_PATH];
  char *temp;
  #ifdef _MSC_VER
  PANSI_STRING imageFileName = (PANSI_STRING)XeImageFileName;
  #else
  PANSI_STRING imageFileName = (PANSI_STRING)&XeImageFileName;
  #endif

  initlog();

  strncpy(devicepath, imageFileName->Buffer, imageFileName->Length);
  devicepath[imageFileName->Length] = '\0';

  temp = strrchr(devicepath, '\\');
  if( temp == NULL )
  {    
    debuglog("ERROR - Can't find launching xbe");
    ErrorHandler(NULL);
  }

  /* move to xbepath buffer */
  strcpy(xbepath, "D:");
  strcat(xbepath, temp);

  /* setup the shortcut path */
  strcpy(shortcut, xbepath);
  
  /* let the device path only include path and not filename */
  *temp = '\0';

  /* extension */
  temp = strstr(shortcut, ".xbe");
  if( temp == NULL )
  {
    debuglog("ERROR - Can't find launching xbe's extension part");
    ErrorHandler(NULL);
  }
  
  /* switch extension */
  temp[1] = 'c';
  temp[2] = 'f';
  temp[3] = 'g';

  debuglog("INFO - Mounting D: to %s", devicepath);

  /* make sure D: is mounted to the launch location */
  XUnmount("D:");
  XMount("D:", devicepath);

  /* now try to launch our cutfile */
  debuglog("INFO - launching cutfile: %s", shortcut);
  LaunchShortcut(shortcut);

  /* if launchshortcut returns, something went wrong */  
  debuglog("ERROR - Can't launch cutfile");
  ErrorHandler(xbepath);
}
