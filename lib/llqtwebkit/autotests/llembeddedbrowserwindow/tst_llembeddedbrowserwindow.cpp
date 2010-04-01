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

#include <QtTest/QtTest>

#include "llembeddedbrowserwindow.h"
#include "llembeddedbrowser.h"

#ifndef QTRY_COMPARE

#define __TRY_TIMEOUT__ 10000
#define __TRY_STEP__    50

#define __QTRY(__expression__, __functionToCall__) \
    do { \
        int __i = 0; \
        while (!(__expression__) &&  __i < __TRY_TIMEOUT__) { \
            QTest::qWait(__TRY_STEP__); \
            __i += __TRY_STEP__; \
        } \
        __functionToCall__; \
    } while(0)

#define QTRY_COMPARE(__expression__, __expected__) \
    __QTRY((__expression__ == __expected__), QCOMPARE(__expression__, __expected__));

#define QTRY_VERIFY(__expression__) \
    __QTRY(__expression__, QVERIFY(__expression__));

#endif // QTRY_COMPARE

class tst_LLEmbeddedBrowserWindow : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void llembeddedbrowserwindow_data();
    void llembeddedbrowserwindow();

    void addObserver_data();
    void addObserver();
    void canNavigateBack_data();
    void canNavigateBack();
    void canNavigateForward_data();
    void canNavigateForward();
    void clr404RedirectUrl_data();
    void clr404RedirectUrl();
    void evaluateJavascript_data();
    void evaluateJavascript();
    void flipWindow_data();
    void flipWindow();
    void focusBrowser_data();
    void focusBrowser();
    void getBrowserDepth_data();
    void getBrowserDepth();
    void getBrowserHeight_data();
    void getBrowserHeight();
    void getBrowserRowSpan_data();
    void getBrowserRowSpan();
    void getBrowserWidth_data();
    void getBrowserWidth();
    void getClickLinkHref_data();
    void getClickLinkHref();
    void getClickLinkTarget_data();
    void getClickLinkTarget();
    void getCurrentUri_data();
    void getCurrentUri();
    void getNoFollowScheme_data();
    void getNoFollowScheme();
    void getPageBuffer_data();
    void getPageBuffer();
    void getPercentComplete_data();
    void getPercentComplete();
    void getStatusMsg_data();
    void getStatusMsg();
    void getWindowId_data();
    void getWindowId();
    void grabWindow_data();
    void grabWindow();
    void keyPress_data();
    void keyPress();
    void load404RedirectUrl_data();
    void load404RedirectUrl();
    void mouseDown_data();
    void mouseDown();
    void mouseLeftDoubleClick_data();
    void mouseLeftDoubleClick();
    void mouseMove_data();
    void mouseMove();
    void mouseUp_data();
    void mouseUp();
    void navigateBack_data();
    void navigateBack();
    void navigateForward_data();
    void navigateForward();
    void navigateReload_data();
    void navigateReload();
    void navigateStop_data();
    void navigateStop();
    void navigateTo_data();
    void navigateTo();
    void remObserver_data();
    void remObserver();
    void scrollByLines_data();
    void scrollByLines();
    void set404RedirectUrl_data();
    void set404RedirectUrl();
    void setBackgroundColor_data();
    void setBackgroundColor();
    void setCaretColor_data();
    void setCaretColor();
    void setEnabled_data();
    void setEnabled();
    void setNoFollowScheme_data();
    void setNoFollowScheme();
    void setParent_data();
    void setParent();
    void setSize_data();
    void setSize();
    void setWindowId_data();
    void setWindowId();
    void unicodeInput_data();
    void unicodeInput();
};

// Subclass that exposes the protected functions.
class SubLLEmbeddedBrowserWindow : public LLEmbeddedBrowserWindow
{
public:

};

// This will be called before the first test function is executed.
// It is only called once.
void tst_LLEmbeddedBrowserWindow::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_LLEmbeddedBrowserWindow::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_LLEmbeddedBrowserWindow::init()
{
}

// This will be called after every test function.
void tst_LLEmbeddedBrowserWindow::cleanup()
{
}

void tst_LLEmbeddedBrowserWindow::llembeddedbrowserwindow_data()
{
}

