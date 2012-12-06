/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _SHADER_H_
#define _SHADER_H_

#include <vector>
#include "d3dx9.h"
#include "Renderer.h"

enum EShaderType
{
	ST_VERTEX,
	ST_PIXEL
};

#define DECLARE_SHADER(InShaderName) \
	public: \
	static	InShaderName StaticType;

#define IMPLEMENT_SHADER(ShaderName, ShaderSrc, ShaderType)\
	ShaderName ShaderName::StaticType = ShaderName( ShaderType, ShaderSrc );

class Shader
{
public:
	Shader( EShaderType ShaderType, char* pShaderSrc ) :
		m_ShaderType( ShaderType ),
		m_pShaderSrc( pShaderSrc )
		{
			std::vector<Shader*>&  ShaderList = GetShaderList();
			ShaderList.push_back( this );
		}

		static std::vector<Shader*>& GetShaderList()
		{
			static std::vector<Shader*> s_Shaders;
			return s_Shaders;
		}

		static bool CompileAllShaders();
		static void ReleaseAllShaders();

		LPDIRECT3DVERTEXSHADER9	GetVertexShader() { return m_pVShader; }
		LPDIRECT3DPIXELSHADER9	GetPixelShader() { return m_pPShader; }

protected:
	EShaderType	m_ShaderType;
	char* m_pShaderSrc;

	bool CompileShader();

 	LPDIRECT3DVERTEXSHADER9	m_pVShader;
	LPDIRECT3DPIXELSHADER9	m_pPShader;
	LPD3DXCONSTANTTABLE m_pConstantTable;
//	LPDIRECT3DVERTEXDECLARATION9* m_ppVertexDecl;
};

class DiffuseUVVertexShader : public Shader
{
	DECLARE_SHADER(DiffuseUVVertexShader);
public:
	DiffuseUVVertexShader( EShaderType ShaderType, char* pShaderSrc ) : Shader( ShaderType, pShaderSrc ){};

	void SetWVP( D3DXMATRIX* pWVPMat )
	{
		Renderer::SetConstantTableMatrix( m_pConstantTable, "WVPMat", pWVPMat );
	}
	void SetWV( D3DXMATRIX* pWVMat )
	{
		Renderer::SetConstantTableMatrix( m_pConstantTable, "WVMat", pWVMat );
	}
};

class DiffuseUVEnvVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(DiffuseUVEnvVertexShader);
public:
	DiffuseUVEnvVertexShader( EShaderType ShaderType, char* pShaderSrc ) : DiffuseUVVertexShader( ShaderType, pShaderSrc ){};
};

class DiffuseUVCubeVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(DiffuseUVCubeVertexShader);
public:
	DiffuseUVCubeVertexShader( EShaderType ShaderType, char* pShaderSrc ) : DiffuseUVVertexShader( ShaderType, pShaderSrc ){};

	void SetScale( D3DXVECTOR4* pvScale )
	{
		Renderer::SetConstantTableVector( m_pConstantTable, "vScale", pvScale);
	}
	void SetColour( D3DXVECTOR4* pvCol )
	{
		Renderer::SetConstantTableVector( m_pConstantTable, "Col", pvCol);
	}

};

class DiffuseUVEnvCubeVertexShader : public DiffuseUVCubeVertexShader
{
	DECLARE_SHADER(DiffuseUVEnvCubeVertexShader);
public:
	DiffuseUVEnvCubeVertexShader( EShaderType ShaderType, char* pShaderSrc ) : DiffuseUVCubeVertexShader( ShaderType, pShaderSrc ){};
};

class DiffuseNormalEnvCubeVertexShader : public DiffuseUVCubeVertexShader
{
	DECLARE_SHADER(DiffuseNormalEnvCubeVertexShader);
public:
	DiffuseNormalEnvCubeVertexShader( EShaderType ShaderType, char* pShaderSrc ) : DiffuseUVCubeVertexShader( ShaderType, pShaderSrc ){};
};

class UVNormalEnvVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(UVNormalEnvVertexShader);
public:
	UVNormalEnvVertexShader( EShaderType ShaderType, char* pShaderSrc ) : DiffuseUVVertexShader( ShaderType, pShaderSrc ){};
};

#endif _SHADER_H_
