#pragma once
#include "../filesystem/file.h"

class CArchive;

class ISerializable
{
public:
	virtual void Serialize(CArchive& ar)=0;
};

class CArchive
{
public:
	CArchive(CFile* pFile, int mode);
	~CArchive();
	//	storing
	CArchive& operator<<(float f);
	CArchive& operator<<(int i);
	CArchive& operator<<(__int64 i64);
	CArchive& operator<<(long l);
	CArchive& operator<<(bool b);
	CArchive& operator<<(const CStdString& str);
	CArchive& operator<<(const CStdStringW& str);
	CArchive& operator<<(const SYSTEMTIME& time);
	CArchive& operator<<(ISerializable& obj);

	//	loading
	CArchive& operator>>(float& f);
	CArchive& operator>>(int& i);
	CArchive& operator>>(__int64& i64);
	CArchive& operator>>(long& l);
	CArchive& operator>>(bool& b);
	CArchive& operator>>(CStdString& str);
	CArchive& operator>>(CStdStringW& str);
	CArchive& operator>>(SYSTEMTIME& time);
	CArchive& operator>>(ISerializable& obj);

	bool IsLoading();
	bool IsStoring();

	void Close();

	enum Mode {load=0, store};

protected:
	void	FlushBuffer();
	CFile* m_pFile;
	int m_iMode;
	LPBYTE m_pBuffer;
	int m_BufferPos;
};

