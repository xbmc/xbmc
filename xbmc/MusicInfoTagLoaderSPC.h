#pragma once

#include "IMusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderSPC: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderSPC(void);
		virtual ~CMusicInfoTagLoaderSPC();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	};
}
