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

#include <sstream>

#include "llembeddedbrowserwindow.h"
#include "llembeddedbrowserwindow_p.h"

#include "llembeddedbrowser.h"
#include "llembeddedbrowser_p.h"
#include "llnetworkaccessmanager.h"


#include <qaction.h>
#include <qwebframe.h>
#include <qwebhistory.h>
#include <qpainter.h>
#include <qevent.h>
#include <qfile.h>
#include <QGLWidget>

#ifdef STATIC_QT
	#include <qplugin.h>
	// Enable gif and jpeg plugins, since web pages look pretty bleak without gifs or jpegs.
	Q_IMPORT_PLUGIN(qgif)
	Q_IMPORT_PLUGIN(qjpeg)
#ifndef LL_LINUX
	// Qt also has its own translators for CJK text encodings we need to pull in.
	Q_IMPORT_PLUGIN(qcncodecs)
	Q_IMPORT_PLUGIN(qjpcodecs)
	Q_IMPORT_PLUGIN(qkrcodecs)
	Q_IMPORT_PLUGIN(qtwcodecs)
#endif
#endif

//#define LLEMBEDDEDBROWSER_DEBUG

#ifdef LLEMBEDDEDBROWSER_DEBUG
#include <qdebug.h>
#endif

#if LL_DARWIN || defined(STATIC_QT)
	// Don't define qt_sendSpontaneousEvent on the mac -- it causes a multiply-defined symbol.
	extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);
#else
	#include <qcoreapplication.h>
	bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event)
	{
		return QCoreApplication::sendSpontaneousEvent(receiver, event);
	}
#endif

//#define WEBHISTORYPATCH

LLEmbeddedBrowserWindow::LLEmbeddedBrowserWindow()
{
    d = new LLEmbeddedBrowserWindowPrivate();
    d->mPage->window = this;

    d->mView = new LLWebView;
    d->mPage->webView = d->mView;
    d->mView->window = this;
    d->mView->setPage(d->mPage);
    d->mGraphicsScene = new LLGraphicsScene;
    d->mGraphicsScene->window = this;
    d->mGraphicsView = new QGraphicsView;
    d->mGraphicsScene->addItem(d->mView);
    d->mGraphicsView->setScene(d->mGraphicsScene);
    d->mGraphicsScene->setStickyFocus(true);
    d->mGraphicsView->viewport()->setParent(0);
    d->m404RedirectUrl = std::string();
}

LLEmbeddedBrowserWindow::~LLEmbeddedBrowserWindow()
{
    delete d;
}

void LLEmbeddedBrowserWindow::setParent(LLEmbeddedBrowser* parent)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << parent;
#endif
    d->mParent = parent;
    if (parent)
    {
        d->mPage->setNetworkAccessManager(parent->d->mNetworkAccessManager);
    } else
    {
        d->mPage->setNetworkAccessManager(0);
    }
}

// change the background color that gets used between pages (usually white)
void LLEmbeddedBrowserWindow::setBackgroundColor(const uint8_t red, const uint8_t green, const uint8_t blue)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << red << green << blue;
#endif
    d->backgroundColor = QColor(red, green, blue);
}

// change the caret color (we have different backgrounds to edit fields - black caret on black background == bad)
void LLEmbeddedBrowserWindow::setCaretColor(const uint8_t red, const uint8_t green, const uint8_t blue)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << red << green << blue;
#endif
    Q_UNUSED(red);
    Q_UNUSED(green);
    Q_UNUSED(blue);
    // QtWebKit paints the caret so we don't have to
}

//
void LLEmbeddedBrowserWindow::setEnabled(bool enabled)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << enabled;
#endif
    d->mEnabled = enabled;
}

// allow consumers of this class to observe events - add themselves as an observer
bool LLEmbeddedBrowserWindow::addObserver(LLEmbeddedBrowserWindowObserver* observer)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << observer;
#endif
    return d->mEventEmitter.addObserver(observer);
}

