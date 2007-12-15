#ifndef SUPPORTED_H
#define SUPPORTED_H

//=============================
//		Supported.h
//=============================

#include "stdafx.h"

class Supported{

private:

public:

	// Format Read
	void 	AdfRead(FILE* in);
	//void 	AdxRead(FILE* in);
	bool 	AfsRead(FILE* in);
	//void 	ArcRead(FILE* in);
	//bool 	AsfMusRead(FILE* in);
	void 	BifRead(FILE* in);
	void 	BigCodeRead(FILE* in);
	bool 	BigEaRead(FILE* in);

	//// Debug ver
	//void 	BorRead(FILE* in);
	////

	//void	CabForzaRead(FILE* in);
	void 	DatRollerRead(FILE* in);
	void 	FsbRead(FILE* in);

	//bool 	Gta5Read();
	void 	HotRead(FILE* in);
	void 	HwxRead(FILE* in);
	bool 	LugRead();
	//void 	M3uRead();
	//bool 	MapHaloRead(FILE* in);
	void 	MpkRead(FILE* in);
	void 	MsxRead(FILE* in);
	bool 	PakGoblinRead(FILE* in);

	//// Debug ver
	//void 	PakDreamRead(FILE* in);
	////

	void 	PizRead(FILE* in);
	void 	PodRead(FILE* in);
	void 	RcfRead(FILE* in);
	void 	RezRead(FILE* in);
	void 	RfaRead(FILE* in);
	bool 	RwsRead(FILE* in);
	bool 	SampRead();
	//void 	Sh4Read(FILE* in);
	//void 	SrRead();
	//void 	StxRead(FILE* in);
	bool 	WadRead();
	//bool 	WavRead(FILE* in);

	//// Debug ver
	//void 	WavRawRead(FILE* in);
	////

	void 	WmapackRead(FILE* in);
	//bool 	WmaRead(FILE* in);
	//void 	WpxRead(FILE* in);
	//bool 	XbpRead();
	//bool 	XisoRead(FILE* in);
	//bool 	XwbRead(FILE* in, unsigned long startPos, const bool& xwb360);
	//bool 	XwcRead(FILE* in);
	//void 	ZsmRead(FILE* in);

	//bool 	MiscRead();

	//// Extract / Convert
	////bool AdxExtract();
	//bool 	EaExtract();
	//bool 	MapExtract(int i, unsigned char* buffer, FILE* in, FILE* out);
	bool StdExtract(const int& pFileNum);
	//bool StxExtract();

	//// FileSwap
	//bool 	StxSwap();
	//bool 	RcfSwap();
	//bool 	WpxSwap();
	//bool 	XwbSwap();


	long pos, offset[4], chunk[4];
	unsigned short sr[4];
	unsigned char chan[2], bits[2];

	unsigned short firstOffset[2];
	unsigned char fmt[4];
	long magic[4];

	// Used for halo maps
	unsigned short sounds, chunksTot[2], files[4];


};
#endif
