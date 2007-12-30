#pragma once

#include "IMusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderMod: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderMod(void);
		virtual ~CMusicInfoTagLoaderMod();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	private:
		bool getFile(CStdString& strFile, const CStdString& strSource);
	};
}
