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

#ifndef LLWEBPAGE_H
#define LLWEBPAGE_H

class QGraphicsWebView;
#include <qwebpage.h>
#include "llqtwebkit.h"

class LLEmbeddedBrowserWindow;
class LLWebPage : public QWebPage
{
    Q_OBJECT

    public:
        LLWebPage(QObject *parent = 0);
        LLEmbeddedBrowserWindow *window;
        bool event(QEvent *event);

        QGraphicsWebView *webView;

        void setWindowOpenBehavior(LLQtWebKit::WindowOpenBehavior behavior);
        void setHostLanguage(const std::string& host_language);

    protected:
        bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

    public slots:
        void loadProgressSlot(int progress);
        void statusBarMessageSlot(const QString &);
        void titleChangedSlot(const QString &);
        void urlChangedSlot(const QUrl& url);
        void loadStarted();
        void loadFinished(bool ok);

    private slots:
	    void extendNavigatorObject();

    protected:
        QString chooseFile(QWebFrame* parentFrame, const QString& suggestedFile);
        void javaScriptAlert(QWebFrame* frame, const QString& msg);
        bool javaScriptConfirm(QWebFrame* frame, const QString& msg);
        bool javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result);
        QWebPage *createWindow(WebWindowType type);

    private:
        QPoint currentPoint;
	    LLQtWebKit::WindowOpenBehavior windowOpenBehavior;
	    std::string mHostLanguage;
};

#endif

