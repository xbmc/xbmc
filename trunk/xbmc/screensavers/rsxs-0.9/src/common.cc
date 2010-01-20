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

#if HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#if HAVE_SYS_SELECT_H
	#include <sys/select.h>
#endif
#include <sys/time.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <hack.hh>
#include <resource.hh>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#if defined(__vms)
#include <X11/StdCmap.h>  /* for XmuLookupStandardColormap */
#else
#include <X11/Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#endif
#include <vroot.hh>
#include <ctime>

#define MAX_DELAY 10000
#define MIN_DELAY  1000

namespace Common {
	std::string program;
	Display* display;
	unsigned int screen;
	XVisualInfo *visualInfo;
	Window window;
	GLXContext context;
	std::string resourceDir;

	unsigned int width, height, depth;
	unsigned int centerX, centerY;
	float aspectRatio;
	Colormap colormap;
	bool doubleBuffered;

	bool running;
	unsigned int elapsedMicros;
	float elapsedSecs;
	float speed;
	float elapsedTime;

	ResourceManager* resources;

	error_t parse(int, char*, struct argp_state*);
	void init(int argc, char** argv);
	void run();
	void fini();
};

namespace Common {
	char* _displayName;
	Window _windowID;
	int _x, _y, _w, _h;
	bool _reverseX, _reverseY;
	bool _useOffset;
	bool _fullScreen;
	bool _onRoot;
#ifndef NDEBUG
	bool _showFPS;
#endif // !NDEBUG

	enum Arguments {
		ARG_ROOT = 1,
#ifndef NDEBUG
		ARG_FPS,
#endif // !NDEBUG
		ARG_GEOMETRY,
		ARG_FULLSCREEN,
		ARG_WINDOWID,
		ARG_RESOURCE_DIR,
	};

	struct HasCurrentVisualID;

	Colormap getColormap();
	Window createWindow(int, char**);
	void updateAttributes();
	void dumpErrors(const std::string&);
};

error_t Common::parse(int key, char* arg, struct argp_state* state) {
	switch (key) {
	case ARGP_KEY_INIT:
		visualInfo = NULL;
		window = None;
		context = None;
		running = false;
		resourceDir = std::string(PKGDATADIR "/") + Hack::getShortName();
		_displayName = NULL;
		_onRoot = false;
		_windowID = 0;
		_x = _y = 0;
		_reverseX = _reverseY = false;
		_w = 640;
		_h = 480;
		_useOffset = _fullScreen = false;
		return 0;
	case ARG_ROOT:
		_onRoot = true;
		return 0;
	case ARG_GEOMETRY:
		if (std::sscanf(arg, "%dx%d+%d+%d", &_w, &_h, &_x, &_y) == 4)
			_useOffset = true;
		else if (std::sscanf(arg, "%dx%d-%d+%d", &_w, &_h, &_x, &_y) == 4) {
			_useOffset = true; _reverseX = true;
		} else if (std::sscanf(arg, "%dx%d+%d-%d", &_w, &_h, &_x, &_y) == 4) {
			_useOffset = true; _reverseY = true;
		} else if (std::sscanf(arg, "%dx%d-%d-%d", &_w, &_h, &_x, &_y) == 4) {
			_useOffset = true; _reverseX = true; _reverseY = true;
		} else if (std::sscanf(arg, "%dx%d", &_w, &_h) == 2)
			;
		else if (std::sscanf(arg, "%d%d", &_x, &_y) == 2)
			_useOffset = true;
		else {
			argp_error(state, "could not parse geometry `%s'", arg);
			return ARGP_ERR_UNKNOWN;
		}
		return 0;
	case ARG_FULLSCREEN:
		_fullScreen = true;
		return 0;
	case ARG_WINDOWID:
		if ((_windowID = std::strtol(arg, NULL, 0)) == 0) {
			argp_error(state, "invalid window ID `%s'", arg);
			return ARGP_ERR_UNKNOWN;
		}
		return 0;
	case ARG_RESOURCE_DIR:
		resourceDir = arg;
		return 0;
#ifndef NDEBUG
	case ARG_FPS:
		_showFPS = true;
		return 0;
#endif // !NDEBUG
	default:
		return ARGP_ERR_UNKNOWN;
	}
}

static struct timeval now;
static struct timeval then;

void Common::init(int argc, char** argv) {
#ifdef NOXBMC
	display = XOpenDisplay(_displayName);
	if (!display) {
		if (_displayName != "")
			throw Exception(stdx::oss() << "Could not open display " << _displayName);
		else
			throw Exception("Could not open default display (DISPLAY variable not set?)");
	}
	screen = DefaultScreen(display);
	_displayName = XDisplayString(display);

	window = createWindow(argc, argv);
	if (!window) return;

	updateAttributes();
	XMapRaised(display, window);
#endif
	running = true;
	speed = 1.0f;

	resources = new ResourceManager;

	gettimeofday(&now, NULL);
}


