#pragma once

#include "musicinfotag.h"
#include "stdstring.h"
#include "IMusicInfoTagLoader.h"

#include "filesystem/file.h"


using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderWMA:public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderWMA(void);
		virtual ~CMusicInfoTagLoaderWMA();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	protected:
	};
};