// allow consumers of this class to observe events - remove themselves as an observer
bool LLEmbeddedBrowserWindow::remObserver(LLEmbeddedBrowserWindowObserver* observer)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << observer;
#endif
    return d->mEventEmitter.remObserver(observer);
}

int LLEmbeddedBrowserWindow::getObserverNumber()
{
    return d->mEventEmitter.getObserverNumber();
}

// used by observers of this class to get the current URI
std::string& LLEmbeddedBrowserWindow::getCurrentUri()
{
    d->mCurrentUri = QString(d->mPage->mainFrame()->url().toEncoded()).toStdString();
    return d->mCurrentUri;
}

// utility method that is used by observers to retrieve data after an event
int16_t LLEmbeddedBrowserWindow::getPercentComplete()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mPercentComplete;
}

// utility method that is used by observers to retrieve data after an event
std::string& LLEmbeddedBrowserWindow::getStatusMsg()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mStatusText;
}

// utility method that is used by observers to retrieve data after an event
std::string& LLEmbeddedBrowserWindow::getClickLinkHref()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mClickHref;
}

// utility method that is used by observers to retrieve data after an event
std::string& LLEmbeddedBrowserWindow::getClickLinkTarget()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mClickTarget;
}

// render a page into memory and grab the window
unsigned char* LLEmbeddedBrowserWindow::grabWindow(int x, int y, int width, int height)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << x << y << width << height;
#endif
    // only grab the window if it's enabled
    if (!d->mEnabled)
        return 0;

    if (!d->mDirty)
        return d->mPageBuffer;

    Q_ASSERT(d->mImage.size() == d->mView->size());
    if (!d->mPage->mainFrame()->url().isValid())
    {
        d->mImage.fill(d->backgroundColor.value());
    } else
    {
        QPainter painter(&d->mImage);
#if 1   // Paint from the graphics view
        QRectF r(x, y, width, height);
        QRect g(0, 0, d->mView->width(), d->mView->height());
        d->mGraphicsView->render(&painter, r, g);
#else   // Paint straight from the web page
        QRegion clip(x, y, width, height);
        d->mPage->mainFrame()->render(&painter, clip);
#endif
        painter.end();
        if (!d->mFlipBitmap)
        {
            d->mImage = d->mImage.mirrored();
        }
    }
    d->mImage = QGLWidget::convertToGLFormat(d->mImage);
    d->mPageBuffer = d->mImage.bits();
    d->mDirty = false;
    return d->mPageBuffer;
}

// return the buffer that contains the rendered page
unsigned char* LLEmbeddedBrowserWindow::getPageBuffer()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mPageBuffer;
}

int16_t LLEmbeddedBrowserWindow::getBrowserWidth()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mImage.width();
}

int16_t LLEmbeddedBrowserWindow::getBrowserHeight()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mImage.height();
}

int16_t LLEmbeddedBrowserWindow::getBrowserDepth()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return 4;
}

int32_t LLEmbeddedBrowserWindow::getBrowserRowSpan()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return 4 * getBrowserWidth();
}

bool LLEmbeddedBrowserWindow::navigateTo(const std::string uri)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << QString::fromStdString(uri);
#endif
    QUrl url = QUrl::fromUserInput(QString::fromStdString(uri));

    d->mPage->triggerAction(QWebPage::Stop);
    d->mPage->mainFrame()->load(url);
    return true;
}