void tst_LLEmbeddedBrowserWindow::llembeddedbrowserwindow()
{
    SubLLEmbeddedBrowserWindow window;
    QCOMPARE(window.addObserver((LLEmbeddedBrowserWindowObserver*)0), false);
    QCOMPARE(window.canNavigateBack(), false);
    QCOMPARE(window.canNavigateForward(), false);
    QCOMPARE(window.clr404RedirectUrl(), true);
    QCOMPARE(window.evaluateJavascript(std::string()), std::string());
    QCOMPARE(window.flipWindow(false), true);
    window.focusBrowser(false);
    QCOMPARE(window.getBrowserDepth(), (int16_t)4);
    QCOMPARE(window.getBrowserHeight(), (int16_t)0);
    QCOMPARE(window.getBrowserRowSpan(), (int32_t)0);
    QCOMPARE(window.getBrowserWidth(), (int16_t)0);
    QCOMPARE(window.getClickLinkHref(), std::string());
    QCOMPARE(window.getClickLinkTarget(), std::string());
    QCOMPARE(window.getCurrentUri(), std::string());
    QCOMPARE(window.getNoFollowScheme(), std::string("secondlife"));
    QCOMPARE(window.getPageBuffer(), (unsigned char*)0);
    QCOMPARE(window.getPercentComplete(), (int16_t)0);
    QCOMPARE(window.getStatusMsg(), std::string());
    QCOMPARE(window.getWindowId(), -1);
    QCOMPARE(window.grabWindow(-1, -1, -1, -1), (unsigned char*)0);
    window.keyPress(0);
    window.load404RedirectUrl();
    window.mouseDown(0, 0);
    window.mouseLeftDoubleClick(0, 0);
    window.mouseMove(0, 0);
    window.mouseUp(0, 0);
    window.navigateBack();
    window.navigateForward();
    window.navigateReload();
    window.navigateStop();
    QCOMPARE(window.navigateTo(std::string()), true);
    QCOMPARE(window.remObserver((LLEmbeddedBrowserWindowObserver*)0), false);
    window.scrollByLines(0);
    QCOMPARE(window.set404RedirectUrl(std::string()), true);
    window.setBackgroundColor(0, 0, 0);
    window.setCaretColor(0, 0, 0);
    window.setEnabled(false);
    window.setNoFollowScheme(std::string());
    window.setParent((LLEmbeddedBrowser*)0);
    QCOMPARE(window.setSize(0, 0), true);
    window.setWindowId(-1);
    window.unicodeInput((uint32_t)0);
}

void tst_LLEmbeddedBrowserWindow::addObserver_data()
{
#if 0
    QTest::addColumn<int>("observerCount");
    QTest::addColumn<bool>("addObserver");
    QTest::newRow("null") << 0 << false;
#endif
}

