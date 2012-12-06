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

#include "Shader.h"
#include "Renderer.h"

void Shader::ReleaseAllShaders()
{
	std::vector<Shader*>&  ShaderList = GetShaderList();

	std::vector<Shader*>::iterator ShaderIterator;

	for (ShaderIterator = ShaderList.begin(); 
		ShaderIterator != ShaderList.end();
		ShaderIterator++ )
	{
		if ( (*ShaderIterator)->m_pVShader )
			(*ShaderIterator)->m_pVShader->Release();
		if ( (*ShaderIterator)->m_pPShader )
			(*ShaderIterator)->m_pPShader->Release();
	}
}

bool Shader::CompileAllShaders()
{
	std::vector<Shader*>&  ShaderList = GetShaderList();

	std::vector<Shader*>::iterator ShaderIterator;

	for (ShaderIterator = ShaderList.begin(); 
		ShaderIterator != ShaderList.end();
		ShaderIterator++ )
	{
		if ( (*ShaderIterator)->CompileShader() == false )
			return false;
	}
	return true;
}

bool Shader::CompileShader()
{
	DWORD dwShaderFlags = D3DXSHADER_PARTIALPRECISION;
	LPD3DXBUFFER pCode;
	LPD3DXBUFFER pErrors;

	int iSrcLen = strlen( m_pShaderSrc );
	if ( iSrcLen == 0 )
		return false;

	HRESULT hr = ( D3DXCompileShader( m_pShaderSrc,
		iSrcLen,
		NULL,											// defines
		NULL,											// include
		"Main",											// Function name
		m_ShaderType == ST_PIXEL ? "ps_2_0": "vs_2_0",	// profile
		dwShaderFlags,									// Flags
		&pCode,											// Output compiled shader
		&pErrors,										// error messages
		&m_pConstantTable								// constant table
		) );

	if( FAILED( hr ) )
	{
		char* errors = (char*)pErrors->GetBufferPointer();
		OutputDebugStringA( errors );
		return false;
	}

	if ( m_ShaderType == ST_PIXEL )
	{
		m_pPShader = Renderer::CreatePixelShader( ( DWORD* )pCode->GetBufferPointer() );
	}
	else
	{
		m_pVShader = Renderer::CreateVertexShader( ( DWORD* )pCode->GetBufferPointer() );
	}

	pCode->Release();
	return true;
}

char DiffuseUVVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, float4(input.vPosition.xyz, 1.0f));	\n"
	"	output.Colour = input.Colour;										\n"
	"	output.Tex0 = input.Tex0;											\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

char DiffuseUVEnvVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 vNormal : NORMAL;									\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4x4 WVMat;														\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, float4(input.vPosition.xyz, 1.0f));	\n"
	"	output.Colour = input.Colour;										\n"
	"	float4 NormalCamSpace = mul(WVMat, float4(input.vNormal.xyz, 0.0f));\n"
	"	output.Tex0.xy = (NormalCamSpace.xy * 0.8f) + 0.5f;					\n"
//	"	output.Colour = float4(NormalCamSpace.xyz, 1);						\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

char DiffuseUVCubeVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4 Col;															\n"
	" float4 vScale;														\n"
	" float4x4 WVMat;														\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	float4 vPos = float4(input.vPosition.xyz * vScale, 1);				\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, vPos);								\n"
	"	output.Colour = Col;												\n"
	"	output.Tex0.xy = input.Tex0;										\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

char DiffuseUVEnvCubeVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 vNormal : NORMAL;									\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4 Col;															\n"
	" float4 vScale;														\n"
	" float4x4 WVMat;														\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	float4 vPos = float4(input.vPosition.xyz * vScale, 1);				\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, vPos);								\n"
	"	output.Colour = Col;												\n"
	"	float4 NormalCamSpace = mul(WVMat, float4(input.vNormal.xyz, 0.0f));\n"
	"	output.Tex0.xy = (NormalCamSpace.xy * 0.5f) + 0.5f;					\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

char DiffuseNormalEnvCubeVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 vNormal : NORMAL;									\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4 Col;															\n"
	" float4x4 WVMat;														\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	float4 vPos = float4(input.vPosition.xyz, 1);						\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, vPos);								\n"
	"	output.Colour = 0xffffffff;											\n"
	"	float4 NormalCamSpace = mul(WVMat, float4(input.vNormal.xyz, 0.0f));\n"
	"	output.Tex0.xy = (NormalCamSpace.xy * 0.2f) + 0.5f;					\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

char UVNormalEnvVertexShaderSrc[] =
{
	"		struct VS_INPUT													\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 vNormal : NORMAL;									\n"
	"		};																\n"
	"		struct VS_OUTPUT												\n"
	"		{																\n"
	"			float4 vPosition : POSITION;								\n"
	"			float4 Colour : COLOR;										\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" float4x4 WVMat;														\n"
	" float4x4 WVPMat;														\n"
	" VS_OUTPUT Main( VS_INPUT input )										\n"
	" {																		\n"
	"	VS_OUTPUT output;													\n"
	"	// Transform coord													\n"
	"	output.vPosition = mul(WVPMat, float4(input.vPosition.xyz, 1.0f));	\n"
	"	output.Colour = 0xffffffff;											\n"
	"	float4 NormalCamSpace = mul(WVMat, float4(input.vNormal.xyz, 0.0f));\n"
	"	output.Tex0.xy = (NormalCamSpace.xy * 0.2f) + 0.5f;					\n"
	"	return output;														\n"
	" }																		\n"
	"																		\n"
};

IMPLEMENT_SHADER(DiffuseUVVertexShader, DiffuseUVVertexShaderSrc, ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVEnvVertexShader, DiffuseUVEnvVertexShaderSrc, ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVCubeVertexShader, DiffuseUVCubeVertexShaderSrc, ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVEnvCubeVertexShader, DiffuseUVEnvCubeVertexShaderSrc, ST_VERTEX);
IMPLEMENT_SHADER(DiffuseNormalEnvCubeVertexShader, DiffuseNormalEnvCubeVertexShaderSrc, ST_VERTEX);
IMPLEMENT_SHADER(UVNormalEnvVertexShader, UVNormalEnvVertexShaderSrc, ST_VERTEX);