bool LLEmbeddedBrowserWindow::userAction(LLQtWebKit::EUserAction action)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << action;
#endif
	bool result = true;

	switch(action)
	{
		case LLQtWebKit::UA_EDIT_CUT:
			d->mPage->triggerAction(QWebPage::Cut);
		break;
		case LLQtWebKit::UA_EDIT_COPY:
			d->mPage->triggerAction(QWebPage::Copy);
		break;
		case LLQtWebKit::UA_EDIT_PASTE:
			d->mPage->triggerAction(QWebPage::Paste);
		break;
		case LLQtWebKit::UA_NAVIGATE_STOP:
			d->mPage->triggerAction(QWebPage::Stop);
		break;
		case LLQtWebKit::UA_NAVIGATE_BACK:
			d->mPage->triggerAction(QWebPage::Back);
		break;
		case LLQtWebKit::UA_NAVIGATE_FORWARD:
			d->mPage->triggerAction(QWebPage::Forward);
		break;
		case LLQtWebKit::UA_NAVIGATE_RELOAD:
			d->mPage->triggerAction(QWebPage::Reload);
		break;
		default:
			result = false;
		break;
	}

	return result;
}

bool LLEmbeddedBrowserWindow::userActionIsEnabled(LLQtWebKit::EUserAction action)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << action;
#endif

	bool result;

	switch(action)
	{
		case LLQtWebKit::UA_EDIT_CUT:
			result = d->mPage->action(QWebPage::Cut)->isEnabled();
		break;
		case LLQtWebKit::UA_EDIT_COPY:
			result = d->mPage->action(QWebPage::Copy)->isEnabled();
		break;
		case LLQtWebKit::UA_EDIT_PASTE:
			result = d->mPage->action(QWebPage::Paste)->isEnabled();
		break;
		case LLQtWebKit::UA_NAVIGATE_STOP:
			result = true;
		break;
		case LLQtWebKit::UA_NAVIGATE_BACK:
			result = d->mPage->history()->canGoBack();
		break;
		case LLQtWebKit::UA_NAVIGATE_FORWARD:
			result = d->mPage->history()->canGoForward();
		break;
		case LLQtWebKit::UA_NAVIGATE_RELOAD:
			result = true;
		break;
		default:
			result = false;
		break;
	}
	return result;
}

// set the size of the browser window
bool LLEmbeddedBrowserWindow::setSize(int16_t width, int16_t height)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << width << height;
#endif
    d->mPageBuffer = NULL;
    d->mImage = QImage(QSize(width, height), QImage::Format_RGB32);
    d->mGraphicsView->resize(width, height);
    d->mView->resize(width, height);
    d->mImage.fill(d->backgroundColor.rgb());
    return true;
}

bool LLEmbeddedBrowserWindow::flipWindow(bool flip)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << flip;
#endif
    d->mFlipBitmap = flip;
    return true;
}

static Qt::KeyboardModifiers convert_modifiers(LLQtWebKit::EKeyboardModifier modifiers)
{
	Qt::KeyboardModifiers result = Qt::NoModifier;

	if(modifiers & LLQtWebKit::KM_MODIFIER_SHIFT)
		result |= Qt::ShiftModifier;

	if(modifiers & LLQtWebKit::KM_MODIFIER_CONTROL)
		result |= Qt::ControlModifier;

	if(modifiers & LLQtWebKit::KM_MODIFIER_ALT)
		result |= Qt::AltModifier;

	if(modifiers & LLQtWebKit::KM_MODIFIER_META)
		result |= Qt::MetaModifier;

	return result;
}

static Qt::MouseButton qt_button_from_button_number(int button)
{
	Qt::MouseButton result;

	switch(button)
	{
		default:	result = Qt::NoButton;			break;
		case 0:		result = Qt::LeftButton;		break;
		case 1:		result = Qt::RightButton;		break;
		case 2:		result = Qt::MidButton;			break;
		case 3:		result = Qt::XButton1;			break;
		case 4:		result = Qt::XButton2;			break;
	}

	return result;
}

static QEvent::Type event_from_mouse_event(LLQtWebKit::EMouseEvent mouse_event)
{
	QEvent::Type result;

	switch(mouse_event)
	{
		default:
			result = QEvent::None;
		break;

		case LLQtWebKit::ME_MOUSE_MOVE:
			result = QEvent::MouseMove;
		break;

		case LLQtWebKit::ME_MOUSE_DOWN:
			result = QEvent::MouseButtonPress;
		break;

		case LLQtWebKit::ME_MOUSE_UP:
			result = QEvent::MouseButtonRelease;
		break;

		case LLQtWebKit::ME_MOUSE_DOUBLE_CLICK:
			result = QEvent::MouseButtonDblClick;
		break;
	}

	return result;
}