// public bool addObserver(LLEmbeddedBrowserWindowObserver* observer)
void tst_LLEmbeddedBrowserWindow::addObserver()
{
#if 0
    QFETCH(int, observerCount);
    QFETCH(bool, addObserver);

    SubLLEmbeddedBrowserWindow window;

    QCOMPARE(window.addObserver(observer), addObserver);
#endif
    QSKIP("Test is same with remObserver.", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::canNavigateBack_data()
{
#if 0
    QTest::addColumn<bool>("canNavigateBack");
    QTest::newRow("true") << true;
    QTest::newRow("false") << false;
#endif
}

// public bool canNavigateBack()
void tst_LLEmbeddedBrowserWindow::canNavigateBack()
{
    //QFETCH(bool, canNavigateForward);

    SubLLEmbeddedBrowserWindow window;
    window.setSize(800,600);
    window.setParent(new LLEmbeddedBrowser());
    QCOMPARE(window.canNavigateForward(), false);
    window.navigateTo(std::string("http://www.google.com"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateBack(), false);
    window.navigateTo(std::string("http://www.cnn.com"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateBack(), true);
    window.navigateBack();
    QTRY_COMPARE(window.canNavigateForward(), true);
    window.navigateForward();
    QTRY_COMPARE(window.canNavigateBack(), true);
}

void tst_LLEmbeddedBrowserWindow::canNavigateForward_data()
{
#if 0
    QTest::addColumn<bool>("canNavigateForward");
    QTest::newRow("true") << true;
    QTest::newRow("false") << false;
#endif
}

// public bool canNavigateForward()
void tst_LLEmbeddedBrowserWindow::canNavigateForward()
{
    QSKIP("Test is same with canNavigateBack().", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::clr404RedirectUrl_data()
{
    QTest::addColumn<bool>("clr404RedirectUrl");
    QTest::newRow("true") << true;
}

// public bool clr404RedirectUrl()
void tst_LLEmbeddedBrowserWindow::clr404RedirectUrl()
{
    QFETCH(bool, clr404RedirectUrl);

    SubLLEmbeddedBrowserWindow window;
    window.set404RedirectUrl(std::string("http://www.google.com"));
    window.clr404RedirectUrl();
    window.load404RedirectUrl();
    window.getCurrentUri();

    QCOMPARE(window.getCurrentUri(), std::string());
}

Q_DECLARE_METATYPE(std::string)
void tst_LLEmbeddedBrowserWindow::evaluateJavascript_data()
{
    QTest::addColumn<std::string>("script");
    QTest::addColumn<std::string>("evaluateJavascript");
    QTest::newRow("null") << std::string() << std::string();
    //QTest::newRow("valid") << std::string("alert(\"hey!\")") << std::string("alert(\"hey!\")");
}

// public std::string evaluateJavascript(std::string script)
void tst_LLEmbeddedBrowserWindow::evaluateJavascript()
{
    QFETCH(std::string, script);
    QFETCH(std::string, evaluateJavascript);

    SubLLEmbeddedBrowserWindow window;

    window.evaluateJavascript(script);
}

void tst_LLEmbeddedBrowserWindow::flipWindow_data()
{
    QTest::addColumn<bool>("flip");
    QTest::addColumn<bool>("flipWindow");
    QTest::newRow("false") << false << true;
    QTest::newRow("true") << true << true;
}

// public bool flipWindow(bool flip)
void tst_LLEmbeddedBrowserWindow::flipWindow()
{
    QFETCH(bool, flip);
    QFETCH(bool, flipWindow);

    SubLLEmbeddedBrowserWindow window;

    QCOMPARE(window.flipWindow(flip), flipWindow);
}

void tst_LLEmbeddedBrowserWindow::focusBrowser_data()
{
    QTest::addColumn<bool>("focus_browser");
    QTest::newRow("true") << true;
    QTest::newRow("false") << false;
}

// public void focusBrowser(bool focus_browser)
void tst_LLEmbeddedBrowserWindow::focusBrowser()
{
    QFETCH(bool, focus_browser);

    SubLLEmbeddedBrowserWindow window;
    window.focusBrowser(focus_browser);
}

Q_DECLARE_METATYPE(int16_t)
void tst_LLEmbeddedBrowserWindow::getBrowserDepth_data()
{
#if 0
    QTest::addColumn<int16_t>("getBrowserDepth");
    QTest::newRow("null") << int16_t();
#endif
}

// public int16_t getBrowserDepth()
void tst_LLEmbeddedBrowserWindow::getBrowserDepth()
{
    //QFETCH(int16_t, getBrowserDepth);

    SubLLEmbeddedBrowserWindow window;

    QCOMPARE(window.getBrowserDepth(), int16_t(4));
}

void tst_LLEmbeddedBrowserWindow::getBrowserHeight_data()
{
#if 0
    QTest::addColumn<int16_t>("getBrowserHeight");
    QTest::newRow("null") << int16_t();
#endif
}

// public int16_t getBrowserHeight()
void tst_LLEmbeddedBrowserWindow::getBrowserHeight()
{
#if 0
    QFETCH(int16_t, getBrowserHeight);

    SubLLEmbeddedBrowserWindow window;

    QCOMPARE(window.getBrowserHeight(), getBrowserHeight);
#endif
    QSKIP("Test is same with setSize().", SkipAll);
}

Q_DECLARE_METATYPE(int32_t)
void tst_LLEmbeddedBrowserWindow::getBrowserRowSpan_data()
{
#if 0
    QTest::addColumn<int32_t>("getBrowserRowSpan");
    QTest::newRow("null") << int32_t();
#endif
}

// public int32_t getBrowserRowSpan()
void tst_LLEmbeddedBrowserWindow::getBrowserRowSpan()
{
#if 0
    SubLLEmbeddedBrowserWindow window;
    window.setSize(0, 0);

    QCOMPARE(window.getBrowserWidth(), int16_t(0));
    QCOMPARE(window.getBrowserRowSpan(), int32_t(0));
    window.setSize(100, 100);

    QCOMPARE(window.getBrowserWidth(), int16_t(100));
    QCOMPARE(window.getBrowserRowSpan(), int32_t(400));
#endif
    QSKIP("Test is same with setSize().", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::getBrowserWidth_data()
{
#if 0
    QTest::addColumn<int16_t>("getBrowserWidth");
    QTest::newRow("null") << int16_t();
#endif
}

// public int16_t getBrowserWidth()
void tst_LLEmbeddedBrowserWindow::getBrowserWidth()
{
#if 0
    //QFETCH(int16_t, getBrowserWidth);

    SubLLEmbeddedBrowserWindow window;
    window.setSize(0, 0);

    QCOMPARE(window.getBrowserWidth(), int16_t(0));
    QCOMPARE(window.getBrowserHeight(), int16_t(0));
    window.setSize(100, 100);

    QCOMPARE(window.getBrowserWidth(), int16_t(100));
    QCOMPARE(window.getBrowserHeight(), int16_t(100));
#endif
    QSKIP("Test is same with setSize().", SkipAll);
}

//Q_DECLARE_METATYPE(std::string const)
void tst_LLEmbeddedBrowserWindow::getClickLinkHref_data()
{
#if 0
    QTest::addColumn<std::string const>("getClickLinkHref");
    QTest::newRow("null") << std::string const();
#endif
}

// public std::string const getClickLinkHref()
void tst_LLEmbeddedBrowserWindow::getClickLinkHref()
{
    //QFETCH(std::string const, getClickLinkHref);

    SubLLEmbeddedBrowserWindow window;

    window.getClickLinkHref();
}

void tst_LLEmbeddedBrowserWindow::getClickLinkTarget_data()
{
#if 0
    QTest::addColumn<std::string const>("getClickLinkTarget");
    QTest::newRow("null") << std::string const();
#endif
}

// public std::string const getClickLinkTarget()
void tst_LLEmbeddedBrowserWindow::getClickLinkTarget()
{
    //QFETCH(std::string const, getClickLinkTarget);

    SubLLEmbeddedBrowserWindow window;

    window.getClickLinkTarget();
}

void tst_LLEmbeddedBrowserWindow::getCurrentUri_data()
{
#if 0
    QTest::addColumn<std::string const>("getCurrentUri");
    QTest::newRow("null") << std::string const();
#endif
}

// public std::string const getCurrentUri()
void tst_LLEmbeddedBrowserWindow::getCurrentUri()
{
    //QFETCH(std::string const, getCurrentUri);

    SubLLEmbeddedBrowserWindow window;
    window.navigateTo(std::string("http://www.google.ca/"));
    QTRY_COMPARE(QString::fromStdString(window.getCurrentUri()), QString::fromStdString(std::string("http://www.google.ca/")));
}

void tst_LLEmbeddedBrowserWindow::getNoFollowScheme_data()
{
#if 0
    QTest::addColumn<std::string>("getNoFollowScheme");
    QTest::newRow("FTP") << std::string("FTP");
#endif
}

// public std::string getNoFollowScheme()
void tst_LLEmbeddedBrowserWindow::getNoFollowScheme()
{
    //QFETCH(std::string, getNoFollowScheme);

    SubLLEmbeddedBrowserWindow window;
    window.setNoFollowScheme("FTP://www.google.com");

    QCOMPARE(window.getNoFollowScheme(), std::string("FTP"));
}

//Q_DECLARE_METATYPE(unsigned char*)
void tst_LLEmbeddedBrowserWindow::getPageBuffer_data()
{
#if 0
    QTest::addColumn<unsigned char*>("getPageBuffer");
    QTest::newRow("null") << unsigned char*();
#endif
}

// public unsigned char* getPageBuffer()
void tst_LLEmbeddedBrowserWindow::getPageBuffer()
{
    //QFETCH(unsigned char*, getPageBuffer);

    SubLLEmbeddedBrowserWindow window;
    window.setSize(100,100);
    window.grabWindow(0, 0, 100, 100);

    QVERIFY(window.getPageBuffer() != NULL);
}

//Q_DECLARE_METATYPE(int16_t const)
void tst_LLEmbeddedBrowserWindow::getPercentComplete_data()
{
#if 0
    QTest::addColumn<int16_t const>("getPercentComplete");
    QTest::newRow("null") << int16_t const();
#endif
}

// public int16_t const getPercentComplete()
void tst_LLEmbeddedBrowserWindow::getPercentComplete()
{
    //QFETCH(int16_t const, getPercentComplete);
    SubLLEmbeddedBrowserWindow window;
    window.navigateTo(std::string("http://www.google.com"));
    QTest::qWait(1000);
    QVERIFY(window.getPercentComplete() > 0);
}

void tst_LLEmbeddedBrowserWindow::getStatusMsg_data()
{
#if 0
    QTest::addColumn<std::string const>("getStatusMsg");
    QTest::newRow("null") << std::string const();
#endif
}

// public std::string const getStatusMsg()
void tst_LLEmbeddedBrowserWindow::getStatusMsg()
{
    //QFETCH(std::string const, getStatusMsg);

    SubLLEmbeddedBrowserWindow window;
    window.navigateTo(std::string("http://www.google.com"));
    QTest::qWait(1000);
    window.navigateStop();
    window.navigateTo(std::string("http://www.yahoo.com"));
    // Seems status msg will always be null during navigating.
    //QTRY_VERIFY(QString::fromStdString(window.getStatusMsg())!= NULL);
    QSKIP("Status msg will always be null during navigating", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::getWindowId_data()
{
#if 0
    QTest::addColumn<int>("getWindowId");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public int getWindowId()
void tst_LLEmbeddedBrowserWindow::getWindowId()
{
    //QFETCH(int, getWindowId);

    SubLLEmbeddedBrowserWindow window;
    window.setWindowId(0);
    QCOMPARE(window.getWindowId(), 0);
    window.setWindowId(100);
    QCOMPARE(window.getWindowId(), 100);
}

void tst_LLEmbeddedBrowserWindow::grabWindow_data()
{
#if 0
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<unsigned char*>("grabWindow");
    QTest::newRow("null") << 0 << 0 << 0 << 0 << 0;
#endif
}

// public unsigned char* grabWindow(int x, int y, int width, int height)
void tst_LLEmbeddedBrowserWindow::grabWindow()
{
    QSKIP("Test is same with getPageBuffer().", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::keyPress_data()
{
    QTest::addColumn<int16_t>("key_code");
    QTest::newRow("null") << int16_t(0);
    QTest::newRow("valid") << int16_t(0x0E);
}

// public void keyPress(int16_t key_code)
void tst_LLEmbeddedBrowserWindow::keyPress()
{
    QFETCH(int16_t, key_code);

    SubLLEmbeddedBrowserWindow window;
    window.keyPress(key_code);
}

void tst_LLEmbeddedBrowserWindow::load404RedirectUrl_data()
{
#if 0
    QTest::addColumn<int>("foo");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void load404RedirectUrl()
void tst_LLEmbeddedBrowserWindow::load404RedirectUrl()
{
    //QFETCH(int, foo);
    SubLLEmbeddedBrowserWindow window;
    window.set404RedirectUrl("http://www.google.ca/");
    window.load404RedirectUrl();
    QTRY_COMPARE(QString::fromStdString(window.getCurrentUri()), QString("http://www.google.ca/"));
}

void tst_LLEmbeddedBrowserWindow::mouseDown_data()
{
    QTest::addColumn<int16_t>("x");
    QTest::addColumn<int16_t>("y");
    QTest::newRow("0") << int16_t(0) << int16_t(0);
    QTest::newRow("bignumber") << int16_t(100000) << int16_t(100000);
    QTest::newRow("valid") << int16_t(100) << int16_t(100);
}

// public void mouseDown(int16_t x, int16_t y)
void tst_LLEmbeddedBrowserWindow::mouseDown()
{
    QFETCH(int16_t, x);
    QFETCH(int16_t, y);

    SubLLEmbeddedBrowserWindow window;
    window.mouseDown(x, y);
}

void tst_LLEmbeddedBrowserWindow::mouseLeftDoubleClick_data()
{
    QTest::addColumn<int16_t>("x");
    QTest::addColumn<int16_t>("y");
    QTest::newRow("0") << int16_t(0) << int16_t(0);
    QTest::newRow("bignumber") << int16_t(100000) << int16_t(100000);
    QTest::newRow("valid") << int16_t(100) << int16_t(100);
}

// public void mouseLeftDoubleClick(int16_t x, int16_t y)
void tst_LLEmbeddedBrowserWindow::mouseLeftDoubleClick()
{
    QFETCH(int16_t, x);
    QFETCH(int16_t, y);

    SubLLEmbeddedBrowserWindow window;
    window.mouseLeftDoubleClick(x, y);
}

void tst_LLEmbeddedBrowserWindow::mouseMove_data()
{
    QTest::addColumn<int16_t>("x");
    QTest::addColumn<int16_t>("y");
    QTest::newRow("0") << int16_t(0) << int16_t(0);
    QTest::newRow("bignumber") << int16_t(100000) << int16_t(100000);
    QTest::newRow("valid") << int16_t(100) << int16_t(100);
}

// public void mouseMove(int16_t x, int16_t y)
void tst_LLEmbeddedBrowserWindow::mouseMove()
{
    QFETCH(int16_t, x);
    QFETCH(int16_t, y);

    SubLLEmbeddedBrowserWindow window;
    window.mouseMove(x, y);
}

void tst_LLEmbeddedBrowserWindow::mouseUp_data()
{
    QTest::addColumn<int16_t>("x");
    QTest::addColumn<int16_t>("y");
    QTest::newRow("0") << int16_t(0) << int16_t(0);
    QTest::newRow("bignumber") << int16_t(100000) << int16_t(100000);
    QTest::newRow("valid") << int16_t(100) << int16_t(100);
}

// public void mouseUp(int16_t x, int16_t y)
void tst_LLEmbeddedBrowserWindow::mouseUp()
{
    QFETCH(int16_t, x);
    QFETCH(int16_t, y);

    SubLLEmbeddedBrowserWindow window;
    window.mouseUp(x, y);
}

void tst_LLEmbeddedBrowserWindow::navigateBack_data()
{
#if 0
    QTest::addColumn<int>("foo");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void navigateBack()
void tst_LLEmbeddedBrowserWindow::navigateBack()
{
    //QFETCH(int, foo);

    SubLLEmbeddedBrowserWindow window;
    window.navigateTo(std::string("http://www.google.ca/"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateForward(), false);
    window.navigateTo(std::string("http://www.yahoo.com/"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateBack(), true);
    window.navigateBack();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://www.google.ca/"));
    window.navigateBack();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://www.google.ca/"));
}

void tst_LLEmbeddedBrowserWindow::navigateForward_data()
{
#if 0
    QTest::addColumn<int>("foo");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void navigateForward()
void tst_LLEmbeddedBrowserWindow::navigateForward()
{
    //    QFETCH(int, foo);
    SubLLEmbeddedBrowserWindow window;
    window.navigateTo(std::string("http://www.google.ca/"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateForward(), false);
    window.navigateTo(std::string("http://www.yahoo.ca/"));
    QTest::qWait(__TRY_TIMEOUT__);
    QCOMPARE(window.canNavigateBack(), true);
    window.navigateBack();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://www.google.ca/"));
    window.navigateForward();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://ca.yahoo.com/"));
    window.navigateForward();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://ca.yahoo.com/"));
}

void tst_LLEmbeddedBrowserWindow::navigateReload_data()
{
#if 0
    QTest::addColumn<int>("foo");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void navigateReload()
void tst_LLEmbeddedBrowserWindow::navigateReload()
{
    SubLLEmbeddedBrowserWindow window;

    window.navigateTo(std::string("http://www.google.ca/"));
    QTest::qWait(__TRY_TIMEOUT__);
    window.navigateReload();
    QTRY_COMPARE(QString::fromStdString((window.getCurrentUri())), QString("http://www.google.ca/"));
}

void tst_LLEmbeddedBrowserWindow::navigateStop_data()
{
#if 0
    QTest::addColumn<int>("foo");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void navigateStop()
void tst_LLEmbeddedBrowserWindow::navigateStop()
{
    SubLLEmbeddedBrowserWindow window;
    window.navigateTo("www.google.com");
    window.navigateStop();
}

void tst_LLEmbeddedBrowserWindow::navigateTo_data()
{
    QTest::addColumn<std::string>("uri");
    QTest::addColumn<std::string>("navigateTo");
    QTest::newRow("null") << std::string() << std::string();
    QTest::newRow("valid") << std::string("http://www.google.ca/") << std::string("http://www.google.ca/");
}

// public bool navigateTo(std::string const uri)
void tst_LLEmbeddedBrowserWindow::navigateTo()
{
    QSKIP("Test is same with navigateBack(), navigateForward().", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::remObserver_data()
{
#if 0
    QTest::addColumn<int>("observerCount");
    QTest::addColumn<bool>("remObserver");
    QTest::newRow("null") << 0 << false;
#endif
}

// public bool remObserver(LLEmbeddedBrowserWindowObserver* observer)
void tst_LLEmbeddedBrowserWindow::remObserver()
{
//    QFETCH(int, observerCount);
//    QFETCH(bool, remObserver);

    SubLLEmbeddedBrowserWindow window;
    LLEmbeddedBrowserWindowObserver* observer = new LLEmbeddedBrowserWindowObserver();
    window.addObserver(observer);
    QCOMPARE(window.getObserverNumber(), 1);
    window.remObserver(observer);
    QCOMPARE(window.getObserverNumber(), 0);
}

void tst_LLEmbeddedBrowserWindow::scrollByLines_data()
{
    QTest::addColumn<int16_t>("lines");
    QTest::newRow("null") << int16_t(0);
    QTest::addColumn<int16_t>("lines");
    QTest::newRow("100") << int16_t(100);
}

// public void scrollByLines(int16_t lines)
void tst_LLEmbeddedBrowserWindow::scrollByLines()
{
    QFETCH(int16_t, lines);

    SubLLEmbeddedBrowserWindow window;

    window.scrollByLines(lines);
}

void tst_LLEmbeddedBrowserWindow::set404RedirectUrl_data()
{
    QTest::addColumn<std::string>("redirect_url");
    QTest::addColumn<bool>("set404RedirectUrl");
    QTest::newRow("null") << std::string() << true;
    QTest::newRow("valid") << std::string("http://www.google.ca/") << true;
}

// public bool set404RedirectUrl(std::string redirect_url)
void tst_LLEmbeddedBrowserWindow::set404RedirectUrl()
{
    QFETCH(std::string, redirect_url);
    QFETCH(bool, set404RedirectUrl);

    SubLLEmbeddedBrowserWindow window;
    window.set404RedirectUrl(redirect_url);
    window.load404RedirectUrl();
    QTRY_COMPARE(window.getCurrentUri(), redirect_url);
}

Q_DECLARE_METATYPE(uint8_t)
void tst_LLEmbeddedBrowserWindow::setBackgroundColor_data()
{
    QTest::addColumn<uint8_t>("red");
    QTest::addColumn<uint8_t>("green");
    QTest::addColumn<uint8_t>("blue");
    QTest::newRow("black") << uint8_t(0) << uint8_t(0) << uint8_t(0);
    QTest::newRow("red") << uint8_t(255) << uint8_t(0) << uint8_t(0);
    QTest::newRow("green") << uint8_t(0) << uint8_t(255) << uint8_t(0);
    QTest::newRow("blue") << uint8_t(0) << uint8_t(0) << uint8_t(255);
}

// public void setBackgroundColor(uint8_t const red, uint8_t const green, uint8_t const blue)
void tst_LLEmbeddedBrowserWindow::setBackgroundColor()
{
    QFETCH(uint8_t, red);
    QFETCH(uint8_t, green);
    QFETCH(uint8_t, blue);

    SubLLEmbeddedBrowserWindow window;

    window.setBackgroundColor(red, green, blue);
}

void tst_LLEmbeddedBrowserWindow::setCaretColor_data()
{
    QTest::addColumn<uint8_t>("red");
    QTest::addColumn<uint8_t>("green");
    QTest::addColumn<uint8_t>("blue");
    QTest::newRow("black") << uint8_t(0) << uint8_t(0) << uint8_t(0);
    QTest::newRow("red") << uint8_t(255) << uint8_t(0) << uint8_t(0);
    QTest::newRow("green") << uint8_t(0) << uint8_t(255) << uint8_t(0);
    QTest::newRow("blue") << uint8_t(0) << uint8_t(0) << uint8_t(255);
}

// public void setCaretColor(uint8_t const red, uint8_t const green, uint8_t const blue)
void tst_LLEmbeddedBrowserWindow::setCaretColor()
{
    QFETCH(uint8_t, red);
    QFETCH(uint8_t, green);
    QFETCH(uint8_t, blue);

    SubLLEmbeddedBrowserWindow window;

    window.setCaretColor(red, green, blue);
}

void tst_LLEmbeddedBrowserWindow::setEnabled_data()
{
    QTest::addColumn<bool>("enabledIn");
    QTest::newRow("true") << true;
    QTest::newRow("false") << false;
}

// public void setEnabled(bool enabledIn)
void tst_LLEmbeddedBrowserWindow::setEnabled()
{
    QFETCH(bool, enabledIn);

    SubLLEmbeddedBrowserWindow window;

    window.setEnabled(enabledIn);
}

void tst_LLEmbeddedBrowserWindow::setNoFollowScheme_data()
{
    QTest::addColumn<std::string>("scheme");
    QTest::addColumn<std::string>("result");
    QTest::newRow("null") << std::string() << std::string();
    QTest::newRow("valid") << std::string("ftp://www.google.com") << std::string("ftp");;
}

// public void setNoFollowScheme(std::string scheme)
void tst_LLEmbeddedBrowserWindow::setNoFollowScheme()
{
    QFETCH(std::string, scheme);
    QFETCH(std::string, result);

    SubLLEmbeddedBrowserWindow window;

    window.setNoFollowScheme(scheme);
    QCOMPARE(window.getNoFollowScheme(), result);
}

void tst_LLEmbeddedBrowserWindow::setParent_data()
{
#if 0
    QTest::addColumn<int>("parentCount");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
#endif
}

// public void setParent(LLEmbeddedBrowser* parent)
void tst_LLEmbeddedBrowserWindow::setParent()
{
#if 0
    QFETCH(int, parentCount);

    SubLLEmbeddedBrowserWindow window;
    LLEmbeddedBrowser* parent = new LLEmbeddedBrowser();

    window.setParent(parent);
#endif
    QSKIP("Has been tested before.", SkipAll);
}

void tst_LLEmbeddedBrowserWindow::setSize_data()
{
    QTest::addColumn<int16_t>("width");
    QTest::addColumn<int16_t>("height");
    QTest::addColumn<bool>("setSize");
    QTest::newRow("null") << int16_t(0) << int16_t(0) << true;
    QTest::newRow("valid") << int16_t(100) << int16_t(200) << true;
}

// public bool setSize(int16_t width, int16_t height)
void tst_LLEmbeddedBrowserWindow::setSize()
{
    QFETCH(int16_t, width);
    QFETCH(int16_t, height);
    QFETCH(bool, setSize);

    SubLLEmbeddedBrowserWindow window;
    QCOMPARE(window.setSize(width, height), setSize);
    window.grabWindow(0, 0, 800, 600);

    QCOMPARE(window.getBrowserWidth(), width);
    QCOMPARE(window.getBrowserHeight(), height);
    QCOMPARE(window.getBrowserRowSpan(), (int32_t)width * 4);
}

void tst_LLEmbeddedBrowserWindow::setWindowId_data()
{
    QTest::addColumn<int>("window_id");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
    QTest::newRow("100") << 100;
}

// public void setWindowId(int window_id)
void tst_LLEmbeddedBrowserWindow::setWindowId()
{
    QFETCH(int, window_id);

    SubLLEmbeddedBrowserWindow window;

    window.setWindowId(window_id);
    QCOMPARE(window.getWindowId(), window_id);
}

Q_DECLARE_METATYPE(uint32_t)
void tst_LLEmbeddedBrowserWindow::unicodeInput_data()
{
    QTest::addColumn<uint32_t>("unicode_char");
    QTest::newRow("null") << uint32_t();
    QTest::newRow("valid") << uint32_t(54);
}

// public void unicodeInput(uint32_t unicode_char)
void tst_LLEmbeddedBrowserWindow::unicodeInput()
{
    QFETCH(uint32_t, unicode_char);

    SubLLEmbeddedBrowserWindow window;

    window.unicodeInput(unicode_char);
}

QTEST_MAIN(tst_LLEmbeddedBrowserWindow)
#include "tst_llembeddedbrowserwindow.moc"

