//--------------------------------------------------------------------------------------
// File: SOParser.h
//
// Direct3D 11 Effects Stream Out Decl Parser
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <string.h>

namespace D3DX11Effects
{
    
//////////////////////////////////////////////////////////////////////////
// CSOParser
//////////////////////////////////////////////////////////////////////////

class CSOParser
{

    CEffectVector<D3D11_SO_DECLARATION_ENTRY>   m_vDecls;                                       // Set of parsed decl entries
    D3D11_SO_DECLARATION_ENTRY                  m_newEntry;                                     // Currently parsing entry
    LPSTR                                       m_SemanticString[D3D11_SO_BUFFER_SLOT_COUNT];   // Copy of strings

    static const size_t MAX_ERROR_SIZE = 254;
    char                                        m_pError[ MAX_ERROR_SIZE + 1 ];                 // Error buffer

public:
    CSOParser() noexcept :
        m_newEntry{},
        m_SemanticString{},
        m_pError{}
    {
    }

    ~CSOParser()
    {
        for( size_t Stream = 0; Stream < D3D11_SO_STREAM_COUNT; ++Stream )
        {
            SAFE_DELETE_ARRAY( m_SemanticString[Stream] );
        }
    }

    // Parse a single string, assuming stream 0
    HRESULT Parse( _In_z_ LPCSTR pString )
    {
        m_vDecls.Clear();
        return Parse( 0, pString );
    }

    // Parse all 4 streams
    HRESULT Parse( _In_z_ LPSTR pStreams[D3D11_SO_STREAM_COUNT] )
    {
        HRESULT hr = S_OK;
        m_vDecls.Clear();
        for( uint32_t iDecl=0; iDecl < D3D11_SO_STREAM_COUNT; ++iDecl )
        {
            hr = Parse( iDecl, pStreams[iDecl] );
            if( FAILED(hr) )
            {
                char str[16];
                sprintf_s( str, 16, " in stream %u.", iDecl );
                str[15] = 0;
                strcat_s( m_pError, MAX_ERROR_SIZE, str );
                return hr;
            }
        }
        return hr;
    }

    // Return resulting declarations
    D3D11_SO_DECLARATION_ENTRY *GetDeclArray()
    {
        return &m_vDecls[0];
    }

    char* GetErrorString()
    {
        return m_pError;
    }

    uint32_t GetDeclCount() const
    {
        return m_vDecls.GetSize();
    }

    // Return resulting buffer strides
    void GetStrides( uint32_t strides[4] )
    {
        size_t  len = GetDeclCount();
        strides[0] = strides[1] = strides[2] = strides[3] = 0;

        for( size_t  i=0; i < len; i++ )
        {
            strides[m_vDecls[i].OutputSlot] += m_vDecls[i].ComponentCount * sizeof(float);
        }
    }

protected:

    // Parse a single string "[<slot> :] <semantic>[<index>][.<mask>]; [[<slot> :] <semantic>[<index>][.<mask>][;]]"
    HRESULT Parse( _In_ uint32_t Stream, _In_z_ LPCSTR pString )
    {
        HRESULT hr = S_OK;

        m_pError[0] = 0;

        if( pString == nullptr )
            return S_OK;

        uint32_t len = (uint32_t)strlen( pString );
        if( len == 0 )
            return S_OK;

        SAFE_DELETE_ARRAY( m_SemanticString[Stream] );
        VN( m_SemanticString[Stream] = new char[len + 1] );
        strcpy_s( m_SemanticString[Stream], len + 1, pString );

        LPSTR pSemantic = m_SemanticString[Stream];

        while( true )
        {
            // Each decl entry is delimited by a semi-colon
            LPSTR pSemi = strchr( pSemantic, ';' );

            // strip leading and trailing spaces
            LPSTR pEnd;
            if( pSemi != nullptr )
            {
                *pSemi = '\0';
                pEnd = pSemi - 1;
            }
            else
            {
                pEnd = pSemantic + strlen( pSemantic );
            }
            while( isspace( (unsigned char)*pSemantic ) )
                pSemantic++;
            while( pEnd > pSemantic && isspace( (unsigned char)*pEnd ) )
            {
                *pEnd = '\0';
                pEnd--;
            }

            if( *pSemantic != '\0' )
            {
                VH( AddSemantic( pSemantic ) );
                m_newEntry.Stream = Stream;

                VH( m_vDecls.Add( m_newEntry ) );
            }
            if( pSemi == nullptr )
                break;
            pSemantic = pSemi + 1;
        }

lExit:
        return hr;
    }

