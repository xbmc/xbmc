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
#include <QtNetwork/QtNetwork>
#include <networkcookiejar.h>

class CookieJarBenchmark: public QObject {
    Q_OBJECT

private slots:
    void setCookiesFromUrl();
    void cookiesForUrl();
    void player();

private:
    QNetworkCookieJar *getJar(bool populate = true);
    QList<QNetworkCookie> generateCookies(int size);
};

QNetworkCookieJar *CookieJarBenchmark::getJar(bool populate)
{
    QNetworkCookieJar *jar;
    if (qgetenv("JAR") == "CookieJar") {
        jar = new NetworkCookieJar(this);
    } else {
        jar = new QNetworkCookieJar(this);
    }
    if (!populate)
        return jar;

    // pre populate
    for (int i = 0; i < 500; ++i) {
        QList<QNetworkCookie> cookies = generateCookies(1);
        QUrl url = QUrl(QString("http://%1").arg(cookies[0].domain()));
        jar->setCookiesFromUrl(cookies, url);
    }

    return jar;
}

QList<QNetworkCookie> CookieJarBenchmark::generateCookies(int size)
{
    QList<QNetworkCookie> cookies;
    for (int i = 0; i < size; ++i) {
        QNetworkCookie cookie;

        QString tld;
        int c = qrand() % 3;
        if (c == 0) tld = "com";
        if (c == 1) tld = "net";
        if (c == 2) tld = "org";

        QString mid;
        int size = qrand() % 6 + 3;
        while (mid.count() < size)
            mid += QString(QChar::fromAscii(qrand() % 26 + 65));

        QString sub;
        c = qrand() % 3;
        if (c == 0) sub = ".";
        if (c == 1) sub = ".www.";
        if (c == 2) sub = ".foo";

        cookie.setDomain(QString("%1%2.%3").arg(sub).arg(mid).arg(tld));
        cookie.setName("a");
        cookie.setValue("b");
        cookie.setPath("/");
        cookies.append(cookie);
    }
    return cookies;
}

void CookieJarBenchmark::setCookiesFromUrl()
{
    QNetworkCookieJar *jar = getJar();
    QList<QNetworkCookie> cookies = generateCookies(1);
    QUrl url = QUrl(QString("http://%1").arg(cookies[0].domain()));
    QBENCHMARK {
        jar->setCookiesFromUrl(cookies, url);
    }
    delete jar;
}

void CookieJarBenchmark::cookiesForUrl()
{
    QNetworkCookieJar *jar = getJar();
    QList<QNetworkCookie> cookies = generateCookies(1);
    cookies[0].setDomain("www.foo.tld");
    QUrl url = QUrl("http://www.foo.tld");
    //QUrl url = QUrl(QString("http://foo%1/").arg(cookies[0].domain()));
    jar->setCookiesFromUrl(cookies, url);
    //qDebug() << cookies << url;
    int c = 0;
    QBENCHMARK {
        c += jar->cookiesForUrl(url).count();
    }
    delete jar;
}

// Grab the cookie.log file from the manualtest/browser directory
void CookieJarBenchmark::player()
{
    QBENCHMARK {
    QFile file("cookie.log");
    file.open(QFile::ReadOnly);
    QDataStream stream(&file);
    QNetworkCookieJar *jar = getJar(false);
    while (!stream.atEnd()) {
        QString command;
        QUrl url;
        stream >> command;
        stream >> url;
        //qDebug() << command << url;
        if (command == "cookiesForUrl") {
            jar->cookiesForUrl(url);
        }
        if (command == "setCookiesFromUrl") {
            QByteArray data;
            stream >> data;
            QDataStream dstream(&data, QIODevice::ReadWrite);
            qint32 c;
            dstream >> c;
            QList<QNetworkCookie> cookies;
            for (int i = 0; i < c; ++i) {
                QByteArray text;
                dstream >> text;
                cookies += QNetworkCookie::parseCookies(text);
            }
            jar->setCookiesFromUrl(cookies, url);
        }
    }
    }
}

QTEST_MAIN(CookieJarBenchmark)
#include "main.moc"
