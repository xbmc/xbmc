/***************************************************************************
                          config.h  -  Redirect to real config.h
                             -------------------
    begin                : Tues Dec 4 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: config.h,v $
 *  Revision 1.6  2001/12/07 00:40:56  s_a_white
 *  Windows fixes.
 *
 *  Revision 1.5  2001/12/06 18:29:33  s_a_white
 *  Now redirects to real config.h
 *
 ***************************************************************************/

#if defined(HAVE_UNIX)
#   include "../unix/config.h"
#elif defined(HAVE_MSWINDOWS)
#   include "../win/VC/config.h"
#else
#   error Platform not supported!
#endif
