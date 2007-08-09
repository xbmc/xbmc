/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
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
 * $Id: browser.h,v 1.1.1.1 2005/12/23 18:05:03 psperl Exp $
 *
 * Browser interface
 *
 */

#ifndef _BROWSER_H
#define _BROWSER_H

#include "event.h"

void browser_key_handler( projectMEvent event, projectMKeycode keycode, projectMModifier modifier );
int loadBrowser();
int closeBrowser();
void refreshBrowser();

#endif /** !_BROWSER_H */
