#pragma once
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include <string>
struct Vec3;
class StaticMesh;
enum FaceWinding
{
	FACE_WINDING_COUNTER_CLOCKWISE,
	FACE_WINDING_CLOCKWISE,
	NUM_FACE_WINDINGS
};
struct MeshInfo
{
	Mat44 m_transform = Mat44::IDENTITY;
	FaceWinding m_faceWinding = FACE_WINDING_COUNTER_CLOCKWISE;
	bool m_indexedMesh = false;

};

struct VertexKey
{
	Vec3 position;
	Vec3 normal;
	Vec2 uv;

	bool operator==(VertexKey const& other) const
	{
		return (position == other.position) &&
			(normal == other.normal) &&
			(uv == other.uv);
	}
};

struct VertexKeyHasher
{
	size_t operator()(VertexKey const& key) const
	{
		size_t hash = 0;

		hash ^= std::hash<float>()(key.position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<float>()(key.position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<float>()(key.position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		hash ^= std::hash<float>()(key.normal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<float>()(key.normal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<float>()(key.normal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		hash ^= std::hash<float>()(key.uv.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<float>()(key.uv.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		return hash;
	}
};


bool LoadFromOBJ(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed = false);
bool LoadFromXML(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed = false);
bool LoadOBJMeshFile(VertTBNs& out_verts, std::string const& meshFolderPath);
bool LoadOBJMeshFile(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed = false);

bool LoadFromOBJ(StaticMesh* out_mesh, std::string const& meshFolderPath, bool indexed = false);
bool LoadFromXML(StaticMesh* out_mesh, std::string const& meshFolderPath, bool indexed = false);
bool LoadOBJMeshFile(StaticMesh* out_staticMesh, std::string const& meshFolderPath, bool indexed = false);

bool ParseOBJMeshTextBuffer(VertTBNs& out_verts, std::vector<unsigned int>& out_indexes, std::string const& objFileContents, MeshInfo meshInfo = MeshInfo(), char const* fileNameForErrorReporting = "DefaultOBJName");
bool GetBasisFromXMLText(Vec3& out_basis, std::string const& text);

