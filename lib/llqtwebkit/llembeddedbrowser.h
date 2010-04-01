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

#ifndef LLEMBEDDEDBROWSER_H
#define LLEMBEDDEDBROWSER_H

#include <string>
#include <map>

class LLEmbeddedBrowserWindow;
class LLEmbeddedBrowserWindowObserver;

class LLEmbeddedBrowserPrivate;
class LLEmbeddedBrowser
{
    public:
        LLEmbeddedBrowser();
        virtual ~LLEmbeddedBrowser();

        static LLEmbeddedBrowser* getInstance();

        bool init(std::string application_directory,
                  std::string component_directory,
                  std::string profile_directory,
                  void* native_window_handle);
        bool reset();
        bool clearCache();
        bool enableProxy(bool enabled, std::string host_name, int port);
        bool enableCookies(bool enabled);
        bool clearAllCookies();
        bool enablePlugins(bool enabled);
        bool enableJavascript(bool enabled);
        std::string getGREVersion();
        void setBrowserAgentId(std::string id);
        void setHostLanguage( const std::string& host_language );
        LLEmbeddedBrowserWindow* createBrowserWindow(int width, int height);
        bool destroyBrowserWindow(LLEmbeddedBrowserWindow* browser_window);
        void setLastError(int error_number);
        void clearLastError();
        int getLastError();
        int getWindowCount() const;
        void pump(int max_milliseconds);

    private:
        friend class LLEmbeddedBrowserWindow;
        friend class LLEmbeddedBrowserWindowPrivate;
        LLEmbeddedBrowserPrivate *d;
        std::string mHostLanguage;

        static LLEmbeddedBrowser* sInstance;
};

#endif    // LLEMBEDDEDBROWSER_H

