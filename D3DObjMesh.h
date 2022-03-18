// This header contains stuff to facilitate drawing obj meshes using direct3d.

# ifndef D3D_OBJ_MESH_H
# define D3D_IBJ_MESH_H


#include "ObjLoader.h"
//# include "Win32Util.h"


#ifndef SAFE_DELETE
/// For pointers allocated with new.
#define SAFE_DELETE(p)			{ if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
/// For arrays allocated with new [].
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);   (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
/// For use with COM pointers.
#define SAFE_RELEASE(p)			{ if(p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef MIN
#define MIN(x,y)				( (x)<(y)? (x) : (y) )
#endif

#ifndef MAX
#define MAX(x,y)				( (x)>(y)? (x) : (y) )
#endif



// This structure describes a face vertex in an obj mesh. A face vertex simply contains
// indexes to the actual vertex component data in the obj mesh.
struct ObjTriVertex
{
	INT iPos;
	INT iNormal;
	INT iTex;

	void Init() { iPos = iNormal = iTex = -1; }
};
//---------------------------------------------------------------//
//struct VBVertex
//{
//	DirectX::XMFLOAT3 pos, normal;
//	DirectX::XMFLOAT2 tex;
//};
//---------------------------------------------------------------//
// This structure describes a triangle in the obj mesh. Obj meshes are composed of faces, or polygons.
// Each polygon consists of one or more triangles. Each triangle is made of 3 face vertices.
// See [...] for a description of the difference between a vertex and a face vertex.
struct ObjTriangle
{
	ObjTriVertex vertex[3];
	//INT subsetIndex;

	void Init() { vertex[0].Init(); vertex[1].Init(); vertex[2].Init(); /*subsetIndex = -1;*/ }
};


typedef std::vector< ObjTriangle > ObjTriangleList;

class CD3DMesh
{
public:
	CD3DMesh()
	{
		vertexSize = 0;
		FVF = 0;
		pVB = NULL;
	}

	~CD3DMesh() { SAFE_RELEASE(pVB); }
 
	HRESULT Create(ID3D11Device* pD3DDevice, const TObjMesh& objMesh, BOOL flipTriangles, BOOL flipUVs);

	UINT triCount;
	UINT vertexSize;
	DWORD FVF;
	/*ID3D11Buffer* pIB;*/
	ID3D11Buffer* pVB;
	DirectX::XMFLOAT3 bbmin, bbmax; // bounding box.

	std::map<float, std::map<float, std::map<float, long>>> VertexMap;
 

	ObjTriangleList m_triList;
	std::vector<VBVertex> vMeshVertices;
	std::vector<UINT> vMeshIndices;
	std::vector< DirectX::XMFLOAT3> vertices;
	std::vector< DirectX::XMFLOAT3> normals; 

protected:
	// This method is probably the most important in the sample after the obj loader.
	// This methdo is called by the Create() method to construct the D3D vertex buffer and fill it
	// with the triangles of the obj mesh. Each triangle has its own copy of the vertices, so it's not
	// really very efficient. Instead, in a real world application, the vertex buffer should only contain
	// unique vertices, and an index buffer is neaded to create the triangles. That is, if many face
	// vertices are identical (in ALL their components), only one corresponding vertex needs to be copied
	// to the vertex buffer.
	// This optimization requires sorting and/or searching which can be quite slow for heavy meshes.
	HRESULT InitVB(ID3D11Device* pD3DDevice, const TObjMesh& objMesh, BOOL flipTriangles, BOOL flipUVs);
};



# endif // inclusion guard