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
    if ((*ShaderIterator)->m_pConstantBuffer)
      (*ShaderIterator)->m_pConstantBuffer->Release();
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
	if ( m_iCodeLen == 0 )
		return false;

	if ( m_ShaderType == ST_PIXEL )
		Renderer::CreatePixelShader( m_pShaderSrc, m_iCodeLen, &m_pPShader );
	else
		Renderer::CreateVertexShader( m_pShaderSrc, m_iCodeLen, &m_pVShader );

	return true;
}

void Shader::CommitConstants(ID3D11DeviceContext* pContext)
{
  if (m_ShaderType != ST_VERTEX)
    return;

  if (!m_pConstantBuffer)
    CreateConstantBuffer();

  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    *(ShaderContants*)res.pData = m_ShaderContants;
    pContext->Unmap(m_pConstantBuffer, 0);
  }

  pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
}

void Shader::CreateConstantBuffer()
{
  CD3D11_BUFFER_DESC bDesc(sizeof(ShaderContants), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  Renderer::GetDevice()->CreateBuffer(&bDesc, NULL, &m_pConstantBuffer);
}


IMPLEMENT_SHADER(DiffuseUVVertexShader, DiffuseUVVertexShaderCode, sizeof(DiffuseUVVertexShaderCode), ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVEnvVertexShader, DiffuseUVEnvVertexShaderCode, sizeof(DiffuseUVEnvVertexShaderCode), ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVCubeVertexShader, DiffuseUVCubeVertexShaderCode, sizeof(DiffuseUVCubeVertexShaderCode), ST_VERTEX);
IMPLEMENT_SHADER(DiffuseUVEnvCubeVertexShader, DiffuseUVEnvCubeVertexShaderCode, sizeof(DiffuseUVEnvCubeVertexShaderCode), ST_VERTEX);
IMPLEMENT_SHADER(DiffuseNormalEnvCubeVertexShader, DiffuseNormalEnvCubeVertexShaderCode, sizeof(DiffuseNormalEnvCubeVertexShaderCode), ST_VERTEX);
IMPLEMENT_SHADER(UVNormalEnvVertexShader, UVNormalEnvVertexShaderCode, sizeof(UVNormalEnvVertexShaderCode), ST_VERTEX);
