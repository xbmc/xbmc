/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _config_h_
#define _config_h_

#define PACKAGE "libsidplay"
#define VERSION "2.1.1"

#define S_A_WHITE_EMAIL "sidplay2@yahoo.com"

/* Define if your C++ compiler implements exception-handling.  */
/* Note: exception specification is only available for MSVC > 6 */
#if _MSC_VER > 1200
//#   define HAVE_EXCEPTIONS
#endif

/* Define if you support file names longer than 14 characters.  */
#define HAVE_LONG_FILE_NAMES

/* Define if you have the <sstream> header file.  */
#define HAVE_SSTREAM

/* Define if ``ios::nocreate'' is supported. */
//#define HAVE_IOS_NOCREATE

#define HAVE_USER_RESID

#endif // _config_h_
