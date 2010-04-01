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

#include <QtGui/QtGui>
#include <QtWebKit/QtWebKit>
#include <QtNetwork/QtNetwork>
#include <networkcookiejar.h>

QFile file;
QDataStream stream;

class CookieLog : public NetworkCookieJar {

    Q_OBJECT

public:
    CookieLog(QObject *parent = 0) : NetworkCookieJar(parent)
    {
        file.setFileName("cookie.log");
        file.open(QFile::WriteOnly);
        stream.setDevice(&file);
    };

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl & url) const
    {
        stream << QString("cookiesForUrl") << url;
        QList<QNetworkCookie> cookies = NetworkCookieJar::cookiesForUrl(url);
        //stream << "#" << cookies;
        file.flush();
        return cookies;
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
    {
        QByteArray data;
        QDataStream dstream(&data, QIODevice::ReadWrite);
        qint32 c = cookieList.count();
        dstream << c;
        qDebug() << cookieList.count();
        for (int i = 0; i < c; ++i)
            dstream << cookieList[i].toRawForm();
        dstream.device()->close();
        stream << QString("setCookiesFromUrl") << url << data;// << cookieList;
        bool set = NetworkCookieJar::setCookiesFromUrl(cookieList, url);
        file.flush();
        return set;
    }

};

int main(int argc, char**argv) {
    QApplication application(argc, argv);
    QWebView view;
    QString url = "http://www.google.com";
    if (argc > 1)
        url = argv[1];
    view.load(QUrl(url));
    view.page()->networkAccessManager()->setCookieJar(new CookieLog());
    view.show();
    return application.exec();
}

#include "main.moc"
