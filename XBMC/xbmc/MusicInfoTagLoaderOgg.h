#pragma once

#include "musicinfotag.h"
#include "stdstring.h"
#include "IMusicInfoTagLoader.h"

#include "filesystem/file.h"

#include "OggTag.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderOgg:public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderOgg(void);
		virtual ~CMusicInfoTagLoaderOgg();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
};