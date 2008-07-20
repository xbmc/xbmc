/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2002 Terence M. Welsh, available from www.reallyslick.com
 */
#ifndef _EUPHORIA_HH
#define _EUPHORIA_HH

#include <common.hh>

namespace Hack {
	extern unsigned int numWisps;
	extern unsigned int numBackWisps;
	extern unsigned int density;
	extern float visibility;
	extern float speed;
	extern float feedback;
	extern float feedbackSpeed;
	extern unsigned int feedbackSize;
	extern std::string texture;
	extern bool wireframe;
};

#endif // _EUPHORIA_HH
