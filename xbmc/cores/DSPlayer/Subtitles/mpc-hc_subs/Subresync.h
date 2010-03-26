#pragma once
#include "..\subtitles\STS.h"

class CSubresync
{
public:
	CSubresync(void);
	~CSubresync(void);
	
	void AddShift(REFERENCE_TIME time, int val);
	void RemoveAll();
	bool IsModified() { return !m_delayTime.empty(); };
	bool SaveToDisk(ISubStream* pSubStream, double fps, const CStdString & movieName);
private:
	std::vector<REFERENCE_TIME> m_delayTime;
	std::vector<int> m_delayVal; 

	enum {NONE = 0, VOBSUB, TEXTSUB};
	int m_mode;

	CSimpleTextSubtitle m_sts;

	void SetSubtitle(ISubStream* pSubStream, double fps);
	int FindNearestSub(__int64 rtPos);
	void ShiftSubtitle(int nItem, long lValue);

};
