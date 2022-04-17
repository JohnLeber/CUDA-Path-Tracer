
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <Windows.h>//needed to save output as a bitmap file.
#include <stdio.h>
#include "PathTracer.h"
#include "PTProgress.h"
//#include <thrust/random.h>
#include <curand_kernel.h>
//#include <thrust/random.h>
// 
//--------------------------------------------------------------------//
const int MAX_BOUNCES = 3;
#define FLT_MAX          3.402823466e+38F        // max value
#define M_PI 3.14159265f
//--------------------------------------------------------------------//
__device__ float3 Vec3TransformCoord(float3 in, float m[4][4])
{
    float3 out;
    out.x = in.x * m[0][0] + in.y * m[1][0] + in.z * m[2][0] + 1 * m[3][0];
    out.y = in.x * m[0][1] + in.y * m[1][1] + in.z * m[2][1] + 1 * m[3][1];
    out.z = in.x * m[0][2] + in.y * m[1][2] + in.z * m[2][2] + 1 * m[3][2];
    return out;
};
//--------------------------------------------------------------------//
__device__ float3 Vec3Norm(float3 in)
{
    float3 out = in;
    float k = 1.0 / sqrt(in.x * in.x + in.y * in.y + in.z * in.z);
    out.x *= k; out.y *= k; out.z *= k;
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3Subtract(float3 in1, float3 in2)
{
    float3 out;
    out.x = in1.x - in2.x;
    out.y = in1.y - in2.y;
    out.z = in1.z - in2.z;
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3Add(float3 in1, float3 in2)
{
    float3 out;
    out.x = in1.x + in2.x;
    out.y = in1.y + in2.y;
    out.z = in1.z + in2.z;
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3MultScalar(float3 in, float scalar)
{
    float3 out;
    out.x = in.x * scalar;
    out.y = in.y * scalar;
    out.z = in.z * scalar;
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3DivScalar(float3 in, float scalar)
{
    float3 out;
    out.x = in.x / scalar;
    out.y = in.y / scalar;
    out.z = in.z / scalar;
    return out;
}
//--------------------------------------------------------------------//
__device__ float Vec3Dot(float3 v1, float3 v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3Cross(float3 v1, float3 v2)
{
    float3 out;
    out.x =  v1.y * v2.z - v1.z * v2.y;
    out.y = -v1.x * v2.z + v1.z * v2.x;
    out.z =  v1.x * v2.y - v1.y * v2.x;
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Vec3TransformNormal(float3 in, float m[4][4])
{
    in = Vec3Norm(in);
    float3 out;
    out.x = in.x * m[0][0] + in.y * m[1][0] + in.z * m[2][0];
    out.y = in.x * m[0][1] + in.y * m[1][1] + in.z * m[2][1];
    out.z = in.x * m[0][2] + in.y * m[1][2] + in.z * m[2][2];
    in = Vec3Norm(in);
    return out;
};
//--------------------------------------------------------------------//
__device__ bool IntersectRayAxisAlignedBox(float3& rayOrigin, float3& rayDir, float3& bbmax, float3& bbmin)
{
    //largely copied from the link below
    //https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/18459#18459
     
    // float t = 0;//distance
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
       // t = tmax;
        return false;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax)
    {
       // t = tmax;
        return false;
    }

  //  t = tmin;
    return true;
}
//-----------------------------------------------------------------------// 
__device__ void CreateCoordinateSystem(float3& N, float3& Nt, float3& Nb)
{
    //https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
    if (abs(N.x) > abs(N.y)) {
        float3 t = { N.z, 0, -N.x };
        Nt = Vec3DivScalar(t, sqrt(N.x * N.x + N.z * N.z));
    }
    else {
        float3 t = { 0, -N.z, N.y };
        Nt = Vec3DivScalar(t, sqrt(N.y * N.y + N.z * N.z));
    }
    Nb = Vec3Cross(N, Nt);
}
//--------------------------------------------------------------------//
__device__ bool IntersectRayTriangle(float3 rayOrigin, float3 rayDir, float3 v0, float3 v1, float3 v2, float* pDist, float* pU, float* pV)
{
    //https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
    //Largely based on the XNA  implemention (see CPU version)
    float kEpsilon = 0.00001f;
    float3 e1 = Vec3Subtract(v1, v0);
    float3 e2 = Vec3Subtract(v2, v0);
    float3 p = Vec3Cross(rayDir, e2);
    float det = Vec3Dot(e1, p);
    if (det >= kEpsilon) {

        float3 s = Vec3Subtract(rayOrigin, v0);
        *pU = Vec3Dot(s, p);
        if (*pU < 0.0 || *pU > det) return false;

        float3 q = Vec3Cross(s, e1);
        *pV = Vec3Dot(rayDir, q);
        if (*pV < 0.0 || *pU + *pV > det) return false;
        *pDist = Vec3Dot(e2, q);
        if (*pDist < 0) return false;
    }
    else if (det <= -kEpsilon) {

        float3 s = Vec3Subtract(rayOrigin, v0);
        *pU = Vec3Dot(s, p);
        if (*pU > 0.0 || *pU < det) return false;

        float3 q = Vec3Cross(s, e1);
        *pV = Vec3Dot(rayDir, q);
        if (*pV > 0.0 || *pU + *pV < det) return false;
        *pDist = Vec3Dot(e2, q);
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
//--------------------------------------------------------------------//
__device__ bool Intersect(CUDAMesh& mesh, float3& rayOrigin, float3& rayDir, float& u, float& v, float& dist, float3& hitpoint, float3& nml)
{
    if (!IntersectRayAxisAlignedBox(rayOrigin, rayDir, mesh.bbmax, mesh.bbmin))
    {
        return false;
    }
  
    if (mesh.pMesh)
    {
        bool bHit = false;
        for (int h = 0; h < 8; h++) { 
            if (Intersect(mesh.pMesh[h], rayOrigin, rayDir, u, v, dist, hitpoint, nml)) {
                //return true;
                bHit = true;
            }
        }
        return bHit;
    }
    bool bHit = false;
    for (int k = 0; k < mesh.nNumTriangles; k++)
    {
        float t = 0.0f;
        FLOAT ua = 0;
        FLOAT va = 0;
        // We have to iterate over all the triangles in order to find the nearest intersection.
        float3 v1 = mesh.pVertices[3 * k + 0].pos;
        float3 v2 = mesh.pVertices[3 * k + 1].pos;
        float3 v3 = mesh.pVertices[3 * k + 2].pos;
        if (IntersectRayTriangle(rayOrigin, rayDir, v1, v2, v3, &t, &ua, &va))
        {
            if (t < dist) {
                dist = t;
                u = (1 - ua - va) * mesh.pVertices[3 * k + 0].tex.x + ua * mesh.pVertices[3 * k + 1].tex.x + va * mesh.pVertices[3 * k + 2].tex.x;
                v = (1 - ua - va) * mesh.pVertices[3 * k + 0].tex.y + ua * mesh.pVertices[3 * k + 1].tex.y + va * mesh.pVertices[3 * k + 2].tex.y;

                hitpoint.x = (1 - ua - va) * mesh.pVertices[3 * k + 0].pos.x + ua * mesh.pVertices[3 * k + 1].pos.x + va * mesh.pVertices[3 * k + 2].pos.x;
                hitpoint.y = (1 - ua - va) * mesh.pVertices[3 * k + 0].pos.y + ua * mesh.pVertices[3 * k + 1].pos.y + va * mesh.pVertices[3 * k + 2].pos.y;
                hitpoint.z = (1 - ua - va) * mesh.pVertices[3 * k + 0].pos.z + ua * mesh.pVertices[3 * k + 1].pos.z + va * mesh.pVertices[3 * k + 2].pos.z;

                nml.x = (mesh.pVertices[3 * k + 0].normal.x + mesh.pVertices[3 * k + 1].normal.x + mesh.pVertices[3 * k + 2].normal.x) / 3;
                nml.y = (mesh.pVertices[3 * k + 0].normal.y + mesh.pVertices[3 * k + 1].normal.y + mesh.pVertices[3 * k + 2].normal.y) / 3;
                nml.z = (mesh.pVertices[3 * k + 0].normal.z + mesh.pVertices[3 * k + 1].normal.z + mesh.pVertices[3 * k + 2].normal.z) / 3;

                nml = Vec3Norm(nml);
                bHit = true;
            }
        }
    } 
    return bHit;
}
//--------------------------------------------------------------------//
__device__ bool TraceRays(CUDAMesh* pMesh, long nNumMeshs, float3& rayOrigin, float3& rayDir, bool bHitOnly, bool bUseTextures, float3& hitpoint, float3& nml, float3& rgb, float& nHitDist)
{
    float tmin = FLT_MAX;
    float u = 0;
    float v = 0;
    long nTexWidth = 0;
    long nTexHeight = 0;
    float3* pTexData = 0;
    float3 diffuse = {0, 0, 0};
    bool bHit = false;
    float dist = FLT_MAX;
    for (int h = 0; h < nNumMeshs; h++)
    {
        if (pMesh[h].bLight) {
            continue;
        }
        float ua = 0;
        float va = 0;  
        if (!Intersect(pMesh[h], rayOrigin, rayDir, ua, va, dist, hitpoint, nml)) {
            continue;
        }
        bHit = true;
        u = ua;
        v = va;
        nTexWidth = 0;
        nTexHeight = 0;
        pTexData = 0;
        if (pMesh[h].pMaterial)
        {
            nTexWidth = pMesh[h].pMaterial->nWidth;
            nTexHeight = pMesh[h].pMaterial->nHeight;
            pTexData = pMesh[h].pMaterial->pTexData;
            diffuse = pMesh[h].pMaterial->diffuse;
        }
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
        {
            //gray/clay model
            rgb.x = 0.5f;
            rgb.y = 0.5f;
            rgb.z = 0.5f;
        }
        else
        {
            //use the textures
            rgb.x = 0.5f;
            rgb.y = 0.5f;
            rgb.z = 0.5f;
            if (nTexWidth > 0 && pTexData) { 
                long nIndex = h * nTexWidth + j;
                if (nIndex >= 0 && nIndex < nTexWidth * nTexHeight)
                {
                    rgb.x = (float)(pTexData[nIndex].x) / 255.0f;
                    rgb.y = (float)(pTexData[nIndex].y) / 255.0f;
                    rgb.z = (float)(pTexData[nIndex].z) / 255.0f;
                }
            }
            else
            {
                rgb = diffuse;
            }
        }
    }
    return bHit;
}
//-----------------------------------------------------------------------// 
__device__ float3 uniformSampleHemisphere(const float& r1, const float& r2)
{ 
    //https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
    float sinTheta = sqrtf(1 - r1 * r1);
    float phi = 2 * M_PI * r2;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    float3 out = { x, r1, z };
    return out;
}
//--------------------------------------------------------------------//
__device__ float3 Radiance(long nNumSamples, curandState& s, CUDAMesh* pVB, long nNumMeshs, float3& rayOrigin, float3& rayDir, const int& depth, unsigned short* Xi, 
    bool bGlobalIllumination, float3 nSunPos, float3 nSunDir, float nSunIntensity, bool bUseTextures)
{ 
    float3 rgb = { 0, 0, 0 };
    float3 nml = { 0, 0, 0 };
    float3 hitpoint = { 0, 0, 0 };
    if (depth > MAX_BOUNCES) return rgb;
    bool bHit = false;
    float tmin = FLT_MAX;
    float dist = FLT_MAX;
    float nHitDist = 0;
    float3 directLighting = { 0, 0, 0 };
    float3 indirectLighting = { 0, 0, 0 };
    bHit = TraceRays(pVB, nNumMeshs, rayOrigin, rayDir, false, bUseTextures, hitpoint, nml, rgb, nHitDist);

    if (bHit)
    {
        //assume diffuse material
		//Direct light - cast shadow ray towards sun
        float3 hitPoint = hitpoint;
        float3 hitNml = nml;
        float3 sunPos = nSunPos;
        float3 sunDir = Vec3Subtract(sunPos, hitPoint);// sunPos - hitPoint;
        sunDir = Vec3Norm(sunDir);

        float bias = 0.01f;
        float3 hitsunnml = { 0, 0, 0 };
        float3 sunhitpoint = { 0, 0, 0 };
        float3 hitrgb = { 0, 0, 0 };
        float3 rorigin = Vec3Add(hitPoint, Vec3MultScalar(hitNml, bias));
        bHit = TraceRays(pVB, nNumMeshs, rorigin, sunDir, true, bUseTextures, sunhitpoint, hitsunnml, hitrgb, nHitDist);
        if (!bHit)
        { 
            float dp = Vec3Dot(hitNml, sunDir);
            directLighting.x = nSunIntensity * max(0.0f, dp);
            directLighting.y = nSunIntensity * max(0.0f, dp);
            directLighting.z = nSunIntensity * max(0.0f, dp);
        }
        if (bGlobalIllumination)
        {
            //https://www.scratchapixel.com/code.php?id=34&origin=/lessons/3d-basic-rendering/global-illumination-path-tracing
            int N = nNumSamples;
            float3 Nt = { 0, 0, 0 };
            float3 Nb = { 0, 0, 0 }; 
            CreateCoordinateSystem(hitNml, Nt, Nb);
            float pdf = 1 / (2 * M_PI);
            for (int n = 0; n < N; ++n) {
                float r1 = curand_uniform(&s);
                float r2 = curand_uniform(&s);
                float3 sample = uniformSampleHemisphere(r1, r2);
                float3 sampleWorld = 
                {
                    sample.x * Nb.x + sample.y * hitNml.x + sample.z * Nt.x,
                    sample.x * Nb.y + sample.y * hitNml.y + sample.z * Nt.y,
                    sample.x * Nb.z + sample.y * hitNml.z + sample.z * Nt.z 
                };
                float3 ray = Vec3Add(hitPoint, Vec3MultScalar(sampleWorld, bias));
                //set number of samples to 1 for 2nd, 4rd... bounces
                float3 rd = Radiance(1, s, pVB, nNumMeshs, ray, sampleWorld,
                    depth + 1, Xi, bGlobalIllumination, nSunPos, nSunDir, nSunIntensity, bUseTextures);
                indirectLighting.x = indirectLighting.x + r1 * rd.x / pdf;
                indirectLighting.y = indirectLighting.y + r1 * rd.y / pdf;
                indirectLighting.z = indirectLighting.z + r1 * rd.z / pdf; 
            }
            indirectLighting = Vec3DivScalar(indirectLighting, N);
        }
        //check - not sure if I am meant to multiply indirectLighting by sunintensity - but the images look better...
        rgb.x = (directLighting.x + nSunIntensity * indirectLighting.x) * rgb.x / M_PI;
        rgb.y = (directLighting.y + nSunIntensity * indirectLighting.y) * rgb.y / M_PI;
        rgb.z = (directLighting.z + nSunIntensity * indirectLighting.z) * rgb.z / M_PI;
    } 
    rgb.x = min(rgb.x, 1.0f);
    rgb.y = min(rgb.y, 1.0f);
    rgb.z = min(rgb.z, 1.0f);
    return rgb;
}
//--------------------------------------------------------------------//
void CCUDAPathTracer::CopyMesh(CUDAMesh* pDst, CUDAMesh* pSrc, thrust::host_vector<CCUDAVertex*>& vVertexBuffers, thrust::host_vector<CUDAMesh*>& vMeshBuffers)
{ 
    cudaError_t cudaStatus;
    if (pSrc->pMesh)
    {
        CUDAMesh* pChildMesh = 0;
        cudaError_t cudaStatus = cudaMalloc((void**)&pChildMesh, 8 * sizeof(CUDAMesh));//allocate memory on device for child meshes
        if (cudaStatus == cudaSuccess) {
            vMeshBuffers.push_back(pChildMesh);
            for (int h = 0; h < 8; h++)
            {
                CopyMesh(&pChildMesh[h], &(pSrc->pMesh[h]), vVertexBuffers, vMeshBuffers);
            }
            cudaStatus = cudaMemcpy(&(pDst->pMesh), &(pChildMesh), sizeof(CUDAMesh*), cudaMemcpyHostToDevice);//cleanup pointer
        }
    }
    
    CCUDAVertex* pVert = 0;
    if (pSrc->nNumTriangles > 0)
    {
        cudaStatus = cudaMalloc((void**)&pVert, 3 * pSrc->nNumTriangles * sizeof(CCUDAVertex));//allocate memory for vertices on device
        if (cudaStatus == cudaSuccess) {
            vVertexBuffers.push_back(pVert);
            cudaStatus = cudaMemcpy(pVert, pSrc->pVertices, 3 * pSrc->nNumTriangles * sizeof(CCUDAVertex), cudaMemcpyHostToDevice);//copy array of vertices to device
        }
    }
    cudaStatus = cudaMemcpy(&(pDst->pVertices), &pVert, sizeof(CCUDAVertex*), cudaMemcpyHostToDevice);//cleanup pointer (which may be zero if no triangles);

    cudaStatus = cudaMemcpy(&(pDst->bbmin), &(pSrc->bbmin), sizeof(float3), cudaMemcpyHostToDevice);
    cudaStatus = cudaMemcpy(&(pDst->bbmax), &(pSrc->bbmax), sizeof(float3), cudaMemcpyHostToDevice);
    cudaStatus = cudaMemcpy(&(pDst->nNumTriangles), &(pSrc->nNumTriangles), sizeof(long), cudaMemcpyHostToDevice);
    cudaStatus = cudaMemcpy(&(pDst->bLight), &(pSrc->bLight), sizeof(bool), cudaMemcpyHostToDevice);
    cudaStatus = cudaMemcpy(&(pDst->BB), &(pSrc->BB), sizeof(CCUDAAxisAlignedBox), cudaMemcpyHostToDevice);
}
//--------------------------------------------------------------------//
__global__ void PTKernel(volatile int* progress, float3* pOutout, long nClientWidth, long nClientHeight, long nNumSamples, long nDiv, float P0, float P1, float* pToLocal,
    float3 nSunPos, float3 nSunDir, float nSunIntensity, bool bGlobalIllumination, bool bUseTextures, CUDAMesh* pVB, long nNumMeshs)
{
    int threadsPerBlock = blockDim.x * blockDim.y * blockDim.z;
    int threadPosInBlock = threadIdx.x +
        blockDim.x * threadIdx.y +
        blockDim.x * blockDim.y * threadIdx.z;
    int blockPosInGrid = blockIdx.x +
        gridDim.x * blockIdx.y +
        gridDim.x * gridDim.y * blockIdx.z;
    int tid = blockPosInGrid * threadsPerBlock + threadPosInBlock;//calculate global index to array
    long nImageWidth = nClientWidth / nDiv;
    long nImageHeight = nClientHeight / nDiv;
    if (tid < nImageWidth * nImageHeight)
    {
        curandState s;
        // seed a random number generator
        curand_init(tid * tid, 0, 0, &s);
        int j = tid % nImageWidth;//width
        int h = tid / nImageWidth;//height

        float vx = (+2.0f * (float)nDiv * j / (float)nClientWidth - 1.0f) / P0;// P(0, 0);
        float vy = (-2.0f * (float)nDiv * h / (float)nClientHeight + 1.0f) / P1;// P(1, 1);

        float3 rayOrigin = { 0.0f, 0.0f, 0.0f };
        float3 rayDir = { vx, vy, 1.0f };

        float ToLocal[4][4];
        for (int w = 0; w < 4; w++)
        {
            for (int q = 0; q < 4; q++)
            {
                ToLocal[w][q] = pToLocal[w * 4 + q];
            }
        }
        rayDir = Vec3Norm(rayDir);
        rayOrigin = Vec3TransformCoord(rayOrigin, ToLocal);
        rayDir = Vec3TransformNormal(rayDir, ToLocal);
        rayDir = Vec3Norm(rayDir);
        unsigned short Xi[3] = { 0, 0, (short)j * (short)j * (short)j }; // *** Moved outside for VS2012
        int depth = 0;
        float3 rgb = Radiance(nNumSamples, s, pVB, nNumMeshs, rayOrigin, rayDir, depth, Xi, bGlobalIllumination, nSunPos, nSunDir, nSunIntensity, bUseTextures);

        atomicAdd((int*)progress, 1);//increment the progress count that will be propagated back to the progress bar in the UI

        if (rgb.x > 0.1)
        {
            long nStop = 0;
        }
        pOutout[tid].x = 255 * rgb.x;
        pOutout[tid].y = 255 * rgb.y;
        pOutout[tid].z = 255 * rgb.z;
    }
}
//--------------------------------------------------------------------//
void CCUDAPathTracer::CalcRays(CPTCallback* pCallback, float3* pOutputImage, long nClientWidth, long nClientHeight, long nNumSamples, long nDiv, float P0, float P1, float ToLocal[4][4],
                           float3 nSunPos, float3 nSunDir, float nSunIntensity, bool bGlobalIllumination, bool bUseTextures, CUDAMesh* pVB, long nNumMeshs, CUDAMaterial* pMaterials, long nNumMaterials)
{
    long nWidth = nClientWidth / nDiv;
    long nHeight = nClientHeight / nDiv;
    thrust::host_vector<CCUDAVertex*> vVertexBuffers;
    thrust::host_vector<CUDAMesh*> vMeshBuffers;
    thrust::host_vector<float3*> vTextures;

    thrust::host_vector<CUDAMaterial*> vHostMaterials;
    thrust::host_vector<CUDAMaterial*> vDeviceMaterials;
  
   // short* devHGTData = 0;
    float3* pCUDAOutputImage = 0;
    float* pToLocal = 0;
    cudaError_t cudaStatus;    
    CUDAMesh* pCUDAVB = 0;
    CUDAMaterial* pCUDAMaterials = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }
  
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Allocate GPU buffers for output image
    cudaStatus = cudaMalloc((void**)&pCUDAOutputImage, nWidth * nHeight * sizeof(float3));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }
      
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Copy materials/textures
    cudaStatus = cudaMalloc((void**)&pCUDAMaterials, nNumMaterials * sizeof(CUDAMaterial));
    if (cudaStatus == cudaSuccess) {
        for (int h = 0; h < nNumMaterials; h++) {
            vDeviceMaterials.push_back(&pCUDAMaterials[h]);//store the host and device version of this structure so we can map them later
            vHostMaterials.push_back(&pMaterials[h]);
            float3* pTexData = 0;
            cudaStatus = cudaMalloc((void**)&pTexData, pMaterials[h].nWidth * pMaterials[h].nHeight * sizeof(float3));
            if (cudaStatus != cudaSuccess) {
                fprintf(stderr, "cudaMalloc failed!");
                goto Error;
            }
            
            cudaStatus = cudaMemcpy(&pCUDAMaterials[h].nWidth, &pMaterials[h].nWidth, sizeof(long), cudaMemcpyHostToDevice);
            cudaStatus = cudaMemcpy(&pCUDAMaterials[h].nHeight, &pMaterials[h].nHeight, sizeof(long), cudaMemcpyHostToDevice);
            cudaStatus = cudaMemcpy(pTexData, pMaterials[h].pTexData, pMaterials[h].nWidth * pMaterials[h].nHeight * sizeof(float3), cudaMemcpyHostToDevice);
            cudaStatus = cudaMemcpy(&pCUDAMaterials[h].diffuse, &pMaterials[h].diffuse, sizeof(float3), cudaMemcpyHostToDevice);
            cudaStatus = cudaMemcpy(&(pCUDAMaterials[h].pTexData), &pTexData, sizeof(float3*), cudaMemcpyHostToDevice);
            //cudaStatus = cudaMemcpy(&(pDst->pMesh), &(pChildMesh), sizeof(CUDAMesh*), cudaMemcpyHostToDevice);//cleanup pointer
       
            vTextures.push_back(pTexData);//store in a vector so we can delete them later
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Allocate GPU buffers for 4 by 4 matrix
    cudaStatus = cudaMalloc((void**)&pToLocal, 16 * sizeof(float));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }
    {
        //copy 4 by 4 matrix to device
        float* pTemp = new float[16];
        for (int w = 0; w < 4; w++) {
            for (int q = 0; q < 4; q++) {
                pTemp[w * 4 + q] = ToLocal[w][q];
            }
        }
        cudaError_t cudaStatus = cudaMemcpy(pToLocal, pTemp, 16 * sizeof(float), cudaMemcpyHostToDevice);
        delete[] pTemp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Increase stack size otherwise the recursion will generate a 719 error
    cudaStatus = cudaThreadSetLimit(cudaLimitStackSize, 12000);
    // Allocate GPU buffers for mesh array
    cudaStatus = cudaMalloc((void**)&pCUDAVB, nNumMeshs * sizeof(CUDAMesh));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Copy meshes
    cudaStatus = cudaMemset(pCUDAVB, 0, nNumMeshs * sizeof(CUDAMesh));
    for (int h = 0; h < nNumMeshs; h++)
    {        
        CopyMesh(&pCUDAVB[h], &pVB[h], vVertexBuffers, vMeshBuffers);
        //replace pointer to host material to pointer to the same material but on the device (that we computed a moment ago)
        if (vHostMaterials.size() != vDeviceMaterials.size()) {
            long nStop = 0;
        }
        for (int j = 0; j < vHostMaterials.size(); j++)
        {
            CUDAMaterial* pTest = vHostMaterials[j];
            if (pVB[h].pMaterial == pTest)
            {
                cudaStatus = cudaMemcpy(&pCUDAVB[h].pMaterial, &vDeviceMaterials[j], sizeof(CUDAMaterial*), cudaMemcpyHostToDevice);
                break;
            }            
        }
    }
     
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Used to send progress from CUDA device back to host
    volatile int* d_progress, * h_progress;    
    cudaStatus = cudaSetDeviceFlags(cudaDeviceMapHost);
    cudaStatus = cudaHostAlloc((void**)&h_progress, sizeof(int), cudaHostAllocMapped);
    cudaStatus = cudaHostGetDevicePointer((int**)&d_progress, (int*)h_progress, 0);
    *h_progress = 0;
    *d_progress = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Launch a kernel on the GPU with one thread for each element.
    cudaStatus = cudaDeviceSynchronize();

    long tx = 8;
    long ty = 8;
    dim3 block(nWidth / tx + 1, nHeight / ty + 1);
    dim3 threads(tx, ty);

    cudaEvent_t start, stop;
    cudaEventCreate(&start); 
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //The Kernel
    PTKernel << <block, threads >> > (d_progress, pCUDAOutputImage, nClientWidth, nClientHeight, nNumSamples, nDiv, P0, P1, pToLocal, nSunPos, nSunDir, nSunIntensity, bGlobalIllumination, bUseTextures, pCUDAVB, nNumMeshs);

    cudaEventRecord(stop);
  
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "HGTToNormalKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
        goto Error;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //send progress back to caller
    if (pCallback)
    {
        pCallback->UpdateProgress(0, nWidth * nHeight);
        //https://stackoverflow.com/questions/20345702/how-can-i-check-the-progress-of-matrix-multiplication
        int nProgress = 0;
        do {
            cudaEventQuery(stop);
            int nProgress1 = (*h_progress) * 100 / nWidth / nHeight;
            if (nProgress1 > nProgress) 
            {
                nProgress = nProgress1;
                pCallback->UpdateProgress( *h_progress, nWidth * nHeight);
            }
        } while (*h_progress < nWidth * nHeight - 2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching HGTToNormalKernel!\n", cudaStatus);
        goto Error;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy output image from GPU buffer to host memory.
    cudaStatus = cudaMemcpy(pOutputImage, pCUDAOutputImage, nWidth * nHeight * sizeof(float3), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }

Error:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Free Memory
    cudaFree(pCUDAOutputImage);
    for(auto& i : vVertexBuffers)
    {
        cudaFree(i);
    }
    for (auto& i : vMeshBuffers)
    {
        cudaFree(i);
    }
    for (auto& i : vTextures)
    {
        cudaFree(i);
    }
    cudaFree(pCUDAMaterials);
    cudaFree(pCUDAVB);
    cudaFree(pToLocal);
}
//--------------------------------------------------------------------//
