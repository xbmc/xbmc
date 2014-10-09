/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef SMARTLIST_H
#define SMARTLIST_H

#ifndef _SMARTPTR_H
#include "SmartPtr.h"
#endif

namespace Com
{

#include "threads/CriticalSection.h"
template <class T>
struct NoOp
{
  void operator()(T& t)
  {
  }
};

template <class T>
class List
{
protected:

  // Nodes in the linked list
  struct Node
  {
    Node *prev;
    Node *next;
    T    item;

    Node() : prev(NULL), next(NULL)
    {
    }

    Node(T item) : prev(NULL), next(NULL)
    {
      this->item = item;
    }

    T Item() const { return item; }
  };

public:

  // Object for enumerating the list.
  class POSITION
  {
    friend class List<T>;

  public:
    POSITION() : pNode(NULL)
    {
    }

    bool operator==(const POSITION &p) const
    {
      return pNode == p.pNode;
    }

    bool operator!=(const POSITION &p) const
    {
      return pNode != p.pNode;
    }

  private:
    const Node *pNode;

    POSITION(Node *p) : pNode(p) 
    {
    }
  };

protected:
  Node    m_anchor;  // Anchor node for the linked list.
  DWORD   m_count;   // Number of items in the list.

  Node* Front() const
  {
    return m_anchor.next;
  }

  Node* Back() const
  {
    return m_anchor.prev;
  }

  virtual HRESULT InsertAfter(T item, Node *pBefore)
  {
    if (pBefore == NULL)
    {
      return E_POINTER;
    }

    Node *pNode = new Node(item);
    if (pNode == NULL)
    {
      return E_OUTOFMEMORY;
    }

    Node *pAfter = pBefore->next;

    pBefore->next = pNode;
    pAfter->prev = pNode;

    pNode->prev = pBefore;
    pNode->next = pAfter;

    m_count++;

    return S_OK;
  }

  virtual HRESULT GetItem(const Node *pNode, T* ppItem)
  {
    if (pNode == NULL || ppItem == NULL)
    {
      return E_POINTER;
    }

    *ppItem = pNode->item;
    return S_OK;
  }

  // RemoveItem:
  // Removes a node and optionally returns the item.
  // ppItem can be NULL.
  virtual HRESULT RemoveItem(Node *pNode, T *ppItem)
  {
    if (pNode == NULL)
    {
      return E_POINTER;
    }

    assert(pNode != &m_anchor); // We should never try to remove the anchor node.
    if (pNode == &m_anchor)
    {
      return E_INVALIDARG;
    }


    T item;

    // The next node's previous is this node's previous.
    pNode->next->prev = pNode->prev;

    // The previous node's next is this node's next.
    pNode->prev->next = pNode->next;

    item = pNode->item;
    delete pNode;

    m_count--;

    if (ppItem)
    {
      *ppItem = item;
    }

    return S_OK;
  }

public:

  List()
  {
    m_anchor.next = &m_anchor;
    m_anchor.prev = &m_anchor;

    m_count = 0;
  }

  virtual ~List()
  {
    Clear();
  }

  // Insertion functions
  HRESULT InsertBack(T item)
  {
    return InsertAfter(item, m_anchor.prev);
  }


  HRESULT InsertFront(T item)
  {
    return InsertAfter(item, &m_anchor);
  }

  // RemoveBack: Removes the tail of the list and returns the value.
  // ppItem can be NULL if you don't want the item back. (But the method does not release the item.)
  HRESULT RemoveBack(T *ppItem)
  {
    if (IsEmpty())
    {
      return E_FAIL;
    }
    else
    {
      return RemoveItem(Back(), ppItem);
    }
  }

  // RemoveFront: Removes the head of the list and returns the value.
  // ppItem can be NULL if you don't want the item back. (But the method does not release the item.)
  HRESULT RemoveFront(T *ppItem)
  {
    if (IsEmpty())
    {
      return E_FAIL;
    }
    else
    {
      return RemoveItem(Front(), ppItem);
    }
  }

  // GetBack: Gets the tail item.
  HRESULT GetBack(T *ppItem)
  {
    if (IsEmpty())
    {
      return E_FAIL;
    }
    else
    {
      return GetItem(Back(), ppItem);
    }
  }

  // GetFront: Gets the front item.
  HRESULT GetFront(T *ppItem)
  {
    if (IsEmpty())
    {
      return E_FAIL;
    }
    else
    {
      return GetItem(Front(), ppItem);
    }
  }


  // GetCount: Returns the number of items in the list.
  DWORD GetCount() const { return m_count; }

