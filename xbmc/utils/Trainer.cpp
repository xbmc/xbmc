#include "stdafx.h"
#include "Trainer.h"
#include "../FileSystem/File.h"
#include "../Util.h"
#include "log.h"

using namespace XFILE;

// header offsets etm
#define ETM_SELECTIONS_OFFSET 0x0A // option states (0 or 1)
#define ETM_SELECTIONS_TEXT_OFFSET 0x0E // option labels
#define ETM_ID_LIST 0x12 // TitleID(s) trainer is meant for
#define ETM_ENTRYPOINT 0x16 // entry point for etm file (really just com file)

// header offsets xbtf
#define XBTF_SELECTIONS_OFFSET 0x0A // option states (0 or 1)
#define XBTF_SELECTIONS_TEXT_OFFSET 0x0E // option labels
#define XBTF_ID_LIST 0x12 // TitleID(s) trainer is meant for
#define XBTF_SECTION 0x16 // section to patch the locations in memory our xbtf support functions end up
#define XBTF_ENTRYPOINT 0x1A // entry point for xbtf file (really com).  

CTrainer::CTrainer()
{
  m_pData = m_pTrainerData = NULL;
  m_iSize = 0;
  m_bIsXBTF = false;
}

CTrainer::~CTrainer()
{
  if (m_pData)
    delete[] m_pData;
  m_pData = NULL;
  m_pTrainerData = NULL;
  m_iSize = 0;
}

bool CTrainer::Load(const CStdString& strPath)
{
  CFile file;
  if (!file.Open(strPath))
    return false;

  if (CUtil::GetExtension(strPath).Equals(".xbtf"))
    m_bIsXBTF = true;
  else
    m_bIsXBTF = false;

  m_iSize = (unsigned int)file.GetLength();
  if (m_iSize < ETM_SELECTIONS_OFFSET)
  {
    CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
    return false;
  }
  m_pData = new unsigned char[(unsigned int)file.GetLength()+1];
  m_pData[file.GetLength()] = '\0'; // to make sure strlen doesn't crash
  file.Read(m_pData,m_iSize);
  file.Close();

  unsigned int iTextOffset;
  if (m_bIsXBTF)
  {
    void* buffer = m_pData;
    unsigned int trainerbytesread = m_iSize;

    __asm // unmangle trainer
    {
      pushad

      mov esi, buffer
      xor eax, eax
      add al, byte ptr [esi+027h]
      add al, byte ptr [esi+02Fh]
      add al, byte ptr [esi+037h]
      mov	ecx, 0FFFFFFh
      imul ecx
      xor dword ptr [esi], eax
      mov ebx, dword ptr [esi]
      add esi, 4
      xor eax, eax
      mov ecx, trainerbytesread
      sub ecx, 4
    loopme:
      xor byte ptr [esi], bl
      sub byte ptr [esi], al
      add eax, 3
      add eax, ecx
      inc esi
      loop loopme

      popad
    }

    strncpy(m_szCreationKey,(char*)(m_pData+4),200);
    unsigned int iKeyLength = strlen(m_szCreationKey)+1;
    if (m_szCreationKey[6] != '-')
    {
      CLog::Log(LOGERROR,"CTrainer::Load: Broken trainer %s",strPath.c_str());
      return false;
    }

    m_pTrainerData = m_pData+4+iKeyLength;
    unsigned int iTextLength = strlen((char*)m_pTrainerData)+1;
    // read scroller text here if interested
    m_pTrainerData += iTextLength;
    m_iSize -= 4+iKeyLength+iTextLength;
    iTextOffset = *((unsigned int*)(m_pTrainerData+XBTF_SELECTIONS_TEXT_OFFSET));
    m_iOptions = *((unsigned int*)(m_pTrainerData+XBTF_SELECTIONS_OFFSET));
  }
  else
  {
    iTextOffset = *((unsigned int*)(m_pData+ETM_SELECTIONS_TEXT_OFFSET));
    m_iOptions = *((unsigned int*)(m_pData+ETM_SELECTIONS_OFFSET));
    m_pTrainerData = m_pData;
  }
  
  if (iTextOffset > m_iSize)
  {
    CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
    return false;
  }

  m_iNumOptions = iTextOffset-m_iOptions;
     
  char temp[85];
  unsigned int i;
  for (i=0;i<m_iNumOptions+2;++i)
  {
    unsigned int iOffset;
    memcpy(&iOffset,m_pTrainerData+iTextOffset+4*i,4);
    if (!iOffset)
      break;

    if (iOffset > m_iSize || iTextOffset+4*i > m_iSize)
    {
      CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
      return false;
    }
    strcpy(temp,(char*)(m_pTrainerData+iOffset));
    m_vecText.push_back(temp);
  }

  m_iNumOptions = i-1;
 
  m_strPath = strPath;
  return true;
}

void CTrainer::GetTitleIds(unsigned int& title1, unsigned int& title2, unsigned int& title3) const
{
  if (m_pData)
  {
    DWORD ID_List;
    unsigned char* pList = m_pTrainerData+(m_bIsXBTF?XBTF_ID_LIST:ETM_ID_LIST); 
    memcpy(&ID_List, pList, 4);
    memcpy(&title1,m_pTrainerData+ID_List,4);
    memcpy(&title2,m_pTrainerData+ID_List+4,4);
    memcpy(&title3,m_pTrainerData+ID_List+8,4);
  }
  else
    title1 = title2 = title3 = 0;
}

void CTrainer::GetOptionLabels(std::vector<CStdString>& vecOptionLabels) const
{
  vecOptionLabels.clear();
  for (int i=0;i<int(m_vecText.size())-2;++i)
  {
    vecOptionLabels.push_back(m_vecText[i+2]);
  }
}

void CTrainer::SetOptions(unsigned char* options)
{
  memcpy(m_pTrainerData+m_iOptions,options,100);
}

