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
#include <llembeddedbrowser.h>
#include <llembeddedbrowserwindow.h>

class tst_LLEmbeddedBrowser : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void llembeddedbrowser_data();
    void llembeddedbrowser();

    void clearAllCookies();
    void clearCache_data();
    void clearCache();
    void clearLastError_data();
    void clearLastError();
    void createBrowserWindow_data();
    void createBrowserWindow();
    void destroyBrowserWindow();
    void enableCookies_data();
    void enableCookies();
    void enablePlugins_data();
    void enablePlugins();
    void enableProxy_data();
    void enableProxy();
    void getGREVersion_data();
    void getGREVersion();
    void getInstance();
    void getLastError_data();
    void getLastError();
    void initBrowser_data();
    void initBrowser(); //change function init as initbrowser
    void reset();
    void setBrowserAgentId_data();
    void setBrowserAgentId();
    void setLastError_data();
    void setLastError();
};

// Subclass that exposes the protected functions.
class SubLLEmbeddedBrowser : public LLEmbeddedBrowser
{
public:

};

// This will be called before the first test function is executed.
// It is only called once.
void tst_LLEmbeddedBrowser::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_LLEmbeddedBrowser::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_LLEmbeddedBrowser::init()
{
}

// This will be called after every test function.
void tst_LLEmbeddedBrowser::cleanup()
{
}

void tst_LLEmbeddedBrowser::llembeddedbrowser_data()
{
}

void tst_LLEmbeddedBrowser::llembeddedbrowser()
{
    SubLLEmbeddedBrowser browser;
    QCOMPARE(browser.clearAllCookies(), false);
    QCOMPARE(browser.clearCache(), false);
    browser.clearLastError();
    QCOMPARE(browser.enableCookies(false), false);
    QCOMPARE(browser.enablePlugins(false), true);
    QCOMPARE(browser.enableProxy(false, std::string(""), -1), true);
    QCOMPARE(browser.getGREVersion(), std::string(QT_VERSION_STR));
    QVERIFY(browser.getInstance() != NULL);
    QCOMPARE(browser.getLastError(), 0);
    browser.setBrowserAgentId("uBrowser");
    browser.setLastError(-1);
    QCOMPARE(browser.reset(), true);
    browser.destroyBrowserWindow(0);
    browser.destroyBrowserWindow((LLEmbeddedBrowserWindow*)6);
    QCOMPARE(browser.getWindowCount(), 0);
    QCOMPARE(browser.init(std::string(""),std::string(""),std::string(""),0), true);
}

// public bool clearAllCookies()
void tst_LLEmbeddedBrowser::clearAllCookies()
{
    SubLLEmbeddedBrowser browser;

    QCOMPARE(browser.clearAllCookies(), false);
    browser.reset();
    QCOMPARE(browser.clearAllCookies(), true);
}

void tst_LLEmbeddedBrowser::clearCache_data()
{
    QTest::addColumn<bool>("clearCache");
#if QT_VERSION < 0x040500
    QTest::newRow("QTVersion < 4.5") << false;
#else
    QTest::newRow("QTVersion > 4.5") << true;
#endif
}

// public bool clearCache()
void tst_LLEmbeddedBrowser::clearCache()
{
    QFETCH(bool, clearCache);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    QCOMPARE(browser.clearCache(), clearCache);
}

void tst_LLEmbeddedBrowser::clearLastError_data()
{
    QTest::addColumn<int>("lastError");
    QTest::newRow("1") << 1;
}

// public void clearLastError()
void tst_LLEmbeddedBrowser::clearLastError()
{
    SubLLEmbeddedBrowser browser;
    QFETCH(int, lastError);

    browser.setLastError(lastError);
    browser.clearLastError();
    QCOMPARE(browser.getLastError(), 0);
}

void tst_LLEmbeddedBrowser::createBrowserWindow_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::newRow("0,0") << 0 << 0;
    QTest::newRow("800,600") << 800 << 600;
}

// public LLEmbeddedBrowserWindow* createBrowserWindow(int width, int height)
void tst_LLEmbeddedBrowser::createBrowserWindow()
{
    QFETCH(int, width);
    QFETCH(int, height);
    SubLLEmbeddedBrowser browser;

    LLEmbeddedBrowserWindow *window = browser.createBrowserWindow(width, height);
    QVERIFY(window);
    QCOMPARE(browser.getLastError(), 0);
    QCOMPARE(browser.getWindowCount(), 1);
    QCOMPARE(window->getBrowserWidth(), (int16_t)width);
    QCOMPARE(window->getBrowserHeight(), (int16_t)height);
}

// public bool destroyBrowserWindow(LLEmbeddedBrowserWindow* browser_window)
void tst_LLEmbeddedBrowser::destroyBrowserWindow()
{
    SubLLEmbeddedBrowser browser;
    browser.reset();
    LLEmbeddedBrowserWindow* browser_window = browser.createBrowserWindow(200, 100);
    if (browser_window)
    {
        QCOMPARE(browser.getWindowCount(), 1);
        browser.destroyBrowserWindow(browser_window);
        QCOMPARE(browser.getLastError(), 0);
        QCOMPARE(browser.getWindowCount(), 0);
    }

    browser_window = browser.createBrowserWindow(800, 600);
    if (browser_window)
    {
        QCOMPARE(browser.getWindowCount(), 1);
        browser.destroyBrowserWindow(browser_window);
        QCOMPARE(browser.getLastError(), 0);
        QCOMPARE(browser.getWindowCount(), 0);
    }
}

