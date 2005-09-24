#pragma once

#include "IMusicInfoTagLoader.h"
#include "cores/paplayer/dllsnes9xapu.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderSPC: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderSPC(void);
		virtual ~CMusicInfoTagLoaderSPC();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  private:
    int m_spc;
    DllSnes9xApu m_dll;
	};
};

extern CStdString strNSFDLL;
