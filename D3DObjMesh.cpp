
#include "D3DObjMesh.h"
#include <assert.h>
#include "framework.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

// Adds an OBJ face to the specified triangle list. If the face is not triangular, it is
// triangulated by taking, for the nth vertex, where n starts from 2, the triangle that
// consists of the first vertex, the nth vertex, and the (n-1)th vertex. This triangulation
// method is fast but may yield weird overlapping triangles.

VOID AddObjFace(ObjTriangleList& objTriangleList, const TObjMesh& objMesh,
	UINT objFaceIndex, BOOL bFlipTriangles, BOOL bFlipUVs)
{
	const TObjMesh::TFace& objFace = objMesh.faces[objFaceIndex];
	UINT triCount = objFace.vCount - 2;

	for (INT fv = 2; fv < objFace.vCount; fv++)
	{
		ObjTriangle tri;
		tri.Init();

		tri.vertex[0].iPos = objMesh.faceVertices[objFace.firstVertex];
		tri.vertex[1].iPos = objMesh.faceVertices[objFace.firstVertex + fv - 1];
		tri.vertex[2].iPos = objMesh.faceVertices[objFace.firstVertex + fv];
# ifdef DEBUG
		if (tri.vertex[0].iPos >= objMesh.vertices.size()) DebugBreak();
		if (tri.vertex[1].iPos >= objMesh.vertices.size()) DebugBreak();
		if (tri.vertex[2].iPos >= objMesh.vertices.size()) DebugBreak();
# endif


		if (!objMesh.normals.empty() && objFace.firstNormal >= 0)
		{
			tri.vertex[0].iNormal = objMesh.faceNormals[objFace.firstNormal];
			tri.vertex[1].iNormal = objMesh.faceNormals[objFace.firstNormal + fv - 1];
			tri.vertex[2].iNormal = objMesh.faceNormals[objFace.firstNormal + fv];
# ifdef DEBUG
			if (tri.vertex[0].iNormal >= objMesh.normals.size()) DebugBreak();
			if (tri.vertex[1].iNormal >= objMesh.normals.size()) DebugBreak();
			if (tri.vertex[2].iNormal >= objMesh.normals.size()) DebugBreak();
# endif
		}

		if (!objMesh.texCoords.empty() && objFace.firstTexCoord >= 0)
		{
			tri.vertex[0].iTex = objMesh.faceTexCoords[objFace.firstTexCoord];
			tri.vertex[1].iTex = objMesh.faceTexCoords[objFace.firstTexCoord + fv - 1];
			tri.vertex[2].iTex = objMesh.faceTexCoords[objFace.firstTexCoord + fv];
		}

		objTriangleList.push_back(tri);
	}
}
 
HRESULT CD3DMesh::Create(ID3D11Device* pD3DDevice, const TObjMesh& objMesh, BOOL bFlipTriangles, BOOL bFlipUVs)
{
	if (objMesh.vertices.empty() || objMesh.numTriangles == 0)
	{
		OutputDebugString(TEXT(__FUNCTION__)TEXT(": obj mesh is invalid!"));
		return E_FAIL;
	}

	// Get bounding box info.
	bbmin = objMesh.bbmin;
	bbmax = objMesh.bbmax;

	return InitVB(pD3DDevice, objMesh, bFlipTriangles, bFlipUVs);
}

