/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: console_interface.h,v 1.1.1.1 2005/12/23 18:05:03 psperl Exp $
 *
 * $Log$
 */

#ifndef _KEY_HANDLER_HPP
#define _KEY_HANDLER_HPP

#include "event.h"
class projectM;
void default_key_handler(projectM *PM, projectMEvent event, projectMKeycode keycode);
void refreshConsole();
#if defined(__CPLUSPLUS) && !defined(MACOS)
extern "C" void key_handler(projectM *PM, projectMEvent event, projectMKeycode keycode, projectMModifier modifier );
#else
extern void key_handler(projectM *PM, projectMEvent event, projectMKeycode keycode, projectMModifier modifier );
#endif
#endif /** !_KEY_HANDLER_HPP */
