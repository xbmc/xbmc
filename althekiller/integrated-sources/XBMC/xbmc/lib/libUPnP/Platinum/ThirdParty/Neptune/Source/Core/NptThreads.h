/*****************************************************************
|
|   Neptune - Threads
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_THREADS_H_
#define _NPT_THREADS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptConstants.h"
#include "NptInterfaces.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
const int NPT_ERROR_CALLBACK_HANDLER_SHUTDOWN = NPT_ERROR_BASE_THREADS-0;
const int NPT_ERROR_CALLBACK_NOTHING_PENDING  = NPT_ERROR_BASE_THREADS-1;

/*----------------------------------------------------------------------
|   NPT_MutexInterface
+---------------------------------------------------------------------*/
class NPT_MutexInterface
{
 public:
    // methods
    virtual           ~NPT_MutexInterface() {}
    virtual NPT_Result Lock()   = 0;
    virtual NPT_Result Unlock() = 0;
};

/*----------------------------------------------------------------------
|   NPT_Mutex
+---------------------------------------------------------------------*/
class NPT_Mutex : public NPT_MutexInterface
{
 public:
    // methods
               NPT_Mutex();
              ~NPT_Mutex() { delete m_Delegate; }
    NPT_Result Lock()   { return m_Delegate->Lock();   }
    NPT_Result Unlock() { return m_Delegate->Unlock(); }

 private:
    // members
    NPT_MutexInterface* m_Delegate;
};

/*----------------------------------------------------------------------
|   NPT_AutoLock
+---------------------------------------------------------------------*/
class NPT_AutoLock
{
 public:
    // methods
     NPT_AutoLock(NPT_Mutex &mutex) : m_Mutex(mutex)   {
        m_Mutex.Lock();
    }
    ~NPT_AutoLock() {
        m_Mutex.Unlock(); 
    }
        
 private:
    // members
    NPT_Mutex& m_Mutex;
};

/*----------------------------------------------------------------------
|   NPT_Lock
+---------------------------------------------------------------------*/
template <typename T> 
class NPT_Lock : public T,
                 public NPT_Mutex
{
};

/*----------------------------------------------------------------------
|   NPT_SharedVariableInterface
+---------------------------------------------------------------------*/
class NPT_SharedVariableInterface
{
 public:
    // methods
    virtual           ~NPT_SharedVariableInterface() {}
    virtual NPT_Result SetValue(NPT_Integer value)        = 0;
    virtual NPT_Result GetValue(NPT_Integer& value)       = 0;
    virtual NPT_Result WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) = 0;
    virtual NPT_Result WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) = 0;
};

/*----------------------------------------------------------------------
|   NPT_SharedVariable
+---------------------------------------------------------------------*/
class NPT_SharedVariable : public NPT_SharedVariableInterface
{
 public:
    // methods
               NPT_SharedVariable(NPT_Integer value = 0);
              ~NPT_SharedVariable() { delete m_Delegate; }
    NPT_Result SetValue(NPT_Integer value) { 
        return m_Delegate->SetValue(value); 
    }
    NPT_Result GetValue(NPT_Integer& value) { 
        return m_Delegate->GetValue(value); 
    }
    NPT_Result WaitUntilEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) { 
        return m_Delegate->WaitUntilEquals(value, timeout); 
    }
    NPT_Result WaitWhileEquals(NPT_Integer value, NPT_Timeout timeout = NPT_TIMEOUT_INFINITE) { 
        return m_Delegate->WaitWhileEquals(value, timeout); 
    }

 private:
    // members
    NPT_SharedVariableInterface* m_Delegate;
};

/*----------------------------------------------------------------------
|   NPT_AtomicVariableInterface
+---------------------------------------------------------------------*/
class NPT_AtomicVariableInterface
{
 public:
    // methods
    virtual             ~NPT_AtomicVariableInterface() {}
    virtual  NPT_Integer Increment() = 0;
    virtual  NPT_Integer Decrement() = 0;
    virtual  NPT_Integer GetValue()  = 0;
    virtual  void        SetValue(NPT_Integer value)  = 0;
};