// IMPORTANT: See the comment above the method's declaration in the header file.
HRESULT CD3DMesh::InitVB(ID3D11Device* pD3DDevice, const TObjMesh& objMesh, BOOL bFlipTriangles, BOOL bFlipUVs)
{
	HRESULT hr;
	SAFE_RELEASE(pVB);

	vertexSize = sizeof(DirectX::XMFLOAT3); // Has at least positional data.
	FVF = D3DFVF_XYZ;

	BOOL hasNormals = TRUE;// We'll compute them when needed. !objMesh.normals.empty();
	BOOL hasTexCoords = !objMesh.texCoords.empty();

	if (hasNormals) { vertexSize += sizeof(DirectX::XMFLOAT3); FVF |= D3DFVF_NORMAL; }
	if (hasTexCoords) { vertexSize += sizeof(DirectX::XMFLOAT3); FVF |= (D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0)); }

	m_triList.reserve(objMesh.numTriangles);

	for (UINT i = 0; i < objMesh.faces.size(); i++)
		AddObjFace(m_triList, objMesh, i, FALSE, FALSE);

	triCount = m_triList.size();


	UINT bufferSize = triCount * vertexSize * 3;

	/*hr = pD3DDevice->CreateVertexBuffer( bufferSize, D3DUSAGE_WRITEONLY, FVF, D3DPOOL_DEFAULT, &pVB, NULL );
	if( FAILED( hr ) ) return hr;*/

	BYTE* pVBData = new BYTE[bufferSize];
	BYTE* pVBOrignalData = pVBData;

	UINT vertexOrder[] = { 0, 1, 2 };
	if (bFlipTriangles)
	{
		vertexOrder[1] = 2; vertexOrder[2] = 1;
	}
	//std::vector<VBVertex> vMeshVertices;
	//std::vector<UINT> vMeshIndices;
	vMeshIndices.reserve(triCount * 3);
	UINT nIndex = 0;

	//i->vb.z * (1.0f / 0.33f) + 1000);

	for (UINT i = 0; i < m_triList.size(); i++)
	{
		ObjTriangle& tri = m_triList[i];

		// Compute the triangle's normal if the obj mesh does not have normals info.
		DirectX::XMFLOAT3 triNormal;
		/*	if( tri.vertex[0].iNormal < 0 )
			{
				D3DXVECTOR3 vec1 = objMesh.vertices[ tri.vertex[2].iPos ] - objMesh.vertices[ tri.vertex[0].iPos ];
				D3DXVECTOR3 vec2 = objMesh.vertices[ tri.vertex[2].iPos ] - objMesh.vertices[ tri.vertex[1].iPos ];
				if( bFlipTriangles )
					D3DXVec3Cross( &triNormal, &vec2, &vec1 );
				else
					D3DXVec3Cross( &triNormal, &vec1, &vec2 );
				D3DXVec3Normalize( &triNormal, &triNormal );
			}*/

		vertices = objMesh.vertices;
		normals = objMesh.normals;
		for (UINT tv = 0; tv < 3; tv++)
		{
			UINT v = vertexOrder[tv];

			VBVertex* pVBVertex = (VBVertex*)pVBData;
			pVBVertex->pos = objMesh.vertices[tri.vertex[v].iPos];
			if (tri.vertex[v].iNormal < 0) {
				pVBVertex->normal = triNormal;
			}
			else {
				pVBVertex->normal = objMesh.normals[tri.vertex[v].iNormal];
			}



			vMeshVertices.push_back(*pVBVertex);
			vMeshIndices.push_back(nIndex++);

			/*	if (hasTexCoords && tri.vertex[v].iTex >= 0)
				{
					if (bFlipUVs)
					{
						DirectX::XMFLOAT2 tex;
						tex.x = objMesh.texCoords[tri.vertex[v].iTex].x;
						tex.y = objMesh.texCoords[tri.vertex[v].iTex].y;
						tex.y = 1 - tex.y;
						pVBVertex->tex = tex;
					}
					else
					{
						pVBVertex->tex.x = objMesh.texCoords[tri.vertex[v].iTex].x;
						pVBVertex->tex.y = objMesh.texCoords[tri.vertex[v].iTex].y;
					}
				}*/

			pVBData += vertexSize;
		}
	}
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = bufferSize;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = pVBOrignalData;

	hr = pD3DDevice->CreateBuffer(&vbd, &vinitData, &pVB);


	delete[] pVBOrignalData;
	pVBData = 0;
	if (FAILED(hr)) return hr;
	return S_OK;
}