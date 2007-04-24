/*****************************************************************
|
|   Neptune - References
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#ifndef _NPT_REFERENCES_H_
#define _NPT_REFERENCES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConstants.h"

/*----------------------------------------------------------------------
|   NPT_Reference
+---------------------------------------------------------------------*/
template <typename T>
class NPT_Reference
{
public:
    // constructors and destructor
    NPT_Reference() : m_Object(NULL), m_Counter(NULL) {}
    explicit NPT_Reference(T* object) : 
        m_Object(object), m_Counter(object?new NPT_Cardinal(1):NULL) {}

    NPT_Reference(const NPT_Reference<T>& ref) :
        m_Object(ref.m_Object), m_Counter(ref.m_Counter) {
        if (m_Counter) ++(*m_Counter);
    }

    // this methods should be private, but this causes a problem on some
    // compilers, because we need this function in order to implement
    // the cast operator operator NPT_Reference<U>() below, which would
    // have to be marked as a friend, and friend declarations with the 
    // same class name confuses some compilers
    NPT_Reference(T* object, NPT_Cardinal* counter) : 
        m_Object(object), m_Counter(counter) {
        if (m_Counter) ++(*m_Counter);
    }

    ~NPT_Reference() {
        Release();
    }

    // overloaded operators
    NPT_Reference<T>& operator=(const NPT_Reference<T>& ref) {
        if (this != &ref) {
            Release();
            m_Object = ref.m_Object;
            m_Counter = ref.m_Counter;
            if (m_Counter) ++(*m_Counter);
        }
        return *this;
    }
    NPT_Reference<T>& operator=(T* object) {
        Release();
        m_Object = object;
        m_Counter = object?new NPT_Cardinal(1):NULL;
        return *this;
    }
    T& operator*() const { return *m_Object; }
    T* operator->() const { return m_Object; }

    bool operator==(const NPT_Reference<T>& ref) {
        return m_Object == ref.m_Object;
    } 
    bool operator!=(const NPT_Reference<T>& ref) {
        return m_Object != ref.m_Object;
    }

    // overloaded cast operators
    template <typename U> operator NPT_Reference<U>() {
        return NPT_Reference<U>(m_Object, m_Counter);
    }

    // methods
    T* AsPointer() const { return m_Object; }
    bool IsNull() const { return m_Object == 0; }

private:
    // methods
    void Release() {
        if (m_Counter && --(*m_Counter) == 0) {
            delete m_Counter;
            delete m_Object;
        }
    }

    // members
    T*            m_Object;
    NPT_Cardinal* m_Counter;
};

#endif // _NPT_REFERENCES_H_
