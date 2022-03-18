//***************************************************************************************
// Vertex.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Defines vertex structures and input layouts.
//***************************************************************************************

#ifndef VERTEX_H
#define VERTEX_H

#include "d3dUtil.h"

namespace Vertex
{
	// Basic 32-byte vertex structure.
	struct Basic32
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Tex;
	};
	struct PosNormal
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
	};
	struct VertexPos
	{
		DirectX::XMFLOAT3 Pos;
	};
}

class InputLayoutDesc
{
public:
	// Init like const int A::a[4] = {0, 1, 2, 3}; in .cpp file.
	static const D3D11_INPUT_ELEMENT_DESC VertexPos[1];
	static const D3D11_INPUT_ELEMENT_DESC PosNormal[2];
	static const D3D11_INPUT_ELEMENT_DESC Basic32[3];
};

class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11InputLayout* VertexPos;
	static ID3D11InputLayout* PosNormal;
	static ID3D11InputLayout* Basic32;
};

#endif // VERTEX_H