void Common::run() {
#ifdef NOXBMC
	Hack::start();

#ifndef NDEBUG
	dumpErrors("start");
#endif // !NDEBUG

	while (running) {
		Hack::tick();
#ifndef NDEBUG
		dumpErrors("tick");
#endif // !NDEBUG
		while (XPending(display)) {
			XEvent event;
			XNextEvent(display, &event);
			switch (event.type) {
			case ConfigureNotify:
				updateAttributes();
				TRACE("Reshaping window");
				Hack::reshape();
				break;
			case MappingNotify:
				TRACE("Key mapping changed");
				XRefreshKeyboardMapping(&event.xmapping);
				break;
			case KeyPress:
				{
					char c;
					KeySym keysym;
					XLookupString(&event.xkey, &c, 1, &keysym, 0);
					TRACE("Key pressed: " << c);
					Hack::keyPress(c, keysym);
				}
				break;
			case KeyRelease:
				{
					char c;
					KeySym keysym;
					XLookupString(&event.xkey, &c, 1, &keysym, 0);
					TRACE("Key released: " << c);
					Hack::keyRelease(c, keysym);
				}
				break;
			case ButtonPress:
				{
					unsigned int button = event.xbutton.button;
					TRACE("Button pressed: " << button << " (" << event.xbutton.x <<
						',' << event.xbutton.y << ')');
					Hack::buttonPress(button);
				}
				break;
			case ButtonRelease:
				{
					unsigned int button = event.xbutton.button;
					TRACE("Button released: " << button << " (" << event.xbutton.x <<
						',' << event.xbutton.y << ')');
					Hack::buttonRelease(button);
				}
				break;
			case MotionNotify:
				{
					int x = event.xmotion.x;
					int y = event.xmotion.y;
					Hack::pointerMotion(x, y);
				}
				break;
			case EnterNotify:
				if (event.xcrossing.state & (
					Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask
				))
					TRACE("Ignoring pointer entering window");
				else {
					TRACE("Pointer entered window");
					Hack::pointerEnter();
				}
				break;
			case LeaveNotify:
				if (event.xcrossing.state & (
					Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask
				))
					TRACE("Ignoring pointer leaving window");
				else {
					TRACE("Pointer exited window");
					Hack::pointerLeave();
				}
				break;
			}
		}
#endif

		then = now;
		gettimeofday(&now, NULL);

#ifndef NDEBUG
		if (_showFPS) {
			elapsedMicros = 1000000 * (now.tv_sec - then.tv_sec) +
				now.tv_usec - then.tv_usec;
			elapsedSecs = float(elapsedMicros) / 1000000.0f;

			static float secsSinceUpdate = 0.0f;
			static unsigned int frames = 0;
			secsSinceUpdate += elapsedSecs;
			++frames;
			if (secsSinceUpdate >= 5.0f) {
				float fps = float(frames) / secsSinceUpdate;
				std::cerr << frames << " frames in " << secsSinceUpdate <<
					" seconds = " << fps << " FPS" << std::endl;
				secsSinceUpdate = 0.0f;
				frames = 0;
			}
		} else {
#endif // !NDEBUG
			elapsedMicros *= 4;
			elapsedMicros += 1000000 * (now.tv_sec - then.tv_sec) +
				now.tv_usec - then.tv_usec;
			elapsedMicros /= 5;
			elapsedSecs = float(elapsedMicros) / 1000000.0f;

			unsigned int remainingMicros =
				(elapsedMicros > MAX_DELAY - MIN_DELAY) ?
				MIN_DELAY : MAX_DELAY - elapsedMicros;

			struct timeval tv;
			tv.tv_sec  = remainingMicros / 1000000L;
			tv.tv_usec = remainingMicros % 1000000L;
			select(0, 0, 0, 0, &tv);
#ifndef NDEBUG
		}
#endif // !NDEBUG
		elapsedTime = speed * elapsedSecs;
#ifdef NOXBMC
	}
	Hack::stop();
#ifndef NDEBUG
	dumpErrors("stop");
#endif // !NDEBUG
#endif
}

void Common::fini() {
	delete resources;
	if (context) glXDestroyContext(display, context);
	if (visualInfo) XFree(visualInfo);
	if (window) XDestroyWindow(display, window);
	if (display) XCloseDisplay(display);
}

