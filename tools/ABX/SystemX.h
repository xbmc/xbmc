#ifndef SYSTEMX_H
#define SYSTEMX_H

//=============================
//		SystemX.h
//=============================

#include "stdafx.h"


class SystemX{

protected:
	 vector<string>SupportedFormats;

private:

	unsigned char* Buf;
	double Time;
	MEMORYSTATUS Memory;

	unsigned int SampleRate[4], Channels[4];
	unsigned long Offset[4], Chunk[4];
	char Name[100];
	bool  IsDirSet;
	

public:

	//int GetFiles();

	//void SetFiles(int files);

	//Ansistring GetExt(int index);

	//vector<Ansistring> GetExtVector();

	//void SetExt(Ansistring ext);

	//string GetOpenFileName();

	//void SetOpenFileName(string fileName);

	void SetSupportedFormats(string fmts);
	bool GetSupportedFormat(string fmtExt);
	u_char* MemoryAlloc(const u_long& size, FILE* stream);
	void MemoryDel();

	void ExtractError(string fileName, string dirPath);
	void TimerStart();
	double TimerStop();
	int Delay(const bool& fast);
	long GetAvailPhyMem();
	long GetAvailVirMem();
	bool FileExists(string name);
	bool FileIsReadOnly(string name);
	int GetWinVer();
	void ExecuteAndWait(string path);
	void BuildDirStruct(const char* path);
	int GetOpenFileNameExtLen();
	void DirNameSet();
	void DirPathSet(string path);
	void DirPathSet();
	void DirPathSetSilent(string path);
	string DirPathGet();
	bool DirPathIsSet();
	void DirBrowse();
	void DeleteDir(string path);
	string GetTempDir();
	void GetAllFiles(string path, bool recursive);
	string CalcSize (const u_long& size);
	u_long GetFileSize(FILE* in, const bool& updateSizeLbl);
	int HeaderScanFast(const int& startPos, const int& fileSize,
						const u_char* sig, FILE* in);
	int HeaderScan (const u_long& startPos, const u_long& fileSize,
						const char* sig, const u_short& sigSize, FILE* in);
	void IniDump(string& homeDir);

	void WritePcmHeader
	(
		const u_long& fileSize, const short& channels, const short& bits,
		const u_int& sampleRate, FILE* out
	);
	#ifdef DEBUG
	void WriteMsadpcmHeader
	(
		const u_long& fileSize, const short& channels, const u_int& sampleRate,
		FILE* out
	);
	#endif
	void WriteXadpcmHeader
	(
		const u_long& fileSize, const short& channels, const u_int& sampleRate,
		FILE* out
	);

	void PushFiles(const int& files);
	void PushFiles(FILE* in, const int& size);
	void PushOffset(FILE* in, const u_short& size);
	void PushOffset(const u_long& offset);
	void PushChunk(FILE* in, const u_short& size);
	void PushChunk(const u_long& chunk);
	void PushFileName(FILE* in, const u_short& size);
	void PushFileName(string name);
	void PushExtension(string ext);
	void PushAudioFormat(const char* audioFmt);
	void PushBits(const short& bits);
	void PushChannel(FILE* in, const u_int& size);
	void PushChannel(const short& channel);
	void PushSampleRate(FILE* in, const u_int& size);
	void PushSampleRate(const u_short& sampleRate);
	void ClearVariables();

	int RiffHeaderSize(FILE* in, const u_int& offset);
	u_int RiffSize(FILE* in, const u_int& offset);
	u_int WmaSize(FILE* in, const u_int& offset);
	u_int AsfSize(FILE* in, const u_int& offset);
	void RiffSpecs(FILE* in, const bool& getSizes);
	void WmaSpecs(FILE* in, const bool& getSizes);

	SystemX();
	~SystemX();


};// End System

extern SystemX sys; // Global instance

#endif
