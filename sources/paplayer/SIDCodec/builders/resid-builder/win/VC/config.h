/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _config_h_
#define _config_h_

/* Define if your C++ compiler implements exception-handling.  */
/* #define HAVE_EXCEPTIONS */

/* Name of package */
#define PACKAGE "resid-builder"

/* Version number of package */
#define VERSION "0.0.2"

/* Defines to indicate that resid is in an non standard location.
   SID_HAVE_LOCAL_RESID indicates that an include path has been used
   to where the resid directory is located but does not include
   the resid directory name.
   SID_HAVE_USER_RESID indicates that a fully qualified include path
   has been provided that includes the resid directory name and therefore
   resids include files are available by calling them directly.
*/
#define HAVE_LOCAL_RESID

#endif // _config_h_
