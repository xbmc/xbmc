#pragma once

#include "musicinfotag.h"
#include "IMusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderMP4:public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderMP4(void);
		virtual ~CMusicInfoTagLoaderMP4();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
};
