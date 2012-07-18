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

#include "ThumbStore.h"
#include "SxThumb.h"
#include "../SxSettings.h"
#include "../Logger.h"
#include "../session/Session.h"
#include "../Utils.h"
#include <string.h>
#include "URL.h"
#include "../../../filesystem/CurlFile.h"
#include "../utils/XBMCTinyXML.h"
#include <fstream>
#include "../../../filesystem/File.h"

using namespace XFILE;

namespace addon_music_spotify {

	using namespace std;

	ThumbStore::ThumbStore() {
		Logger::printOut("ThumbStore creating paths");
		//Utils::removeDir(Settings::getInstance()->getThumbPath());
		Utils::createDir(Settings::getInstance()->getThumbPath());
		//Utils::removeDir(Settings::getInstance()->getArtistThumbPath());
		//Utils::createDir(Settings::getInstance()->getArtistThumbPath());
		Logger::printOut("ThumbStore creating paths done");
		m_stdFanart = new CStdString(Settings::getInstance()->getFanart());

		//load the fanartmap from file
		string path = Settings::getInstance()->getCachePath() + "fanarts.txt";
		Logger::printOut("loading fanart list");
		ifstream file(path.c_str());
		if (file.is_open()) {
			while (file.good()) {
				string name;
				string path;
				getline(file, name);
				getline(file, path);
				CStdString *fanartUrl;
				if (path[0] == '/') {
					// Don't create a new string if its not necessary
					fanartUrl = m_stdFanart;
				}
				else {
					fanartUrl= new CStdString(path);
				}
				m_fanarts.insert(stringMap::value_type(name, fanartUrl));
			}
		}
		file.close();
	}

	void ThumbStore::deInit() {
		delete m_instance;
		Utils::removeDir(Settings::getInstance()->getThumbPath());
	}

	ThumbStore::~ThumbStore() {
		for (thumbMap::iterator it = m_thumbs.begin(); it != m_thumbs.end(); ++it) {
			delete it->second;
		}
	}

	ThumbStore* ThumbStore::m_instance = 0;
	ThumbStore *ThumbStore::getInstance() {
		return m_instance ? m_instance : (m_instance = new ThumbStore);
	}

	SxThumb *ThumbStore::getThumb(const unsigned char* image) {
		//check if we got the thumb
		thumbMap::iterator it = m_thumbs.find(image);
		SxThumb *thumb;
		if (it == m_thumbs.end()) {
			//we need to create it
			//Logger::printOut("create thumb");
			sp_image* spImage = sp_image_create(
					Session::getInstance()->getSpSession(), (unsigned char*) image);

			if (!spImage) {
				Logger::printOut("no image");
				return NULL;
			}

			string path = Settings::getInstance()->getThumbPath();
			thumb = new SxThumb(spImage, path);
			m_thumbs.insert(thumbMap::value_type(image, thumb));
		} else {
			//Logger::printOut("loading thumb from store");
			thumb = it->second;
			thumb->addRef();
		}

		return thumb;
	}

	void ThumbStore::removeThumb(const unsigned char* image) {
		thumbMap::iterator it = m_thumbs.find(image);
		SxThumb *thumb;
		if (it != m_thumbs.end()) {
			thumb = it->second;
			if (thumb->getReferencesCount() <= 1) {
				m_thumbs.erase(image);
				delete thumb;
			} else
				thumb->rmRef();
		}
	}

	void ThumbStore::removeThumb(SxThumb* thumb) {
		removeThumb((const unsigned char*) thumb->m_image);
	}

	CStdString *ThumbStore::getFanart(const char *artistName) {

		if (!Settings::getInstance()->getUseHTFanarts())
			return m_stdFanart;

		//Logger::printOut("Looking for fanart");

		//check if we got the fanart
		string artistNameString = artistName;
		stringMap::iterator it = m_fanarts.find(artistNameString);
		if (it == m_fanarts.end()) {
			//we dont have it so we need to to a search for it

			CCurlFile http;
			CStdString artistString = artistName;
			artistString.Replace(' ', '+');
			CStdString urlString;
			urlString.Format(
					"http://htbackdrops.com/api/afb0f6cdbd412a7888005de34f86e4a5/searchXML?keywords=%s&default_operator=and&aid=1&fields=title,keywords,caption,mb_name,mb_alias&inc=keywords,caption,mb_name,mb_aliases&limit=1",
					artistString);

			CURL url(urlString);

			if (http.Open(url)) {
				Logger::printOut("Looking for fanart, need to fetch a new address");
				Logger::printOut(artistNameString);
				//try to parse the resulting file for a fanart image
				CStdString data;
				http.ReadData(data);

				TiXmlDocument xmlDoc;
				xmlDoc.Parse(data);
				TiXmlNode* element = xmlDoc.RootElement();
				//get the images child
				element = element->FirstChild("images");
				if (element) {

					//get the first image child, (should only be one)
					element = element->FirstChild();
					if (element) {
						//get the id
						TiXmlNode* idElement = element->FirstChild("id");
						CStdString id = idElement->ToElement()->GetText();
						//get the filename
						TiXmlNode* fileNameElement = element->FirstChild("filename");
						CStdString name = fileNameElement->ToElement()->GetText();

						CStdString *fanartUrl = new CStdString();
						fanartUrl->Format(
								"http://htbackdrops.com/api/afb0f6cdbd412a7888005de34f86e4a5/download/%s/fullsize/%s",
								id, name);
						//Logger::printOut("Adding online fanart");

						m_fanarts.insert(
								stringMap::value_type(artistNameString, fanartUrl));

						//save the fanart url to the cachefile
						string path = Settings::getInstance()->getCachePath() + "fanarts.txt";
						Logger::printOut("saving fanart list");
						ofstream file(path.c_str(), ios::app);
						if (file.is_open()) {
							file << artistNameString << "\n" << *fanartUrl << "\n";
						}

						file.close();

						return fanartUrl;
					}
				}
			}
			//Logger::printOut("Adding standard fanart");
			m_fanarts.insert(stringMap::value_type(artistNameString, m_stdFanart));

			//save the fanart url to the cachefile
			string path = Settings::getInstance()->getCachePath() + "fanarts.txt";
			Logger::printOut("saving fanart list");
			ofstream file(path.c_str(), ios::app);
			if (file.is_open()) {
				file << artistNameString << "\n" << *m_stdFanart << "\n";
			}

			file.close();

			return m_stdFanart;
		}
		//Logger::printOut("Returning cached fanart");
		return it->second;
	}

} /* namespace addon_music_spotify */
