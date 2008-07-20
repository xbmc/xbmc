/*
 * mime.c -- Web server mime types
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: mime.c,v 1.7 2003/03/17 20:12:33 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Mime types and file extensions. This module maps URL extensions to
 *	content types.
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"

/******************************** Global Data *********************************/
/*
 *	Addd entries to the MimeList as required for your content
 */


websMimeType websMimeList[] = {
	{ T("application/java"), T(".class") },
	{ T("application/java"), T(".jar") },
	{ T("text/html"), T(".asp") },
	{ T("text/html"), T(".htm") },
	{ T("text/html"), T(".html") },
	{ T("text/xml"), T(".xml") },
	{ T("image/gif"), T(".gif") },
	{ T("image/jpeg"), T(".jpg") },
	{ T("text/css"), T(".css") },
	{ T("text/plain"), T(".txt") },
   { T("application/x-javascript"), T(".js") },
   { T("application/x-shockwave-flash"), T(".swf") },

#ifdef MORE_MIME_TYPES
	{ T("application/binary"), T(".exe") },
	{ T("application/compress"), T(".z") },
	{ T("application/gzip"), T(".gz") },
	{ T("application/octet-stream"), T(".bin") },
	{ T("application/oda"), T(".oda") },
	{ T("application/pdf"), T(".pdf") },
	{ T("application/postscript"), T(".ai") },
	{ T("application/postscript"), T(".eps") },
	{ T("application/postscript"), T(".ps") },
	{ T("application/rtf"), T(".rtf") },
	{ T("application/x-bcpio"), T(".bcpio") },
	{ T("application/x-cpio"), T(".cpio") },
	{ T("application/x-csh"), T(".csh") },
	{ T("application/x-dvi"), T(".dvi") },
	{ T("application/x-gtar"), T(".gtar") },
	{ T("application/x-hdf"), T(".hdf") },
	{ T("application/x-latex"), T(".latex") },
	{ T("application/x-mif"), T(".mif") },
	{ T("application/x-netcdf"), T(".nc") },
	{ T("application/x-netcdf"), T(".cdf") },
	{ T("application/x-ns-proxy-autoconfig"), T(".pac") },
	{ T("application/x-patch"), T(".patch") },
	{ T("application/x-sh"), T(".sh") },
	{ T("application/x-shar"), T(".shar") },
	{ T("application/x-sv4cpio"), T(".sv4cpio") },
	{ T("application/x-sv4crc"), T(".sv4crc") },
	{ T("application/x-tar"), T(".tar") },
	{ T("application/x-tcl"), T(".tcl") },
	{ T("application/x-tex"), T(".tex") },
	{ T("application/x-texinfo"), T(".texinfo") },
	{ T("application/x-texinfo"), T(".texi") },
	{ T("application/x-troff"), T(".t") },
	{ T("application/x-troff"), T(".tr") },
	{ T("application/x-troff"), T(".roff") },
	{ T("application/x-troff-man"), T(".man") },
	{ T("application/x-troff-me"), T(".me") },
	{ T("application/x-troff-ms"), T(".ms") },
	{ T("application/x-ustar"), T(".ustar") },
	{ T("application/x-wais-source"), T(".src") },
	{ T("application/zip"), T(".zip") },
	{ T("audio/basic"), T(".au snd") },
	{ T("audio/x-aiff"), T(".aif") },
	{ T("audio/x-aiff"), T(".aiff") },
	{ T("audio/x-aiff"), T(".aifc") },
	{ T("audio/x-wav"), T(".wav") },
	{ T("audio/x-wav"), T(".ram") },
	{ T("image/ief"), T(".ief") },
	{ T("image/jpeg"), T(".jpeg") },
	{ T("image/jpeg"), T(".jpe") },
	{ T("image/tiff"), T(".tiff") },
	{ T("image/tiff"), T(".tif") },
	{ T("image/x-cmu-raster"), T(".ras") },
	{ T("image/x-portable-anymap"), T(".pnm") },
	{ T("image/x-portable-bitmap"), T(".pbm") },
	{ T("image/x-portable-graymap"), T(".pgm") },
	{ T("image/x-portable-pixmap"), T(".ppm") },
	{ T("image/x-rgb"), T(".rgb") },
	{ T("image/x-xbitmap"), T(".xbm") },
	{ T("image/x-xpixmap"), T(".xpm") },
	{ T("image/x-xwindowdump"), T(".xwd") },
	{ T("text/html"), T(".cfm") },
	{ T("text/html"), T(".shtm") },
	{ T("text/html"), T(".shtml") },
	{ T("text/richtext"), T(".rtx") },
	{ T("text/tab-separated-values"), T(".tsv") },
	{ T("text/x-setext"), T(".etx") },
	{ T("video/mpeg"), T(".mpeg mpg mpe") },
	{ T("video/quicktime"), T(".qt") },
	{ T("video/quicktime"), T(".mov") },
	{ T("video/x-msvideo"), T(".avi") },
	{ T("video/x-sgi-movie"), T(".movie") },
#endif
	{ NULL, NULL},
};

/*****************************************************************************/