    // Parse a single decl  "[<slot> :] <semantic>[<index>][.<mask>]"
    HRESULT AddSemantic( _Inout_z_ LPSTR pSemantic )
    {
        HRESULT hr = S_OK;

        assert( pSemantic );

        ZeroMemory( &m_newEntry, sizeof(m_newEntry) );
        VH( ConsumeOutputSlot( &pSemantic ) );
        VH( ConsumeRegisterMask( pSemantic ) );
        VH( ConsumeSemanticIndex( pSemantic ) );

        // pSenantic now contains only the SemanticName (all other fields were consumed)
        if( strcmp( "$SKIP", pSemantic ) != 0 )
        {
            m_newEntry.SemanticName = pSemantic;
        }

lExit:
        return hr;
    }

    // Parse optional mask "[.<mask>]"
    HRESULT ConsumeRegisterMask( _Inout_z_ LPSTR pSemantic )
    {
        HRESULT hr = S_OK;
        const char *pFullMask1 = "xyzw";
        const char *pFullMask2 = "rgba";
        size_t stringLength;
        size_t startComponent = 0;
        LPCSTR p;

        assert( pSemantic );

        pSemantic = strchr( pSemantic, '.' ); 

        if( pSemantic == nullptr )
        {
            m_newEntry.ComponentCount = 4;
            return S_OK;
        }

        *pSemantic = '\0';
        pSemantic++;

        stringLength = strlen( pSemantic );
        p = strstr(pFullMask1, pSemantic );
        if( p )
        {
            startComponent = (uint32_t)( p - pFullMask1 );
        }
        else
        {
            p = strstr( pFullMask2, pSemantic );
            if( p )
                startComponent = (uint32_t)( p - pFullMask2 );
            else
            {
                sprintf_s( m_pError, MAX_ERROR_SIZE, "ID3D11Effect::ParseSODecl - invalid mask declaration '%s'", pSemantic );
                VH( E_FAIL );
            }

        }

        if( stringLength == 0 )
            stringLength = 4;

        m_newEntry.StartComponent = (uint8_t)startComponent;
        m_newEntry.ComponentCount = (uint8_t)stringLength;

lExit:
        return hr;
    }

    // Parse optional output slot "[<slot> :]"
    HRESULT ConsumeOutputSlot( _Inout_z_ LPSTR* ppSemantic )
    {
        assert( ppSemantic && *ppSemantic );
        _Analysis_assume_( ppSemantic && *ppSemantic );

        HRESULT hr = S_OK;
        LPSTR pColon = strchr( *ppSemantic, ':' ); 

        if( pColon == nullptr )
            return S_OK;

        if( pColon == *ppSemantic )
        {
            strcpy_s( m_pError, MAX_ERROR_SIZE,
                           "ID3D11Effect::ParseSODecl - Invalid output slot" );
            VH( E_FAIL );
        }

        *pColon = '\0';
        int outputSlot = atoi( *ppSemantic );
        if( outputSlot < 0 || outputSlot > 255 )
        {
            strcpy_s( m_pError, MAX_ERROR_SIZE,
                           "ID3D11Effect::ParseSODecl - Invalid output slot" );
            VH( E_FAIL );
        }
        m_newEntry.OutputSlot = (uint8_t)outputSlot;

        while( *ppSemantic < pColon )
        {
            if( !isdigit( (unsigned char)**ppSemantic ) )
            {
                sprintf_s( m_pError, MAX_ERROR_SIZE, "ID3D11Effect::ParseSODecl - Non-digit '%c' in output slot", **ppSemantic );
                VH( E_FAIL );
            }
            (*ppSemantic)++;
        }

        // skip the colon (which is now '\0')
        (*ppSemantic)++;

        while( isspace( (unsigned char)**ppSemantic ) )
            (*ppSemantic)++;

lExit:
        return hr;
    }

    // Parse optional index "[<index>]"
    HRESULT ConsumeSemanticIndex( _Inout_z_ LPSTR pSemantic )
    {
        assert( pSemantic );

        uint32_t uLen = (uint32_t)strlen( pSemantic );

        // Grab semantic index
        while( uLen > 0 && isdigit( (unsigned char)pSemantic[uLen - 1] ) )
            uLen--;

        if( isdigit( (unsigned char)pSemantic[uLen] ) )
        {
            m_newEntry.SemanticIndex = atoi( pSemantic + uLen );
            pSemantic[uLen] = '\0';
        } 
        else
        {
            m_newEntry.SemanticIndex = 0;
        }

        return S_OK;
    }
};

} // end namespace D3DX11Effects