  bool IsEmpty() const
  {
    return (GetCount() == 0);
  }

  // Clear: Takes a functor object whose operator()
  // frees the object on the list.
  template <class FN>
  void Clear(FN& clear_fn)
  {
    Node *n = m_anchor.next;

    // Delete the nodes
    while (n != &m_anchor)
    {
      clear_fn(n->item);

      Node *tmp = n->next;
      delete n;
      n = tmp;
    }

    // Reset the anchor to point at itself
    m_anchor.next = &m_anchor;
    m_anchor.prev = &m_anchor;

    m_count = 0;
  }

  // Clear: Clears the list. (Does not delete or release the list items.)
  virtual void Clear()
  {
    Clear<NoOp<T>>(NoOp<T>());
  }


  // Enumerator functions

  POSITION FrontPosition()
  {
    if (IsEmpty())
    {
      return POSITION(NULL);
    }
    else
    {
      return POSITION(Front());
    }
  }

  POSITION EndPosition() const
  {
    return POSITION();
  }

  HRESULT GetItemPos(POSITION pos, T *ppItem)
  {   
    if (pos.pNode)
    {
      return GetItem(pos.pNode, ppItem);
    }
    else 
    {
      return E_FAIL;
    }
  }

  POSITION Next(const POSITION pos)
  {
    if (pos.pNode && (pos.pNode->next != &m_anchor))
    {
      return POSITION(pos.pNode->next);
    }
    else
    {
      return POSITION(NULL);
    }
  }

  // Remove an item at a position. 
  // The item is returns in ppItem, unless ppItem is NULL.
  // NOTE: This method invalidates the POSITION object.
  HRESULT Remove(POSITION& pos, T *ppItem)
  {
    if (pos.pNode)
    {
      // Remove const-ness temporarily...
      Node *pNode = const_cast<Node*>(pos.pNode);

      pos = POSITION();

      return RemoveItem(pNode, ppItem);
    }
    else
    {
      return E_INVALIDARG;
    }
  }

};

// Typical functors for Clear method.

// ComAutoRelease: Releases COM pointers.
// MemDelete: Deletes pointers to new'd memory.

class ComAutoRelease
{
public: 
  void operator()(IUnknown *p)
  {
    if (p)
    {
      p->Release();
    }
  }
};

class MemDelete
{
public: 
  void operator()(void *p)
  {
    if (p)
    {
      delete p;
    }
  }
};

template <class T, bool NULLABLE = FALSE>
class ComPtrList : public List<T*>
{
public:

  typedef T* Ptr;

  void Clear()
  {
    List<Ptr>::Clear(ComAutoRelease());
  }

  ~ComPtrList()
  {
    Clear();
  }

protected:
  HRESULT InsertAfter(Ptr item, Node *pBefore)
  {
    // Do not allow NULL item pointers unless NULLABLE is true.
    if (!item && !NULLABLE)
    {
      return E_POINTER;
    }

    if (item)
    {
      item->AddRef();
    }

    HRESULT hr = List<Ptr>::InsertAfter(item, pBefore);
    if (FAILED(hr))
    {
      SAFE_RELEASE(item);
    }
    return hr;
  }

  HRESULT GetItem(const Node *pNode, Ptr* ppItem)
  {
    Ptr pItem = NULL;

    // The base class gives us the pointer without AddRef'ing it.
    // If we return the pointer to the caller, we must AddRef().
    HRESULT hr = List<Ptr>::GetItem(pNode, &pItem);
    if (SUCCEEDED(hr))
    {
      assert(pItem || NULLABLE);
      if (pItem)
      {
        *ppItem = pItem;
        (*ppItem)->AddRef();
      }
    }
    return hr;
  }

  HRESULT RemoveItem(Node *pNode, Ptr *ppItem)
  {
    // ppItem can be NULL, but we need to get the
    // item so that we can release it. 

    // If ppItem is not NULL, we will AddRef it on the way out.

    Ptr pItem = NULL;

    HRESULT hr = List<Ptr>::RemoveItem(pNode, &pItem);

    if (SUCCEEDED(hr))
    {
      assert(pItem || NULLABLE);
      if (ppItem && pItem)
      {
        *ppItem = pItem;
        (*ppItem)->AddRef();
      }

      SAFE_RELEASE(pItem);
    }

    return hr;
  }
};

//-----------------------------------------------------------------------------
// ThreadSafeQueue template
// Thread-safe queue of COM interface pointers.
//
// T: COM interface type.
//
// This class is used by the scheduler. 
//
// Note: This class uses a critical section to protect the state of the queue.
// With a little work, the scheduler could probably use a lock-free queue.
//-----------------------------------------------------------------------------

template <class T>
class ThreadSafeQueue
{
public:
  HRESULT Queue(T *p)
  {
    CSingleLock lock(m_lock);
    return m_list.InsertBack(p);
  }

