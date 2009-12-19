#pragma once

//-----------------------------------------------------------------------------
// SamplePool class
//
// Manages a list of allocated samples.
//-----------------------------------------------------------------------------
#include "streams.h"
#include "mfapi.h"
#include <mferror.h>
#include "mftransform.h"
#ifndef CHECK_HR
#define CHECK_HR(hr) IF_FAILED_GOTO(hr, done)
#endif
#ifndef IF_FAILED_GOTO
#define IF_FAILED_GOTO(hr, label) if (FAILED(hr)) { goto label; }
#endif

#define S_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


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


    // ComPtrList class
    // Derived class that makes it safer to store COM pointers in the List<> class.
    // It automatically AddRef's the pointers that are inserted onto the list
    // (unless the insertion method fails). 
    //
    // T must be a COM interface type. 
    // example: ComPtrList<IUnknown>
    //
    // NULLABLE: If true, client can insert NULL pointers. This means GetItem can
    // succeed but return a NULL pointer. By default, the list does not allow NULL
    // pointers.

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
                S_RELEASE(item);
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

                S_RELEASE(pItem);
            }

            return hr;
        }
    };

typedef ComPtrList<IMFSample>           VideoSampleList;
class SamplePool 
{
public:
    SamplePool();
    virtual ~SamplePool();

    HRESULT Initialize(VideoSampleList& samples);
    HRESULT Clear();
   
    HRESULT GetSample(IMFSample **ppSample);    // Does not block.
    HRESULT ReturnSample(IMFSample *pSample);   
    BOOL    AreSamplesPending();

private:
    CCritSec                     m_lock;

    VideoSampleList             m_VideoSampleQueue;			// Available queue

    BOOL                        m_bInitialized;
    DWORD                       m_cPending;
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
	    CAutoLock lock(&m_lock);
	    return m_list.InsertBack(p);
    }

	HRESULT Dequeue(T **pp)
	{
		CAutoLock lock(&m_lock);

        if (m_list.IsEmpty())
        {
            *pp = NULL;
            return S_FALSE;
        }

		return m_list.RemoveFront(pp);
	}

    HRESULT PutBack(T *p)
    {
        CAutoLock lock(&m_lock);
        return m_list.InsertFront(p);
    }

	void Clear() 
	{
		CAutoLock lock(&m_lock);
		m_list.Clear();
    }


private:
	CCritSec			m_lock;	
	ComPtrList<T>	m_list;
};

template<class T>
    class AsyncCallback : public IMFAsyncCallback
    {
    public: 
        typedef HRESULT (T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

        AsyncCallback(T *pParent, InvokeFn fn) : m_pParent(pParent), m_pInvokeFn(fn)
        {
        }

        // IUnknown
        STDMETHODIMP_(ULONG) AddRef() { 
            // Delegate to parent class.
            return m_pParent->AddRef(); 
        }
        STDMETHODIMP_(ULONG) Release() { 
            // Delegate to parent class.
            return m_pParent->Release(); 
        }
        STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
        {
            if (!ppv)
            {
                return E_POINTER;
            }
            if (iid == __uuidof(IUnknown))
            {
                *ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
            }
            else if (iid == __uuidof(IMFAsyncCallback))
            {
                *ppv = static_cast<IMFAsyncCallback*>(this);
            }
            else
            {
                *ppv = NULL;
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }


        // IMFAsyncCallback methods
        STDMETHODIMP GetParameters(DWORD*, DWORD*)
        {
            // Implementation of this method is optional.
            return E_NOTIMPL;
        }

        STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult)
        {
            return (m_pParent->*m_pInvokeFn)(pAsyncResult);
        }

        T *m_pParent;
        InvokeFn m_pInvokeFn;
    };

// ReleaseEventCollection:
    // Release the events that an MFT might have provided in IMFTransform::ProcessOutput().
    inline void ReleaseEventCollection(DWORD cOutputBuffers, MFT_OUTPUT_DATA_BUFFER* pBuffers)
    {
        for (DWORD i = 0; i < cOutputBuffers; i++)
        {
            if (pBuffers[i].pEvents)
            {
              pBuffers[i].pEvents->Release();
              pBuffers[i].pEvents = NULL;
            }
        }
    }

namespace MediaFoundationSamples
{	
    class MediaTypeBuilder;
    class AudioTypeBuilder;
    class VideoTypeBuilder;
    class MPEGVideoTypeBuilder;

    // General media type helpers.
    MFOffset    MakeOffset(float v);
    MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height);
    LONG        GetOffset(const MFOffset& offset);
    HRESULT     GetFrameRate(IMFMediaType *pType, MFRatio *pRatio);
    HRESULT     GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea);
    BOOL        IsFormatInterlaced(IMFMediaType *pType);


    // DXVA media type helpers
#ifdef __dxva2api_h__
    HRESULT GetDXVA2ExtendedFormatFromMFMediaType(IMFMediaType *pType, DXVA2_ExtendedFormat *pFormat);
    HRESULT ConvertMFTypeToDXVAType(IMFMediaType *pType, DXVA2_VideoDesc *pDesc);
