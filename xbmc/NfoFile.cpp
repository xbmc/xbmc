// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile()
{

}

CNfoFile::~CNfoFile()
{

}

HRESULT CNfoFile::Create(LPSTR szPath)
{
	if (FAILED(Load(szPath)))
		return E_FAIL;

	CHAR* szUrl = strstr(m_doc,"http://us.imdb");
	if (!szUrl )
	{
		szUrl = strstr(m_doc,"http://www.imdb.com");
	}
	if (szUrl)
	{
		char szScrape[1024];

		for (int i=0; i<1024; i++)
		{
			char c = szUrl[i];
			if ( (c==32) || (c==10) || (c==13) )
			{
				szScrape[i] = 0;
				break;
			}

			szScrape[i] = c;
		}

		m_strImDbUrl = szScrape;
	}

	Close();

	return S_OK;
}

HRESULT CNfoFile::Load(char* szFile)
{
	FILE* hFile;
	
	hFile  = fopen(szFile,"rb");
	if (hFile==NULL)
	{
		OutputDebugString("No such file: ");
		OutputDebugString(szFile);
		OutputDebugString("\n");
		return E_FAIL;
	}

	fseek(hFile,0,SEEK_END);
	m_size = ftell(hFile);

	fseek(hFile,0,SEEK_SET);
	
	m_doc = (char*) malloc(m_size);
	if (!m_doc)
	{
		m_size = 0;
		fclose(hFile);
		return E_FAIL;
	}

	if (fread(m_doc, m_size, 1, hFile)<=0)
	{
		delete m_doc;
		m_doc  = 0;
		m_size = 0;
		fclose(hFile);
		return E_FAIL;
	}

	fclose(hFile);
	return S_OK;
}

void CNfoFile::Close()
{
	if (m_doc!=NULL)
	{
		delete m_doc;
		m_doc  =0;
	}

	m_size =0;
}

