#pragma once

#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/DllNosefart.h"

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
