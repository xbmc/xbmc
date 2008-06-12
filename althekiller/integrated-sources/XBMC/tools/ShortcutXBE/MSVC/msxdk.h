#include "xtl.h"

#ifdef __cplusplus
extern "C" {
#endif

  void XReboot();
  int  XGetTickCount();
  void XSleep(int milliseconds);
  void XLaunchXBE(const char* path);

  long XUnmount(const char* szDrive);
  long XMount(const char* szDrive, char* szDevice);

  void debugPrint(const char*, ...);

  typedef struct _ANSI_STRING
  {
    unsigned short Length;
    unsigned short MaximumLength;
    char* Buffer;
  } ANSI_STRING, *PANSI_STRING;

  extern PANSI_STRING XeImageFileName;

#ifdef __cplusplus
}
#endif