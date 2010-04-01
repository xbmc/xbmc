/* Copyright (c) 2006-2010, Linden Research, Inc.
 * 
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#ifndef LLQTWEBKIT_H
#define LLQTWEBKIT_H

#ifdef _MSC_VER
// no pstdint.h in the client where this header is used
typedef unsigned long uint32_t;
#else
#include <stdint.h>        // Use the C99 official header
#endif

#include <string>
#include <map>

class LLEmbeddedBrowser;
class LLEmbeddedBrowserWindow;

#ifdef _WINDOWS
typedef unsigned long uint32_t;
#endif

////////////////////////////////////////////////////////////////////////////////
// data class that is passed with an event
class LLEmbeddedBrowserWindowEvent
{
	public:
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri) :
			mEventWindowId(window_id),
			mEventUri(uri)
		{
		};

		// single int passed with the event - e.g. progress
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, int value) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mIntVal(value)
		{
		};

		// string passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mStringVal(value)
		{
		};

		// 2 strings passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value, std::string value2) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mStringVal(value),
			mStringVal2(value2)
		{
		};

		// 2 strings plus an int passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value, std::string value2, int link_type ) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mStringVal(value),
			mStringVal2(value2),
			mLinkType(link_type)
		{
		};

		// string and an int passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value, int value2) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mIntVal(value2),
			mStringVal(value)
		{
		}

		// 4 ints passed (semantically as a rectangle but could be anything - didn't want to make a RECT type structure)
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, int x, int y, int width, int height) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mXVal(x),
			mYVal(y),
			mWidthVal(width),
			mHeightVal(height)
		{
		};

		virtual ~LLEmbeddedBrowserWindowEvent()
		{
		};

		int getEventWindowId() const
		{
			return mEventWindowId;
		};

		std::string getEventUri() const
		{
			return mEventUri;
		};

		int getIntValue() const
		{
			return mIntVal;
		};

		std::string getStringValue() const
		{
			return mStringVal;
		};

		std::string getStringValue2() const
		{
			return mStringVal2;
		};

		int getLinkType() const
		{
			return mLinkType;
		};

		void getRectValue(int& x, int& y, int& width, int& height) const
		{
			x = mXVal;
			y = mYVal;
			width = mWidthVal;
			height = mHeightVal;
		};

	private:
		int mEventWindowId;
		std::string mEventUri;
		int mIntVal;
		std::string mStringVal;
		std::string mStringVal2;
		int mLinkType;
		int mXVal;
		int mYVal;
		int mWidthVal;
		int mHeightVal;
};

////////////////////////////////////////////////////////////////////////////////
// derrive from this class and override these methods to observe these events
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
class LLEmbeddedBrowserWindowObserver
{
	public:
		virtual ~LLEmbeddedBrowserWindowObserver();
		typedef LLEmbeddedBrowserWindowEvent EventType;

		virtual void onCursorChanged(const EventType& event);
		virtual void onPageChanged(const EventType& event);
		virtual void onNavigateBegin(const EventType& event);
		virtual void onNavigateComplete(const EventType& event);
		virtual void onUpdateProgress(const EventType& event);
		virtual void onStatusTextChange(const EventType& event);
		virtual void onTitleChange(const EventType& event);
		virtual void onLocationChange(const EventType& event);
		virtual void onClickLinkHref(const EventType& event);
		virtual void onClickLinkNoFollow(const EventType& event);
};
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

////////////////////////////////////////////////////////////////////////////////
// main library class

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
class LLQtWebKit
{
	public:
		typedef enum e_cursor
                {
                    C_ARROW,
                    C_IBEAM,
                    C_SPLITV,
                    C_SPLITH,
                    C_POINTINGHAND
                } ECursor;

		typedef enum e_user_action
		{
			UA_EDIT_CUT,
			UA_EDIT_COPY,
			UA_EDIT_PASTE,
			UA_NAVIGATE_STOP,
			UA_NAVIGATE_BACK,
			UA_NAVIGATE_FORWARD,
			UA_NAVIGATE_RELOAD
		} EUserAction;

		typedef enum e_key_event
		{
			KE_KEY_DOWN,
			KE_KEY_REPEAT,
			KE_KEY_UP
		}EKeyEvent;

		typedef enum e_mouse_event
		{
			ME_MOUSE_MOVE,
			ME_MOUSE_DOWN,
			ME_MOUSE_UP,
			ME_MOUSE_DOUBLE_CLICK
		}EMouseEvent;

		typedef enum e_mouse_button
		{
			MB_MOUSE_BUTTON_LEFT,
			MB_MOUSE_BUTTON_RIGHT,
			MB_MOUSE_BUTTON_MIDDLE,
			MB_MOUSE_BUTTON_EXTRA_1,
			MB_MOUSE_BUTTON_EXTRA_2,
		}EMouseButton;

		typedef enum e_keyboard_modifier
		{
			KM_MODIFIER_NONE = 0x00,
			KM_MODIFIER_SHIFT = 0x01,
			KM_MODIFIER_CONTROL = 0x02,
			KM_MODIFIER_ALT = 0x04,
			KM_MODIFIER_META = 0x08
		}EKeyboardModifier;

		virtual ~LLQtWebKit();

		// singleton access
		static LLQtWebKit* getInstance();

		// housekeeping
		bool init(std::string application_directory,
                          std::string component_directory,
                          std::string profile_directory,
                          void* native_window_handle);
		bool reset();
		bool clearCache();
		int getLastError();
		std::string getVersion();
		void setBrowserAgentId(std::string id);
		bool enableProxy(bool enabled, std::string host_name, int port);
		
		bool enableCookies(bool enabled);
		bool clearAllCookies();
		bool enablePlugins(bool enabled);
		bool enableJavascript(bool enabled);

		// updates value of 'hostLanguage' in JavaScript 'Navigator' obect that 
		// embedded pages can query to see what language the host app is set to
		void setHostLanguage(const std::string& host_language);

		// browser window - creation/deletion, mutation etc.
		int createBrowserWindow(int width, int height);
		bool destroyBrowserWindow(int browser_window_id);
		bool setSize(int browser_window_id, int width, int height);
		bool scrollByLines(int browser_window_id, int lines);
		bool setBackgroundColor(int browser_window_id, const int red, const int green, const int blue);
		bool setCaretColor(int browser_window_id, const int red, const int green, const int blue);
		bool setEnabled(int browser_window_id, bool enabled);

		// add/remove yourself as an observer on browser events - see LLEmbeddedBrowserWindowObserver declaration
		bool addObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject);
		bool remObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject);

		// navigation - self explanatory
		bool navigateTo(int browser_window_id, const std::string uri);
		bool userAction(int browser_window_id, EUserAction action);
		bool userActionIsEnabled(int browser_window_id, EUserAction action);

		// javascript access/control
		std::string evaluateJavascript(int browser_window_id, const std::string script);

		enum WindowOpenBehavior
		{
		    WOB_IGNORE,
		    WOB_REDIRECT_TO_SELF
		};
		void setWindowOpenBehavior(int browser_window_id, WindowOpenBehavior behavior);

		// set/clear URL to redirect to when a 404 page is reached
		bool set404RedirectUrl(int browser_window_in, std::string redirect_url);
		bool clr404RedirectUrl(int browser_window_in);

		// access to rendered bitmap data
		const unsigned char* grabBrowserWindow(int browser_window_id);		// renders page to memory and returns pixels
		const unsigned char* getBrowserWindowPixels(int browser_window_id);	// just returns pixels - no render
		bool flipWindow(int browser_window_id, bool flip);			// optionally flip window (pixels) you get back
		int getBrowserWidth(int browser_window_id);						// current browser width (can vary slightly after page is rendered)
		int getBrowserHeight(int browser_window_id);					// current height
		int getBrowserDepth(int browser_window_id);						// depth in bytes
		int getBrowserRowSpan(int browser_window_id);					// width in pixels * depth in bytes

		// mouse/keyboard interaction
		bool mouseEvent(int browser_window_id, EMouseEvent mouse_event, int button, int x, int y, EKeyboardModifier modifiers); // send a mouse event to a browser window at given XY in browser space
		bool scrollWheelEvent(int browser_window_id, int x, int y, int scroll_x, int scroll_y, EKeyboardModifier modifiers);
		bool keyboardEvent(
			int browser_window_id,
			EKeyEvent key_event, 
			uint32_t key_code, 
			const char *utf8_text, 
			EKeyboardModifier modifiers, 
			uint32_t native_scan_code = 0,
			uint32_t native_virtual_key = 0,
			uint32_t native_modifiers = 0);

		bool focusBrowser(int browser_window_id, bool focus_browser);			// set/remove focus to given browser window

		// accessor/mutator for scheme that browser doesn't follow - e.g. secondlife.com://
		void setNoFollowScheme(int browser_window_id, std::string scheme);
		std::string getNoFollowScheme(int browser_window_id);

		// accessor/mutator for names of HREF attributes for blank and external targets
		void setExternalTargetName(int browser_window_id, std::string name);
		std::string getExternalTargetName(int browser_window_id);
		void setBlankTargetName(int browser_window_id, std::string name);
		std::string getBlankTargetName(int browser_window_id);

		void pump(int max_milliseconds);

		void prependHistoryUrl(int browser_window_id, std::string url);
		void clearHistory(int browser_window_id);
		std::string dumpHistory(int browser_window_id);

		// Copied from indra_constants.h.
		// The key_code argument to keyboardEvent should either be one of these or a 7-bit ascii character.
		enum keyCodes
		{
			// Leading zeroes ensure that these won't sign-extend when assigned to a larger type.
			KEY_RETURN			= 0x0081,
			KEY_LEFT			= 0x0082,
			KEY_RIGHT			= 0x0083,
			KEY_UP				= 0x0084,
			KEY_DOWN			= 0x0085,
			KEY_ESCAPE			= 0x0086,
			KEY_BACKSPACE		= 0x0087,
			KEY_DELETE			= 0x0088,
			KEY_SHIFT			= 0x0089,
			KEY_CONTROL			= 0x008A,
			KEY_ALT				= 0x008B,
			KEY_HOME			= 0x008C,
			KEY_END				= 0x008D,
			KEY_PAGE_UP			= 0x008E,
			KEY_PAGE_DOWN		= 0x008F,
			KEY_HYPHEN			= 0x0090,
			KEY_EQUALS			= 0x0091,
			KEY_INSERT			= 0x0092,
			KEY_CAPSLOCK		= 0x0093,
			KEY_TAB				= 0x0094,
			KEY_ADD				= 0x0095,
			KEY_SUBTRACT		= 0x0096,
			KEY_MULTIPLY		= 0x0097,
			KEY_DIVIDE			= 0x0098,
			KEY_F1				= 0x00A1,
			KEY_F2				= 0x00A2,
			KEY_F3				= 0x00A3,
			KEY_F4				= 0x00A4,
			KEY_F5				= 0x00A5,
			KEY_F6				= 0x00A6,
			KEY_F7				= 0x00A7,
			KEY_F8				= 0x00A8,
			KEY_F9				= 0x00A9,
			KEY_F10				= 0x00AA,
			KEY_F11				= 0x00AB,
			KEY_F12				= 0x00AC,

			KEY_PAD_UP			= 0x00C0,
			KEY_PAD_DOWN		= 0x00C1,
			KEY_PAD_LEFT		= 0x00C2,
			KEY_PAD_RIGHT		= 0x00C3,
			KEY_PAD_HOME		= 0x00C4,
			KEY_PAD_END			= 0x00C5,
			KEY_PAD_PGUP		= 0x00C6,
			KEY_PAD_PGDN		= 0x00C7,
			KEY_PAD_CENTER		= 0x00C8, // the 5 in the middle
			KEY_PAD_INS			= 0x00C9,
			KEY_PAD_DEL			= 0x00CA,
			KEY_PAD_RETURN		= 0x00CB,
			KEY_PAD_ADD			= 0x00CC,
			KEY_PAD_SUBTRACT	= 0x00CD,
			KEY_PAD_MULTIPLY	= 0x00CE,
			KEY_PAD_DIVIDE		= 0x00CF,

			KEY_NONE			= 0x00FF // not sent from keyboard.  For internal use only.
		};
		
		enum linkTargetType
		{
			LTT_TARGET_UNKNOWN		= 0x00,
			LTT_TARGET_NONE			= 0x01,
			LTT_TARGET_BLANK		= 0x02,
			LTT_TARGET_EXTERNAL		= 0x04,
			LTT_TARGET_OTHER		= 0x08
		};
		
	private:
		LLQtWebKit();
		LLEmbeddedBrowserWindow* getBrowserWindowFromWindowId(int browser_window_id);
		static LLQtWebKit* sInstance;
		const int mMaxBrowserWindows;
		typedef std::map< int, LLEmbeddedBrowserWindow* > BrowserWindowMap;
		typedef std::map< int, LLEmbeddedBrowserWindow* >::iterator BrowserWindowMapIter;
		BrowserWindowMap mBrowserWindowMap;
};

#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#endif // LLQTWEBKIT_H
