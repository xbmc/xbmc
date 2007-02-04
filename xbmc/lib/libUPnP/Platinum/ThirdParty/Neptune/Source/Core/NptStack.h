/*****************************************************************
|
|   Neptune - Stack
|
|   (c) 2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#ifndef _NPT_STACK_H_
#define _NPT_STACK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptResults.h"
#include "NptTypes.h"
#include "NptList.h"

/*----------------------------------------------------------------------
|   NPT_Stack
+---------------------------------------------------------------------*/
template <typename T> 
class NPT_Stack : public NPT_List<T>
{
public:
    // methods
    NPT_Result Push(const T& value) {
        return Add(value);
    }

    NPT_Result Peek(T& value) {
        // NOTE: we must use the this-> accessor here because the standard
        // requires it when the member to look up is in a parent template
        if (this->m_ItemCount == 0) return NPT_ERROR_NO_SUCH_ITEM;
        value = this->m_Tail->m_Data;
        return NPT_SUCCESS;
    }

    NPT_Result Pop(T& value) {
        // NOTE: we must use the this-> accessor here because the standard
        // requires it when the member to look up is in a parent template
        if (this->m_ItemCount == 0) return NPT_ERROR_NO_SUCH_ITEM;
        typename NPT_List<T>::Iterator tail = this->GetLastItem();
        value = *tail;
        return this->Erase(tail);
    }
};

#endif // _NPT_STACK_H_
