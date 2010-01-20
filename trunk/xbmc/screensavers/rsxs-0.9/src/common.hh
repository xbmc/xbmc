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
#ifndef _COMMON_HH
#define _COMMON_HH

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#if ! HAVE_CXX_BOOL
	enum _boolType { false, true };
	typedef enum _boolType bool;
#endif

#if ! HAVE_CXX_NEW_FOR_SCOPING
#	define for if (0) {} else for
#endif

#define GLX_GLXEXT_PROTOTYPES 1

#include <algorithm>
#include <argp.h>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#if HAVE_CXX_STRINGSTREAM
	#include <sstream>
#endif
#if HAVE_CXX_STRSTREAM
	#include <strstream>
#endif
#include <string>
#include <utility>
#include <vector>

#if HAVE_GL_GL_H
	#include <GL/gl.h>
#endif
#if HAVE_GL_GLEXT_H
	#include <GL/glext.h>
#endif
#if HAVE_GL_GLU_H
	#include <GL/glu.h>
#endif
#if HAVE_GL_GLX_H
	#include <GL/glx.h>
#endif
#if HAVE_X11_XLIB_H
	#include <X11/Xlib.h>
#endif

namespace std {
#if HAVE_CXX_STRSTREAM && !HAVE_CXX_STRINGSTREAM
	typedef istrstream istringstream;
	typedef ostrstream ostringstream;
	typedef strstream stringstream;
#endif
};

class ResourceManager;

namespace Common {
	typedef std::string Exception;

	extern std::string program;
	extern Display* display;
	extern XVisualInfo* visualInfo;
	extern unsigned int screen;
	extern Window window;
	extern GLXContext context;
	extern std::string resourceDir;

	extern unsigned int width, height, depth;
	extern unsigned int centerX, centerY;
	extern float aspectRatio;
	extern Colormap colormap;
	extern bool doubleBuffered;

	extern bool running;
	extern unsigned int elapsedMicros;
	extern float elapsedSecs;
	extern float speed;
	extern float elapsedTime;

	extern ResourceManager* resources;

#ifndef NOXBMC
         void init(int argc, char** argv);
         void run();
#endif

	static inline int randomInt(int x) {
		return std::rand() % x;
	}

	static inline float randomFloat(float x) {
		return float(std::rand()) * x / float(RAND_MAX);
	}

	static inline void flush(bool swap = doubleBuffered) {
		if (swap)
			glXSwapBuffers(display, window);
		else
			glFlush();
	}

	template <typename T>
	static inline const T& clamp(
		const T& v, const T& min, const T& max
	) {
		return std::max(std::min(v, max), min);
	}

	template <typename T>
	static inline bool parseArg(
		const char* arg, T& item, const T& min, const T& max
	) {
		return
			!(std::istringstream(arg) >> item) ||
			item < min || item > max;
	}

	template <typename T>
	static inline bool parseArg(
		const char* arg, T& item, const T& min, const T& max, const T& extra
	) {
		return
			!(std::istringstream(arg) >> item) ||
			(item != extra && (item < min || item > max));
	}
};

#ifdef NDEBUG
	#define TRACE(x) do {} while (0)
#else
	#define TRACE(x) do { \
		std::cerr << "[" << __PRETTY_FUNCTION__ << " @ " << __FILE__ << \
			":" << __LINE__ << "]" << std::endl << "  " << x << std::endl; \
	} while (0)
#endif

#define WARN(x) do { \
	std::cerr << Common::program << ": " << x << std::endl; \
} while (0)

namespace stdx {
	class oss {
	private:
		std::ostringstream _oss;
	public:
		operator std::string() const { return _oss.str(); }
		template <typename T>
		oss& operator<<(const T& t) {
			_oss << t;
			return *this;
		}
	};

	template <typename In, typename MFunc>
	static inline void call_each(In p, In q, MFunc f) {
		for ( ; p != q; ++p)
			((*p).*f)();
	}

	template <typename In, typename MFunc, typename Arg>
	static inline void call_each(In p, In q, MFunc f, const Arg& a) {
		for ( ; p != q; ++p)
			((*p).*f)(a);
	}

	template <typename Con, typename MFunc>
	static inline void call_all(Con& c, MFunc f) {
		call_each(c.begin(), c.end(), f);
	}

	template <typename Con, typename MFunc, typename Arg>
	static inline void call_all(Con& c, MFunc f, const Arg& a) {
		call_each(c.begin(), c.end(), f, a);
	}

	template <typename In, typename Out, typename UFunc>
	static inline void map_each(In p, In q, Out o, UFunc f) {
		for ( ; p != q; ++p)
			*o++ = ((*p).*f)();
	}

	template <typename Con1, typename Con2, typename UFunc>
	static inline void map_all(const Con1& in, Con2& out, UFunc f) {
		map_each(in.begin(), in.end(), out.begin(), f);
	}

