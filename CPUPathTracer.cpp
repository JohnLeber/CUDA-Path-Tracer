#include "pch.h"
#include "framework.h"
#include "Vertex.h"
#include "Effects.h" 
#include "PTProgress.h"
#include "CPUPathTracer.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <omp.h> 

//#define FLT_MAX          3.402823466e+38F        // max value
#define M_PI 3.14159265f
//-----------------------------------------------------------------------//
bool CCPUPathTracer::RayTriangleIntersect(
	const DirectX::XMFLOAT3& v0, DirectX::XMFLOAT3& v1, DirectX::XMFLOAT3& v2,
	const DirectX::XMFLOAT3& orig, const DirectX::XMFLOAT3& dir,
	float& tnear, float& u, float& v)
{
	DirectX::XMFLOAT3 edge1 = { 0,0,0 };
	DirectX::XMFLOAT3 edge2 = { 0,0,0 };
	DirectX::XMFLOAT3 pvec = { 0,0,0 }; //= crossProduct(dir, edge2);
	DirectX::XMFLOAT3 det2 = { 0,0,0 };
	DirectX::XMFLOAT3 ut = { 0,0,0 };
	DirectX::XMFLOAT3 vt = { 0,0,0 };
	DirectX::XMFLOAT3 tnear2 = { 0,0,0 };
	float det = 0;
	XMStoreFloat3(&edge1, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v0))));
	XMStoreFloat3(&edge2, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&v2), XMLoadFloat3(&v0))));
	XMStoreFloat3(&pvec, DirectX::XMVector3Cross(XMLoadFloat3(&dir), XMLoadFloat3(&edge2)));
	XMStoreFloat3(&det2, DirectX::XMVector3Dot(XMLoadFloat3(&edge1), XMLoadFloat3(&pvec)));
	//float det = dotProduct(edge1, pvec);
	det = det2.x;
	if (det == 0 || det < 0) return false;

	DirectX::XMFLOAT3 tvec = { 0, 0, 0 };// orig - v0;
	XMStoreFloat3(&tvec, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&orig), XMLoadFloat3(&v0))));
	//u = dotProduct(tvec, pvec);

	XMStoreFloat3(&ut, DirectX::XMVector3Dot(XMLoadFloat3(&tvec), XMLoadFloat3(&pvec)));
	u = ut.x;
	if (u < 0 || u > det) return false;

	DirectX::XMFLOAT3 qvec = { 0,0,0 };// crossProduct(tvec, edge1);
	XMStoreFloat3(&qvec, DirectX::XMVector3Cross(XMLoadFloat3(&tvec), XMLoadFloat3(&edge1)));
	//v = dotProduct(dir, qvec);
	XMStoreFloat3(&vt, DirectX::XMVector3Dot(XMLoadFloat3(&dir), XMLoadFloat3(&qvec)));
	v = vt.x;
	if (v < 0 || u + v > det) return false;

	float invDet = 1 / det;

	//tnear = dotProduct(edge2, qvec) * invDet;
	XMStoreFloat3(&tnear2, DirectX::XMVector3Dot(XMLoadFloat3(&edge2), XMLoadFloat3(&qvec)));
	tnear = tnear2.x * invDet;

	u *= invDet;
	v *= invDet;

	return true;
}
//--------------------------------------------------------------------//
bool CMesh::DEBUGIntersectRayAxisAlignedBox(DirectX::XMVECTOR& rayOrigin2, DirectX::XMVECTOR& rayDir2, DirectX::XMFLOAT3& bbmax, DirectX::XMFLOAT3& bbmin)
{
	rayDir2 = XMVector3Normalize(rayDir2);

	float nScale = 1;
	DirectX::XMFLOAT3 f1;
	DirectX::XMFLOAT3 f2;
	DirectX::XMStoreFloat3(&f1, rayOrigin2);
	DirectX::XMStoreFloat3(&f2, rayDir2);
	float3 rayOrigin = { f1.x, f1.y, f1.z };
	float3 rayDir = { f2.x * nScale, f2.y * nScale, f2.z * nScale };

	//https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/18459#18459
	float t = 0;
	float3 dirfrac;
	dirfrac.x = 1.0f / rayDir.x;
	dirfrac.y = 1.0f / rayDir.y;
	dirfrac.z = 1.0f / rayDir.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (bbmin.x - rayOrigin.x) * dirfrac.x;
	float t2 = (bbmax.x - rayOrigin.x) * dirfrac.x;
	float t3 = (bbmin.y - rayOrigin.y) * dirfrac.y;
	float t4 = (bbmax.y - rayOrigin.y) * dirfrac.y;
	float t5 = (bbmin.z - rayOrigin.z) * dirfrac.z;
	float t6 = (bbmax.z - rayOrigin.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t = tmax;
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t = tmax;
		return false;
	}

	t = tmin;
	return true;
}
//--------------------------------------------------------------------//
float3 DEBUGVec3Subtract(float3& in1, float3& in2)
{
	float3 out;
	out.x = in1.x - in2.x;
	out.y = in1.y - in2.y;
	out.z = in1.z - in2.z;
	return out;
}
//--------------------------------------------------------------------//
float DEBUGVec3Dot(float3& v1, float3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
//--------------------------------------------------------------------//
float3 DEBUGVec3Cross(float3& v1, float3& v2)
{
	float3 out;
	out.x = v1.y * v2.z - v1.z * v2.y;
	out.y = -v1.x * v2.z + v1.z * v2.x;
	out.z = v1.x * v2.y - v1.y * v2.x;
	return out;
}
//--------------------------------------------------------------------//
bool CMesh::DEBUGIntersectRayTriangle(DirectX::XMVECTOR rayOrigin2, DirectX::XMVECTOR rayDir2, DirectX::XMVECTOR v0B, DirectX::XMVECTOR v1B, DirectX::XMVECTOR v2B, float* pDist, float* pU, float* pV)
{
	DirectX::XMFLOAT3 f1;
	DirectX::XMFLOAT3 f2;
	DirectX::XMStoreFloat3(&f1, rayOrigin2);
	DirectX::XMStoreFloat3(&f2, rayDir2);

	DirectX::XMFLOAT3 vv0;
	DirectX::XMFLOAT3 vv1;
	DirectX::XMFLOAT3 vv2;
	DirectX::XMStoreFloat3(&vv0, v0B);
	DirectX::XMStoreFloat3(&vv1, v1B);
	DirectX::XMStoreFloat3(&vv2, v2B);
	float3 rayOrigin = { f1.x, f1.y, f1.z };
	float3 rayDir = { f2.x, f2.y, f2.z };
	float3 v0 = { vv0.x, vv0.y, vv0.z };
	float3 v1 = { vv1.x, vv1.y, vv1.z };
	float3 v2 = { vv2.x, vv2.y, vv2.z };

	//https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
	float kEpsilon = 0.00001f;
	float3 e1 = DEBUGVec3Subtract(v1, v0);
	float3 e2 = DEBUGVec3Subtract(v2, v0);
	float3 p = DEBUGVec3Cross(rayDir, e2);
	float det = DEBUGVec3Dot(e1, p);

	if (det >= kEpsilon) {

		float3 s = DEBUGVec3Subtract(rayOrigin, v0);

		*pU = DEBUGVec3Dot(s, p);
		if (*pU < 0.0 || *pU > det) return false;

		float3 q = DEBUGVec3Cross(s, e1);
		*pV = DEBUGVec3Dot(rayDir, q);
		if (*pV < 0.0 || *pU + *pV > det) return false;
		*pDist = DEBUGVec3Dot(e2, q);
		if (*pDist < 0) return false;
	}
	else if (det <= -kEpsilon) {
		float3 s = DEBUGVec3Subtract(rayOrigin, v0);

		*pU = DEBUGVec3Dot(s, p);
		if (*pU > 0.0 || *pU < det) return false;

		float3 q = DEBUGVec3Cross(s, e1);
		*pV = DEBUGVec3Dot(rayDir, q);
		if (*pV > 0.0 || *pU + *pV < det) return false;
		*pDist = DEBUGVec3Dot(e2, q);
		if (*pDist > 0) return false;
	}
	else
	{
		return false;
	}
	float invDet = 1.0f / det;
	*pU = *pU * invDet;
	*pV = *pV * invDet;
	*pDist = *pDist * invDet;
	return true;
}
//-----------------------------------------------------------------------//
bool CMesh::Intersect(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, float& u, float& v, float& dist, DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nml)
{
	float dist2 = 0;
	float3 rayOrigin2 = {};

	/*if (!DEBUGIntersectRayAxisAlignedBox(rayOrigin, rayDir, bbmax, bbmin))
	{
		return false;
	}*/

	if (!XNA::IntersectRayAxisAlignedBox(rayOrigin, rayDir, &boundingbox, &dist2))
	{
		return false;
	}
	if (m_pMesh)
	{
		bool bHit = false;
		for (int h = 0; h < 8; h++) {
			if (m_pMesh[h].Intersect(rayOrigin, rayDir, u, v, dist, hitpoint, nml)) {
				//return true;
				bHit = true;
			}
		}
		return bHit;
	}
 
	bool bHit = false;
	for (int k = 0; k < nNumTriangles; k++)
	{
		float t = 0.0f;
		FLOAT ua = 0;
		FLOAT va = 0;
		// We have to iterate over all the triangles in order to find the nearest intersection.
		DirectX::XMVECTOR v1 = XMLoadFloat3(&vTriangles[3 * k + 0].pos);
		DirectX::XMVECTOR v2 = XMLoadFloat3(&vTriangles[3 * k + 1].pos);
		DirectX::XMVECTOR v3 = XMLoadFloat3(&vTriangles[3 * k + 2].pos);
		if (DEBUGIntersectRayTriangle(rayOrigin, rayDir, v1, v2, v3, &t, &ua, &va))
			//if (XNA::IntersectRayTriangle(rayOrigin, rayDir, v1, v2, v3, &t, &ua, &va))
		{
			if (t < dist) {
				dist = t;
				u = (1 - ua - va) * vTriangles[3 * k + 0].tex.x + ua * vTriangles[3 * k + 1].tex.x + va * vTriangles[3 * k + 2].tex.x;
				v = (1 - ua - va) * vTriangles[3 * k + 0].tex.y + ua * vTriangles[3 * k + 1].tex.y + va * vTriangles[3 * k + 2].tex.y;

				hitpoint.x = (1 - ua - va) * vTriangles[3 * k + 0].pos.x + ua * vTriangles[3 * k + 1].pos.x + va * vTriangles[3 * k + 2].pos.x;
				hitpoint.y = (1 - ua - va) * vTriangles[3 * k + 0].pos.y + ua * vTriangles[3 * k + 1].pos.y + va * vTriangles[3 * k + 2].pos.y;
				hitpoint.z = (1 - ua - va) * vTriangles[3 * k + 0].pos.z + ua * vTriangles[3 * k + 1].pos.z + va * vTriangles[3 * k + 2].pos.z;

				nml.x = (vTriangles[3 * k + 0].normal.x + vTriangles[3 * k + 1].normal.x + vTriangles[3 * k + 2].normal.x) / 3;
				nml.y = (vTriangles[3 * k + 0].normal.y + vTriangles[3 * k + 1].normal.y + vTriangles[3 * k + 2].normal.y) / 3;
				nml.z = (vTriangles[3 * k + 0].normal.z + vTriangles[3 * k + 1].normal.z + vTriangles[3 * k + 2].normal.z) / 3;
				DirectX::XMStoreFloat3(&nml, XMVector3Normalize(XMLoadFloat3(&nml)));
				bHit = true;
			}
		}
	}
	return bHit;
}
//-----------------------------------------------------------------------//
bool CCPUPathTracer::Trace(bool bUseTextures, DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, bool bHitOnly, DirectX::XMFLOAT3& hitpoint, DirectX::XMFLOAT3& nml, DirectX::XMFLOAT3& rgb, float& nHitDist)
{
	float tmin = MathHelper::Infinity;
	float u = 0;
	float v = 0;
	long nTexWidth = 0;
	long nTexHeight = 0;
	DWORD* pTexData = 0;
	bool bHit = false;
	float dist = MathHelper::Infinity;
	for (auto& m : gGlobalData->m_vMeshes)
	{
		if (m.bLight) continue;
		float ua = 0;
		float va = 0;
		if (!m.Intersect(rayOrigin, rayDir, ua, va, dist, hitpoint, nml)) {
			continue;
		}
		bHit = true;
		u = ua;
		v = va;
		//meshhit = m;
		nTexWidth = m.nWidth;
		nTexHeight = m.nHeight;
		pTexData = m.m_pTexData;
	}
	if (bHit)
	{
		if (bHitOnly) return true;

		//Get surface properties (texture and uv...).
		/*if (v >= 1) { v = fmod(v, 1); }
		if (u >= 1) { u = fmod(u, 1); }*/
		while (u > 1) { u = u - 1; }
		while (v > 1) { v = v - 1; }
		while (u < 0) { u = u + 1; }
		while (v < 0) { v = v + 1; }
		long j = nTexWidth * v;
		long h = nTexHeight * u;
		if (!bUseTextures)
		{//gray/clay model
			rgb.x = 0.5f;
			rgb.y = 0.5f;
			rgb.z = 0.5f;
		}
		else
		{//use the textures
			if (nTexWidth > 0) {
				DWORD dwPixel = pTexData[j * nTexWidth + h];
				rgb.x = (float)(LOBYTE(LOWORD(dwPixel))) / 255.0f;
				rgb.y = (float)(HIBYTE(LOWORD(dwPixel))) / 255.0f;
				rgb.z = (float)(LOBYTE(HIWORD(dwPixel))) / 255.0f;
			}
		}
	}

	return bHit;
}
std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);
//-----------------------------------------------------------------------//
DirectX::XMFLOAT3 CCPUPathTracer::Radiance(CLight& sun, bool bGlobalIllumination, long nNumSamples, bool bUseTextures, DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir, const int& depth, unsigned short* Xi)
{
	//CMesh meshhit;
	DirectX::XMFLOAT3 rgb = { 0, 0, 0 };
	DirectX::XMFLOAT3 nml = { 0, 0, 0 };
	DirectX::XMFLOAT3 hitpoint = { 0, 0, 0 };
	if (depth > 2) return rgb;
	bool bHit = false;
	float tmin = MathHelper::Infinity;
	float dist = MathHelper::Infinity;
	float nHitDist = 0;
	DirectX::XMFLOAT3 directLighting = { 0, 0, 0 };
	DirectX::XMFLOAT3 indirectLighting = { 0, 0, 0 };

	bHit = Trace(bUseTextures, rayOrigin, rayDir, false, hitpoint, nml, rgb, nHitDist);
	if (bHit)
	{
		//assume diffuse material
		//Direct light - cast shadow ray towards sun
		DirectX::XMVECTOR hitPoint = XMLoadFloat3(&hitpoint);
		DirectX::XMVECTOR hitNml = XMLoadFloat3(&nml);
		DirectX::XMVECTOR sunPos = XMLoadFloat3(&sun.pos);
		DirectX::XMVECTOR sunDir = sunPos - hitPoint;
		sunDir = XMVector3Normalize(sunDir);

		float bias = 0.01f;
		DirectX::XMFLOAT3 hitsunnml = { 0, 0, 0 };
		DirectX::XMFLOAT3 sunhitpoint = { 0, 0, 0 };
		DirectX::XMFLOAT3 hitrgb = { 0, 0, 0 };
		bHit = Trace(bUseTextures, hitPoint + bias * hitNml, sunDir, true, sunhitpoint, hitsunnml, hitrgb, nHitDist);
		if (!bHit)
		{
			DirectX::XMFLOAT3 dp = { 0,0,0 };
			//the sun is a distance light, so lets not divide by sqrt(r*r) and 4/pi/r2 etc
			XMStoreFloat3(&dp, DirectX::XMVector3Dot(hitNml, sunDir));
			directLighting.x = sun.nIntensity * max(0.0f, dp.x);
			directLighting.y = sun.nIntensity * max(0.0f, dp.y);
			directLighting.z = sun.nIntensity * max(0.0f, dp.z);

		}
		//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing&src=0
		if (bGlobalIllumination)
		{
			uint32_t N = nNumSamples;
			DirectX::XMFLOAT3 Nt;
			DirectX::XMFLOAT3 Nb;
			DirectX::XMFLOAT3 hitNormal = { 0, 0, 0 };
			XMStoreFloat3(&hitNormal, hitNml);
			CreateCoordinateSystem(hitNml, Nt, Nb);

			float pdf = 1 / (2 * M_PI);
			for (uint32_t n = 0; n < N; ++n) {
				float r1 = distribution(generator);
				float r2 = distribution(generator);
				DirectX::XMFLOAT3 sample = uniformSampleHemisphere(r1, r2);
				DirectX::XMFLOAT3 sampleWorldF(
					sample.x * Nb.x + sample.y * hitNormal.x + sample.z * Nt.x,
					sample.x * Nb.y + sample.y * hitNormal.y + sample.z * Nt.y,
					sample.x * Nb.z + sample.y * hitNormal.z + sample.z * Nt.z);
				DirectX::XMVECTOR sampleWorld = DirectX::XMLoadFloat3(&sampleWorldF);
				DirectX::XMFLOAT3 rd = Radiance(sun, bGlobalIllumination, nNumSamples, bUseTextures, hitPoint + bias * sampleWorld, sampleWorld, depth + 1, Xi);
				indirectLighting.x = indirectLighting.x + r1 * rd.x / pdf;
				indirectLighting.y = indirectLighting.y + r1 * rd.y / pdf;
				indirectLighting.z = indirectLighting.z + r1 * rd.z / pdf;
				if (indirectLighting.x > 0.001)
				{
					long nStop = 0;
				}
				//indirectLighting += r1 * castRay(hitPoint + sampleWorld * options.bias,
			//		sampleWorld, objects, lights, options, depth + 1) / pdf;
			}
			indirectLighting.x = indirectLighting.x / N;
			indirectLighting.y = indirectLighting.y / N;
			indirectLighting.z = indirectLighting.z / N;
		}
		rgb.x = (directLighting.x / M_PI + 2 * indirectLighting.x) * rgb.x;// *isect.hitObject->albedo;
		rgb.y = (directLighting.y / M_PI + 2 * indirectLighting.y) * rgb.y;// *isect.hitObject->albedo;
		rgb.z = (directLighting.z / M_PI + 2 * indirectLighting.z) * rgb.z;// *isect.hitObject->albedo;
	}
	return rgb;
}
//-----------------------------------------------------------------------//
DirectX::XMFLOAT3 CCPUPathTracer::uniformSampleHemisphere(const float& r1, const float& r2)
{
	//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
	float sinTheta = sqrtf(1 - r1 * r1);
	float phi = 2 * M_PI * r2;
	float x = sinTheta * cosf(phi);
	float z = sinTheta * sinf(phi);
	return DirectX::XMFLOAT3(x, r1, z);
}
//-----------------------------------------------------------------------//
void CCPUPathTracer::CreateCoordinateSystem(const DirectX::XMVECTOR& N2, DirectX::XMFLOAT3& NtF, DirectX::XMFLOAT3& Nb)
{
	//https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
	DirectX::XMFLOAT3 N = { 0,0,0 };
	DirectX::XMVECTOR Nt;
	XMStoreFloat3(&N, N2);
	if (std::fabs(N.x) > std::fabs(N.y)) {
		Nt = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(N.z, 0, -N.x)) / _hypot(N.x, N.z);// (N.x * N.x + N.z * N.z);
	}
	else {
		Nt = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(0, -N.z, N.y)) / _hypot(N.y, N.z);//sqrtf(N.y * N.y + N.z * N.z);
	}
	XMStoreFloat3(&NtF, Nt);
	XMStoreFloat3(&Nb, DirectX::XMVector3Cross(XMLoadFloat3(&N), Nt));
}
//-----------------------------------------------------------------------//
void CCPUPathTracer::CalcRays(CPTCallback* pCallback, DWORD* pImageData, long nClientWidth, long nClientHeight, long nDiv, long nNumSamples, float P0, float P1, DirectX::XMMATRIX toLocal,
								CLight sun, bool bGlobalIllumination, bool bUseTextures)
{ 
	long nImageWidth = nClientWidth / nDiv;
	long nImageHeight = nClientHeight / nDiv;
	long nProgress = 0;//used to update progressbar

omp_set_num_threads(16);
#pragma omp parallel for
	for (int j = 0; j < nImageWidth; ++j) {
		unsigned short Xi[3] = { 0 ,0, j * j * j }; // *** Moved outside for VS2012
		for (int h = 0; h < nImageHeight; ++h) {
			DWORD dwPixel = MAKELONG(MAKEWORD(0, 0), MAKEWORD(0, 255));
			// generate primary ray direction
			float vx = (+2.0f * (float)nDiv * j / (float)nClientWidth - 1.0f) / P0;
			float vy = (-2.0f * (float)nDiv * h / (float)nClientHeight + 1.0f) / P1;

			//Ray definition in view space.
			DirectX::XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			DirectX::XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);
			rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
			rayDir = XMVector3TransformNormal(rayDir, toLocal);
			rayDir = XMVector3Normalize(rayDir);
			//transform to ray origin to local space should equal the camera position (eyepos).

			DirectX::XMFLOAT3 dir;
			DirectX::XMFLOAT3 origin;
			DirectX::XMStoreFloat3(&dir, rayDir);
			DirectX::XMStoreFloat3(&origin, rayOrigin);

			int depth = 0;
			DirectX::XMFLOAT3 rgb = Radiance(sun, bGlobalIllumination, nNumSamples, bUseTextures, rayOrigin, rayDir, depth, Xi);

			if (pCallback) pCallback->UpdateProgress(nProgress++, nImageWidth * nImageHeight);
			pImageData[h * nImageWidth + j] = MAKELONG(MAKEWORD(rgb.x * 255, rgb.y * 255), MAKEWORD(rgb.z * 255, 255));

			/*if (h == 0 && j == 0)
			{
				//render debug ray
				m_rayDir = rayDir;
				m_rayStart = rayOrigin;
				DirectX::XMFLOAT3 dir;
				DirectX::XMFLOAT3 origin;
				DirectX::XMStoreFloat3(&dir, rayDir);
				DirectX::XMStoreFloat3(&origin, rayOrigin);
				DirectX::XMFLOAT3 pos = m_Camera.GetPosition();
				r[0].x = origin.x;
				r[0].y = origin.y;
				r[0].z = origin.z;
				r[1].x = origin.x + dir.x * 2000;
				r[1].y = origin.y + dir.y * 2000;
				r[1].z = origin.z + dir.z * 2000;
			}*/
		}
	}

}