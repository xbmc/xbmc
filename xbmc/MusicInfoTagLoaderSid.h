#pragma once

#include "ImusicInfoTagLoader.h"

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
