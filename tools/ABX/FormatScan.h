#ifndef FORMAT_SCAN_H
#define FORMAT_SCAN_H

//=============================
//		FormatScan.h
//=============================

#include "stdafx.h"

class FormatScan{

private:

	std::string Ext;
	unsigned char Header[22];
	int Offset;
	bool Xwb360;

public:

	bool Detect(FILE* in);
	bool Parse(FILE* in);

};
#endif