/*----------------------------------------------------------------------
|   NPT_AtomicVariable
+---------------------------------------------------------------------*/
class NPT_AtomicVariable : public NPT_AtomicVariableInterface
{
 public:
    // methods
                NPT_AtomicVariable(NPT_Integer value = 0);
               ~NPT_AtomicVariable()        { delete m_Delegate;             }
    NPT_Integer Increment()                 { return m_Delegate->Increment();}
    NPT_Integer Decrement()                 { return m_Delegate->Decrement();}
    void        SetValue(NPT_Integer value) { m_Delegate->SetValue(value);   }
    NPT_Integer GetValue()                  { return m_Delegate->GetValue(); }

 private:
    // members
    NPT_AtomicVariableInterface* m_Delegate;
};

/*----------------------------------------------------------------------
|   NPT_Runnable
+---------------------------------------------------------------------*/
class NPT_Runnable
{
public:
    virtual ~NPT_Runnable() {}  
    virtual void Run() = 0;
};

/*----------------------------------------------------------------------
|   NPT_ThreadInterface
+---------------------------------------------------------------------*/
class NPT_ThreadInterface: public NPT_Runnable, public NPT_Interruptible
{
 public:
    // methods
    virtual           ~NPT_ThreadInterface() {}
    virtual NPT_Result Start() = 0;
    virtual NPT_Result Wait(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE)  = 0;
};

/*----------------------------------------------------------------------
|   NPT_Thread
+---------------------------------------------------------------------*/
class NPT_Thread : public NPT_ThreadInterface
{
 public:
    // types
    typedef unsigned long ThreadId;

    // class methods
    static ThreadId GetCurrentThreadId();

    // methods
    NPT_Thread(bool detached = false);
    NPT_Thread(NPT_Runnable& target, bool detached = false);
   ~NPT_Thread() { delete m_Delegate; }

    // NPT_ThreadInterface methods
    NPT_Result Start() { 
        return m_Delegate->Start(); 
    } 
    NPT_Result Wait(NPT_Timeout timeout = NPT_TIMEOUT_INFINITE)  { 
        return m_Delegate->Wait(timeout);  
    }

    // NPT_Runnable methods
    virtual void Run() {}

    // NPT_Interruptible methods
    virtual NPT_Result Interrupt() { 
        return m_Delegate->Interrupt(); 
    }

 private:
    // members
    NPT_ThreadInterface* m_Delegate;
};


/*----------------------------------------------------------------------
|   NPT_ThreadCallbackReceiver
+---------------------------------------------------------------------*/
class NPT_ThreadCallbackReceiver
{
public:
    virtual ~NPT_ThreadCallbackReceiver() {}
    virtual void OnCallback(void* args) = 0;
};

/*----------------------------------------------------------------------
|   NPT_ThreadCallbackSlot
+---------------------------------------------------------------------*/
class NPT_ThreadCallbackSlot
{
public:
    // types
    class NotificationHelper {
    public:
        virtual ~NotificationHelper() {};
        virtual void Notify(void) = 0;
    };

    // constructor
    NPT_ThreadCallbackSlot();

    // methods
    NPT_Result ReceiveCallback(NPT_ThreadCallbackReceiver& receiver, NPT_Timeout timeout = 0);
    NPT_Result SendCallback(void* args);
    NPT_Result SetNotificationHelper(NotificationHelper* helper);
    NPT_Result Shutdown();

protected:
    // members
    volatile void*      m_CallbackArgs;
    volatile bool       m_Shutdown;
    NPT_SharedVariable  m_Pending;
    NPT_SharedVariable  m_Ack;
    NPT_Mutex           m_ReadLock;
    NPT_Mutex           m_WriteLock;
    NotificationHelper* m_NotificationHelper;
};

#endif // _NPT_THREADS_H_
