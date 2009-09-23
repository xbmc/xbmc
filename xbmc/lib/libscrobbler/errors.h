/*
    This file is part of libscrobbler.

    libscrobbler is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libscrobbler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libscrobbler; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Copyright © 2003 Russell Garrett (russ-scrobbler@garrett.co.uk)
*/
#ifndef _SCROBBLER_ERRORS_H
#define _SCROBBLER_ERRORS_H

class EScrobbler {
public:
	EScrobbler(){};
	~EScrobbler(){};

	const char *getText() const { return "Unknown Error"; }
};

class EOutOfMemory : public EScrobbler {
public:
	const char *getText() const { return "Out of Memory"; }
};

#endif

