// MeshConv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "NVMeshMender.h"

LPDIRECT3D9 pD3D;
LPDIRECT3DDEVICE9 pD3DDevice;

#define MESHFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

struct MESHVERT
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 norm;
	float s, t;
};

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		puts("Usage: MeshConv meshfile rdffile");
		return 1;
	}

	// Initialize DirectDraw
	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL)
	{
		puts("Cannot init D3D");
		return 1;
	}

	MeshMender mender;
	std::vector<MeshMender::Vertex> MendVerts;
	std::vector<unsigned int> MendIndices;
	std::vector<unsigned int> mappingNewToOld;

	HRESULT hr;
	D3DDISPLAYMODE dispMode;
	D3DPRESENT_PARAMETERS presentParams;

	pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dispMode);

	ZeroMemory(&presentParams, sizeof(presentParams));
	presentParams.Windowed = TRUE;
	presentParams.hDeviceWindow = GetConsoleWindow();
	presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
	presentParams.BackBufferWidth = 8;
	presentParams.BackBufferHeight = 8;
	presentParams.BackBufferFormat = dispMode.Format;

	hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &pD3DDevice);
	if (FAILED(hr))
	{
		printf("Cannot init D3D device: %08x\n", hr);
		pD3D->Release();
		return 1;
	}

	printf("Loading mesh %s: ", argv[1]);

	LPD3DXBUFFER pAdjacency, pMaterials, pEffects;
	DWORD n;
	LPD3DXMESH pLoadMesh;
	hr = D3DXLoadMeshFromX(argv[1], D3DXMESH_SYSTEMMEM, pD3DDevice, &pAdjacency, &pMaterials, &pEffects, &n, &pLoadMesh);
	if (FAILED(hr))
	{
		printf("ERROR: %08x\n", hr);
		goto mesherror;
	}
	pEffects->Release();
	pMaterials->Release();
	printf("%d faces, %d verts\n", pLoadMesh->GetNumFaces(), pLoadMesh->GetNumVertices());

	LPD3DXMESH pMesh;
	if (pLoadMesh->GetFVF() != MESHFVF)
	{
		hr = pLoadMesh->CloneMeshFVF(D3DXMESH_SYSTEMMEM, MESHFVF, pD3DDevice, &pMesh);
		pLoadMesh->Release();
		if (FAILED(hr))
		{
			printf("CloneMesh error: %08x\n", hr);
			goto mesherror;
		}
	}
	else
		pMesh = pLoadMesh;

	printf("Welding verts: ");

	DWORD* pAdj = new DWORD[pAdjacency->GetBufferSize() / 4];

	D3DXWELDEPSILONS Eps;
	memset(&Eps, 0, sizeof(Eps));
	hr = D3DXWeldVertices(pMesh, D3DXWELDEPSILONS_WELDPARTIALMATCHES, &Eps, (DWORD*)pAdjacency->GetBufferPointer(), pAdj, NULL, NULL);
	if (FAILED(hr))
	{
		printf("ERROR: %08x\n", hr);
		goto mesherror;
	}
	
	hr = pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, pAdj, (DWORD*)pAdjacency->GetBufferPointer(), NULL, NULL);
	if (FAILED(hr))
	{
		printf("ERROR: %08x\n", hr);
		goto mesherror;
	}
	
	pAdjacency->Release();
	delete [] pAdj;

	printf("%d faces, %d verts\n", pMesh->GetNumFaces(), pMesh->GetNumVertices());

	printf("Mending mesh: ");

	DWORD NumVerts = pMesh->GetNumVertices();
	DWORD NumFaces = pMesh->GetNumFaces();

	MESHVERT* MeshVert;
	pMesh->LockVertexBuffer(0, (LPVOID*)&MeshVert);

	//fill up Mend vectors with your mesh's data
	MendVerts.reserve(NumVerts);
	for(DWORD i = 0; i < NumVerts; ++i)
	{
		MeshMender::Vertex v;
		v.pos = MeshVert[i].pos;
		v.s = MeshVert[i].s;
		v.t = MeshVert[i].t;
		v.normal = MeshVert[i].norm;
		MendVerts.push_back(v);
	}
	pMesh->UnlockVertexBuffer();

	WORD* MeshIdx;
	pMesh->LockIndexBuffer(0, (LPVOID*)&MeshIdx);

	MendIndices.reserve(NumFaces * 3);
	for(DWORD i = 0; i < NumFaces * 3; ++i)
	{
		MendIndices.push_back(MeshIdx[i]);
	}
	pMesh->UnlockIndexBuffer();

	pMesh->Release();
	pMesh = 0;

	//pass it in to Mend mender to do it's stuff
	mender.Mend(MendVerts, MendIndices, mappingNewToOld, 0.9f, 0.9f, 0.9f, 1.0f, MeshMender::DONT_CALCULATE_NORMALS, MeshMender::RESPECT_SPLITS);
	
	mappingNewToOld.clear();

	printf("%d faces, %d verts\n", MendIndices.size() / 3, MendVerts.size());

	printf("Saving data: ");

	FILE* fp = fopen("meshdata.bin", "wb");
	n = MendIndices.size() / 3;
	fwrite(&n, 4, 1, fp);
	n = MendVerts.size();
	fwrite(&n, 4, 1, fp);
	fclose(fp);

	// Load existing file
	HANDLE hFile = CreateFile(argv[2], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("ERROR: %08x\n", GetLastError());
		goto mesherror;
	}
	DWORD Size = GetFileSize(hFile, 0);
	char* FileData = (char*)VirtualAlloc(0, 64*1024*1024, MEM_RESERVE, PAGE_NOACCESS);
	VirtualAlloc(FileData, Size, MEM_COMMIT, PAGE_READWRITE);
	ReadFile(hFile, FileData, Size, &n, 0);
	FileData[n] = 0;
	Size = n;

	char *p, *q;
	// Find vertex data
	p = strstr(FileData, "VertexBuffer");
	if (!p)
	{
		printf("ERROR: Invalid output file\n");
		goto mesherror;
	}
	p = strchr(p, '{');
	q = p+1;
	int depth = 1;
	do {
		if (*q == '}')
			--depth;
		else if (*q == '{')
			++depth;
		++q;
	} while (depth > 0);

	// move post-vertex data to temp buffer
	Size = (FileData + Size) - q;
	char* TempData = (char*)VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
	memcpy(TempData, q, Size);

	// write vertex data
	strcpy(p, "{\r\n    VertexFormat {D3DVSDT_FLOAT3 D3DVSDT_NORMPACKED3 D3DVSDT_FLOAT2 D3DVSDT_NORMPACKED3 D3DVSDT_NORMPACKED3}\r\n    VertexData\r\n    {\r\n");
	p += strlen(p);

	for (std::vector<MeshMender::Vertex>::iterator i = MendVerts.begin(); i != MendVerts.end(); ++i)
	{
		VirtualAlloc(p, 500, MEM_COMMIT, PAGE_READWRITE);
		p += sprintf(p, "        %12f %12f %12f  %12f %12f %12f  %12f %12f  %12f %12f %12f  %12f %12f %12f\r\n",
			i->pos.x, i->pos.y, i->pos.z, i->normal.x, i->normal.y, i->normal.z,
			i->s, i->t,
			i->tangent.x, i->tangent.y, i->tangent.z, i->binormal.x, i->binormal.y, i->binormal.z);
	}

	strcpy(p, "    }\r\n}");
	p += strlen(p);

	VirtualAlloc(p, Size, MEM_COMMIT, PAGE_READWRITE);
	memcpy(p, TempData, Size);
	Size += p - FileData;
	VirtualFree(TempData, 0, MEM_RELEASE);

	// Find index data
	p = strstr(FileData, "IndexBuffer");
	if (!p)
	{
		printf("ERROR: Invalid output file\n");
		goto mesherror;
	}
	p = strchr(p, '{');
	q = p+1;
	depth = 1;
	do {
		if (*q == '}')
			--depth;
		else if (*q == '{')
			++depth;
		++q;
	} while (depth > 0);

	// move post-index data to temp buffer
	Size = (FileData + Size) - q;
	TempData = (char*)VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
	memcpy(TempData, q, Size);

	// write index data
	strcpy(p, "{\r\n    IndexData\r\n    {\r\n        ");
	p += strlen(p);

	n = 0;
	for (std::vector<unsigned>::iterator i = MendIndices.begin(); i != MendIndices.end(); ++i)
	{
		VirtualAlloc(p, 20, MEM_COMMIT, PAGE_READWRITE);
		p += sprintf(p, " %5hu", *i);
		if (n++ == 2)
		{
			p += sprintf(p, "\r\n       ");
			n = 0;
		}
	}

	strcpy(p-3, "}\r\n}");
	p += strlen(p);

	VirtualAlloc(p, Size, MEM_COMMIT, PAGE_READWRITE);
	memcpy(p, TempData, Size);
	Size += p - FileData;
	VirtualFree(TempData, 0, MEM_RELEASE);

	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	WriteFile(hFile, FileData, Size, &n, 0);
	SetEndOfFile(hFile);

	CloseHandle(hFile);
	VirtualFree(FileData, 0, MEM_RELEASE);

	printf("Done\n");

	pD3D->Release();
	pD3DDevice->Release();

	return 0;

mesherror:
	pD3D->Release();
	pD3DDevice->Release();
	
	return 1;
}