/* See http://www.mesa3d.org/brianp/sig97/glxport.htm */
Colormap Common::getColormap() {
	if (visualInfo->visual == DefaultVisual(display, screen))
		return DefaultColormap(display, screen);

	std::string serverString(glXQueryServerString(display, screen, GLX_VERSION));
	bool mesa = serverString.find("Mesa") != std::string::npos;

	if (mesa) {
		Atom atom = XInternAtom(display, "_HP_RGB_SMOOTH_MAP_LIST", True);
		if (
			atom &&
			visualInfo->visual->c_class == TrueColor &&
			depth == 8
		) {
			XStandardColormap *colormaps;
			int numColormaps;
			Colormap result = None;
			if (
				XGetRGBColormaps(display, RootWindow(display, screen),
				&colormaps, &numColormaps, atom)
			) {
				for (int i = 0; i < numColormaps; ++i)
					if (colormaps[i].visualid == Common::visualInfo->visualid)
						result = colormaps[i].colormap;
				XFree(colormaps);
			}
			if (result) return result;
		}
	}

#ifndef SOLARIS_BUG
	if (XmuLookupStandardColormap(
		display, screen, visualInfo->visualid, depth,
		XA_RGB_DEFAULT_MAP, False, True
	)) {
		XStandardColormap* colormaps;
		int numColormaps;
		Colormap result = None;
    if (XGetRGBColormaps(
			display, RootWindow(display, screen),
			&colormaps, &numColormaps, XA_RGB_DEFAULT_MAP
		)) {
			for (int i = 0; i < numColormaps; ++i)
				if (colormaps[i].visualid == Common::visualInfo->visualid)
					result = colormaps[i].colormap;
			XFree(colormaps);
		}
		if (result) return result;
	}
#endif

	return XCreateColormap(display, RootWindow(display, screen),
		visualInfo->visual, AllocNone);
}

Window Common::createWindow(int argc, char** argv) {
	Window window = 0;
	
	if (_onRoot || _windowID) {
		window = _windowID ? _windowID : RootWindow(display, screen);
		TRACE("Drawing on window: " << window);

		XWindowAttributes gwa;
		XGetWindowAttributes(display, window, &gwa);
		Visual* visual = gwa.visual;
		_w = gwa.width;
		_h = gwa.height;

		XVisualInfo templ;
		templ.screen = screen;
		templ.visualid = XVisualIDFromVisual(visual);

		int outCount;
		visualInfo = XGetVisualInfo(display, VisualScreenMask | VisualIDMask,
			&templ, &outCount);

		if (!visualInfo) {
			std::cerr << program << ": could not retrieve visual information for "
				"root window" << std::endl;
			return 0;
		}
	} else {
# define R GLX_RED_SIZE
# define G GLX_GREEN_SIZE
# define B GLX_BLUE_SIZE
# define D GLX_DEPTH_SIZE
# define I GLX_BUFFER_SIZE
# define DB GLX_DOUBLEBUFFER

		static int attributeLists[][20] = {
			{ GLX_RGBA, R, 8, G, 8, B, 8, D, 8, DB, None }, // rgb double
			{ GLX_RGBA, R, 4, G, 4, B, 4, D, 4, DB, None },
			{ GLX_RGBA, R, 2, G, 2, B, 2, D, 2, DB, None },
			{ GLX_RGBA, R, 8, G, 8, B, 8, D, 8,     None }, // rgb single
			{ GLX_RGBA, R, 4, G, 4, B, 4, D, 4,     None },
			{ GLX_RGBA, R, 2, G, 2, B, 2, D, 2,     None },
			{ I, 8,                       D, 8, DB, None }, // cmap double
			{ I, 4,                       D, 4, DB, None },
			{ I, 8,                       D, 8,     None }, // cmap single
			{ I, 4,                       D, 4,     None },
			{ GLX_RGBA, R, 1, G, 1, B, 1, D, 1,     None }  // monochrome
		};
		int fullWidth = WidthOfScreen(DefaultScreenOfDisplay(display));
		int fullHeight = HeightOfScreen(DefaultScreenOfDisplay(display));

		if (_fullScreen) {
			_w = fullWidth;
			_h = fullHeight;
			_x = _y = 0;
			_useOffset = true;
		} else if (_useOffset) {
			if (_reverseX)
				_x = fullWidth - _w - _x;
			if (_reverseY)
				_y = fullHeight - _h - _y;
		}

		for (
			unsigned int i = 0;
			i < sizeof(attributeLists) / sizeof(*attributeLists);
			++i
		) {
			visualInfo = glXChooseVisual(display, screen, attributeLists[i]);
			if (visualInfo) break;
		}

		if (!visualInfo) {
			std::cerr << program <<
				": could not find a GL-capable visual on display " <<
				_displayName << std::endl;
			return 0;
		}
		depth = visualInfo->depth;

		XSetWindowAttributes swa;
		swa.colormap = getColormap();
		swa.border_pixel = swa.background_pixel = swa.backing_pixel =
			BlackPixel(display, screen);
		swa.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
			ButtonReleaseMask | PointerMotionMask | EnterWindowMask |
			LeaveWindowMask | StructureNotifyMask;

		window = XCreateWindow(display, RootWindow(display, screen),
			_x, _y, _w, _h, 0, visualInfo->depth, InputOutput, visualInfo->visual,
			CWBorderPixel | CWBackPixel | CWBackingPixel | CWColormap | CWEventMask,
			&swa);
		TRACE("Created window: 0x" << std::hex << window << std::dec);

		XSizeHints hints;
		hints.flags = USSize;
		hints.width = _w;
		hints.height = _h;
		if (_useOffset) {
			hints.flags |= USPosition;
			hints.x = _x;
			hints.y = _y;
		}
		XWMHints wmHints;
		wmHints.flags = InputHint;
		wmHints.input = True;

		XmbSetWMProperties(display, window, Hack::getName().c_str(),
			Hack::getName().c_str(), argv, argc, &hints, &wmHints, NULL);
	}

	int temp;
	if (glXGetConfig(display, visualInfo, GLX_DOUBLEBUFFER, &temp)) {
		std::cerr << program <<
			": could not get GLX_DOUBLEBUFFER attribute from visual 0x" <<
			std::hex << visualInfo->visualid << std::dec << std::endl;
		return 0;
	}
	doubleBuffered = (temp != False);

	context = glXCreateContext(display, visualInfo, NULL, True);
	if (!context) {
		std::cerr << program << ": could not create rendering context" << std::endl;
		return 0;
	}

	if (!glXMakeCurrent(display, window, context)) {
		std::cerr << program << ": could not activate rendering context" <<
			std::endl;
		return 0;
	}

	return window;
}

