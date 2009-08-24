#ifndef ACCUM_H_
#define ACCUM_H_

class CPayloadAccumulator
{
public:
  CPayloadAccumulator();
  virtual ~CPayloadAccumulator();
  void StartPayload(unsigned int len);
  void StartPayloadUnbounded();
  bool AddData(unsigned char* pData, unsigned int* bytes); // Returns true if expected payload is complete
  unsigned int GetLen();
  unsigned char* GetData(unsigned int len = 0);
  unsigned int GetPayloadLen();
  void Reset();
  bool IsUnbounded();
  unsigned char* Detach(bool release = false);
protected:
  unsigned char* CreateBuffer(unsigned int len);
  void FreeBuffer(unsigned char* pBuf);
  unsigned char* m_pData;
  unsigned int m_DataLen;
  unsigned int m_PayloadLen;
  unsigned int m_BufferLen;
  bool m_Unbounded;

  unsigned int m_MaxLen;
};

#endif // ACCUM_H_