static QEvent::Type event_from_keyboard_event(LLQtWebKit::EKeyEvent keyboard_event)
{
	QEvent::Type result;

	switch(keyboard_event)
	{
		default:
			result = QEvent::None;
		break;

		case LLQtWebKit::KE_KEY_DOWN:
		case LLQtWebKit::KE_KEY_REPEAT:
			result = QEvent::KeyPress;
		break;

		case LLQtWebKit::KE_KEY_UP:
			result = QEvent::KeyRelease;
		break;
	}

	return result;
}

void LLEmbeddedBrowserWindow::mouseEvent(LLQtWebKit::EMouseEvent mouse_event, int16_t button, int16_t x, int16_t y, LLQtWebKit::EKeyboardModifier modifiers)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << x << y;
#endif

	QEvent::Type type = event_from_mouse_event(mouse_event);
	Qt::MouseButton qt_button = qt_button_from_button_number(button);
	Qt::KeyboardModifiers qt_modifiers = convert_modifiers(modifiers);

	if(type == QEvent::MouseMove)
	{
		// Mouse move events should always use "no button".
		qt_button = Qt::NoButton;
	}

	// FIXME: should the current button state be updated before or after constructing the event?
	switch(type)
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonDblClick:
			d->mCurrentMouseButtonState |= qt_button;
		break;

		case QEvent::MouseButtonRelease:
			d->mCurrentMouseButtonState &= ~qt_button;
		break;

		default:
		break;
	}

    QMouseEvent event(type, QPoint(x, y), qt_button, d->mCurrentMouseButtonState, qt_modifiers);

    qt_sendSpontaneousEvent(d->mGraphicsView->viewport(), &event);
}

void LLEmbeddedBrowserWindow::scrollWheelEvent(int16_t x, int16_t y, int16_t scroll_x, int16_t scroll_y, LLQtWebKit::EKeyboardModifier modifiers)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << x << y;
#endif

	Qt::KeyboardModifiers qt_modifiers = convert_modifiers(modifiers);

	if(scroll_y != 0)
	{
	    QWheelEvent event(QPoint(x, y), scroll_y, d->mCurrentMouseButtonState, qt_modifiers, Qt::Vertical);
	    qApp->sendEvent(d->mGraphicsView->viewport(), &event);
	}

	if(scroll_x != 0)
	{
	    QWheelEvent event(QPoint(x, y), scroll_x, d->mCurrentMouseButtonState, qt_modifiers, Qt::Horizontal);
	    qApp->sendEvent(d->mGraphicsView->viewport(), &event);
	}
}


// utility methods to set an error message so something else can look at it
void LLEmbeddedBrowserWindow::scrollByLines(int16_t lines)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << lines;
#endif
    int currentScrollValue = d->mPage->mainFrame()->scrollBarValue(Qt::Vertical);
    d->mPage->mainFrame()->setScrollBarValue(Qt::Vertical, currentScrollValue + lines);
}

