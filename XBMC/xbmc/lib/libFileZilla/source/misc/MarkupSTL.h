// Markup.h: interface for the CMarkupSTL class.
//
// Markup Release 6.3
// Copyright (C) 1999-2002 First Objective Software, Inc. All rights reserved
// Go to www.firstobject.com for the latest CMarkupSTL and EDOM documentation
// Use in commercial applications requires written permission
// This software is provided "as is", with no warranty.

#if !defined(AFX_MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_)
#define AFX_MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _DEBUG
#define _DS(i) (i?&((LPCTSTR)m_csDoc)[m_aPos[i].nStartL]:0)
#define MARKUP_SETDEBUGSTATE m_pMainDS=_DS(m_iPos); m_pChildDS=_DS(m_iPosChild)
#else
#define MARKUP_SETDEBUGSTATE
#endif

class CMarkupSTL  
{
public:
	CMarkupSTL() { SetDoc( NULL ); };
	CMarkupSTL( LPCTSTR szDoc ) { SetDoc( szDoc ); };
	CMarkupSTL( const CMarkupSTL& markup ) { *this = markup; };
	void operator=( const CMarkupSTL& markup );
	virtual ~CMarkupSTL() {};

	// Navigate
	bool Load( LPCTSTR szFileName );
	bool SetDoc( LPCTSTR szDoc );
	bool IsWellFormed();
	bool FindElem( LPCTSTR szName=NULL );
	bool FindChildElem( LPCTSTR szName=NULL );
	bool IntoElem();
	bool OutOfElem();
	void ResetChildPos() { x_SetPos(m_iPosParent,m_iPos,0); };
	void ResetMainPos() { x_SetPos(m_iPosParent,0,0); };
	void ResetPos() { x_SetPos(0,0,0); };
	CStdString GetTagName() const;
	CStdString GetChildTagName() const { return x_GetTagName(m_iPosChild); };
	CStdString GetData() const { return x_GetData(m_iPos); };
	CStdString GetChildData() const { return x_GetData(m_iPosChild); };
	CStdString GetAttrib( LPCTSTR szAttrib ) const { return x_GetAttrib(m_iPos,szAttrib); };
	CStdString GetChildAttrib( LPCTSTR szAttrib ) const { return x_GetAttrib(m_iPosChild,szAttrib); };
	CStdString GetAttribName( int n ) const;
	bool SavePos( LPCTSTR szPosName=_T("") );
	bool RestorePos( LPCTSTR szPosName=_T("") );
	bool GetOffsets( int& nStart, int& nEnd ) const;
	CStdString GetError() const { return m_csError; };

	enum MarkupNodeType
	{
		MNT_ELEMENT					= 1,  // 0x01
		MNT_TEXT					= 2,  // 0x02
		MNT_WHITESPACE				= 4,  // 0x04
		MNT_CDATA_SECTION			= 8,  // 0x08
		MNT_PROCESSING_INSTRUCTION	= 16, // 0x10
		MNT_COMMENT					= 32, // 0x20
		MNT_DOCUMENT_TYPE			= 64, // 0x40
		MNT_EXCLUDE_WHITESPACE		= 123,// 0x7b
	};

	// Create
	bool Save( LPCTSTR szFileName );
	CStdString GetDoc() const { return m_csDoc; };
	bool AddElem( LPCTSTR szName, LPCTSTR szData=NULL ) { return x_AddElem(szName,szData,false,false); };
	bool InsertElem( LPCTSTR szName, LPCTSTR szData=NULL ) { return x_AddElem(szName,szData,true,false); };
	bool AddChildElem( LPCTSTR szName, LPCTSTR szData=NULL ) { return x_AddElem(szName,szData,false,true); };
	bool InsertChildElem( LPCTSTR szName, LPCTSTR szData=NULL ) { return x_AddElem(szName,szData,true,true); };
	bool AddAttrib( LPCTSTR szAttrib, LPCTSTR szValue ) { return x_SetAttrib(m_iPos,szAttrib,szValue); };
	bool AddChildAttrib( LPCTSTR szAttrib, LPCTSTR szValue ) { return x_SetAttrib(m_iPosChild,szAttrib,szValue); };
	bool AddAttrib( LPCTSTR szAttrib, int nValue ) { return x_SetAttrib(m_iPos,szAttrib,nValue); };
	bool AddChildAttrib( LPCTSTR szAttrib, int nValue ) { return x_SetAttrib(m_iPosChild,szAttrib,nValue); };
	bool AddChildAttrib( LPCTSTR szAttrib, __int64 nValue ) { return x_SetAttrib(m_iPosChild,szAttrib,nValue); };
	bool AddChildSubDoc( LPCTSTR szSubDoc ) { return x_AddSubDoc(szSubDoc,false,true); };
	bool InsertChildSubDoc( LPCTSTR szSubDoc ) { return x_AddSubDoc(szSubDoc,true,true); };
	CStdString GetChildSubDoc() const;

