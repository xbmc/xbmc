#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderSid: public IMusicInfoTagLoader
	{
		public:
			CMusicInfoTagLoaderSid(void);
			virtual ~CMusicInfoTagLoaderSid();

			virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
}
