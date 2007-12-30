#pragma once

#include "IMusicInfoTagLoader.h"
#include "cores/paplayer/dllnosefart.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{
	class CMusicInfoTagLoaderNSF: public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderNSF(void);
		virtual ~CMusicInfoTagLoaderNSF();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	  virtual int GetStreamCount(const CStdString& strFileName);
  private:
    int m_nsf;
    DllNosefart m_dll;
	};
}

extern CStdString strNSFDLL;