	// Modify
	bool RemoveElem();
	bool RemoveChildElem();
	bool SetAttrib( LPCTSTR szAttrib, LPCTSTR szValue ) { return x_SetAttrib(m_iPos,szAttrib,szValue); };
	bool SetChildAttrib( LPCTSTR szAttrib, LPCTSTR szValue ) { return x_SetAttrib(m_iPosChild,szAttrib,szValue); };
	bool SetAttrib( LPCTSTR szAttrib, int nValue ) { return x_SetAttrib(m_iPos,szAttrib,nValue); };
	bool SetChildAttrib( LPCTSTR szAttrib, int nValue ) { return x_SetAttrib(m_iPosChild,szAttrib,nValue); };
	bool SetData( LPCTSTR szData, int nCDATA=0 ) { return x_SetData(m_iPos,szData,nCDATA); };
	bool SetChildData( LPCTSTR szData, int nCDATA=0 ) { return x_SetData(m_iPosChild,szData,nCDATA); };

protected:

#ifdef _DEBUG
	LPCTSTR m_pMainDS;
	LPCTSTR m_pChildDS;
#endif

	CStdString m_csDoc;
	CStdString m_csError;

	struct ElemPos
	{
		ElemPos() { Clear(); };
		ElemPos( const ElemPos& pos ) { *this = pos; };
		bool IsEmptyElement() const { return (nStartR == nEndL + 1); };
		void Clear()
		{
			nStartL=0; nStartR=0; nEndL=0; nEndR=0; nReserved=0;
			iElemParent=0; iElemChild=0; iElemNext=0;
		};
		void AdjustStart( int n ) { nStartL+=n; nStartR+=n; };
		void AdjustEnd( int n ) { nEndL+=n; nEndR+=n; };
		int nStartL;
		int nStartR;
		int nEndL;
		int nEndR;
		int nReserved;
		int iElemParent;
		int iElemChild;
		int iElemNext;
	};

	std::vector<ElemPos> m_aPos;
	int m_iPosParent;
	int m_iPos;
	int m_iPosChild;
	int m_iPosFree;
	int m_nNodeType;

	struct TokenPos
	{
		TokenPos( LPCTSTR sz ) { Clear(); szDoc = sz; };
		bool IsValid() const { return (nL <= nR); };
		void Clear() { nL=0; nR=-1; nNext=0; bIsString=false; };
		bool Match( LPCTSTR szName )
		{
			int nLen = nR - nL + 1;
		// To ignore case, define MARKUP_IGNORECASE
		#ifdef MARKUP_IGNORECASE
			return ( (_tcsncicmp( &szDoc[nL], szName, nLen ) == 0)
		#else
			return ( (_tcsnccmp( &szDoc[nL], szName, nLen ) == 0)
		#endif
				&& ( szName[nLen] == _T('\0') || _tcschr(_T(" =/["),szName[nLen]) ) );
		};
		int nL;
		int nR;
		int nNext;
		LPCTSTR szDoc;
		bool bIsString;
	};

	struct SavedPos
	{
		int iPosParent;
		int iPos;
		int iPosChild;
	};
	std::map<CStdString, SavedPos> m_mapSavedPos;

	void x_SetPos( int iPosParent, int iPos, int iPosChild )
	{
		m_iPosParent = iPosParent;
		m_iPos = iPos;
		m_iPosChild = iPosChild;
		m_nNodeType = iPos?MNT_ELEMENT:0;
		MARKUP_SETDEBUGSTATE;
	};

	int x_GetFreePos();
	int x_ReleasePos();

	int x_ParseElem( int iPos );
	int x_ParseError( LPCTSTR szError, LPCTSTR szName = NULL );
	static bool x_FindChar( LPCTSTR szDoc, int& nChar, _TCHAR c );
	static bool x_FindToken( TokenPos& token );
	CStdString x_GetToken( const TokenPos& token ) const;
	int x_FindElem( int iPosParent, int iPos, LPCTSTR szPath );
	CStdString x_GetTagName( int iPos ) const;
	CStdString x_GetData( int iPos ) const;
	CStdString x_GetAttrib( int iPos, LPCTSTR szAttrib ) const;
	bool x_AddElem( LPCTSTR szName, LPCTSTR szValue, bool bInsert, bool bAddChild );
	bool x_AddSubDoc( LPCTSTR szSubDoc, bool bInsert, bool bAddChild );
	bool x_FindAttrib( TokenPos& token, LPCTSTR szAttrib=NULL ) const;
	bool x_SetAttrib( int iPos, LPCTSTR szAttrib, LPCTSTR szValue );
	bool x_SetAttrib( int iPos, LPCTSTR szAttrib, int nValue );
	bool x_SetAttrib( int iPos, LPCTSTR szAttrib, __int64 nValue );
	bool x_CreateNode( CStdString& csNode, int nNodeType, LPCTSTR szText );
	void x_LocateNew( int iPosParent, int& iPosRel, int& nOffset, int nLength, int nFlags );
	int x_ParseNode( TokenPos& token );
	bool x_SetData( int iPos, LPCTSTR szData, int nCDATA );
	int x_RemoveElem( int iPos );
	void x_DocChange( int nLeft, int nReplace, const CStdString& csInsert );
	void x_PosInsert( int iPos, int nInsertLength );
	void x_Adjust( int iPos, int nShift, bool bAfterPos = false );
	CStdString x_TextToDoc( LPCTSTR szText, bool bAttrib = false ) const;
	CStdString x_TextFromDoc( int nLeft, int nRight ) const;
};

#endif // !defined(AFX_MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570__INCLUDED_)
