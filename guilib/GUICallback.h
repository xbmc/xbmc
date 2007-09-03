#if !defined(GUICALLBACK_H)
#define GUICALLBACK_H

#include "assert.h"
#include "memory.h"
#pragma warning( disable : 4786 )

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE FROM RUNTiME: An Event holds a weak reference to class method, it acts similar to a   //
// C# delegate method E.g. instantiate an Event and assign it an EventHandler, whenever the   //
// Event is fired the EventHandler is executed.                                               //
// - There is a 2nd variant of Event called a Callback, this only differs in that a Callback  //
// can return a value after the CallbackHandler has been executed.                            //
// - There is a nasty memcpy that gets us over the compiler restriction that prevents us from //
// casting between two different method pointers, not the cleanest solution granted, but it's //
// the only way I know how to get around this limitation.                                     //
////////////////////////////////////////////////////////////////////////////////////////////////

template <class Cookie>
class GUIEvent
{
public:

  typedef void (GUIEvent::*MethodPtr)(Cookie);

  GUIEvent()
  {
    m_pInstance = NULL;
    m_pMethod = NULL;
  }

  // Assign an EventHandler (EventHandler's are derived from Event)
  GUIEvent<Cookie> &operator=(GUIEvent<Cookie> &aEvent)
  {
    if (&aEvent)
    {
      m_pInstance = aEvent.m_pInstance;
      m_pMethod = aEvent.m_pMethod;
    }
    else
    {
      GUIEvent();
    }

    return *this;
  }

  // Are the class instance and method pointers initialised?
  bool HasAHandler()
  {
    return (m_pInstance && m_pMethod);
  }

  // Execute the associated class method
  void Fire(Cookie aCookie)
  {
    if (HasAHandler())
    {
      (m_pInstance->*m_pMethod)(aCookie);
    }
    else
    {
      // Event is uninitialized, no handler has been assigned
      assert(0);
    }
  }
protected:
  GUIEvent* m_pInstance;
  MethodPtr m_pMethod;
};


template <class Class, class Cookie>
class GUIEventHandler : public GUIEvent<Cookie>
{
public:
  typedef void (Class::*MethodPtr)(Cookie);

  GUIEventHandler(Class* pInstance, MethodPtr aMethodPtr)
  {
    m_pInstance = (GUIEvent<Cookie>*) ((LPVOID) pInstance);
    // Its dirty but it works!
    memcpy(&m_pMethod, &aMethodPtr, sizeof(m_pMethod));
  }
};


// Callbacks are identical to Events except that a Callback returns a result value
template <class Result, class Cookie>
class Callback
{
public:
  typedef Result (Callback::*MethodPtr)(Cookie);

  Callback()
  {
    m_pInstance = NULL;
    m_pMethod = NULL;
  }

  // Assign a CallbackHandler (CallbackHandler's are derived from Callback)
  Callback<Result, Cookie> &operator=(Callback<Result, Cookie> &aCallback)
  {
    if (&aCallback)
    {
      m_pInstance = aCallback.m_pInstance;
      m_pMethod = aCallback.m_pMethod;
    }
    else
    {
      Callback();
    }

    return *this;
  }

  // Are the class instance and method pointers initialised?
  bool HasAHandler()
  {
    return (m_pInstance && m_pMethod);
  }

  // Execute the associated class method and return the result
  Result Fire(Cookie aCookie)
  {
    if (HasAHandler())
    {
      return (m_pInstance->*m_pMethod)(aCookie);
    }

    // Callback is uninitialized, no handler has been assigned
    assert(0);
    return 0;
  }

protected:
  Callback* m_pInstance;
  MethodPtr m_pMethod;
};


template <class Class, class Result, class Cookie>
class CallbackHandler : public Callback<Result, Cookie>
{
public:
  typedef Result (Class::*MethodPtr)(Cookie);

  CallbackHandler (Class* pInstance, MethodPtr aMethodPtr)
  {
    m_pInstance = (Callback<Result, Cookie>*) ((LPVOID) pInstance);
    // Its dirty but it works!
    memcpy(&m_pMethod, &aMethodPtr, sizeof(m_pMethod));
  }
};

#endif // GUICALLBACK_H

