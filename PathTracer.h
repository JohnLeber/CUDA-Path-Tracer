#pragma once
//#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

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
struct CUDAMaterial
{
	float3* pTexData = 0;
	long nWidth = 0;
	long nHeight = 0;
	float3 diffuse = {0, 0, 0};
	float3* pNmlData = 0;
	long nNmlWidth = 0;
	long nNmlHeight = 0;
};
//-------------------------------------------------------------//
struct CUDAMesh
{
	CUDAMesh* pMesh = 0;
	bool bLight = false;
	CCUDAVertex* pVertices = 0;
	long nNumTriangles = 0;
	CCUDAAxisAlignedBox BB;
	float3 bbmin, bbmax; // bounding box.
	CUDAMaterial* pMaterial = 0;
};
//-------------------------------------------------------------//
class CPTCallback;
//-------------------------------------------------------------//
class CCUDAPathTracer
{ 
	static void CopyMesh(CUDAMesh* pDst, CUDAMesh* pSrc, thrust::host_vector<CCUDAVertex*>& vVertexBuffers, thrust::host_vector<CUDAMesh*>& vMeshBuffers);
	static void FreeMesh(CUDAMesh* pDst);
public: 
	static void CalcRays(CPTCallback* pCallback, float3* pOutputImage, long nClientWidth, long nClientHeight, long nNumSamples, long nDiv, float P0, float P1, float ToLocal[4][4],
		float3 nSunPos, float3 nSunDir, float nSunIntensity, bool bGlobalIllumination, bool bUseTextures, bool bUseNormals, CUDAMesh* pVB, long nNumMeshs, CUDAMaterial* pTextures, long nNumTextures);
};
//-------------------------------------------------------------//
