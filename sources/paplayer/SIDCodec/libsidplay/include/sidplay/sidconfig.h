/***************************************************************************
                          sidconfig.h  -  Redirect to real sidconfig.h
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
 *  $Log: sidconfig.h,v $
 *  Revision 1.5  2001/12/07 00:40:22  s_a_white
 *  Windows fixes.
 *
 *  Revision 1.4  2001/12/05 23:37:39  s_a_white
 *  Now redirects to real sidfig.hy/i
 *
 ***************************************************************************/

#if defined(HAVE_UNIX)
#   include "../../unix/sidconfig.h"
#elif defined(HAVE_MSWINDOWS)
#   include "../../win/VC/sidconfig.h"
#else
#   error Platform not supported!
#endif
