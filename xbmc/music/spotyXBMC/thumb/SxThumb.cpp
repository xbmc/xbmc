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

#include <stdio.h>
#include <stdlib.h>
#include "SxThumb.h"
#include "../session/Session.h"
#include "../Logger.h"
#include "../Utils.h"
#include "../../../TextureCache.h"
#include "../../../filesystem/File.h"

namespace addon_music_spotify {

	using namespace std;

	SxThumb::SxThumb(sp_image* image, string path) {
		m_isLoaded = false;
		m_image = image;
		sp_link *link = sp_link_create_from_image(image);
		char linkString[256];
		sp_link_as_string(link, linkString, 256);
		CStdString linkStringClean = linkString;
		linkStringClean.Remove(':');
		sp_link_release(link);
		m_file = path + linkStringClean.c_str() + ".jpg";
		if (XFILE::CFile::Exists(m_file, true)) {
			m_isLoaded = true;
		} else {
		  CStdString pathS(m_file);
		  bool recache;
		  CStdString cached(CTextureCache::Get().CheckCachedImage(pathS,true,recache));
		  if (!cached.IsEmpty() && !recache) {
			//Logger::printOut("Thumb already in XBMC cache, no need to download again");
			m_file = cached;
			m_imageIsFromCache = true;
			m_isLoaded = true;
		  } else {
			m_imageIsFromCache = false;
			sp_image_add_load_callback(m_image, &cb_imageLoaded, this);
		  }
		}

		m_references = 1;

	}

	SxThumb::~SxThumb() {
		if (!m_isLoaded)
			sp_image_remove_load_callback(m_image, &cb_imageLoaded, this);
		sp_image_release(m_image);
		//dont delete it from the cache
		if (!m_imageIsFromCache)
			Utils::removeFile(m_file.c_str());

	}

	void SxThumb::thumbLoaded(sp_image *image) {
		if (sp_image_error(image) != SP_ERROR_OK) {
			Logger::printOut("creating image error");
			m_file = "";
			//well its loaded but without image
			m_isLoaded = true;
			return;
		}

		XFILE::CFile file;
		if (file.OpenForWrite(m_file, true)) {
			const void *buffer;
			size_t len;
			buffer = sp_image_data(image, &len);
			file.Write((const char*) buffer, len);
			file.Close();
		}

		m_isLoaded = true;
		//Logger::printOut("thumb downloaded");
	}

	void SxThumb::cb_imageLoaded(sp_image *image, void *userdata) {
		SxThumb *thumb = (SxThumb*) userdata;
		thumb->thumbLoaded(image);
	}

} /* namespace addon_music_spotify */
