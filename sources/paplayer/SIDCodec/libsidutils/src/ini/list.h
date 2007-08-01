/***************************************************************************
                          list.h - Adds list support to ini files

                             -------------------
    begin                : Fri Apr 21 2000
    copyright            : (C) 2000 by Simon White
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

#ifndef _list_h_
#define _list_h_

#include "ini.h"
#ifdef INI_ADD_LIST_SUPPORT

struct ini_t;
static int   __ini_listEval        (struct ini_t *ini);
static char *__ini_listRead        (struct ini_t *ini);
static int   __ini_listIndexLength (struct ini_t *ini);

#endif // INI_ADD_LIST_SUPPORT
#endif // _list_h_
