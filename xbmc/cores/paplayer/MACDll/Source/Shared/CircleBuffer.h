#ifndef APE_CIRCLEBUFFER_H
#define APE_CIRCLEBUFFER_H

class CCircleBuffer  
{
public:

    // construction / destruction
    CCircleBuffer();
    virtual ~CCircleBuffer();

    // create the buffer
    void CreateBuffer(int nBytes, int nMaxDirectWriteBytes);

    // query
    int MaxAdd();
    int MaxGet();

    // direct writing
    inline unsigned char * GetDirectWritePointer()
    {
        // return a pointer to the tail -- note that it will always be safe to write
        // at least m_nMaxDirectWriteBytes since we use an end cap region
        return &m_pBuffer[m_nTail];
    }

    inline void UpdateAfterDirectWrite(int nBytes)
    {
        // update the tail
        m_nTail += nBytes;

        // if the tail enters the "end cap" area, set the end cap and loop around
        if (m_nTail >= (m_nTotal - m_nMaxDirectWriteBytes))
        {
            m_nEndCap = m_nTail;
            m_nTail = 0;
        }
    }

    // get data
    int Get(unsigned char * pBuffer, int nBytes);

    // remove / empty
    void Empty();
    int RemoveHead(int nBytes);
    int RemoveTail(int nBytes);

private:

    int m_nTotal;
    int m_nMaxDirectWriteBytes;
    int m_nEndCap;
    int m_nHead;
    int m_nTail;
    unsigned char * m_pBuffer;
};


#endif // #ifndef APE_CIRCLEBUFFER_H
