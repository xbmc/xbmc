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

#ifndef LLEMBEDDEDBROWSERWINDOW_H
#define LLEMBEDDEDBROWSERWINDOW_H

#include <string>
#include <list>
#include <algorithm>
#ifdef _MSC_VER
#include "pstdint.h"
#else
#include <stdint.h>        // Use the C99 official header
#endif

#include "llqtwebkit.h"

class LLEmbeddedBrowser;

////////////////////////////////////////////////////////////////////////////////
// class for a "window" that holds a browser - there can be lots of these
class LLEmbeddedBrowserWindowPrivate;
class LLEmbeddedBrowserWindow
{
public:
    LLEmbeddedBrowserWindow();
    virtual ~LLEmbeddedBrowserWindow();

    // housekeeping
    void setParent(LLEmbeddedBrowser* parent);
    bool setSize(int16_t width, int16_t height);
    void focusBrowser(bool focus_browser);
    void scrollByLines(int16_t lines);
    void setWindowId(int window_id);
    int getWindowId();

    // random accessors
    int16_t getPercentComplete();
    std::string& getStatusMsg();
    std::string& getCurrentUri();
    std::string& getClickLinkHref();
    std::string& getClickLinkTarget();

    // memory buffer management
    unsigned char* grabWindow(int x, int y, int width, int height);
    bool flipWindow(bool flip);
    unsigned char* getPageBuffer();
    int16_t getBrowserWidth();
    int16_t getBrowserHeight();
    int16_t getBrowserDepth();
    int32_t getBrowserRowSpan();

    // set background color that you see in between pages - default is white but sometimes useful to change
    void setBackgroundColor(const uint8_t red, const uint8_t green, const uint8_t blue);

    // change the caret color (we have different backgrounds to edit fields - black caret on black background == bad)
    void setCaretColor(const uint8_t red, const uint8_t green, const uint8_t blue);

    // can turn off updates to a page - e.g. when it's hidden by your windowing system
    void setEnabled(bool enabledIn);

    // navigation
    bool userAction(LLQtWebKit::EUserAction action);
    bool userActionIsEnabled(LLQtWebKit::EUserAction action);
    bool navigateTo(const std::string uri);

    // javascript access/control
    std::string evaluateJavascript(std::string script);
    void setWindowOpenBehavior(LLQtWebKit::WindowOpenBehavior behavior);

    // redirection when you hit a missing page
    bool set404RedirectUrl(std::string redirect_url);
    bool clr404RedirectUrl();
    void load404RedirectUrl();
    
    // host language setting
    void setHostLanguage(const std::string host_language);

    // mouse & keyboard events
    void mouseEvent(LLQtWebKit::EMouseEvent mouse_event, int16_t button, int16_t x, int16_t y, LLQtWebKit::EKeyboardModifier modifiers);
    void scrollWheelEvent(int16_t x, int16_t y, int16_t scroll_x, int16_t scroll_y, LLQtWebKit::EKeyboardModifier modifiers);
    void keyboardEvent(
		LLQtWebKit::EKeyEvent key_event, 
		uint32_t key_code, 
		const char *utf8_text, 
		LLQtWebKit::EKeyboardModifier modifiers, 
		uint32_t native_scan_code,
		uint32_t native_virtual_key,
		uint32_t native_modifiers);

    // allow consumers of this class and to observe browser events
    bool addObserver(LLEmbeddedBrowserWindowObserver* observer);
    bool remObserver(LLEmbeddedBrowserWindowObserver* observer);
    int getObserverNumber();

    // accessor/mutator for scheme that browser doesn't follow - e.g. secondlife.com://
    void setNoFollowScheme(std::string scheme);
    std::string getNoFollowScheme();

	// accessor/mutator for names of HREF attributes for blank and external targets
	void setExternalTargetName(std::string name);
	std::string getExternalTargetName();
	void setBlankTargetName(std::string name);
	std::string getBlankTargetName();

	// prepend the current history with the given url
	void prependHistoryUrl(std::string url);
	// clear the URL history
	void clearHistory();
	std::string dumpHistory();

private:
    friend class LLWebPage;
    friend class LLGraphicsScene;
    friend class LLWebView;
    friend class LLEmbeddedBrowserPrivate;
    LLEmbeddedBrowserWindowPrivate *d;

};
#endif // LLEMBEDEDDBROWSERWINDOW_H
