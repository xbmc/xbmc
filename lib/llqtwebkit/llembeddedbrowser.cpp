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

#include "llembeddedbrowser.h"

#include "llembeddedbrowser_p.h"
#include "llembeddedbrowserwindow.h"
#include "llnetworkaccessmanager.h"
#include "llstyle.h"

#include <qvariant.h>
#include <qwebsettings.h>
#include <qnetworkproxy.h>
#include <qfile.h>

// singleton pattern - initialization
LLEmbeddedBrowser* LLEmbeddedBrowser::sInstance = 0;

LLEmbeddedBrowserPrivate::LLEmbeddedBrowserPrivate()
    : mErrorNum(0)
    , mNativeWindowHandle(0)
    , mNetworkAccessManager(0)
    , mApplication(0)
#if QT_VERSION >= 0x040500
    , mDiskCache(0)
#endif
    , mNetworkCookieJar(0)
{
    if (!qApp)
    {
        static int argc = 0;
        static const char* argv[] = {""};
        mApplication = new QApplication(argc, (char **)argv);
        mApplication->addLibraryPath(qApp->applicationDirPath());
    }
    qApp->setStyle(new LLStyle());
    mNetworkAccessManager = new LLNetworkAccessManager(this);
#if LL_DARWIN
	// HACK: Qt installs CarbonEvent handlers that steal events from our main event loop.
	// This uninstalls them.
	// It's not clear whether calling this internal function is really a good idea.  It's probably not.
	// It does, however, seem to fix at least one problem ( https://jira.secondlife.com/browse/MOZ-12 ).
	extern void qt_release_app_proc_handler();
	qt_release_app_proc_handler();
#endif
}

LLEmbeddedBrowserPrivate::~LLEmbeddedBrowserPrivate()
{
    delete mApplication;
    delete mNetworkAccessManager;
    delete mNetworkCookieJar;
}



LLEmbeddedBrowser::LLEmbeddedBrowser()
    : d(new LLEmbeddedBrowserPrivate)
    , mHostLanguage( "en" )
{
}

LLEmbeddedBrowser::~LLEmbeddedBrowser()
{
    delete d;
}

LLEmbeddedBrowser* LLEmbeddedBrowser::getInstance()
{
    if (!sInstance)
        sInstance = new LLEmbeddedBrowser;
    return sInstance;
}

void LLEmbeddedBrowser::setLastError(int error_number)
{
    d->mErrorNum = error_number;
}

void LLEmbeddedBrowser::clearLastError()
{
    d->mErrorNum = 0x0000;
}

int LLEmbeddedBrowser::getLastError()
{
    return d->mErrorNum;
}

std::string LLEmbeddedBrowser::getGREVersion()
{
    // take the string directly from Qt
    return std::string(QT_VERSION_STR);
}

bool LLEmbeddedBrowser::init(std::string application_directory,
                             std::string component_directory,
                             std::string profile_directory,
                             void* native_window_handle)
{
    Q_UNUSED(application_directory);
    Q_UNUSED(component_directory);
    Q_UNUSED(native_window_handle);
    d->mStorageDirectory = QString::fromStdString(profile_directory);
    QWebSettings::setIconDatabasePath(d->mStorageDirectory);
	// The gif and jpeg libraries should be installed in component_directory/imageformats/
	QCoreApplication::addLibraryPath(QString::fromStdString(component_directory));

	// turn on plugins by default
	enablePlugins( true );

    // Until QtWebkit defaults to 16
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFontSize, 16);
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFixedFontSize, 16);

	// use default text encoding - not sure how this helps right now so commenting out until we
	// understand how to use it a little better.
    //QWebSettings::globalSettings()->setDefaultTextEncoding ( "" );

    return reset();
}

bool LLEmbeddedBrowser::reset()
{
    foreach(LLEmbeddedBrowserWindow *window, d->windows)
        delete window;
    d->windows.clear();
    delete d->mNetworkAccessManager;
    d->mNetworkAccessManager = new LLNetworkAccessManager(d);
#if QT_VERSION >= 0x040500
    d->mDiskCache = new QNetworkDiskCache(d->mNetworkAccessManager);
    d->mDiskCache->setCacheDirectory(d->mStorageDirectory + "/cache");
    if (QLatin1String(qVersion()) != QLatin1String("4.5.1"))
        d->mNetworkAccessManager->setCache(d->mDiskCache);
#endif
    d->mNetworkCookieJar = new LLNetworkCookieJar(d->mNetworkAccessManager, d->mStorageDirectory + "/cookies");
    d->mNetworkCookieJar->load();
    d->mNetworkAccessManager->setCookieJar(d->mNetworkCookieJar);
    clearLastError();
    return true;
}

bool LLEmbeddedBrowser::clearCache()
{
#if QT_VERSION >= 0x040500
    if (d->mDiskCache)
    {
        d->mDiskCache->clear();
        return true;
    }
#endif
    return false;
}

