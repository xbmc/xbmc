/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _config_h_
#define _config_h_

/* Define the sidbuilder modules at appropriate */
#define HAVE_RESID_BUILDER 
//#define HAVE_HARDSID_BUILDER

/* Define if your C++ compiler implements exception-handling.  */
#define HAVE_EXCEPTIONS 1

/* Define if standard member ``ios::binary'' is called ``ios::bin''. */
/* #define HAVE_IOS_BIN 1 */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */ 
/* #define WORDS_BIGENDIAN */

/* Define sound driver */
#define HAVE_DIRECTX
#define HAVE_MMSYSTEM

/* Name of package */
#define PACKAGE "sidplay"

/* Version number of package */
#define VERSION "2.0.8"

#endif // _config_h_
