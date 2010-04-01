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

#include "networkcookiejar.h"
#include "networkcookiejar_p.h"
#include "twoleveldomains_p.h"

//#define NETWORKCOOKIEJAR_DEBUG

#ifndef QT_NO_DEBUG
// ^ Prevent being left on in a released product by accident
// qDebug any cookies that are rejected for further inspection
#define NETWORKCOOKIEJAR_LOGREJECTEDCOOKIES
#include <qdebug.h>
#endif

#include <qurl.h>
#include <qdatetime.h>

#if defined(NETWORKCOOKIEJAR_DEBUG)
#include <qdebug.h>
#endif


NetworkCookieJar::NetworkCookieJar(QObject *parent)
        : QNetworkCookieJar(parent)
{
    d = new NetworkCookieJarPrivate;
}

NetworkCookieJar::~NetworkCookieJar()
{
    delete d;
}

static QStringList splitHost(const QString &host) {
    QStringList parts = host.split(QLatin1Char('.'), QString::KeepEmptyParts);
    // Remove empty components that are on the start and end
    while (!parts.isEmpty() && parts.last().isEmpty())
        parts.removeLast();
    while (!parts.isEmpty() && parts.first().isEmpty())
        parts.removeFirst();
    return parts;
}

inline static bool shorterPaths(const QNetworkCookie &c1, const QNetworkCookie &c2)
{
    return c2.path().length() < c1.path().length();
}

QList<QNetworkCookie> NetworkCookieJar::cookiesForUrl(const QUrl &url) const
{
#if defined(NETWORKCOOKIEJAR_DEBUG)
    qDebug() << "NetworkCookieJar::" << __FUNCTION__ << url;
#endif
    // Generate split host
    QString host = url.host();
    if (url.scheme().toLower() == QLatin1String("file"))
        host = QLatin1String("localhost");
    QStringList urlHost = splitHost(host);

    // Get all the cookies for url
    QList<QNetworkCookie> cookies = d->tree.find(urlHost);
    if (urlHost.count() > 2) {
        int top = 2;
        if (d->matchesBlacklist(urlHost.last()))
            top = 3;

        urlHost.removeFirst();
        while (urlHost.count() >= top) {
            cookies += d->tree.find(urlHost);
            urlHost.removeFirst();
        }
    }

    // Prevent doing anything expensive in the common case where
    // there are no cookies to check
    if (cookies.isEmpty())
        return cookies;

    QDateTime now = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    const QString urlPath = d->urlPath(url);
    const bool isSecure = url.scheme().toLower() == QLatin1String("https");
    QList<QNetworkCookie>::iterator i = cookies.begin();
    for (; i != cookies.end();) {
        if (!d->matchingPath(*i, urlPath)) {
#if defined(NETWORKCOOKIEJAR_DEBUG)
            qDebug() << __FUNCTION__ << "Ignoring cookie, path does not match" << *i << urlPath;
#endif
            i = cookies.erase(i);
            continue;
        }
        if (!isSecure && i->isSecure()) {
            i = cookies.erase(i);
#if defined(NETWORKCOOKIEJAR_DEBUG)
            qDebug() << __FUNCTION__ << "Ignoring cookie, security mismatch"
                     << *i << !isSecure;
#endif
            continue;
        }
        if (!i->isSessionCookie() && now > i->expirationDate()) {
            // remove now (expensive short term) because there will
            // probably be many more cookiesForUrl calls for this host
            d->tree.remove(splitHost(i->domain()), *i);
#if defined(NETWORKCOOKIEJAR_DEBUG)
            qDebug() << __FUNCTION__ << "Ignoring cookie, expiration issue"
                     << *i << now;
#endif
            i = cookies.erase(i);
            continue;
        }
        ++i;
    }

    // shorter paths should go first
    qSort(cookies.begin(), cookies.end(), shorterPaths);
#if defined(NETWORKCOOKIEJAR_DEBUG)
    qDebug() << "NetworkCookieJar::" << __FUNCTION__ << "returning" << cookies.count();
    qDebug() << cookies;
#endif
    return cookies;
}

static const qint32 NetworkCookieJarMagic = 0xae;

QByteArray NetworkCookieJar::saveState () const
{
    int version = 1;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(NetworkCookieJarMagic);
    stream << qint32(version);
    stream << d->tree;
    return data;
}

bool NetworkCookieJar::restoreState(const QByteArray &state)
{
    int version = 1;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;
    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != NetworkCookieJarMagic || v != version)
        return false;
    stream >> d->tree;
    if (stream.status() != QDataStream::Ok) {
        d->tree.clear();
        return false;
    }
    return true;
}

/*!
    Remove any session cookies or cookies that have expired.
  */
void NetworkCookieJar::endSession()
{
    const QList<QNetworkCookie> cookies = d->tree.all();
    QDateTime now = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QList<QNetworkCookie>::const_iterator i = cookies.constBegin();
    for (; i != cookies.constEnd();) {
        if (i->isSessionCookie()
            || (!i->isSessionCookie() && now > i->expirationDate())) {
                d->tree.remove(splitHost(i->domain()), *i);
        }
        ++i;
    }
}

static const int maxCookiePathLength = 1024;

