#include "stdafx.h"
#include "XMP4File.h"

using namespace MUSIC_INFO;
using namespace XFILE;

XMP4File::XMP4File(u_int32_t verbosity)
: MP4File(verbosity)
{

}

XMP4File::~XMP4File()
{

}

void XMP4File::Open(const char* fmode)
{
	ASSERT(m_pFile == NULL);
	CFile* pFile=new CFile;
	if (pFile->Open(m_fileName))
	{
		m_pFile=(FILE*)pFile;
	}
	else
	{
		delete pFile;
		pFile=NULL;
		m_pFile=NULL;
	}
	if (m_pFile == NULL) {
		throw new MP4Error(errno, "failed", "MP4Open");
	}

	if (m_mode == 'r') {
		m_orgFileSize = m_fileSize = pFile->GetLength(); //s.st_size;
	} else {
		m_orgFileSize = m_fileSize = 0;
	}
}

void XMP4File::Close()
{
	if (m_mode == 'w') {
		SetIntegerProperty("moov.mvhd.modificationTime", 
			MP4GetAbsTimestamp());

		FinishWrite();
	}

	CFile* pFile=(CFile*)m_pFile;
	pFile->Close();
	delete pFile;
	m_pFile = NULL;
}

u_int32_t XMP4File::ReadBytes(u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile)
{
	// handle degenerate cases
	if (numBytes == 0) {
		return 0;
	}

	ASSERT(pBytes);
	WARNING(m_numReadBits > 0);

	if (pFile == NULL) {
		pFile = m_pFile;
	}
	ASSERT(pFile);

	if (m_memoryBuffer == NULL) {
		CFile* file=(CFile*)pFile;
		if (file->Read(pBytes, numBytes) != numBytes) {
			if (file->GetPosition()>=file->GetLength()) {
				throw new MP4Error(
					"not enough bytes, reached end-of-file",
					"MP4ReadBytes");
			} else {
				throw new MP4Error(errno, "MP4ReadBytes");
			}
		}
	} else {
		if (m_memoryBufferPosition + numBytes > m_memoryBufferSize) {
			throw new MP4Error(
				"not enough bytes, reached end-of-memory",
				"MP4ReadBytes");
		}
		memcpy(pBytes, &m_memoryBuffer[m_memoryBufferPosition], numBytes);
		m_memoryBufferPosition += numBytes;
	}
	return numBytes;
}

void XMP4File::WriteBytes(u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile)
{

}

void XMP4File::SetPosition(u_int64_t pos, FILE* pFile)
{
	if (m_memoryBuffer == NULL) {
		CFile* file=NULL;
		if (pFile == NULL) {
			ASSERT(m_pFile);
			file = (CFile*)m_pFile;
		}
		else
			file = (CFile*)pFile;

		fpos_t fpos;
		VAR_TO_FPOS(fpos, pos);
		if (file->Seek(pos) < 0) {
			throw new MP4Error(errno, "MP4SetPosition");
		}
	} else {
		if (pos >= m_memoryBufferSize) {
		  //		  abort();
			throw new MP4Error("position out of range", "MP4SetPosition");
		}
		m_memoryBufferPosition = pos;
	}
}

u_int64_t XMP4File::GetPosition(FILE* pFile)
{
	if (m_memoryBuffer == NULL) {
		CFile* file=NULL;
		if (pFile == NULL) {
			ASSERT(m_pFile);
			file = (CFile*)m_pFile;
		}
		else
			file = (CFile*)pFile;

		fpos_t fpos;
		uint64_t ret;
		if ((fpos=file->GetPosition()) < 0) {
			throw new MP4Error(errno, "MP4GetPosition");
		}
		FPOS_TO_VAR(fpos, uint64_t, ret);
		return ret;
	} else {
		return m_memoryBufferPosition;
	}
}