// Send a keyboard event with native event data.
void LLEmbeddedBrowserWindow::keyboardEvent(
		LLQtWebKit::EKeyEvent key_event, 
		uint32_t key_code, 
		const char *utf8_text, 
		LLQtWebKit::EKeyboardModifier modifiers, 
		uint32_t native_scan_code,
		uint32_t native_virtual_key,
		uint32_t native_modifiers)
{
	QEvent::Type type = event_from_keyboard_event(key_event);
	Qt::KeyboardModifiers qt_modifiers = convert_modifiers(modifiers);
	bool auto_repeat = (key_event == LLQtWebKit::KE_KEY_REPEAT);
	QString text = QString::fromUtf8(utf8_text);

    Qt::Key key = Qt::Key_unknown;

    switch (key_code)
	{
		case LLQtWebKit::KEY_RETURN:			key = Qt::Key_Return;		break;
		case LLQtWebKit::KEY_LEFT:				key = Qt::Key_Left;			break;
		case LLQtWebKit::KEY_RIGHT:				key = Qt::Key_Right;		break;
		case LLQtWebKit::KEY_UP:				key = Qt::Key_Up;			break;
		case LLQtWebKit::KEY_DOWN:				key = Qt::Key_Down;			break;
		case LLQtWebKit::KEY_ESCAPE:			key = Qt::Key_Escape;		break;
		case LLQtWebKit::KEY_BACKSPACE:			key = Qt::Key_Backspace;	break;
		case LLQtWebKit::KEY_DELETE:			key = Qt::Key_Delete;		break;
		case LLQtWebKit::KEY_SHIFT:				key = Qt::Key_Shift;		break;
		case LLQtWebKit::KEY_CONTROL:			key = Qt::Key_Control;		break;
		case LLQtWebKit::KEY_ALT:				key = Qt::Key_Alt;			break;
		case LLQtWebKit::KEY_HOME:				key = Qt::Key_Home;			break;
		case LLQtWebKit::KEY_END:				key = Qt::Key_End;			break;
		case LLQtWebKit::KEY_PAGE_UP:			key = Qt::Key_PageUp;		break;
		case LLQtWebKit::KEY_PAGE_DOWN:			key = Qt::Key_PageDown;		break;
		case LLQtWebKit::KEY_HYPHEN:			key = Qt::Key_hyphen;		break;
		case LLQtWebKit::KEY_EQUALS:			key = Qt::Key_Equal;		break;
		case LLQtWebKit::KEY_INSERT:			key = Qt::Key_Insert;		break;
		case LLQtWebKit::KEY_CAPSLOCK:			key = Qt::Key_CapsLock;		break;
		case LLQtWebKit::KEY_TAB:				key = Qt::Key_Tab;			break;
		case LLQtWebKit::KEY_ADD:				key = Qt::Key_Plus;			break;
		case LLQtWebKit::KEY_SUBTRACT:			key = Qt::Key_Minus;		break;
		case LLQtWebKit::KEY_MULTIPLY:			key = Qt::Key_Asterisk;		break;
		case LLQtWebKit::KEY_DIVIDE:			key = Qt::Key_Slash;		break;
		case LLQtWebKit::KEY_F1:				key = Qt::Key_F1;			break;
		case LLQtWebKit::KEY_F2:				key = Qt::Key_F2;			break;
		case LLQtWebKit::KEY_F3:				key = Qt::Key_F3;			break;
		case LLQtWebKit::KEY_F4:				key = Qt::Key_F4;			break;
		case LLQtWebKit::KEY_F5:				key = Qt::Key_F5;			break;
		case LLQtWebKit::KEY_F6:				key = Qt::Key_F6;			break;
		case LLQtWebKit::KEY_F7:				key = Qt::Key_F7;			break;
		case LLQtWebKit::KEY_F8:				key = Qt::Key_F8;			break;
		case LLQtWebKit::KEY_F9:				key = Qt::Key_F9;			break;
		case LLQtWebKit::KEY_F10:				key = Qt::Key_F10;			break;
		case LLQtWebKit::KEY_F11:				key = Qt::Key_F11;			break;
		case LLQtWebKit::KEY_F12:				key = Qt::Key_F12;			break;

		case LLQtWebKit::KEY_PAD_UP:			key = Qt::Key_Up;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_DOWN:			key = Qt::Key_Down;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_LEFT:			key = Qt::Key_Left;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_RIGHT:			key = Qt::Key_Right;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_HOME:			key = Qt::Key_Home;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_END:			key = Qt::Key_End;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_PGUP:			key = Qt::Key_PageUp;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_PGDN:			key = Qt::Key_PageDown;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_CENTER:		key = Qt::Key_5;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_INS:			key = Qt::Key_Insert;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_DEL:			key = Qt::Key_Delete;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_RETURN:		key = Qt::Key_Enter;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_ADD:			key = Qt::Key_Plus;			qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_SUBTRACT:		key = Qt::Key_Minus;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_MULTIPLY:		key = Qt::Key_Asterisk;		qt_modifiers |= Qt::KeypadModifier; 	break;
		case LLQtWebKit::KEY_PAD_DIVIDE:		key = Qt::Key_Slash;		qt_modifiers |= Qt::KeypadModifier; 	break;
		
		case LLQtWebKit::KEY_NONE:			key = Qt::Key_unknown;		break;
		
		default:
			key = (Qt::Key)toupper(key_code);
		break;
    }


	QKeyEvent *event = 
		QKeyEvent::createExtendedKeyEvent(
			type, 
			key, 
			qt_modifiers,
			native_scan_code, 
			native_virtual_key, 
			native_modifiers,
			text,
			auto_repeat,
			text.count());

    qApp->sendEvent(d->mGraphicsScene, event);
	
	delete event;
}


