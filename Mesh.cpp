#include "pch.h"
#include "framework.h"
#include "Vertex.h"
//-------------------------------------------------------------//
void CMesh::CreateBuffers(ID3D11Device* pD3DDevice, long nNumFaces, BYTE* pVBData0, int* pIndex)
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * nNumFaces * 3;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = pVBData0;

	//bbmin, bbmax; // bounding box.
	HRESULT hr = pD3DDevice->CreateBuffer(&vbd, &vinitData, &pVB);

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * nNumFaces * 3;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = pIndex;
	hr = pD3DDevice->CreateBuffer(&ibd, &iinitData, &pIB);
}
//-------------------------------------------------------------//
void CMesh::Clear()
{
	if (pIB) pIB->Release();
	if (pVB) pVB->Release();
	if (m_pMesh)
	{
		for (int h = 0; h < 8; h++)
		{
			m_pMesh[h].Clear();
		}
		delete[] m_pMesh;
	}
}
//-------------------------------------------------------------//
void CMesh::Count(long& nTris, long& nMaxDepth)
{
	if (m_pMesh)
	{
		nMaxDepth++;
		for (int h = 0; h < 8; h++)
		{
			nTris = nTris + m_pMesh[h].nNumTriangles;
			m_pMesh[h].Count(nTris, nMaxDepth);
		}
	}
	nTris = nTris + nNumTriangles;
}
//-------------------------------------------------------------//
void CMesh::RecomputeBB()
{
	//	return;
	for (int h = 0; h < nNumTriangles; h++)
	{
		if (h == 0) {
			bbmin = bbmax = vTriangles[3 * h + 0].pos;
		}
		if (vTriangles[3 * h + 0].pos.x < bbmin.x) bbmin.x = vTriangles[3 * h + 0].pos.x; else if (vTriangles[3 * h + 0].pos.x > bbmax.x) bbmax.x = vTriangles[3 * h + 0].pos.x;
		if (vTriangles[3 * h + 0].pos.y < bbmin.y) bbmin.y = vTriangles[3 * h + 0].pos.y; else if (vTriangles[3 * h + 0].pos.y > bbmax.y) bbmax.y = vTriangles[3 * h + 0].pos.y;
		if (vTriangles[3 * h + 0].pos.z < bbmin.z) bbmin.z = vTriangles[3 * h + 0].pos.z; else if (vTriangles[3 * h + 0].pos.z > bbmax.z) bbmax.z = vTriangles[3 * h + 0].pos.z;

		if (vTriangles[3 * h + 1].pos.x < bbmin.x) bbmin.x = vTriangles[3 * h + 1].pos.x; else if (vTriangles[3 * h + 1].pos.x > bbmax.x) bbmax.x = vTriangles[3 * h + 1].pos.x;
		if (vTriangles[3 * h + 1].pos.y < bbmin.y) bbmin.y = vTriangles[3 * h + 1].pos.y; else if (vTriangles[3 * h + 1].pos.y > bbmax.y) bbmax.y = vTriangles[3 * h + 1].pos.y;
		if (vTriangles[3 * h + 1].pos.z < bbmin.z) bbmin.z = vTriangles[3 * h + 1].pos.z; else if (vTriangles[3 * h + 1].pos.z > bbmax.z) bbmax.z = vTriangles[3 * h + 1].pos.z;

		if (vTriangles[3 * h + 2].pos.x < bbmin.x) bbmin.x = vTriangles[3 * h + 2].pos.x; else if (vTriangles[3 * h + 2].pos.x > bbmax.x) bbmax.x = vTriangles[3 * h + 2].pos.x;
		if (vTriangles[3 * h + 2].pos.y < bbmin.y) bbmin.y = vTriangles[3 * h + 2].pos.y; else if (vTriangles[3 * h + 2].pos.y > bbmax.y) bbmax.y = vTriangles[3 * h + 2].pos.y;
		if (vTriangles[3 * h + 2].pos.z < bbmin.z) bbmin.z = vTriangles[3 * h + 2].pos.z; else if (vTriangles[3 * h + 2].pos.z > bbmax.z) bbmax.z = vTriangles[3 * h + 2].pos.z;

	}
	boundingbox.Center.x = (bbmin.x + bbmax.x) / 2;
	boundingbox.Center.y = (bbmin.y + bbmax.y) / 2;
	boundingbox.Center.z = (bbmin.z + bbmax.z) / 2;

	boundingbox.Extents.x = (bbmax.x - bbmin.x) / 2;
	boundingbox.Extents.y = (bbmax.y - bbmin.y) / 2;
	boundingbox.Extents.z = (bbmax.z - bbmin.z) / 2;
}
//-------------------------------------------------------------//
void CMesh::ComputeSubmeshes(long& nDepth)
{
	m_pMesh = new CMesh[8];
	float nExtentX = boundingbox.Extents.x / 2;
	float nExtentY = boundingbox.Extents.y / 2;
	float nExtentZ = boundingbox.Extents.z / 2;

	m_pMesh[0].boundingbox.Center.x = boundingbox.Center.x - nExtentX;
	m_pMesh[0].boundingbox.Center.y = boundingbox.Center.y - nExtentY;
	m_pMesh[0].boundingbox.Center.z = boundingbox.Center.z - nExtentZ;

	m_pMesh[1].boundingbox.Center.x = boundingbox.Center.x + nExtentX;
	m_pMesh[1].boundingbox.Center.y = boundingbox.Center.y - nExtentY;
	m_pMesh[1].boundingbox.Center.z = boundingbox.Center.z - nExtentZ;

	m_pMesh[2].boundingbox.Center.x = boundingbox.Center.x - nExtentX;
	m_pMesh[2].boundingbox.Center.y = boundingbox.Center.y + nExtentY;
	m_pMesh[2].boundingbox.Center.z = boundingbox.Center.z - nExtentZ;

	m_pMesh[3].boundingbox.Center.x = boundingbox.Center.x + nExtentX;
	m_pMesh[3].boundingbox.Center.y = boundingbox.Center.y + nExtentY;
	m_pMesh[3].boundingbox.Center.z = boundingbox.Center.z - nExtentZ;

	m_pMesh[4].boundingbox.Center.x = boundingbox.Center.x - nExtentX;
	m_pMesh[4].boundingbox.Center.y = boundingbox.Center.y - nExtentY;
	m_pMesh[4].boundingbox.Center.z = boundingbox.Center.z + nExtentZ;

	m_pMesh[5].boundingbox.Center.x = boundingbox.Center.x + nExtentX;
	m_pMesh[5].boundingbox.Center.y = boundingbox.Center.y - nExtentY;
	m_pMesh[5].boundingbox.Center.z = boundingbox.Center.z + nExtentZ;

	m_pMesh[6].boundingbox.Center.x = boundingbox.Center.x - nExtentX;
	m_pMesh[6].boundingbox.Center.y = boundingbox.Center.y + nExtentY;
	m_pMesh[6].boundingbox.Center.z = boundingbox.Center.z + nExtentZ;

	m_pMesh[7].boundingbox.Center.x = boundingbox.Center.x + nExtentX;
	m_pMesh[7].boundingbox.Center.y = boundingbox.Center.y + nExtentY;
	m_pMesh[7].boundingbox.Center.z = boundingbox.Center.z + nExtentZ;

	for (int h = 0; h < 8; h++)
	{
		m_pMesh[h].boundingbox.Extents.x = nExtentX;
		m_pMesh[h].boundingbox.Extents.y = nExtentY;
		m_pMesh[h].boundingbox.Extents.z = nExtentZ;
	}

	for (int h = 0; h < 8; h++)
	{
		m_pMesh[h].bbmax.x = m_pMesh[h].boundingbox.Center.x + m_pMesh[h].boundingbox.Extents.x;
		m_pMesh[h].bbmax.y = m_pMesh[h].boundingbox.Center.y + m_pMesh[h].boundingbox.Extents.y;
		m_pMesh[h].bbmax.z = m_pMesh[h].boundingbox.Center.z + m_pMesh[h].boundingbox.Extents.z;
		m_pMesh[h].bbmin.x = m_pMesh[h].boundingbox.Center.x - m_pMesh[h].boundingbox.Extents.x;
		m_pMesh[h].bbmin.y = m_pMesh[h].boundingbox.Center.y - m_pMesh[h].boundingbox.Extents.y;
		m_pMesh[h].bbmin.z = m_pMesh[h].boundingbox.Center.z - m_pMesh[h].boundingbox.Extents.z;
	}
	long nTotalChildTris = 0;
	for (int h = 0; h < 8; h++)
	{
		for (int j = 0; j < nNumTriangles; j++)
		{
			m_pMesh[h].AddTriangle(vTriangles[3 * j + 0], vTriangles[3 * j + 1], vTriangles[3 * j + 2]);
		}
		nTotalChildTris = nTotalChildTris + m_pMesh[h].nNumTriangles;
		long nStop = 0;
	}
	for (int h = 0; h < 8; h++)
	{
		m_pMesh[h].RecomputeBB();
	}
	if (nDepth > 4) return;

	for (int h = 0; h < 8; h++)
	{
		if (m_pMesh[h].nNumTriangles >= 200)
		{
			m_pMesh[h].ComputeSubmeshes(nDepth);
		}
	}
}
//-------------------------------------------------------------//
void CMesh::AddTriangle(VBVertex& v1, VBVertex& v2, VBVertex& v3)
{
	DirectX::FXMVECTOR Point1 = XMLoadFloat3(&v1.pos);
	DirectX::FXMVECTOR Point2 = XMLoadFloat3(&v2.pos);
	DirectX::FXMVECTOR Point3 = XMLoadFloat3(&v3.pos);

	if (IntersectPointAxisAlignedBox(Point1, &boundingbox) || IntersectPointAxisAlignedBox(Point2, &boundingbox) || IntersectPointAxisAlignedBox(Point3, &boundingbox))
	{
		vTriangles.push_back(v1);
		vTriangles.push_back(v2);
		vTriangles.push_back(v3);

		vTrianglesEx.push_back(Point1);
		vTrianglesEx.push_back(Point2);
		vTrianglesEx.push_back(Point3);

		nNumTriangles++;
	}
}