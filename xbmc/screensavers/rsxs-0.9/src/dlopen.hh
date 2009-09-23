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
#ifndef _DLOPEN_HH
#define _DLOPEN_HH

#include <common.hh>

#if USE_DLOPEN

#include <ltdl.h>

class Loader : public ResourceManager::Resource<void> {
public:
	typedef std::string Exception;
public:
	Loader() {
		if (lt_dlinit())
			throw Exception(lt_dlerror());
	}
	~Loader() { lt_dlexit(); }
	void operator()() const {}
};

class Library;
class Library : public ResourceManager::Resource<const Library*> {
public:
	typedef std::string Exception;
private:
	lt_dlhandle _handle;
public:
	Library(const std::string& library) {
		static bool inited = false;
		if (!inited) {
			Common::resources->manage(new Loader);
			inited = true;
		}
		_handle = lt_dlopenext(library.c_str());
		if (!_handle)
			throw Exception(lt_dlerror());
	}
	~Library() { lt_dlclose(_handle); }
	const Library* operator()() const { return this; }
	lt_ptr operator()(const std::string& function) const {
		lt_ptr ptr = lt_dlsym(_handle, function.c_str());
		if (!ptr)
			throw Exception(lt_dlerror());
		return ptr;
	}
};

#endif // USE_DLOPEN

#endif // _DLOPEN_HH
