#ifndef APE_SMARTPTR_H
#define APE_SMARTPTR_H

// disable the operator -> on UDT warning
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4284)
#endif

/*************************************************************************************************
CSmartPtr - a simple smart pointer class that can automatically initialize and free memory
    note: (doesn't do garbage collection / reference counting because of the many pitfalls)
*************************************************************************************************/
template <class TYPE> class CSmartPtr
{
public:
    TYPE * m_pObject;
    BOOL m_bArray;
    BOOL m_bDelete;

    CSmartPtr()
    {
        m_bDelete = TRUE;
        m_pObject = NULL;
    }
    CSmartPtr(TYPE * a_pObject, BOOL a_bArray = FALSE, BOOL a_bDelete = TRUE)
    {
        m_bDelete = TRUE;
        m_pObject = NULL;
        Assign(a_pObject, a_bArray, a_bDelete);
    }

    ~CSmartPtr()
    {
        Delete();
    }

    void Assign(TYPE * a_pObject, BOOL a_bArray = FALSE, BOOL a_bDelete = TRUE)
    {
        Delete();

        m_bDelete = a_bDelete;
        m_bArray = a_bArray;
        m_pObject = a_pObject;
    }

    void Delete()
    {
        if (m_bDelete && m_pObject)
        {
            if (m_bArray)
                delete [] m_pObject;
            else
                delete m_pObject;

            m_pObject = NULL;
        }
    }

    void SetDelete(const BOOL a_bDelete)
    {
        m_bDelete = a_bDelete;
    }

    __inline TYPE * GetPtr() const
    {
        return m_pObject;
    }

    __inline operator TYPE * () const
    {
        return m_pObject;
    }

    __inline TYPE * operator ->() const
    {
        return m_pObject;
    }

    // declare assignment, but don't implement (compiler error if we try to use)
    // that way we can't carelessly mix smart pointers and regular pointers
    __inline void * operator =(void *) const;
};

#ifdef _MSC_VER
    #pragma warning(pop)
#endif _MSC_VER

#endif // #ifndef APE_SMARTPTR_H