bool NetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
{
#if defined(NETWORKCOOKIEJAR_DEBUG)
    qDebug() << "NetworkCookieJar::" << __FUNCTION__ << url;
    qDebug() << cookieList;
#endif
    QDateTime now = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    bool changed = false;
    QString fullUrlPath = url.path();
    QString defaultPath = fullUrlPath.mid(0, fullUrlPath.lastIndexOf(QLatin1Char('/')) + 1);
    if (defaultPath.isEmpty())
        defaultPath = QLatin1Char('/');

    QString urlPath = d->urlPath(url);
    foreach (QNetworkCookie cookie, cookieList) {
        if (cookie.path().length() > maxCookiePathLength)
            continue;

        bool alreadyDead = !cookie.isSessionCookie() && cookie.expirationDate() < now;

        if (cookie.path().isEmpty()) {
            cookie.setPath(defaultPath);
        }
        // Matching the behavior of Firefox, no path checking is done when setting cookies
        // Safari does something even odder, when that paths don't match it keeps
        // the cookie, but changes the paths to the default path
#if 0
        else if (!d->matchingPath(cookie, urlPath)) {
#ifdef NETWORKCOOKIEJAR_LOGREJECTEDCOOKIES
            qDebug() << "NetworkCookieJar::" << __FUNCTION__
                     << "Blocked cookie because: path doesn't match: " << cookie << url;
#endif
            continue;
        }
#endif

        if (cookie.domain().isEmpty()) {
            QString host = url.host().toLower();
            if (host.isEmpty())
                continue;
            cookie.setDomain(host);
        } else if (!d->matchingDomain(cookie, url)) {
#ifdef NETWORKCOOKIEJAR_LOGREJECTEDCOOKIES
            qDebug() << "NetworkCookieJar::" << __FUNCTION__
                     << "Blocked cookie because: domain doesn't match: " << cookie << url;
#endif
            continue;
        }

        // replace/remove existing cookies
        QString domain = cookie.domain();
        Q_ASSERT(!domain.isEmpty());
        QStringList urlHost = splitHost(domain);

        QList<QNetworkCookie> cookies = d->tree.find(urlHost);
        QList<QNetworkCookie>::const_iterator it = cookies.constBegin();
        for (; it != cookies.constEnd(); ++it) {
            if (cookie.name() == it->name() &&
                cookie.domain() == it->domain() &&
                cookie.path() == it->path()) {
                d->tree.remove(urlHost, *it);
                break;
            }
        }

        if (alreadyDead)
            continue;

        changed = true;
        d->tree.insert(urlHost, cookie);
    }

    return changed;
}

QList<QNetworkCookie> NetworkCookieJar::allCookies() const
{
#if defined(NETWORKCOOKIEJAR_DEBUG)
    qDebug() << "NetworkCookieJar::" << __FUNCTION__;
#endif
    return d->tree.all();
}

void NetworkCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)
{
#if defined(NETWORKCOOKIEJAR_DEBUG)
    qDebug() << "NetworkCookieJar::" << __FUNCTION__ << cookieList.count();
#endif
    d->tree.clear();
    foreach (const QNetworkCookie &cookie, cookieList) {
        QString domain = cookie.domain();
        d->tree.insert(splitHost(domain), cookie);
    }
}

QString NetworkCookieJarPrivate::urlPath(const QUrl &url) const
{
    QString urlPath = url.path();
    if (!urlPath.endsWith(QLatin1Char('/')))
        urlPath += QLatin1Char('/');
    return urlPath;
}

bool NetworkCookieJarPrivate::matchingPath(const QNetworkCookie &cookie, const QString &urlPath) const
{
    QString cookiePath = cookie.path();
    if (!cookiePath.endsWith(QLatin1Char('/')))
        cookiePath += QLatin1Char('/');

    return urlPath.startsWith(cookiePath);
}

bool NetworkCookieJarPrivate::matchesBlacklist(const QString &string) const
{
    if (!setSecondLevelDomain) {
        // Alternatively to save a little bit of ram we could just
        // use bsearch on twoLevelDomains in place
        for (int j = 0; twoLevelDomains[j]; ++j)
            secondLevelDomains += QLatin1String(twoLevelDomains[j]);
        setSecondLevelDomain = true;
    }
    QStringList::const_iterator i =
         qBinaryFind(secondLevelDomains.constBegin(), secondLevelDomains.constEnd(), string);
        return (i != secondLevelDomains.constEnd());
}

bool NetworkCookieJarPrivate::matchingDomain(const QNetworkCookie &cookie, const QUrl &url) const
{
    QString domain = cookie.domain().simplified().toLower();
    domain.remove(QLatin1Char(' '));
    QStringList parts = splitHost(domain);
    if (parts.isEmpty())
        return false;

    // When there is only one part only file://localhost/ is accepted
    if (parts.count() == 1) {
        QString s = parts.first();
        if (parts.first() != QLatin1String("localhost"))
            return false;
        if (url.scheme().toLower() == QLatin1String("file"))
            return true;
    }

    // Check for blacklist
    if (parts.count() == 2 && matchesBlacklist(parts.last()))
        return false;

    QStringList urlParts = url.host().toLower().split(QLatin1Char('.'), QString::SkipEmptyParts);
    if (urlParts.isEmpty())
        return false;
    while (urlParts.count() > parts.count())
        urlParts.removeFirst();

    for (int j = 0; j < urlParts.count(); ++j) {
        if (urlParts.at(j) != parts.at(j)) {
            return false;
        }
    }

    return true;
}

void NetworkCookieJar::setSecondLevelDomains(const QStringList &secondLevelDomains)
{
    d->setSecondLevelDomain = true;
    d->secondLevelDomains = secondLevelDomains;
    qSort(d->secondLevelDomains);
}

