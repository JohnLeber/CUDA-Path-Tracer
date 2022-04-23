#pragma once
class CPTCallback;
class CCPUPathTracer
{
	bool Trace(bool bUseTextures, const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir, bool bHitOnly,
		DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nmlMesh, DirectX::XMFLOAT3& nml, 
		DirectX::XMFLOAT3& rgb, float& nHitDist);
	DirectX::XMFLOAT3 Radiance(CLight& sun, bool bGlobalIllumination, long nNumSamples, bool bUseTextures, bool bUseNormals, DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, const int& depth, unsigned short* Xi);
	void CreateCoordinateSystem(const DirectX::XMVECTOR& N, DirectX::XMFLOAT3& Nt, DirectX::XMFLOAT3& Nb);
	DirectX::XMFLOAT3 uniformSampleHemisphere(const float& r1, const float& r2);
	bool RayTriangleIntersect(
		const DirectX::XMFLOAT3& v0, DirectX::XMFLOAT3& v1, DirectX::XMFLOAT3& v2,
		const DirectX::XMFLOAT3& orig, const DirectX::XMFLOAT3& dir,
		float& tnear, float& u, float& v);
public:
 
	void CalcRays(CPTCallback* pCallback, DWORD* pOutputImage, long nClientWidth, long nClientHeight, long nDiv, long nNumSamples, float P0, float P1, DirectX::XMMATRIX ToLocal,
		CLight sun, bool bGlobalIllumination, bool bUseTextures, bool bUseNormals);
};
