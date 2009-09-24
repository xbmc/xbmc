#include "Boxalizer.h"
#include "Settings.h"

void CBoxalizer::CleanUp()
{
	while(myLines)
		DelLine(myLines);

	if(m_pTexture)
		m_pTexture->Release();
}

bool CBoxalizer::Init(LPDIRECT3DDEVICE8 pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
	myLines = NULL;
	m_dwLastNewBlock = timeGetTime() - g_stSettings.m_iLingerTime;

	m_fNextZ = 40.0f;
	m_pTexture = NULL;

	//load the texture
	char szTexFile[1024];
	strcpy(szTexFile, "Q:\\Visualisations\\");
	strcat(szTexFile, g_stSettings.m_szTextureFile);

	if(FAILED(D3DXCreateTextureFromFile(m_pd3dDevice, szTexFile, &m_pTexture)))
	{
		m_pTexture = NULL;
	}
	return true;
}

bool CBoxalizer::Set(float *pFreqData)
{
	//if static cam and now rows, or its time to add another row, add one
	if((g_stSettings.m_bCamStatic && !myLines) || (!g_stSettings.m_bCamStatic && timeGetTime() - m_dwLastNewBlock > (DWORD)g_stSettings.m_iLingerTime))
	{
		LineListLine *newLine = AddLine();
		newLine->myLine.Set(pFreqData);
		m_lllCurrentActive = newLine;
		m_fNextZ += g_stSettings.m_fBarDepth;
		m_dwLastNewBlock = timeGetTime();
	} 
	else //else keep updating the current row
	{	
		m_lllCurrentActive->myLine.Set(pFreqData);
	}

	return true;
}

void CBoxalizer::Render(float fCamZ)
{
	LineListLine *searchList = myLines;
	
	if(!myLines)
		return;
	
	while(searchList != NULL)
	{
		//if this row is visible draw it, else remove it (theres no going back!"£!?"£!23132mwahhahahah)
		if(searchList->myLine.CheckVisible(fCamZ))
		{
			searchList->myLine.Render(m_pTexture);
		} else {
			LineListLine *tmp = searchList->next;
			DelLine(searchList);
			searchList = tmp;
		}

		if(searchList->next)
			searchList = searchList->next;
		else
			searchList = NULL;
	}

}

//Add a line to the list
LineListLine *CBoxalizer::AddLine()
{
	LineListLine *searchList = myLines;

	if(searchList == NULL)
	{
		myLines = (LineListLine*)malloc(sizeof(LineListLine));
		myLines->next = myLines->prev = NULL;
		myLines->myLine.Init(m_pd3dDevice, m_fNextZ);
		
		return myLines;

	} else {
		//jump to the end of the list
		for(; searchList->next != NULL; searchList = searchList->next);
		searchList->next = (LineListLine*)malloc(sizeof(LineListLine));
		searchList->next->next = NULL;
		searchList->next->prev = searchList;
		searchList->next->myLine.Init(m_pd3dDevice, m_fNextZ);

		return searchList->next;
	}

}

void CBoxalizer::DelLine(LineListLine *myListItem)
{
	if(!myListItem)
		return;

	if(!myListItem->prev && !myListItem->next)
	{
		myLines = NULL;
	} else if(!myListItem->prev && myListItem->next)
	{
		myLines = myLines->next;
		myLines->prev = NULL;
	} else if(myListItem->prev && myListItem->next)
	{
		myListItem->next->prev = myListItem->prev;
		myListItem->prev->next = myListItem->next;
	} else {
		myListItem->prev->next = NULL;
	}

	myListItem->myLine.CleanUp();
	free(myListItem);
}

float CBoxalizer::GetNextZ()
{
	return m_fNextZ;
}

DWORD CBoxalizer::GetLastRowTime()
{
	return m_dwLastNewBlock;
}
