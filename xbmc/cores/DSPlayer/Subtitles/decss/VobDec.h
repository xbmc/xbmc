#pragma once

class CVobDec
{
	int m_lfsr0, m_lfsr1;

	void ClockLfsr0Forward(int& lfsr0);
	void ClockLfsr1Forward(int& lfsr1);
	void ClockBackward(int& lfsr0, int& lfsr1);
	void Salt(const BYTE salt[5], int& lfsr0, int& lfsr1);
	int FindLfsr(const BYTE* crypt, int offset, const BYTE* plain);

public:
	CVobDec();
	virtual ~CVobDec();

	bool m_fFoundKey;

	bool FindKey(BYTE* buff);
	void Decrypt(BYTE* buff);
};
