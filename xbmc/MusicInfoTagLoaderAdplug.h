#pragma once

#include "IMusicInfoTagLoader.h"
#include "cores/paplayer/dlladplug.h"
#include "cores/paplayer/adplugcodec.h"

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