#endif
// RefCountedObject
    // You can use this when implementing IUnknown or any object that uses reference counting.
    class RefCountedObject
    {
    protected:
        volatile long   m_refCount;

    public:
        RefCountedObject() : m_refCount(1) {}
        virtual ~RefCountedObject()
        {
            assert(m_refCount == 0);
        }

        ULONG AddRef()
        {
            return InterlockedIncrement(&m_refCount);
        }
        ULONG Release()
        {
            assert(m_refCount > 0);
            ULONG uCount = InterlockedDecrement(&m_refCount);
            if (uCount == 0)
            {
                delete this;
            }
            return uCount;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // MediaTypeBuilder is a wrapper around an IMFMediaType. It can be used 
    // to read (and modify) an existing type or create a new type. This class
    // does not implement the IMFMediaType interface.
    //
    // The AudioTypeBuilder, VideoTypeBuilder, and MPEGVideoTypeBuilder
    // classes are specialized for those formats.
    //////////////////////////////////////////////////////////////////////////

    class MediaTypeBuilder : public RefCountedObject
    {
    protected:
        IMFMediaType* m_pType;

    protected:
        BOOL IsValid()
        {
            return m_pType != NULL;
        }

        IMFMediaType* GetMediaType()
        {
            assert(IsValid());
            return m_pType;
        }

        // Construct from an existing media type.
        MediaTypeBuilder(IMFMediaType* pType, HRESULT& hr) : m_pType(NULL)
        {
            assert(pType != NULL);

            if (!pType)
            {
                hr = E_POINTER;
            }
            else
            {
                m_pType = pType;
                m_pType->AddRef();	
            }
        }

        // Create a new media type.
        MediaTypeBuilder(HRESULT& hr)
        {

            hr = MFCreateMediaType(&m_pType);
        }

        virtual ~MediaTypeBuilder()
        {
            S_RELEASE(m_pType);
        }

    public:

        // Static creation functions. 

        // Use this version to create a new media type.
        template <class T>
        static HRESULT Create(T** ppTypeBuilder)
        {
            if (ppTypeBuilder == NULL)
            {
                return E_POINTER;
            }

            HRESULT hr = S_OK;

            T *pTypeBuilder = new T(hr);
            if (pTypeBuilder == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                *ppTypeBuilder = pTypeBuilder;
                (*ppTypeBuilder)->AddRef();
            }
            S_RELEASE(pTypeBuilder);
            return hr;
        }

        // Use this version to initialize from an existing media type.
        template <class T>
        static HRESULT Create(IMFMediaType *pType, T** ppTypeBuilder)
        {
            if (ppTypeBuilder == NULL)
            {
                return E_POINTER;
            }

            HRESULT hr = S_OK;

            T *pTypeBuilder = new T(pType, hr);
            if (pTypeBuilder == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                *ppTypeBuilder = pTypeBuilder;
                (*ppTypeBuilder)->AddRef();
            }
            S_RELEASE(pTypeBuilder);
            return hr;
        }



        // Direct wrappers of IMFMediaType methods.
        // (For these methods, we leave parameter validation to the IMFMediaType implementation.)

        // Retrieves the major type GUID.
        HRESULT GetMajorType(GUID *pGuid)
        {
            return GetMediaType()->GetMajorType(pGuid);
        }

        // Specifies whether the media data is compressed
        HRESULT IsCompressedFormat(BOOL *pbCompressed)
        {
            return GetMediaType()->IsCompressedFormat(pbCompressed);
        }

        // Compares two media types and determines whether they are identical.
        HRESULT IsEqual(IMFMediaType *pType, DWORD *pdwFlags)
        {
            return GetMediaType()->IsEqual(pType, pdwFlags);
        }

        // Retrieves an alternative representation of the media type.
        HRESULT GetRepresentation( GUID guidRepresentation, LPVOID *ppvRepresentation)
        {
            return GetMediaType()->GetRepresentation(guidRepresentation, ppvRepresentation);
        }

        // Frees memory that was allocated by the GetRepresentation method.
        HRESULT FreeRepresentation( GUID guidRepresentation, LPVOID pvRepresentation) 
        {
            return GetMediaType()->FreeRepresentation(guidRepresentation, pvRepresentation);
        }


        // Helper methods

        // CopyFrom: Copy all of the attributes from another media type into this type.
        HRESULT CopyFrom(MediaTypeBuilder *pType)
        {
            if (!pType->IsValid())
            {
                return E_UNEXPECTED;
            }
            if (pType == NULL)
            {
                return E_POINTER;
            }
            return CopyFrom(pType->m_pType);
        }

        HRESULT CopyFrom(IMFMediaType *pType)
        {
            if (pType == NULL)
            {
                return E_POINTER;
            }
            return pType->CopyAllItems(m_pType);
        }

        // Returns the underlying IMFMediaType pointer. 
        HRESULT GetMediaType(IMFMediaType** ppType)
        {
            assert(IsValid());
            CheckPointer(ppType, E_POINTER);
            *ppType = m_pType;
            (*ppType)->AddRef();
            return S_OK;
        }

        // Sets the major type GUID.
        HRESULT SetMajorType(GUID guid)
        {
            return GetMediaType()->SetGUID(MF_MT_MAJOR_TYPE, guid);
        }

        // Retrieves the subtype GUID.
        HRESULT GetSubType(GUID* pGuid)
        {
            CheckPointer(pGuid, E_POINTER);
            return GetMediaType()->GetGUID(MF_MT_SUBTYPE, pGuid);
        }

        // Sets the subtype GUID.
        HRESULT SetSubType(const GUID& guid)
        {
            return GetMediaType()->SetGUID(MF_MT_SUBTYPE, guid);
        }

        // Extracts the FOURCC code from the subtype.
        // Not all subtypes follow this pattern.
        HRESULT GetFourCC(DWORD *pFourCC)
        {
            assert(IsValid());
            CheckPointer(pFourCC, E_POINTER);
            HRESULT hr = S_OK;
            GUID guidSubType = GUID_NULL;

            if (SUCCEEDED(hr))
            {
                hr = GetSubType(&guidSubType);
            }

            if (SUCCEEDED(hr))
            {
                *pFourCC = guidSubType.Data1;
            }
            return hr;
        }		

        HRESULT SetFourCC(DWORD FourCC)
        {
            assert(IsValid());
            
            GUID guidSubType = MFVideoFormat_Base;

            guidSubType.Data1 = FourCC;

            return SetSubType(guidSubType);
        }

        //  Queries whether each sample is independent of the other samples in the stream.
        HRESULT GetAllSamplesIndependent(BOOL* pbIndependent)
        {
            CheckPointer(pbIndependent, E_POINTER);
            return GetMediaType()->GetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, (UINT32*)pbIndependent);
        }

        //  Specifies whether each sample is independent of the other samples in the stream.
        HRESULT SetAllSamplesIndependent(BOOL bIndependent)
        {
            return GetMediaType()->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, (UINT32)bIndependent);
        }

        // Queries whether the samples have a fixed size.
        HRESULT GetFixedSizeSamples(BOOL *pbFixed)
        {
            CheckPointer(pbFixed, E_POINTER);
            return GetMediaType()->GetUINT32(MF_MT_FIXED_SIZE_SAMPLES, (UINT32*)pbFixed);
        }

        // Specifies whether the samples have a fixed size.
        HRESULT SetFixedSizeSamples(BOOL bFixed)
        {
            return GetMediaType()->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, (UINT32)bFixed);
        }

        // Retrieves the size of each sample, in bytes. 
        HRESULT GetSampleSize(UINT32 *pnSize)
        {	
            CheckPointer(pnSize, E_POINTER);
            return GetMediaType()->GetUINT32(MF_MT_SAMPLE_SIZE, pnSize);
        }

        // Sets the size of each sample, in bytes. 
        HRESULT SetSampleSize(UINT32 nSize)
        {	
            return GetMediaType()->SetUINT32(MF_MT_SAMPLE_SIZE, nSize);
        }

        // Retrieves a media type that was wrapped by the MFWrapMediaType function.
        HRESULT Unwrap(IMFMediaType **ppOriginal)
        {
            CheckPointer(ppOriginal, E_POINTER);
            return ::MFUnwrapMediaType(GetMediaType(), ppOriginal);
        }

        // The following versions return reasonable defaults if the relevant attribute is not present (zero/FALSE).
        // This is useful for making quick comparisons betweeen media types. 

        BOOL AllSamplesIndependent()
        {
            return (BOOL)MFGetAttributeUINT32(GetMediaType(), MF_MT_ALL_SAMPLES_INDEPENDENT, FALSE);
        }

        BOOL FixedSizeSamples()
        {
            return (BOOL)MFGetAttributeUINT32(GetMediaType(), MF_MT_FIXED_SIZE_SAMPLES, FALSE);
        }

        UINT32 SampleSize()
        {	
            return MFGetAttributeUINT32(GetMediaType(), MF_MT_SAMPLE_SIZE, 0);
        }
    };

	
	//////////////////////////////////////////////////////////////////////////
    //
    // VideoTypeBuilder provides access to video-specific attributes.
    //
    // Note: MPEG-specific attributes are accessible from MPEGVideoTypeBuilder.
    //
  	//////////////////////////////////////////////////////////////////////////

	class VideoTypeBuilder : public MediaTypeBuilder
	{
        friend class MediaTypeBuilder; 

	protected:

		VideoTypeBuilder(IMFMediaType* pType, HRESULT& hr) : 
			 MediaTypeBuilder(pType, hr)
		{
            GUID guidMajorType = GUID_NULL;

            if (SUCCEEDED(hr)) // Base class constructor might fail.
            {
                hr = GetMajorType(&guidMajorType);
            }
            if (SUCCEEDED(hr))
            {
                if (guidMajorType != MFMediaType_Video)
                {
                    hr = MF_E_INVALIDTYPE;
                }
            }			
        }

        VideoTypeBuilder(HRESULT& hr) : MediaTypeBuilder(hr)
        {
            if (SUCCEEDED(hr))
            {
                hr = SetMajorType(MFMediaType_Video);
            }
        }

    public:

		// Retrieves a description of how the frames are interlaced.
		HRESULT GetInterlaceMode(MFVideoInterlaceMode *pmode)
		{
			CheckPointer(pmode, E_POINTER);
	        return GetMediaType()->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)pmode);
		}

		// Sets a description of how the frames are interlaced.
		HRESULT SetInterlaceMode(MFVideoInterlaceMode mode)
		{
	        return GetMediaType()->SetUINT32(MF_MT_INTERLACE_MODE, (UINT32)mode);
		}

        // IsInterlaced: Returns true if the video format is interlaced.
        // Defaults to FALSE if the interlace type is not set.
        BOOL IsInterlaced()
        {
            return IsFormatInterlaced(GetMediaType());
        }

		// This returns the default or attempts to compute it, in its absence.
		HRESULT GetDefaultStride(INT32 *pnStride)
		{
			CheckPointer(pnStride, E_POINTER);

			HRESULT hr = S_OK;
			INT32 nStride = 0;

			// First try to get it from the attribute.
			hr = GetMediaType()->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&nStride);
			if (FAILED(hr))
			{
				// Attribute not set. See if we can calculate the default stride.
				GUID subtype = GUID_NULL;

				UINT32 width = 0;
				UINT32 height = 0;

				// First we need the subtype .
				hr = GetSubType(&subtype);

				// Now we need the image width and height.
				if (SUCCEEDED(hr))
				{
					hr = GetFrameDimensions(&width, &height);
				}

				// Now compute the stride for a particular bitmap type
				if (SUCCEEDED(hr))
				{
					hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, (LONG*)&nStride);
				}

				// Set the attribute for later reference.
				if (SUCCEEDED(hr))
				{
					hr = SetDefaultStride(nStride);
				}
			}

			if (SUCCEEDED(hr))
			{
				*pnStride = nStride;
			}

			return hr;
		}


		// Sets the default stride. Only appropriate for uncompressed data formats.
		HRESULT SetDefaultStride(INT32 nStride)
		{
			return GetMediaType()->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)nStride);
		}

		// Retrieves the width and height of the video frame.
		HRESULT GetFrameDimensions(UINT32 *pdwWidthInPixels, UINT32 *pdwHeightInPixels)
		{
			return MFGetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, pdwWidthInPixels, pdwHeightInPixels);
		}

		// Sets the width and height of the video frame.
		HRESULT SetFrameDimensions(UINT32 dwWidthInPixels, UINT32 dwHeightInPixels)
		{
			return MFSetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, dwWidthInPixels, dwHeightInPixels);
		}

		// Retrieves the data error rate in bit errors per second
		HRESULT GetDataBitErrorRate(UINT32 *pRate)
		{
			CheckPointer(pRate, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AVG_BIT_ERROR_RATE, pRate);
		}

		// Sets the data error rate in bit errors per second
		HRESULT SetDataBitErrorRate(UINT32 rate)
		{
			return GetMediaType()->SetUINT32(MF_MT_AVG_BIT_ERROR_RATE, rate);
		}

		// Retrieves the approximate data rate of the video stream.
		HRESULT GetAverageBitRate(UINT32 *pRate)
		{
			CheckPointer(pRate, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AVG_BITRATE, pRate);
		}

		// Sets the approximate data rate of the video stream.
		HRESULT SetAvgerageBitRate(UINT32 rate)
		{
			return GetMediaType()->SetUINT32(MF_MT_AVG_BITRATE, rate);
		}

		// Retrieves custom color primaries.
		HRESULT GetCustomVideoPrimaries(MT_CUSTOM_VIDEO_PRIMARIES *pPrimaries)
		{
			CheckPointer(pPrimaries, E_POINTER);
			return GetMediaType()->GetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, (UINT8*)pPrimaries, sizeof(MT_CUSTOM_VIDEO_PRIMARIES), NULL);
		}

		// Sets custom color primaries.
		HRESULT SetCustomVideoPrimaries(const MT_CUSTOM_VIDEO_PRIMARIES& primary)
		{
			return GetMediaType()->SetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, (const UINT8*)&primary, sizeof(MT_CUSTOM_VIDEO_PRIMARIES));
		}

		// Gets the number of frames per second.
		HRESULT GetFrameRate(UINT32 *pnNumerator, UINT32 *pnDenominator)
		{
			CheckPointer(pnNumerator, E_POINTER);
			CheckPointer(pnDenominator, E_POINTER);
		    return MFGetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, pnNumerator, pnDenominator);
		}

        // Gets the frames per second as a ratio.
		HRESULT GetFrameRate(MFRatio *pRatio)
		{
            CheckPointer(pRatio, E_POINTER);
            return GetFrameRate((UINT32*)&pRatio->Numerator, (UINT32*)&pRatio->Denominator);
        }
		
		// Sets the number of frames per second.
		HRESULT SetFrameRate(UINT32 nNumerator, UINT32 nDenominator)
		{
		    return MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, nNumerator, nDenominator);
		}

        // Sets the number of frames per second, as a ratio.
        HRESULT SetFrameRate(const MFRatio& ratio)
		{
		    return MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, ratio.Numerator, ratio.Denominator);
		}

		// Queries the geometric aperture.
		HRESULT GetGeometricAperture(MFVideoArea *pArea)
		{
			CheckPointer(pArea, E_POINTER);
		    return GetMediaType()->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), NULL);
		}
		
		// Sets the geometric aperture.
		HRESULT SetGeometricAperture(const MFVideoArea& area)
		{
		    return GetMediaType()->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&area, sizeof(MFVideoArea));
		}
		
		// Retrieves the maximum number of frames from one key frame to the next.
		HRESULT GetMaxKeyframeSpacing(UINT32 *pnSpacing)
		{
			CheckPointer(pnSpacing, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_MAX_KEYFRAME_SPACING, pnSpacing);
		}

		// Sets the maximum number of frames from one key frame to the next.
		HRESULT SetMaxKeyframeSpacing(UINT32 nSpacing)
		{
			return GetMediaType()->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, nSpacing);
		}

		// Retrieves the region that contains the valid portion of the signal.
		HRESULT GetMinDisplayAperture(MFVideoArea *pArea)
		{
			CheckPointer(pArea, E_POINTER);
			return GetMediaType()->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), NULL);
		}

		// Sets the the region that contains the valid portion of the signal.
		HRESULT SetMinDisplayAperture(const MFVideoArea& area)
		{
			return GetMediaType()->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&area, sizeof(MFVideoArea));
		}

 
		// Retrieves the aspect ratio of the output rectangle for a video media type. 
		HRESULT GetPadControlFlags(MFVideoPadFlags *pFlags)
		{
			CheckPointer(pFlags, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_PAD_CONTROL_FLAGS, (UINT32*)pFlags);
		}

		// Sets the aspect ratio of the output rectangle for a video media type. 
		HRESULT SetPadControlFlags(MFVideoPadFlags flags)
		{
			return GetMediaType()->SetUINT32(MF_MT_PAD_CONTROL_FLAGS, flags);
		}

		// Retrieves an array of palette entries for a video media type. 
		HRESULT GetPaletteEntries(MFPaletteEntry *paEntries, UINT32 nEntries)
		{
			CheckPointer(paEntries, E_POINTER);
			return GetMediaType()->GetBlob(MF_MT_PALETTE, (UINT8*)paEntries, sizeof(MFPaletteEntry) * nEntries, NULL);
		}

		// Sets an array of palette entries for a video media type. 
		HRESULT SetPaletteEntries(MFPaletteEntry *paEntries, UINT32 nEntries)
		{
			CheckPointer(paEntries, E_POINTER);
			return GetMediaType()->SetBlob(MF_MT_PALETTE, (UINT8*)paEntries, sizeof(MFPaletteEntry) * nEntries);
		}

		// Retrieves the number of palette entries.
		HRESULT GetNumPaletteEntries(UINT32 *pnEntries)
		{
			CheckPointer(pnEntries, E_POINTER);
			UINT32 nBytes = 0;
			HRESULT hr = S_OK;
			hr = GetMediaType()->GetBlobSize(MF_MT_PALETTE, &nBytes);
			if (SUCCEEDED(hr))
			{
				if (nBytes % sizeof(MFPaletteEntry) != 0)
				{
					hr = E_UNEXPECTED;
				}
			}
			if (SUCCEEDED(hr))
			{
				*pnEntries = nBytes / sizeof(MFPaletteEntry);
			}
			return hr;
		}

		// Queries the 4×3 region of video that should be displayed in pan/scan mode.
		HRESULT GetPanScanAperture(MFVideoArea *pArea)
		{
			CheckPointer(pArea, E_POINTER);
			return GetMediaType()->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), NULL);
		}
		
		// Sets the 4×3 region of video that should be displayed in pan/scan mode.
		HRESULT SetPanScanAperture(const MFVideoArea& area)
		{
			return GetMediaType()->SetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)&area, sizeof(MFVideoArea));
		}
		
		// Queries whether pan/scan mode is enabled.
		HRESULT IsPanScanEnabled(BOOL *pBool)
		{
			CheckPointer(pBool, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_PAN_SCAN_ENABLED, (UINT32*)pBool);
		}

		// Sets whether pan/scan mode is enabled.
		HRESULT SetPanScanEnabled(BOOL bEnabled)
		{
			return GetMediaType()->SetUINT32(MF_MT_PAN_SCAN_ENABLED, (UINT32)bEnabled);
		}

		// Queries the pixel aspect ratio
		HRESULT GetPixelAspectRatio(UINT32 *pnNumerator, UINT32 *pnDenominator)
		{
			CheckPointer(pnNumerator, E_POINTER);
			CheckPointer(pnDenominator, E_POINTER);
			return MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, pnNumerator, pnDenominator);
		}		

		// Sets the pixel aspect ratio
		HRESULT SetPixelAspectRatio(UINT32 nNumerator, UINT32 nDenominator)
		{
			return MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, nNumerator, nDenominator);
		}

		HRESULT SetPixelAspectRatio(const MFRatio& ratio)
		{
			return MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, ratio.Numerator, ratio.Denominator);
		}

		// Queries the intended aspect ratio.
		HRESULT GetSourceContentHint(MFVideoSrcContentHintFlags *pFlags)
		{
			CheckPointer(pFlags, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_SOURCE_CONTENT_HINT, (UINT32*)pFlags);
		}

		// Sets the intended aspect ratio.
		HRESULT SetSourceContentHint(MFVideoSrcContentHintFlags nFlags)
		{
			return GetMediaType()->SetUINT32(MF_MT_SOURCE_CONTENT_HINT, (UINT32)nFlags);
		}

		// Queries an enumeration which represents the conversion function from RGB to R'G'B'.
		HRESULT GetTransferFunction(MFVideoTransferFunction *pnFxn)
		{
			CheckPointer(pnFxn, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_TRANSFER_FUNCTION, (UINT32*)pnFxn);
		}

		// Set an enumeration which represents the conversion function from RGB to R'G'B'.
		HRESULT SetTransferFunction(MFVideoTransferFunction nFxn)
		{
			return GetMediaType()->SetUINT32(MF_MT_TRANSFER_FUNCTION, (UINT32)nFxn);
		}

		// Queries how chroma was sampled for a Y'Cb'Cr' video media type.
		HRESULT GetChromaSiting(MFVideoChromaSubsampling *pSampling)
		{
			CheckPointer(pSampling, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_VIDEO_CHROMA_SITING, (UINT32*)pSampling);
		}
		
		// Sets how chroma was sampled for a Y'Cb'Cr' video media type.
		HRESULT SetChromaSiting(MFVideoChromaSubsampling nSampling)
		{
			return GetMediaType()->SetUINT32(MF_MT_VIDEO_CHROMA_SITING, (UINT32)nSampling);
		}
		
		// Queries the optimal lighting conditions for viewing.
		HRESULT GetVideoLighting(MFVideoLighting *pLighting)
		{
			CheckPointer(pLighting, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_VIDEO_LIGHTING, (UINT32*)pLighting);
		}
		
		// Sets the optimal lighting conditions for viewing.
		HRESULT SetVideoLighting(MFVideoLighting nLighting)
		{
			return GetMediaType()->SetUINT32(MF_MT_VIDEO_LIGHTING, (UINT32)nLighting);
		}
		
		// Queries the nominal range of the color information in a video media type. 
		HRESULT GetVideoNominalRange(MFNominalRange *pRange)
		{
			CheckPointer(pRange, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, (UINT32*)pRange);
		}

		// Sets the nominal range of the color information in a video media type. 
		HRESULT SetVideoNominalRange(MFNominalRange nRange)
		{
			return GetMediaType()->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, (UINT32)nRange);
		}

		// Queries the color primaries for a video media type.
		HRESULT GetVideoPrimaries(MFVideoPrimaries *pPrimaries)
		{
			CheckPointer(pPrimaries, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_VIDEO_PRIMARIES, (UINT32*)pPrimaries);
		}

		// Sets the color primaries for a video media type.
		HRESULT SetVideoPrimaries(MFVideoPrimaries nPrimaries)
		{
			return GetMediaType()->SetUINT32(MF_MT_VIDEO_PRIMARIES, (UINT32)nPrimaries);
		}

		// Gets a enumeration representing the conversion matrix from the 
		// Y'Cb'Cr' color space to the R'G'B' color space.
		HRESULT GetYUVMatrix(MFVideoTransferMatrix *pMatrix)
		{
			CheckPointer(pMatrix, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_YUV_MATRIX, (UINT32*)pMatrix);
		} 

		// Sets an enumeration representing the conversion matrix from the 
		// Y'Cb'Cr' color space to the R'G'B' color space.
		HRESULT SetYUVMatrix(MFVideoTransferMatrix nMatrix)
		{
			return GetMediaType()->SetUINT32(MF_MT_YUV_MATRIX, (UINT32)nMatrix);
		} 

        // 
        // The following versions return reasonable defaults if the relevant attribute is not present (zero/FALSE).
        // This is useful for making quick comparisons betweeen media types. 
        //

		MFRatio GetPixelAspectRatio() // Defaults to 1:1 (square pixels)
		{
            MFRatio PAR = { 0, 0 };
            HRESULT hr = S_OK;

            hr = MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&PAR.Numerator, (UINT32*)&PAR.Denominator);
            if (FAILED(hr))
            {
                PAR.Numerator = 1;
                PAR.Denominator = 1;
            }
            return PAR;
		}		

        BOOL IsPanScanEnabled() // Defaults to FALSE
        {
            return (BOOL)MFGetAttributeUINT32(GetMediaType(), MF_MT_PAN_SCAN_ENABLED, FALSE);
        }



        // Returns (in this order) 
        // 1. The pan/scan region, only if pan/scan mode is enabled.
        // 2. The geometric aperture.
        // 3. The entire video area.
		HRESULT GetVideoDisplayArea(MFVideoArea *pArea)
		{
			CheckPointer(pArea, E_POINTER);

            return MediaFoundationSamples::GetVideoDisplayArea(GetMediaType(), pArea);
        }

	};	


	//////////////////////////////////////////////////////////////////////////
    //
    // AudioTypeWrapper provides access to audio-specific attributes.
    //
  	//////////////////////////////////////////////////////////////////////////

	class AudioTypeBuilder : public MediaTypeBuilder
	{
        friend class MediaTypeBuilder; 

    protected:

		AudioTypeBuilder(IMFMediaType* pType, HRESULT& hr) :
			MediaTypeBuilder(pType, hr)
		{	
			GUID guidMajorType = GUID_NULL;
			
			if (SUCCEEDED(hr))  // Base class constructor might fail.
			{
				hr = GetMajorType(&guidMajorType);
			}

			if (SUCCEEDED(hr))
	        {
				if (guidMajorType != MFMediaType_Audio)
                {
			        hr = MF_E_INVALIDTYPE;
                }
	        }
		}

        AudioTypeBuilder(HRESULT& hr) : MediaTypeBuilder(hr)
        {
            if (SUCCEEDED(hr))
            {
                hr = SetMajorType(MFMediaType_Audio);
            }
        }

            
    public:

        // Query the average number of bytes per second.
		HRESULT GetAvgerageBytesPerSecond(UINT32 *pBytes)
		{
			CheckPointer(pBytes, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, pBytes);
		}

		// Sets the average number of bytes per second.
		HRESULT SetAvgerageBytesPerSecond(UINT32 nBytes)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, nBytes);
		}

		// Query the bits per audio sample
		HRESULT GetBitsPerSample(UINT32 *pBits)
		{
			CheckPointer(pBits, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, pBits);
		}
		
		// Sets the bits per audio sample
		HRESULT SetBitsPerSample(UINT32 nBits)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, nBits);
		}
		
		// Query the block aignment in bytes
		HRESULT GetBlockAlignment(UINT32 *pBytes)
		{
			CheckPointer(pBytes, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, pBytes);
		}
		
		// Sets the block aignment in bytes
		HRESULT SetBlockAlignment(UINT32 nBytes)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, nBytes);
		}
		
		// Query the assignment of audio channels to speaker positions.
		HRESULT GetChannelMask(UINT32 *pMask)
		{
			CheckPointer(pMask, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_CHANNEL_MASK, pMask);
		}
		
		// Sets the assignment of audio channels to speaker positions.
		HRESULT SetChannelMask(UINT32 nMask)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK, nMask);
		}
		
		// Query the number of audio samples per second (floating-point value).
		HRESULT GetFloatSamplesPerSecond(double *pfSampleRate)
		{
			CheckPointer(pfSampleRate, E_POINTER);
			return GetMediaType()->GetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, pfSampleRate);
		}

		// Sets the number of audio samples per second (floating-point value).
		HRESULT SetFloatSamplesPerSecond(double fSampleRate)
		{
			return GetMediaType()->SetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, fSampleRate);
		}

		// Queries the number of audio channels
		HRESULT GetNumChannels(UINT32 *pnChannels)
		{
			CheckPointer(pnChannels, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, pnChannels);
		}

		// Sets the number of audio channels
		HRESULT SetNumChannels(UINT32 nChannels)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, nChannels);
		}

		// Query the number of audio samples contained in one compressed block of audio data.
		HRESULT GetSamplesPerBlock(UINT32 *pnSamples)
		{
			CheckPointer(pnSamples, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, pnSamples);
		}

		// Sets the number of audio samples contained in one compressed block of audio data.
		HRESULT SetSamplesPerBlock(UINT32 nSamples)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, nSamples);
		}

		// Query the number of audio samples per second as an integer value.
		HRESULT GetSamplesPerSecond(UINT32 *pnSampleRate)
		{
			CheckPointer(pnSampleRate, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, pnSampleRate);
		}
 
		// Set the number of audio samples per second as an integer value.
		HRESULT SetSamplesPerSecond(UINT32 nSampleRate)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, nSampleRate);
		}
 
		// Query number of valid bits of audio data in each sample.
		HRESULT GetValidBitsPerSample(UINT32 *pnBits)
		{
			CheckPointer(pnBits, E_POINTER);
			HRESULT hr = GetMediaType()->GetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, pnBits);
			if (!SUCCEEDED(hr))
			{
				hr = GetBitsPerSample(pnBits);
			}
			return hr;
		} 
		
		// Set the number of valid bits of audio data in each sample.
		HRESULT SetValidBitsPerSample(UINT32 nBits)
		{
			return GetMediaType()->SetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, nBits);
		} 


        // The following versions return zero if the relevant attribute is not present. 
        // This is useful for making quick comparisons betweeen media types. 

		UINT32 AvgerageBytesPerSecond()
		{
			return MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
		}

		UINT32 BitsPerSample()
		{
			return MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
		}
		
		UINT32 GetBlockAlignment()
		{
			return MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
		}
		
		double FloatSamplesPerSecond()
		{
			return MFGetAttributeDouble(GetMediaType(), MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, 0.0);
		}

		UINT32 NumChannels()
		{
			return MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_NUM_CHANNELS, 0);
		}

		UINT32 SamplesPerSecond()
		{
			return MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
		}

	};	


	//////////////////////////////////////////////////////////////////////////
    //
    // MPEGVideoTypeBuilder provides access to MPEG-1 and MPEG-2 video 
    // attributes, plus the general video attributes.
    //
  	//////////////////////////////////////////////////////////////////////////

    class MPEGVideoTypeBuilder : public VideoTypeBuilder
    {
        friend class MediaTypeBuilder; 

    protected:

		MPEGVideoTypeBuilder(IMFMediaType* pType, HRESULT& hr) :
			VideoTypeBuilder(pType, hr)
		{	
            // We could enforce an MPEG subtype, but that would limit the usefulness of
            // the class. Possibly you might encounter an MPEG media type with a subtype
            // not on the list. Therefore, it's up to the caller to confirm whether
            // the media type is MPEG.
		}

        MPEGVideoTypeBuilder(HRESULT& hr) : VideoTypeBuilder(hr)
        {
        }

    public:

		// Retrieves the MPEG sequence header for a video media type.
		HRESULT GetMpegSeqHeader(BYTE *pData, UINT32 cbSize)
		{
			CheckPointer(pData, E_POINTER);
			return GetMediaType()->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize, NULL);
		}

		// Sets the MPEG sequence header for a video media type.
		HRESULT SetMpegSeqHeader(const BYTE *pData, UINT32 cbSize)
		{
			CheckPointer(pData, E_POINTER);
			return GetMediaType()->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize);
		}

		// Retrieves the size of the MPEG sequence header.
		HRESULT GetMpegSeqHeaderSize(UINT32 *pcbSize)
		{
			CheckPointer(pcbSize, E_POINTER);
			return GetMediaType()->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, pcbSize);
		}
		
		// Retrieve the group-of-pictures (GOP) start time code, for an MPEG-1 or MPEG-2 video media type.
		HRESULT GetStartTimeCode(UINT32 *pnTime)
		{
			CheckPointer(pnTime, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_MPEG_START_TIME_CODE, pnTime);
		}

		// Sets the group-of-pictures (GOP) start time code, for an MPEG-1 or MPEG-2 video media type.
		HRESULT SetStartTimeCode(UINT32 nTime)
		{
			return GetMediaType()->SetUINT32(MF_MT_MPEG_START_TIME_CODE, nTime);
		}

		// Retrieves assorted flags for MPEG-2 video media type
		HRESULT GetMPEG2Flags(UINT32 *pnFlags)
		{
			CheckPointer(pnFlags, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_MPEG2_FLAGS, pnFlags);
		}

		// Sets assorted flags for MPEG-2 video media type
		HRESULT SetMPEG2Flags(UINT32 nFlags)
		{
			return GetMediaType()->SetUINT32(MF_MT_MPEG2_FLAGS, nFlags);
		}

		// Retrieves the MPEG-2 level in a video media type.
		HRESULT GetMPEG2Level(UINT32 *pLevel)
		{
			CheckPointer(pLevel, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_MPEG2_LEVEL, pLevel);
		}
 
		// Sets the MPEG-2 level in a video media type.
		HRESULT SetMPEG2Level(UINT32 nLevel)
		{
			return GetMediaType()->SetUINT32(MF_MT_MPEG2_LEVEL, nLevel);
		}
 
		// Retrieves the MPEG-2 profile in a video media type
		HRESULT GetMPEG2Profile(UINT32 *pProfile)
		{
			CheckPointer(pProfile, E_POINTER);
			return GetMediaType()->GetUINT32(MF_MT_MPEG2_PROFILE, pProfile);
		}
 
		// Sets the MPEG-2 profile in a video media type
		HRESULT SetMPEG2Profile(UINT32 nProfile)
		{
			return GetMediaType()->SetUINT32(MF_MT_MPEG2_PROFILE, nProfile);
		}

    };


    //////////////////////////////////////////////////////////////////////////

    // Helper functions for manipulating media types.

    inline MFOffset MakeOffset(float v)
    {
        MFOffset offset;
        offset.value = short(v);
        offset.fract = WORD(65536 * (v-offset.value));
        return offset;
    }

    inline MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
    {
        MFVideoArea area;
        area.OffsetX = MakeOffset(x);
        area.OffsetY = MakeOffset(y);
        area.Area.cx = width;
        area.Area.cy = height;
        return area;
    }

    inline LONG GetOffset(const MFOffset& offset)
    {
        return (LONG)((float)offset.value + offset.fract/65536.0f);
    }

    // Get the frame rate from a video media type.
    inline HRESULT GetFrameRate(IMFMediaType *pType, MFRatio *pRatio)
    {
        return MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, (UINT32*)&pRatio->Numerator, (UINT32*)&pRatio->Denominator);
    }

    // Get the correct display area from a video media type.
    inline HRESULT GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea)
    {
        HRESULT hr = S_OK;
        BOOL bPanScan = FALSE;
        UINT32 width = 0, height = 0;

        bPanScan = MFGetAttributeUINT32(pType, MF_MT_PAN_SCAN_ENABLED, FALSE);

        // In pan/scan mode, try to get the pan/scan region.
        if (bPanScan)
        {
            hr = pType->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), NULL);
        }

        // If not in pan/scan mode, or there is not pan/scan region, get the geometric aperture.
        if (!bPanScan || hr == MF_E_ATTRIBUTENOTFOUND)
        {
	        hr = pType->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), NULL);
        }

        // Default: Use the entire video area.
        if (!bPanScan || hr == MF_E_ATTRIBUTENOTFOUND)
        {
            hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
            if (SUCCEEDED(hr))
            {
                *pArea = MakeArea(0.0, 0.0, width, height);
            }
        }
        return hr;
    }

    inline BOOL IsFormatInterlaced(IMFMediaType *pType)
    {
        UINT32 mode = 0;
        return (
            mode = MFGetAttributeUINT32(pType, MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive) !=
            MFVideoInterlace_Progressive
            );
    }

    // DXVA media type helper functions