// give focus to the browser so that input keyboard events work
void LLEmbeddedBrowserWindow::focusBrowser(bool focus_browser)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << focus_browser;
#endif
    QEvent ev(QEvent::WindowActivate);
    qApp->sendEvent(d->mGraphicsScene, &ev);

    QEvent ev2(QEvent::ActivationChange);
    qApp->sendEvent(d->mGraphicsScene, &ev2);

    QFocusEvent event(focus_browser ? QEvent::FocusIn : QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    qApp->sendEvent(d->mPage, &event);
}

void LLEmbeddedBrowserWindow::setWindowId(int window_id)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << window_id;
#endif
    d->mWindowId = window_id;
}

int LLEmbeddedBrowserWindow::getWindowId()
{
    return d->mWindowId;
}

std::string LLEmbeddedBrowserWindow::evaluateJavascript(std::string script)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << QString::fromStdString(script);
#endif
    QString q_script = QString::fromStdString(script);
    QString result = d->mPage->mainFrame()->evaluateJavaScript(q_script).toString();
    return result.toStdString();
}

void LLEmbeddedBrowserWindow::setWindowOpenBehavior(LLQtWebKit::WindowOpenBehavior behavior)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << window_id;
#endif
    d->mPage->setWindowOpenBehavior(behavior);
}

bool LLEmbeddedBrowserWindow::set404RedirectUrl(std::string redirect_url)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << QString::fromStdString(redirect_url);
#endif
    d->m404RedirectUrl = redirect_url;
    return true;
}

void LLEmbeddedBrowserWindow::setHostLanguage(const std::string host_language)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << QString::fromStdString(host_language);
#endif
	if ( d )
		if ( d->mPage )
    		d->mPage->setHostLanguage( host_language );
}

bool LLEmbeddedBrowserWindow::clr404RedirectUrl()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    d->m404RedirectUrl = std::string();
    return true;
}

void LLEmbeddedBrowserWindow::load404RedirectUrl()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    if ( ! d->m404RedirectUrl.empty() )
    {
        QUrl url = QUrl(QString::fromStdString(d->m404RedirectUrl));
    	d->mPage->triggerAction(QWebPage::Stop);
        d->mPage->mainFrame()->load(url);
    }
}

void LLEmbeddedBrowserWindow::setNoFollowScheme(std::string scheme)
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__ << QString::fromStdString(scheme);
#endif
    d->mNoFollowScheme = QString::fromStdString(scheme);
    // The scheme part of the url is what is before '://'
    d->mNoFollowScheme = d->mNoFollowScheme.mid(0, d->mNoFollowScheme.indexOf("://"));
}

