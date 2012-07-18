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

#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <string>
#include "../../utils/log.h"
#include <fstream>
#include "SxSettings.h"

namespace addon_music_spotify {

  using namespace std;

  class Logger {
  public:
    Logger();
    virtual ~Logger();
    static void printOut(char* text) {
	  CLog::Log(LOGDEBUG, "spotifylog: %s", text);
      cout << text << "\n";
      if (Settings::getInstance()->saveLogToFile()){
        string path = Settings::getInstance()->getCachePath() + "log.txt";
        ofstream file(path.c_str(), ios::app);
        if (file.is_open()) {
      	  file << text << "\n";
        }
        file.close();
      }
    }
    static void printOut(const char* text) {
      CLog::Log(LOGDEBUG, "spotifylog: %s", text);
      cout << text << "\n";
      if (Settings::getInstance()->saveLogToFile()){
        string path = Settings::getInstance()->getCachePath() + "log.txt";
        ofstream file(path.c_str(), ios::app);
        if (file.is_open()) {
      	  file << text << "\n";
        }
        file.close();
      }
    }
    static void printOut(char text) {
      CLog::Log(LOGDEBUG, "spotifylog: %c", text);
      cout << text << "\n";
      if (Settings::getInstance()->saveLogToFile()){
        string path = Settings::getInstance()->getCachePath() + "log.txt";
        ofstream file(path.c_str(), ios::app);
        if (file.is_open()) {
      	  file << text << "\n";
        }
        file.close();
      }
    }
    static void printOut(string text) {
    	CLog::Log(LOGDEBUG, "spotifylog: %s", text.c_str());
      cout << text << "\n";
      if (Settings::getInstance()->saveLogToFile()){
        string path = Settings::getInstance()->getCachePath() + "log.txt";
        ofstream file(path.c_str(), ios::app);
        if (file.is_open()) {
      	  file << text << "\n";
        }
        file.close();
      }
    }

  };

} /* namespace addon_music_spotify */
#endif /* LOG_H_ */

