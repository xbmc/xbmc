#pragma once

#include "musicinfotag.h"
#include "stdstring.h"
#include "IMusicInfoTagLoader.h"
using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderMP3:public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderMP3(void);
		virtual ~CMusicInfoTagLoaderMP3();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
};