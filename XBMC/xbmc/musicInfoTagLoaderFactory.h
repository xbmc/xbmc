#pragma once

#include "ImusicInfoTagLoader.h"
#include "stdstring.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderFactory
	{
	public:
		CMusicInfoTagLoaderFactory(void);
		virtual ~CMusicInfoTagLoaderFactory();

		IMusicInfoTagLoader* CreateLoader(const CStdString& strFileName);
	};
};