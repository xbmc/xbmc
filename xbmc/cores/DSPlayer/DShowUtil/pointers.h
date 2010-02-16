#ifndef POINTERS_H
#define POINTERS_H

#define DBGOUT(n) false;

// warning C4150: deletion of pointer to incomplete type 'DnsblCache'; no destructor called
// ?? should be fully defined when deletor is called
#pragma warning(disable:4150)
template<class T>
__inline void deletor(T* t){delete t;}
#pragma warning(default:4150)

template<class T>
class Ptr
{
public:
	//operator T*(){return t;}
	//operator const T*()const {return t;}
	T& operator*(){return *t;}
	const T& operator*()const{return *t;}
	T* operator->(){return t;}
	const T* operator->()const{return t;}
	operator bool(){return t!=NULL;}
	T* ptr(){return t;}
	const T* ptr()const{return t;}
protected:
	Ptr(T* _t=NULL):t(_t){}
	virtual ~Ptr(){}
	T* t;
	
};

template<class Clonable,void*(*Deletor)(Clonable*)=deletor<Clonable> >
class SoleOwnerPtr: public Ptr<Clonable>
{
public:
	SoleOwnerPtr(Clonable* ptr=NULL):Ptr<Clonable>(ptr){}
	SoleOwnerPtr(const SoleOwnerPtr<Clonable,Deletor>& other):Ptr<Clonable>(NULL)
	{
		operator=(other);
	}
	~SoleOwnerPtr(){Deletor(t);}
	void operator=(const SoleOwnerPtr<Clonable,Deletor>& other)
	{
		if(this!=&other)
		{
			Deletor(t);
			t=other.t->clone();
		}
	}
};

template<class T,void(*Deletor)(T*)=deletor<T> >
class RefCountedPtr: public Ptr<T>
{
public:
	explicit RefCountedPtr(T* ptr=NULL):Ptr<T>(ptr),nRef(new int(1)){}
	RefCountedPtr(const RefCountedPtr<T,Deletor>& other):Ptr<T>(other.t){
		++*(nRef=other.nRef);
	}
	~RefCountedPtr(){
		release();
	}
	void operator=(const RefCountedPtr<T,Deletor>& other){
		if(this!=&other)
		{
			release();
			nRef=other.nRef;
			++(*nRef);
			t=other.t;
		}
	}
	void operator=(T*const& ptr)
	{
		release();
		t=ptr;
		nRef=new int(1);
	}
private:
	int* nRef;
	void release()
	{
#ifdef _DEBUG
		if(nRef)
			DBGOUT(*nRef)
		else
			DBGOUT(0);
#endif
		if(!--(*nRef))
		{
			Deletor(t);
			delete nRef;
		}
		nRef=NULL;
		t=NULL;
	}
};
#endif