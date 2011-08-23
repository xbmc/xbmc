///////////////////////////////////////////////////////////////////////
// targetver.h is used to define the Windows API macros that target the 
// version of the Windows operating system you wish to support.


// For Windows 95
//#define WINVER          0x0400
//#define _WIN32_WINDOWS  0x0400
//#define _WIN32_IE       0x0300

// For Windows 98
#define WINVER          0x0410
#define _WIN32_WINDOWS  0x0410
#define _WIN32_IE       0x0401

// For Windows NT4
//#define WINVER          0x0400
//#define _WIN32_WINNT    0x0400
//#define _WIN32_IE       0x0200
//#define NTDDI_VERSION   0x05000000

// For Windows ME
//#define WINVER          0x0500
//#define _WIN32_WINNT    0x0500
//#define _WIN32_IE       0x0500

// For Windows 2000
//#define WINVER          0x0500
//#define _WIN32_WINNT    0x0500
//#define _WIN32_IE       0x0500
//#define NTDDI_VERSION   0x05000000

// For Windows XP
//#define WINVER          0x0501
//#define _WIN32_WINNT    0x0501
//#define _WIN32_IE       0x0501
//#define NTDDI_VERSION   0x05010000

// For Windows Vista
//#define WINVER          0x0600
//#define _WIN32_WINNT    0x0600
//#define _WIN32_IE       0x0600
//#define NTDDI_VERSION   0x06000000

// For Windows 7
//#define WINVER          0x0601
//#define _WIN32_WINNT    0x0601
//#define _WIN32_IE       0x0601
//#define NTDDI_VERSION   0x06010000


// Users of Visual Studio 10 can do this instead
// #include "SDKDDKver.h"
