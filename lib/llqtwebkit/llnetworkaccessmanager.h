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

#ifndef LLNETWORKACCESSMANAGER_H
#define LLNETWORKACCESSMANAGER_H

#include <qnetworkaccessmanager.h>

#include "ui_passworddialog.h"

class QGraphicsProxyWidget;

class AuthDialog
{
public:
    AuthDialog() : authenticationDialog(0), passwordDialog(0), proxyWidget(0) {}
    QDialog *authenticationDialog;
    Ui::PasswordDialog *passwordDialog;
    QString realm;
    QGraphicsProxyWidget *proxyWidget;
};

class LLEmbeddedBrowserPrivate;
class LLNetworkAccessManager: public QNetworkAccessManager
{
    Q_OBJECT
public:
    LLNetworkAccessManager(LLEmbeddedBrowserPrivate* browser, QObject* parent = 0);

private slots:
    void finishLoading(QNetworkReply* reply);
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);

private:
    LLEmbeddedBrowserPrivate* mBrowser;

    QList<AuthDialog> authDialogs;

};

#endif    // LLNETWORKACCESSMANAGER_H

