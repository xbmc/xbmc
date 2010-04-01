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

#ifndef LLEMBEDDEDBROWSER_P_H
#define LLEMBEDDEDBROWSER_P_H

#include <qnetworkaccessmanager.h>
#include <qapplication.h>
#if QT_VERSION >= 0x040500
#include <qnetworkdiskcache.h>
#endif

#include "networkcookiejar.h"
#include "llembeddedbrowser.h"

#include <qgraphicswebview.h>

class LLNetworkCookieJar : public NetworkCookieJar
{
public:
    LLNetworkCookieJar(QObject *parent, const QString &filename);
    ~LLNetworkCookieJar();

    void load();
    void save();

    QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookie_list, const QUrl& url);
    void clear();

    QString mCookieStorageFileName;
    bool mAllowCookies;
};

class LLNetworkAccessManager;
class LLEmbeddedBrowserPrivate
{
public:
    LLEmbeddedBrowserPrivate();
    ~LLEmbeddedBrowserPrivate();

    int mErrorNum;
    void* mNativeWindowHandle;
    LLNetworkAccessManager *mNetworkAccessManager;
    QApplication *mApplication;
#if QT_VERSION >= 0x040500
    QNetworkDiskCache *mDiskCache;
#endif
    LLNetworkCookieJar *mNetworkCookieJar;

    QGraphicsWebView *findView(QNetworkReply *);

    QString mStorageDirectory;
    QList<LLEmbeddedBrowserWindow*> windows;
};

#endif

