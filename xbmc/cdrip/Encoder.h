#ifndef _ENCODER_H
#define _ENCODER_H

#include <xtl.h>

#define ENC_ARTIST  11
#define ENC_TITLE   12
#define ENC_ALBUM   13
#define ENC_YEAR    14
#define ENC_COMMENT 15
#define ENC_TRACK   16
#define ENC_GENRE   17

class CEncoder
{
public:
	virtual bool Init(const char* strFile)=0;
	virtual int  Encode(int nNumBytesRead, BYTE* pbtStream)=0;
	virtual bool Close()=0;
	virtual void AddTag(int key,const char* value)=0;
};

#endif // _ENCODER_H