  HRESULT Dequeue(T **pp)
  {
    CSingleLock lock(m_lock);

    if (m_list.IsEmpty())
    {
      *pp = NULL;
      return S_FALSE;
    }

    return m_list.RemoveFront(pp);
  }

  HRESULT PutBack(T *p)
  {
    CSingleLock lock(m_lock);
    return m_list.InsertFront(p);
  }

  void Clear() 
  {
    CSingleLock lock(m_lock);
    m_list.Clear();
  }


private:
  CCriticalSection m_lock;  
  ComPtrList<T>  m_list;
};

//This is just easier than auto_ptr which not allow copy constructors for stl containers
template< typename T >
class Auto_Ptr
{
public:
	Auto_Ptr() throw() :
		m_p( NULL )
	{
	}
	template< typename Y >
	Auto_Ptr( Auto_Ptr< Y >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}

	Auto_Ptr( Auto_Ptr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}

	explicit Auto_Ptr( T* p ) throw() :
		m_p( p )
	{
	}
	~Auto_Ptr() throw()
	{
		Release();
	}

	// Templated version to allow pBase = pDerived
	template< typename Y >
	Auto_Ptr< T >& operator=( Auto_Ptr< Y >& p ) throw()
	{
		if(m_p==p.m_p)
		{
			// This means that two Auto_Ptr of two different types had the same m_p in them
			// which is never correct
			_ASSERTE(FALSE);
		}
		else
		{
			Release();
			Attach( p.Detach() );  // Transfer ownership
		}
		return( *this );
	}
	Auto_Ptr< T >& operator=( Auto_Ptr< T >& p ) throw()
	{
		if(*this==p)
		{
			if(this!=&p)
			{
				p.Detach();
			}
			else
			{
			}
		}
		else
		{
			Release();
			Attach( p.Detach() );  // Transfer ownership
		}
		return( *this );
	}

  //OPERATOR FOR RETURNING std::auto_ptr
  operator std::auto_ptr< T >()//(_In_ const std::auto_ptr<Q>& lp) throw()
	{
    return std::auto_ptr<T>(*this);
	}

  Auto_Ptr< T >& get() throw()
  {
    return *this;
  }
  //OPERATOR FOR RETURNING boost::shared_ptr
  /*operator boost::shared_ptr< T >()
	{
    return boost::shared_ptr<T>(*this);
	}*/

	// basic comparison operators
	bool operator!=(Auto_Ptr<T>& p) const
	{
		return !operator==(p);
	}

	bool operator==(Auto_Ptr<T>& p) const
	{
		return m_p==p.m_p;
	}

	operator T*() const throw()
	{
		return( m_p );
	}

	T* operator->() const throw()
	{
		SMARTASSUME( m_p != NULL );
		return( m_p );
	}

	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		SMARTASSUME( m_p == NULL );
		m_p = p;
	}
	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the object pointed to, and set the pointer to NULL
	void Release() throw()
	{
		delete m_p;
		m_p = NULL;
	}
  // Delete the object pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete m_p;
		m_p = NULL;
	}
public:
	T* m_p;
};

#ifdef __STREAMS__
/*template<class T>
class Auto_List : public CGenericList<T>
{
public:
  Auto_List( UINT nBlockSize = 10 ):CGenericList() {}
  bool empty() { return (GetCount() < 1);}
  bool IsEmpty() { return (GetCount() < 1);}
  int size() { return GetCount(); }
  T* GetPrev(POSITION& rp) {return Get(Prev(rp));}
};*/

template< class T >
class Auto_Ptr_List :	public CGenericList< T >
{
public:
  Auto_Ptr_List()  : CGenericList<T>("AutoPtrList"){}
  bool empty() { return (GetCount() < 1);}
  bool IsEmpty() { return (GetCount() < 1);}
  int size() { return GetCount(); }
  T* GetPrev(POSITION& rp) {return Get(Prev(rp));}
private:
	// Private to prevent use
	Auto_Ptr_List( const Auto_Ptr_List& ) throw();
	Auto_Ptr_List& operator=( const Auto_Ptr_List& ) throw();
};
#endif



};//End Com Namespace

#endif