std::string LLEmbeddedBrowserWindow::getNoFollowScheme()
{
#ifdef LLEMBEDDEDBROWSER_DEBUG
    qDebug() << "LLEmbeddedBrowserWindow" << __FUNCTION__;
#endif
    return d->mNoFollowScheme.toStdString();
}

void LLEmbeddedBrowserWindow::setExternalTargetName(std::string name)
{
    d->mExternalTargetName = name;
}

std::string LLEmbeddedBrowserWindow::getExternalTargetName()
{
    return d->mExternalTargetName;
}

void LLEmbeddedBrowserWindow::setBlankTargetName(std::string name)
{
    d->mBlankTargetName = name;
}

std::string LLEmbeddedBrowserWindow::getBlankTargetName()
{
    return d->mBlankTargetName;
}

void LLEmbeddedBrowserWindow::prependHistoryUrl(std::string url)
{
#ifdef WEBHISTORYPATCH
    // *HACK: we only have a URL here, we set a "" title and "current time" as
    // last visited time.
    d->mPage->history()->prependItem(QString::fromStdString(url),
									 QString::fromAscii(""),
									 QDateTime::currentDateTime());
#else
    Q_UNUSED(url);
#endif
}

void LLEmbeddedBrowserWindow::clearHistory()
{
	d->mPage->history()->clear();
}

std::string LLEmbeddedBrowserWindow::dumpHistory()
{
	std::ostringstream oss;
	const QList<QWebHistoryItem> &items = d->mPage->history()->backItems(9999);
	oss << "cur: " << d->mPage->history()->currentItemIndex() << ":"
	<< d->mPage->history()->currentItem().url().toString().toAscii().data() << "\n";
	for (int i=0; i< items.count(); i++) {
		oss << items[i].url().toString().toAscii().data() << "\n";
	}
	return oss.str();
}

LLGraphicsScene::LLGraphicsScene()
    : QGraphicsScene()
    , window(0)
{
    connect(this, SIGNAL(changed(const QList<QRectF> &)),
            this, SLOT(repaintRequestedSlot(const QList<QRectF> &)));
}

void LLGraphicsScene::repaintRequestedSlot(const QList<QRectF> &regions)
{
    if (!window)
        return;
    window->d->mDirty = true;
    for (int i = 0; i < regions.count(); ++i) {
        LLEmbeddedBrowserWindowEvent event(window->getWindowId(), window->getCurrentUri(),
                regions[i].x(), regions[i].y(), regions[i].width(), regions[i].height());

        window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onPageChanged, event);
    }
}

#include <qdebug.h>
#include <qcursor.h>
LLWebView::LLWebView(QGraphicsItem *parent)
    : QGraphicsWebView(parent)
    , window(0)
{
}

bool LLWebView::event(QEvent* event)
{
    if (window && event->type() == QEvent::CursorChange) {
        QCursor cursor = this->cursor();
        if (currentShape != cursor.shape()) {
            currentShape = cursor.shape();
            LLQtWebKit::ECursor llcursor;
            switch(currentShape)
            {
                case Qt::ArrowCursor:
                    llcursor = LLQtWebKit::C_ARROW;
                break;
                case Qt::PointingHandCursor:
                    llcursor = LLQtWebKit::C_POINTINGHAND;
                break;
                case Qt::IBeamCursor:
                    llcursor = LLQtWebKit::C_IBEAM;
                break;
                case Qt::SplitVCursor:
                    llcursor = LLQtWebKit::C_SPLITV;
                break;
                case Qt::SplitHCursor:
                    llcursor = LLQtWebKit::C_SPLITH;
                break;
                default:
                    qWarning() << "Unhandled cursor shape:" << currentShape;
                    llcursor = LLQtWebKit::C_ARROW;
            }

            LLEmbeddedBrowserWindowEvent llevent(
                    window->getWindowId(),
                    window->getCurrentUri(),
                    ((int)llcursor));
            window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onCursorChanged, llevent);
        }
    }
    return QGraphicsWebView::event(event);
}
