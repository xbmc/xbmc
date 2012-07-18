/*
 spotyxbmc2 - A project to integrate Spotify into XBMC
 Copyright (C) 2011  David Erenger

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For contact with the author:
 david.erenger@gmail.com
 */

#include "Session.h"
#include "BackgroundThread.h"
#include "../Logger.h"
#include "../SxSettings.h"

namespace addon_music_spotify {

  BackgroundThread::BackgroundThread() : CThread("Spotify BackgroundThread") {
  }

  BackgroundThread::~BackgroundThread() {
  }

  void BackgroundThread::OnStartup() {
  	Sleep(100);
    Logger::printOut("bgthread OnStartup");
    if (!Settings::getInstance()->init()){
    	Logger::printOut("bgthread quiting, spotyxbmc is not enabled or the addon is missing");
    	return;
    }
  	Sleep(Settings::getInstance()->getStartDelay());
    Session::getInstance()->connect();
    Session::getInstance()->unlock();
    Logger::printOut("bgthread OnStartup done");
  }

  void BackgroundThread::OnExit() {
    Logger::printOut("bgthread OnExit");
  }

  void BackgroundThread::OnException() {
    Logger::printOut("bgthread OnException");
  }

  void BackgroundThread::Process() {

    while (Session::getInstance()->isEnabled()) {
      //Logger::printOut("bgthread Process");
      //if the session is locked, sleep for awhile and try later again
      if (Session::getInstance()->m_nextEvent <= 0 && Session::getInstance()->lock()) {
        Session::getInstance()->processEvents();
        Session::getInstance()->unlock();
      }
      Session::getInstance()->m_nextEvent -= 5;
      Sleep(5);
    }
    Logger::printOut("exiting process thread");
    Session::getInstance()->disConnect();
    delete Session::getInstance();
    Logger::printOut("exiting process thread done");
  }

} /* namespace addon_music_spotify */