	template <typename In, typename MFuncPtr>
	static inline void call_each_ptr(In p, In q, MFuncPtr f) {
		for ( ; p != q; ++p)
			((*p)->*f)();
	}

	template <typename In, typename MFuncPtr, typename Arg>
	static inline void call_each_ptr(
		In p, In q, MFuncPtr f, const Arg& a
	) {
		for ( ; p != q; ++p)
			((*p)->*f)(a);
	}

	template <typename Con, typename MFuncPtr>
	static inline void call_all_ptr(Con& c, MFuncPtr f) {
		call_each_ptr(c.begin(), c.end(), f);
	}

	template <typename Con, typename MFuncPtr, typename Arg>
	static inline void call_all_ptr(Con& c, MFuncPtr f, const Arg& a) {
		call_each_ptr(c.begin(), c.end(), f, a);
	}

	template <typename In, typename Out, typename UFunc>
	static inline void map_each_ptr(In p, In q, Out o, UFunc f) {
		for ( ; p != q; ++p)
			*o++ = ((*p)->*f)();
	}

	template <typename Con1, typename Con2, typename UFunc>
	static inline void map_all_ptr(const Con1& in, Con2& out, UFunc f) {
		map_each_ptr(in.begin(), in.end(), out.begin(), f);
	}

	template <typename In>
	static inline void destroy_each_ptr(In p, const In& q) {
		for ( ; p != q; ++p)
			delete *p;
	}

	template <typename Con>
	static inline void destroy_all_ptr(Con& c) {
		destroy_each_ptr(c.begin(), c.end());
	}

	template <typename T>
	class constructor_t {
	public:
		inline T operator()() { return T(); }
	};

	template <typename T, typename Arg>
	class constructor1_t {
	private:
		const Arg& _x;
	public:
		constructor1_t(const Arg& x) : _x(x) {}
		inline T operator()() { return T(_x); }
	};

	template <typename T>
	static inline constructor_t<T> construct() {
		return constructor_t<T>();
	}

	template <typename T, typename Arg>
	static inline constructor1_t<T, Arg> construct(const Arg& a) {
		return constructor1_t<T, Arg>(a);
	}

	template <typename Con>
	static inline void construct_n(Con& c, typename Con::size_type n) {
		c.reserve(c.size() + n);
		while (n--) c.push_back(typename Con::value_type());
	}

	template <typename Con, typename Arg>
	static inline void construct_n(
		Con& c, typename Con::size_type n, const Arg& a
	) {
		c.reserve(c.size() + n);
		while (n--) c.push_back(typename Con::value_type(a));
	}

	template <typename T, typename std::vector<int>::size_type I = 0>
	class dim2 : public std::vector<T> {
	public:
		typedef typename std::vector<T>::reference reference;
		typedef typename std::vector<T>::const_reference const_reference;
		typedef typename std::vector<T>::size_type size_type;
	private:
		size_type _i;
	public:
		dim2(size_type i = 0, size_type j = 0) : std::vector<T>() {
			resize(i, j);
		}

		void resize(size_type i = 0, size_type j = 0) {
			if (I)
				std::vector<T>::resize(I * i);
			else {
				_i = i;
				std::vector<T>::resize(i * j);
			}
		}

		reference
		operator()(size_type i, size_type j) {
			return (*this)[i * (I ? I : _i) + j];
		}
		const_reference
		operator()(size_type i, size_type j) const {
			return (*this)[i * (I ? I : _i) + j];
		}
	};

	template <
		typename T,
		typename std::vector<int>::size_type J = 0,
		typename std::vector<int>::size_type I = 0
	> class dim3 : public std::vector<T> {
	public:
		typedef typename std::vector<T>::reference reference;
		typedef typename std::vector<T>::const_reference const_reference;
		typedef typename std::vector<T>::size_type size_type;
	private:
		size_type _j, _i;
	public:
		dim3(size_type i = 0, size_type j = 0, size_type k = 0)
			: std::vector<T>()
		{
			resize(i, j, k);
		}

		void resize(size_type i = 0, size_type j = 0, size_type k = 0) {
			if (I && J)
				std::vector<T>::resize(I * J * i);
			else if (J) {
				_i = i;
				std::vector<T>::resize(i * J * j);
			} else {
				_i = i;
				_j = j;
				std::vector<T>::resize(i * j * k);
			}
		}

		reference
		operator()(size_type i, size_type j, size_type k) {
			return (*this)[(i * (I ? I : _i) + j) * (J ? J : _j) + k];
		}
		const_reference
		operator()(size_type i, size_type j, size_type k) const {
			return (*this)[(i * (I ? I : _i) + j) * (J ? J : _j) + k];
		}
	};
};

#include <resource.hh>

#endif // _COMMON_HH
