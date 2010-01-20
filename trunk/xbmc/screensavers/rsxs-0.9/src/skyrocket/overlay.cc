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
#include <common.hh>

#include <overlay.hh>
#include <resource.hh>

namespace Overlay {
	unsigned int _lists;
	float _age;
	float _brightness;

	typedef std::list<unsigned int> OverlayList;
	OverlayList _overlayList;
};

void Overlay::init() {
	_lists = Common::resources->genLists(96);
	Font font = XLoadFont(Common::display,
		"-adobe-helvetica-bold-r-*-*-*-180-*-*-*-*-*-*");
	glXUseXFont(font, ' ', 96, _lists);
	XUnloadFont(Common::display, font);
}

void Overlay::set(const std::string& s) {
	_overlayList.clear();
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
		_overlayList.push_back(_lists + (*it & 0x7f) - ' ');
	_age = 0.0f;
}

void Overlay::update() {
	if (_overlayList.empty())
		return;

	_age += Common::elapsedSecs;
	if (_age < 2.0f)
		_brightness = 1.0f;
	else {
		_brightness = 1.0f - (_age - 2.0f) * 4.0f;
		if (_brightness <= 0.0f)
			_overlayList.clear();
	}
}

void Overlay::draw() {
	if (_overlayList.empty())
		return;

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			gluOrtho2D(0, Common::width, Common::height, 0);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glLoadIdentity();
				glColor4f(1.0f, 1.0f, 1.0f, _brightness);
				glRasterPos2i(20, Common::height - 20);
				std::for_each(
					_overlayList.begin(), _overlayList.end(),
					std::pointer_to_unary_function<unsigned int, void>(&glCallList)
				);
			glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glPopAttrib();
}