bool LLEmbeddedBrowser::enableProxy(bool enabled, std::string host_name, int port)
{
    QNetworkProxy proxy;
    if (enabled)
    {
        proxy.setType(QNetworkProxy::HttpProxy);
        QString q_host_name = QString::fromStdString(host_name);
        proxy.setHostName(q_host_name);
        proxy.setPort(port);
    }
    d->mNetworkAccessManager->setProxy(proxy);
    return true;
}

bool LLEmbeddedBrowser::enableCookies(bool enabled)
{
    if (!d->mNetworkCookieJar)
        return false;
    d->mNetworkCookieJar->mAllowCookies = enabled;
    return false;
}

bool LLEmbeddedBrowser::clearAllCookies()
{
    if (!d->mNetworkCookieJar)
        return false;
    d->mNetworkCookieJar->clear();
    return true;
}

bool LLEmbeddedBrowser::enablePlugins(bool enabled)
{
    QWebSettings *defaultSettings = QWebSettings::globalSettings();
    defaultSettings->setAttribute(QWebSettings::PluginsEnabled, enabled);
    return true;
}

bool LLEmbeddedBrowser::enableJavascript(bool enabled)
{
    QWebSettings *defaultSettings = QWebSettings::globalSettings();
    defaultSettings->setAttribute(QWebSettings::JavascriptEnabled, enabled);
    return true;
}

/*
	Sets a string that should be addded to the user agent to identify the application
*/
void LLEmbeddedBrowser::setBrowserAgentId(std::string id)
{
    QCoreApplication::setApplicationName(QString::fromStdString(id));
}

// updates value of 'hostLanguage' in JavaScript 'Navigator' obect that 
// embedded pages can query to see what language the host app is set to
// IMPORTANT: call this before any windows are created - only gets passed
//            to LLWebPage when new window is created
void LLEmbeddedBrowser::setHostLanguage( const std::string& host_language )
{
	mHostLanguage = host_language;
}

LLEmbeddedBrowserWindow* LLEmbeddedBrowser::createBrowserWindow(int width, int height)
{
    LLEmbeddedBrowserWindow *newWin = new LLEmbeddedBrowserWindow();
    if (newWin)
    {
        newWin->setSize(width, height);
        newWin->setParent(this);
        newWin->setHostLanguage(mHostLanguage);
        clearLastError();
        d->windows.append(newWin);
        return newWin;
    }
    return 0;
}

bool LLEmbeddedBrowser::destroyBrowserWindow(LLEmbeddedBrowserWindow* browser_window)
{
    // check if exists in windows list
    if (d->windows.removeOne(browser_window))
    {
        delete browser_window;
        clearLastError();
        return true;
    }
    return false;
}

int LLEmbeddedBrowser::getWindowCount() const
{
    return d->windows.size();
}

void LLEmbeddedBrowser::pump(int max_milliseconds)
{
#if 0
	// This USED to be necessary on the mac, but with Qt 4.6 it seems to cause trouble loading some pages,
	// and using processEvents() seems to work properly now.
	// Leaving this here in case these issues ever come back.

	// On the Mac, calling processEvents hangs the viewer.
	// I'm not entirely sure this does everything we need, but it seems to work better, and allows things like animated gifs to work.
	qApp->sendPostedEvents();
	qApp->sendPostedEvents(0, QEvent::DeferredDelete);
#else
	qApp->processEvents(QEventLoop::AllEvents, max_milliseconds);
#endif
}

LLNetworkCookieJar::LLNetworkCookieJar(QObject* parent, const QString& cookie_filename)
    : NetworkCookieJar(parent)
    , mCookieStorageFileName(cookie_filename)
    , mAllowCookies(true)
{
}

LLNetworkCookieJar::~LLNetworkCookieJar()
{
    save();
}

void LLNetworkCookieJar::load()
{
    QFile file(mCookieStorageFileName);
    if (!file.open(QFile::ReadOnly))
        return;
    QByteArray state = file.readAll();
    restoreState(state);
    file.close();
}

void LLNetworkCookieJar::save()
{
    if (mCookieStorageFileName.isEmpty())
        return;

    QFile file(mCookieStorageFileName);
    file.open(QFile::ReadWrite);
    QByteArray state = saveState();
    file.write(state);
    file.close();
}

QList<QNetworkCookie> LLNetworkCookieJar::cookiesForUrl(const QUrl& url) const
{
    if (!mAllowCookies)
        return QList<QNetworkCookie>();
    return NetworkCookieJar::cookiesForUrl(url);
}

bool LLNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookie_list, const QUrl& url)
{
    if (!mAllowCookies)
        return false;
    return NetworkCookieJar::setCookiesFromUrl(cookie_list, url);
}

void LLNetworkCookieJar::clear()
{
    setAllCookies(QList<QNetworkCookie>());
}

#include "llembeddedbrowserwindow_p.h"
#include <qnetworkreply.h>

QGraphicsWebView *LLEmbeddedBrowserPrivate::findView(QNetworkReply *reply)
{
    for (int i = 0; i < windows.count(); ++i)
        if (windows[i]->d->mView->url() == reply->url())
            return windows[i]->d->mView;
    return windows[0]->d->mView;
}