#ifdef __dxva2api_h__

    inline HRESULT ConvertMFTypeToDXVAType(IMFMediaType *pType, DXVA2_VideoDesc *pDesc)
    {
        ZeroMemory(pDesc, sizeof(DXVA2_VideoDesc));

        GUID                    subtype = GUID_NULL;
        UINT32                  width = 0;
        UINT32                  height = 0;
        UINT32                  fpsNumerator = 0;
        UINT32                  fpsDenominator = 0;

        // The D3D format is the first DWORD of the subtype GUID.
        pType->GetGUID(MF_MT_SUBTYPE, &subtype);
        pDesc->Format = (D3DFORMAT)subtype.Data1;

        // Frame size.
        MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
        pDesc->SampleWidth = width;
        pDesc->SampleHeight = height;

        // Frame rate.
        MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator);
        pDesc->InputSampleFreq.Numerator = fpsNumerator;
        pDesc->InputSampleFreq.Denominator = fpsDenominator;

        // For progressive or single-field types, the output frequency is the same as
        // the input frequency. For interleaved-field types, the output frequency is
        // twice the input frequency.  
        pDesc->OutputFrameFreq = pDesc->InputSampleFreq;
        if ((pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedEvenFirst) ||
            (pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedOddFirst))
        {
            pDesc->OutputFrameFreq.Numerator *= 2;
        }

        // Extended format information.
        GetDXVA2ExtendedFormatFromMFMediaType(pType, &pDesc->SampleFormat);

        return S_OK;
    }

    // Fills in the DXVA2_ExtendedFormat structure.
    inline HRESULT GetDXVA2ExtendedFormatFromMFMediaType(IMFMediaType *pType, DXVA2_ExtendedFormat *pFormat)
    {
        // Get the interlace mode.
        MFVideoInterlaceMode interlace = 
            (MFVideoInterlaceMode)MFGetAttributeUINT32(
                pType, MF_MT_INTERLACE_MODE, MFVideoInterlace_Unknown
             );

        // The values for interlace mode translate directly, except for
        // "mixed interlace or progressive" mode.
        if (interlace == MFVideoInterlace_MixedInterlaceOrProgressive)
        {
            // Default to interleaved fields.
            pFormat->SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
        }
        else
        {
            pFormat->SampleFormat = (UINT)interlace;
        }
        // The remaining values translate directly.
        // Use the "no-fail" attribute functions and default to "unknown."
        pFormat->VideoChromaSubsampling = MFGetAttributeUINT32(pType, 
            MF_MT_VIDEO_CHROMA_SITING, MFVideoChromaSubsampling_Unknown);

        pFormat->NominalRange = MFGetAttributeUINT32(pType, 
            MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown);

        pFormat->VideoTransferMatrix = MFGetAttributeUINT32(pType, 
            MF_MT_YUV_MATRIX, MFVideoTransferMatrix_Unknown);

        pFormat->VideoLighting = MFGetAttributeUINT32(pType, 
            MF_MT_VIDEO_LIGHTING, MFVideoLighting_Unknown);

        pFormat->VideoPrimaries = MFGetAttributeUINT32(pType, 
            MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_Unknown);

        pFormat->VideoTransferFunction = MFGetAttributeUINT32(pType, 
            MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_Unknown);

        return S_OK;
    }
#endif /* __dxva2api_h__ */
}