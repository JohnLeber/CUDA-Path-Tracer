#pragma once
//-------------------------------------------------------------//
struct CCUDAAxisAlignedBox
{
	float3 center;            // Center of the box.
	float3 extents;           // Distance from the center to each side.
};
//-------------------------------------------------------------//
struct CCUDAVertex
{
	float3 pos;
	float3 normal;
	float2 tex;
};
//-------------------------------------------------------------//
struct CUDAMesh
{
	/*~CUDAMesh()
	{
		if (pVertices)
		{
			delete[] pVertices;
			pVertices = 0;
		}
		if (pMesh)
		{
			delete[] pMesh;
		}
	}*/
	CUDAMesh* pMesh = 0;
	bool bLight = false;
	CCUDAVertex* pVertices = 0;
	long nNumTriangles = 0;
	CCUDAAxisAlignedBox BB;
	float3 bbmin, bbmax; // bounding box.
};
struct CUDAMesh2
{
	~CUDAMesh2()
	{
		if (pVertices)
		{
			delete[] pVertices;
			pVertices = 0;
		}
		if (pMesh)
		{
			delete[] pMesh;
		}
	}
	CUDAMesh* pMesh = 0;
	bool bLight = false;
	CCUDAVertex* pVertices = 0;
	long nNumTriangles = 0;
	CCUDAAxisAlignedBox BB;
	float3 bbmin, bbmax; // bounding box.
};
class CPTCallback;
//-------------------------------------------------------------//
class CCUDAPathTracer
{ 
	static void CopyMesh(CUDAMesh* pDst, CUDAMesh* pSrc);
	static void FreeMesh(CUDAMesh* pDst);
public: 
	static void CalcRays(CPTCallback* pCallback, float3* pOutputImage, long nClientWidth, long nClientHeight, long nNumSamples, long nDiv, float P0, float P1, float ToLocal[4][4],
		float3 nSunPos, float3 nSunDir, float nSunIntensity, bool bGlobalIllumination, CUDAMesh* pVB, long nNumMeshs);
};
//-------------------------------------------------------------//
