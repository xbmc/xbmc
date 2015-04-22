/*
 *  Copyright © 2010-2013 Team XBMC
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
#include "Renderer.h"

namespace
{
  #include "../Shaders/DiffuseUVVertexShader.inc"
  #include "../Shaders/DiffuseUVEnvVertexShader.inc"
  #include "../Shaders/DiffuseUVCubeVertexShader.inc"
  #include "../Shaders/DiffuseUVEnvCubeVertexShader.inc"
  #include "../Shaders/DiffuseNormalEnvCubeVertexShader.inc"
  #include "../Shaders/UVNormalEnvVertexShader.inc"
}

enum EShaderType
{
	ST_VERTEX,
	ST_PIXEL
};

#define DECLARE_SHADER(InShaderName) \
	public: \
	static	InShaderName StaticType;

#define IMPLEMENT_SHADER(ShaderName, ShaderCode, iLenght, ShaderType) \
	ShaderName ShaderName::StaticType = ShaderName( ShaderType, ShaderCode, iLenght );

class Shader
{
public:
	Shader( EShaderType ShaderType, const void* pShaderCode, unsigned int iCodeLen ) :
		m_ShaderType( ShaderType ),
    m_pShaderSrc( pShaderCode ),
    m_iCodeLen( iCodeLen )
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

		ID3D11VertexShader*	GetVertexShader() { return m_pVShader; }
		ID3D11PixelShader*	GetPixelShader() { return m_pPShader; }
    void CommitConstants(ID3D11DeviceContext* pContext);

    void SetWVP(DirectX::XMMATRIX* pWVPMat)
    {
      XMStoreFloat4x4(&m_ShaderContants.WVPMat, *pWVPMat);
    }
    void SetWV(DirectX::XMMATRIX* pWVMat)
    {
      XMStoreFloat4x4(&m_ShaderContants.WVMat, *pWVMat);
    }
    void SetScale(DirectX::XMVECTOR* pvScale)
    {
      XMStoreFloat4(&m_ShaderContants.vScale, *pvScale);
    }
    void SetColour(DirectX::XMFLOAT4* pvCol)
    {
      m_ShaderContants.Col = *pvCol;
    }

    EShaderType	   m_ShaderType;
protected:
  struct ShaderContants
  {
    DirectX::XMFLOAT4   Col;
    DirectX::XMFLOAT4   vScale;
    DirectX::XMFLOAT4X4 WVPMat;
    DirectX::XMFLOAT4X4 WVMat;
  };

	const void*    m_pShaderSrc;
  unsigned int   m_iCodeLen;
  ShaderContants m_ShaderContants;

	bool CompileShader();
  void CreateConstantBuffer();

  ID3D11VertexShader*	m_pVShader;
  ID3D11PixelShader*	m_pPShader;
	ID3D11Buffer*       m_pConstantBuffer;
};

class DiffuseUVVertexShader : public Shader
{
	DECLARE_SHADER(DiffuseUVVertexShader);
public:
	DiffuseUVVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLengh ) : Shader( ShaderType, pShaderCode, iLengh ){};
};

class DiffuseUVEnvVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(DiffuseUVEnvVertexShader);
public:
	DiffuseUVEnvVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLenght ) : DiffuseUVVertexShader( ShaderType, pShaderCode, iLenght ){};
};

class DiffuseUVCubeVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(DiffuseUVCubeVertexShader);
public:
	DiffuseUVCubeVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLenght ) : DiffuseUVVertexShader( ShaderType, pShaderCode, iLenght ){};
};

class DiffuseUVEnvCubeVertexShader : public DiffuseUVCubeVertexShader
{
	DECLARE_SHADER(DiffuseUVEnvCubeVertexShader);
public:
	DiffuseUVEnvCubeVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLenght ) : DiffuseUVCubeVertexShader( ShaderType, pShaderCode, iLenght ){};
};

class DiffuseNormalEnvCubeVertexShader : public DiffuseUVCubeVertexShader
{
	DECLARE_SHADER(DiffuseNormalEnvCubeVertexShader);
public:
	DiffuseNormalEnvCubeVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLenght ) : DiffuseUVCubeVertexShader( ShaderType, pShaderCode, iLenght ){};
};

class UVNormalEnvVertexShader : public DiffuseUVVertexShader
{
	DECLARE_SHADER(UVNormalEnvVertexShader);
public:
	UVNormalEnvVertexShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iLenght ) : DiffuseUVVertexShader( ShaderType, pShaderCode, iLenght ){};
};

#endif _SHADER_H_
