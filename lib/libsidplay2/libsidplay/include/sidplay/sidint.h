/***************************************************************************
                          sidint.h  -  redirect to os dependent sidint
                             -------------------
    begin                : Wed Dec 26 2001
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

#if defined(HAVE_UNIX)
#   include "../../unix/sidint.h"
#elif defined(HAVE_MSWINDOWS)
#   include "../../win/VC/sidint.h"
#else
#   error Platform not supported!
#endif
