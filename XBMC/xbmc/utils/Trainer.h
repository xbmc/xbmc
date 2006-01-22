#ifndef _TRAINER_H_
#define _TRAINER_H_

#include <vector>

#include "../../guilib/StdString.h"

class CTrainer 
{
public:
  CTrainer();
  ~CTrainer();

  bool Load(const CStdString& strPath);
  inline const CStdString& GetName() const
  {
    return m_vecText[0];
  }
  void GetTitleIds(unsigned int& title1, unsigned int& title2, unsigned int& title3) const;
  void GetOptionLabels(std::vector<CStdString>& vecOptionLabels) const;
  
  void SetOptions(unsigned char* options); // copies 100 entries!!!
  inline int GetNumberOfOptions() const
  {
    return m_vecText.size()-2;
  }
  inline const unsigned char* const data() const
  {
    return m_pTrainerData;
  }

  inline unsigned char* data()
  {
    return m_pTrainerData;
  }

  inline unsigned char* GetOptions()
  {
    return m_pTrainerData+m_iOptions;
  }

  inline bool IsXBTF() const
  {
    return m_bIsXBTF;
  }

  inline const CStdString& GetPath()
  {
    return m_strPath;
  }

  inline int Size() const
  {
    return m_iSize;
  }
protected:
  unsigned char* m_pData;
  unsigned char* m_pTrainerData; // no alloc
  unsigned int m_iSize;
  unsigned int m_iNumOptions;
  unsigned int m_iOptions;
  bool m_bIsXBTF;
  char m_szCreationKey[200];
  std::vector<CStdString> m_vecText;
  CStdString m_strPath;
};

#endif
