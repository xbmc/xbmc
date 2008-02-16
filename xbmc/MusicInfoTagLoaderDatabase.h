#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderDatabase: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderDatabase(void);
		virtual ~CMusicInfoTagLoaderDatabase();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
}
