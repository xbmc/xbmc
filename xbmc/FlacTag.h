//------------------------------
// CFlacTag in 2003 by JMarshall
//------------------------------
#include "FileSystem/file.h"
#include "Oggtag.h"

namespace MUSIC_INFO {

#pragma once

	class CFlacTag : public COggTag
	{
	public:
		CFlacTag(void);
		virtual ~CFlacTag(void);
		int GetDuration() { return m_nDuration; }

	protected:
		virtual bool FindVobisTagHeader(void);	// override of COggTag's implementation
		int ReadFlacHeader(void);				// returns the position after the STREAM_INFO metadata
		int FindFlacHeader(void);				// returns the offset in the file of the fLaC data
		virtual bool ReadBit(void);

	private:
		int	m_nDuration;
	};
};