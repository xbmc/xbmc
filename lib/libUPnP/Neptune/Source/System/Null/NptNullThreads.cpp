/*****************************************************************
|
|      Neptune - Threads :: Null Implementation
|
|      (c) 2001-2002 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptThreads.h"

/*----------------------------------------------------------------------
|       NPT_NullAtomicVariable
+---------------------------------------------------------------------*/
class NPT_NullAtomicVariable : public NPT_AtomicVariableInterface
{
 public:
    // methods
    NPT_NullAtomicVariable(int value) : m_Value(value) {}
   ~NPT_NullAtomicVariable() {}
    int  Increment() { return ++m_Value; }
    int  Decrement() { return --m_Value; }
    int  GetValue() { return m_Value; }
    void SetValue(int value) { m_Value = value; }

 private:
    // members
    volatile int m_Value;
};

/*----------------------------------------------------------------------
|       NPT_AtomicVariable::NPT_AtomicVariable
+---------------------------------------------------------------------*/
NPT_AtomicVariable::NPT_AtomicVariable(int value)
{
    m_Delegate = new NPT_NullAtomicVariable(value);
}
