#pragma once
#include <xtl.h>
namespace AUTOPTR
{
	class CAutoPtrHandle
	{
	public:
		CAutoPtrHandle(HANDLE hHandle);
		virtual ~CAutoPtrHandle(void);
		operator HANDLE();
		void		 attach(HANDLE hHandle);
		HANDLE	 release();
		bool		 isValid() const;
		void     reset();
	protected:
		virtual void Cleanup();
		HANDLE m_hHandle;
	};

	class CAutoPtrFind : public CAutoPtrHandle
	{
	public:
		CAutoPtrFind(HANDLE hHandle);
		virtual ~CAutoPtrFind(void);
	protected:
		virtual void Cleanup();
	};


	class CAutoPtrSocket
	{
	public:
		CAutoPtrSocket(SOCKET hSocket);
		virtual ~CAutoPtrSocket(void);
		operator SOCKET();
		void		 attach(SOCKET hSocket);
		SOCKET	 release();
		bool		 isValid() const;
		void     reset();
	protected:
		virtual void Cleanup();
		SOCKET m_hSocket;
	};
};