void Common::updateAttributes() {
	XWindowAttributes attributes;
	XGetWindowAttributes(display, window, &attributes);
	width = attributes.width;
	height = attributes.height;
	depth = attributes.depth;
	centerX = width >> 1;
	centerY = height >> 1;
	aspectRatio = float(width) / float(height);
	colormap = attributes.colormap;
}

#ifndef NDEBUG
void Common::dumpErrors(const std::string& func) {
	GLenum error;
	while ( (error = glGetError()) )
		WARN(func << ": " << gluErrorString(error));

	GLint i;
	glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &i);
	if (i > 1) WARN(func << ": GL_ATTRIB_STACK_DEPTH == " << i);
	glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &i);
	if (i > 1) WARN(func << ": GL_MODELVIEW_STACK_DEPTH == " << i);
	glGetIntegerv(GL_NAME_STACK_DEPTH, &i);
	if (i > 1) WARN(func << ": GL_NAME_STACK_DEPTH == " << i);
	glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &i);
	if (i > 1) WARN(func << ": GL_PROJECTION_STACK_DEPTH == " << i);
	glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &i);
	if (i > 1) WARN(func << ": GL_TEXTURE_STACK_DEPTH == " << i);
}
#endif // !NDEBUG

const char * program_name;

int main(int argc, char** argv) {
	int exit_code = EXIT_FAILURE;

	Common::program = argv[0];
	program_name = Hack::getShortName().c_str();
	argp_program_version = PACKAGE_STRING;
	argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
	struct argp_option options[] = {
		{ NULL, 0, NULL, 0, "Help options:", -1 },
		{ NULL, 0, NULL, 0, "Common options:" },
		{ "root", Common::ARG_ROOT, NULL, 0, "Draw on the root window" },
		{ "geometry", Common::ARG_GEOMETRY, "GEOM", 0,
			"Draw on a window of the specified geometry (WxH+X+Y)" },
		{ "fullscreen", Common::ARG_FULLSCREEN, NULL, 0,
			"Draw on a maximized window" },
		{ "window-id", Common::ARG_WINDOWID, "ID", OPTION_HIDDEN },
		{ "resource-dir", Common::ARG_RESOURCE_DIR, "DIR", OPTION_HIDDEN },
#ifndef NDEBUG
		{ "fps", Common::ARG_FPS, NULL, OPTION_HIDDEN },
#endif // !NDEBUG
		{}
	};
	struct argp_child child[] = {
		{ Hack::getParser(), 0, "" },
		{}
	};
	struct argp parser =
		{ options, Common::parse, NULL, NULL, child };

	std::srand((unsigned int)std::time(NULL));

	// Use ARGP_LONG_ONLY to follow XScreenSaver tradition
	if (argp_parse(&parser, argc, argv, ARGP_LONG_ONLY, NULL, NULL))
		return EXIT_FAILURE;

	try {
		Common::init(argc, argv);
		Common::run();
		exit_code = EXIT_SUCCESS;
	} catch (Common::Exception e) {
		WARN(e);
	}
	Common::fini();

	return exit_code;
}
