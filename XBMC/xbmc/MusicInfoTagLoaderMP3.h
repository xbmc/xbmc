#pragma once

#include "musicinfotag.h"
#include "stdstring.h"
#include "IMusicInfoTagLoader.h"

#include "filesystem/file.h"

#define HAVE_CONFIG
#define ID3LIB_LINKOPTION 1
#include "lib/libID3/id3.h"
#include "lib/libID3/config.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/utils.h"
#include "lib/libID3/misc_support.h"
#include "lib/libID3/readers.h"
#include "lib/libID3/io_helpers.h"
#include "XIStreamReader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

	class CMusicInfoTagLoaderMP3:public IMusicInfoTagLoader
	{
	public:
		CMusicInfoTagLoaderMP3(void);
		virtual ~CMusicInfoTagLoaderMP3();

		virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
	protected:
		bool ReadTag( ID3_Tag& id3tag, CMusicInfoTag& tag );
	};
};