void tst_LLEmbeddedBrowser::enableCookies_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<bool>("enableCookies");
    QTest::newRow("disable") << false << false;
    QTest::newRow("enable") << true << false;
}

// public bool enableCookies(bool enabled)
void tst_LLEmbeddedBrowser::enableCookies()
{
    QFETCH(bool, enabled);
    QFETCH(bool, enableCookies);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    QCOMPARE(browser.enableCookies(enabled), enableCookies);
    // TODO check that cookies are not saved
}

void tst_LLEmbeddedBrowser::enablePlugins_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<bool>("enablePlugins");
    QTest::newRow("disable") << false << true;
    QTest::newRow("enable") << true << true;
}

// public bool enablePlugins(bool enabled)
void tst_LLEmbeddedBrowser::enablePlugins()
{
    QFETCH(bool, enabled);
    QFETCH(bool, enablePlugins);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    QCOMPARE(browser.enablePlugins(enabled), enablePlugins);
    // TODO check that plugins work/do not work
}

Q_DECLARE_METATYPE(std::string)
void tst_LLEmbeddedBrowser::enableProxy_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<std::string>("host_name");
    QTest::addColumn<int>("port");
    QTest::addColumn<bool>("enableProxy");
    QTest::newRow("null") << false << std::string() << 0 << true;
    QTest::newRow("valid") << true << std::string("wtfsurf.com") << 80 << true;
}

// public bool enableProxy(bool enabled, std::string host_name, int port)
void tst_LLEmbeddedBrowser::enableProxy()
{
    QFETCH(bool, enabled);
    QFETCH(std::string, host_name);
    QFETCH(int, port);
    QFETCH(bool, enableProxy);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    QCOMPARE(browser.enableProxy(enabled, host_name, port), enableProxy);
    // TODO need some proxy servers to test this
}

void tst_LLEmbeddedBrowser::getGREVersion_data()
{
    QTest::addColumn<std::string>("getGREVersion");
    QTest::newRow("valid") << std::string(QT_VERSION_STR);
}

// public std::string getGREVersion()
void tst_LLEmbeddedBrowser::getGREVersion()
{
    QFETCH(std::string, getGREVersion);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    QCOMPARE(browser.getGREVersion(), getGREVersion);
}

// public static LLEmbeddedBrowser* getInstance()
void tst_LLEmbeddedBrowser::getInstance()
{
    SubLLEmbeddedBrowser browser;
    QVERIFY(browser.getInstance() != NULL);
}

void tst_LLEmbeddedBrowser::getLastError_data()
{
    QTest::addColumn<int>("error");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
    QTest::newRow("100") << 100;
}

// public int getLastError()
void tst_LLEmbeddedBrowser::getLastError()
{
    QFETCH(int, error);
    SubLLEmbeddedBrowser browser;
    browser.setLastError(error);
    QCOMPARE(browser.getLastError(), error);
}

void tst_LLEmbeddedBrowser::initBrowser_data()
{
    QTest::addColumn<std::string>("application_directory");
    QTest::addColumn<std::string>("component_directory");
    QTest::addColumn<std::string>("profile_directory");
    QTest::addColumn<void *>("native_window_handleCount");
    QTest::addColumn<bool>("init");
    QTest::newRow("null") << std::string() << std::string() << std::string() << (void *)0 << true;
    QTest::newRow("valid") << std::string("/home/crystal/Settings/") << std::string() << std::string() << (void *)0 << true;
}
void tst_LLEmbeddedBrowser::initBrowser()
{
    QFETCH(std::string, application_directory);
    QFETCH(std::string, component_directory);
    QFETCH(std::string, profile_directory);
    QFETCH(void *, native_window_handleCount);
    SubLLEmbeddedBrowser browser;
    browser.init(application_directory,component_directory,profile_directory,native_window_handleCount);
    QCOMPARE(browser.getLastError(), 0);
}

// public bool reset()
void tst_LLEmbeddedBrowser::reset()
{
    SubLLEmbeddedBrowser browser;

    browser.setLastError(100);
    QCOMPARE(browser.getLastError(), 100);
    QVERIFY(browser.reset());
    QCOMPARE(browser.getLastError(), 0);
    // TODO what should reset really do?
}

void tst_LLEmbeddedBrowser::setBrowserAgentId_data()
{
    QTest::addColumn<std::string>("id");
    QTest::newRow("null") << std::string();
    QTest::newRow("valid") << std::string("uBrowser");

}

// public void setBrowserAgentId(std::string id)
void tst_LLEmbeddedBrowser::setBrowserAgentId()
{
    QFETCH(std::string, id);

    SubLLEmbeddedBrowser browser;
    browser.reset();
    browser.setBrowserAgentId(id);
    LLEmbeddedBrowserWindow *window = browser.createBrowserWindow(0, 0);
    Q_UNUSED(window);
    // TODO confirm that the page is actually sending the agent ID
}

void tst_LLEmbeddedBrowser::setLastError_data()
{
    QTest::addColumn<int>("error_number");
    QTest::newRow("0") << 0;
    QTest::newRow("-1") << -1;
    QTest::newRow("100") << 100;
}

// public void setLastError(int error_number)
void tst_LLEmbeddedBrowser::setLastError()
{
    QFETCH(int, error_number);

    SubLLEmbeddedBrowser browser;

    browser.setLastError(error_number);
    QCOMPARE(browser.getLastError(), error_number);
}

QTEST_MAIN(tst_LLEmbeddedBrowser)
#include "tst_llembeddedbrowser.moc"

