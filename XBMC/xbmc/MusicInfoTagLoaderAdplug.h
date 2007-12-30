#pragma once

#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/DllAdplug.h"
#include "cores/paplayer/AdplugCodec.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderAdplug: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderAdplug(void);
		virtual ~CMusicInfoTagLoaderAdplug();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  private:
    int m_adl;
    DllAdplug m_dll;
	};
}
