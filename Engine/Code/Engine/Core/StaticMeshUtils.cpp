#include "Engine/Core/StaticMeshUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>
#include <unordered_map>

bool LoadFromOBJ(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed)
{
	std::string objFileContents;
	Strings filePathStrings = SplitStringOnDelimiter(meshFolderPath, '/');
	if(filePathStrings.size() <= 0)
		return false; //invalid file path

	std::string folder = Stringf("/%s", filePathStrings[filePathStrings.size() - 1].c_str());

	std::string filePath = Stringf("%s%s.obj", meshFolderPath.c_str(), folder.c_str());
	int success = FileReadToString(objFileContents, filePath, true);
	if(success == 0)
		return false; //failed to read string

	MeshInfo meshInfo {Mat44::IDENTITY, FACE_WINDING_COUNTER_CLOCKWISE, indexed};
	return ParseOBJMeshTextBuffer(out_verts, out_indexes, objFileContents, meshInfo, filePath.c_str());
}

bool LoadFromXML(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed)
{
	//Organize File Path
	Strings filePathStrings = SplitStringOnDelimiter(meshFolderPath, '/');
	if (filePathStrings.size() <= 0)
		return false;

	std::string folder = Stringf("/%s", filePathStrings[filePathStrings.size() - 1].c_str());
	std::string xmlPath = Stringf("%s%s.xml", meshFolderPath.c_str(), folder.c_str());

	//Get XML Element
	XmlDocument objXML;
	XmlResult result = objXML.LoadFile(xmlPath.c_str());
	if(result != tinyxml2::XML_SUCCESS)
		return false;

	XmlElement* rootElement = objXML.RootElement();
	if(!rootElement)
		return false; //could not get xml root element

	std::string objFileContents;
	std::string filePath = ParseXmlAttribute(*rootElement, "objFile", "ERROR");
	if(filePath == "ERROR")
		return false; //Could not find .obj

	int success = FileReadToString(objFileContents, filePath, true);
	if(success == 0)
		return false;

	//Get basis transform info
	std::string iBasisText = ParseXmlAttribute(*rootElement, "x", "");
	Vec3 iBasis = Vec3::FORWARD;
	GetBasisFromXMLText(iBasis, iBasisText);

	std::string jBasisText = ParseXmlAttribute(*rootElement, "y", "");
	Vec3 jBasis = Vec3::LEFT;
	GetBasisFromXMLText(jBasis, jBasisText);

	std::string kBasisText = ParseXmlAttribute(*rootElement, "z", "");
	Vec3 kBasis = Vec3::UP;
	GetBasisFromXMLText(kBasis, kBasisText);

	float uniformScale = 1.f / ParseXmlAttribute(*rootElement, "unitsPerMeter", 1.f);
	Vec3 translation = ParseXmlAttribute(*rootElement, "translation", Vec3::ZERO);

	MeshInfo meshInfo;
	meshInfo.m_transform = Mat44(iBasis, jBasis, kBasis, translation);
	meshInfo.m_transform.AppendScaleUniform3D(uniformScale);

	std::string faceWindingText = ParseXmlAttribute(*rootElement, "triangleWinding", "CounterClockwise");
	meshInfo.m_faceWinding = faceWindingText == "CounterClockwise" ? FACE_WINDING_COUNTER_CLOCKWISE : FACE_WINDING_CLOCKWISE;
	meshInfo.m_indexedMesh = indexed;

	return ParseOBJMeshTextBuffer(out_verts, out_indexes, objFileContents, meshInfo, xmlPath.c_str());
}

bool LoadOBJMeshFile(VertTBNs& out_verts, std::string const& meshFolderPath)
{
	IndexList indexList;
	if (LoadFromXML(out_verts, indexList, meshFolderPath))
	{
		return true;
	}

	indexList.clear();
	if (LoadFromOBJ(out_verts, indexList, meshFolderPath))
	{
		return true;
	}

	return false;
}

bool LoadOBJMeshFile(VertTBNs& out_verts, IndexList& out_indexes, std::string const& meshFolderPath, bool indexed)
{
	if (LoadFromXML(out_verts, out_indexes, meshFolderPath, indexed))
	{
		return true;
	}

	out_indexes.clear();
	if (LoadFromOBJ(out_verts, out_indexes, meshFolderPath, indexed))
	{
		return true;
	}

	return false;
}


bool LoadFromOBJ(StaticMesh* out_mesh, std::string const& meshFolderPath, bool indexed)
{
	std::string objFileContents;
	Strings filePathStrings = SplitStringOnDelimiter(meshFolderPath, '/');
	if (filePathStrings.size() <= 0)
		return false; //invalid file path

	std::string meshName = filePathStrings[filePathStrings.size() - 1];
	std::string folder = Stringf("/%s", meshName.c_str());

	std::string filePath = Stringf("%s%s.obj", meshFolderPath.c_str(), folder.c_str());
	int success = FileReadToString(objFileContents, filePath, true);
	if (success == 0)
		return false; //failed to read string

	MeshInfo meshInfo{ Mat44::IDENTITY, FACE_WINDING_COUNTER_CLOCKWISE, indexed };
	std::vector<unsigned int> indexes;
	VertTBNs meshVerts;
	if (!ParseOBJMeshTextBuffer(meshVerts, indexes, objFileContents, meshInfo, filePath.c_str()))
		return false;

	out_mesh->SetBuffers(meshVerts, indexes);
	out_mesh->SetStaticMeshNameAndFilePath(meshName, meshFolderPath);
	return true;
}

bool LoadFromXML(StaticMesh* out_mesh, std::string const& meshFolderPath, bool indexed)
{
	//Organize File Path
	Strings filePathStrings = SplitStringOnDelimiter(meshFolderPath, '/');
	if (filePathStrings.size() <= 0)
		return false;

	std::string meshName = filePathStrings[filePathStrings.size() - 1];
	std::string folder = Stringf("/%s", meshName.c_str());
	std::string xmlPath = Stringf("%s%s.xml", meshFolderPath.c_str(), folder.c_str());

	//Get XML Element
	XmlDocument objXML;
	XmlResult result = objXML.LoadFile(xmlPath.c_str());
	if (result != tinyxml2::XML_SUCCESS)
		return false; //could not find xml file

	XmlElement* rootElement = objXML.RootElement();
	if (!rootElement)
		return false; //could not get xml root element

	std::string objFileContents;
	std::string filePath = ParseXmlAttribute(*rootElement, "objFile", "ERROR");
	if (filePath == "ERROR")
		return false; //Could not find .obj

	int success = FileReadToString(objFileContents, filePath, true);
	if (success == 0)
		return false;


	//Get basis transform info
	std::string iBasisText = ParseXmlAttribute(*rootElement, "x", "");
	Vec3 iBasis = Vec3::FORWARD;
	GetBasisFromXMLText(iBasis, iBasisText);

	std::string jBasisText = ParseXmlAttribute(*rootElement, "y", "");
	Vec3 jBasis = Vec3::LEFT;
	GetBasisFromXMLText(jBasis, jBasisText);

	std::string kBasisText = ParseXmlAttribute(*rootElement, "z", "");
	Vec3 kBasis = Vec3::UP;
	GetBasisFromXMLText(kBasis, kBasisText);

	float uniformScale = 1.f / ParseXmlAttribute(*rootElement, "unitsPerMeter", 1.f);
	Vec3 translation = ParseXmlAttribute(*rootElement, "translation", Vec3::ZERO);

	MeshInfo meshInfo;
	meshInfo.m_transform = Mat44(iBasis, jBasis, kBasis, translation);
	meshInfo.m_transform.AppendScaleUniform3D(uniformScale);

	std::string faceWindingText = ParseXmlAttribute(*rootElement, "triangleWinding", "CounterClockwise");
	meshInfo.m_faceWinding = faceWindingText == "CounterClockwise" ? FACE_WINDING_COUNTER_CLOCKWISE : FACE_WINDING_CLOCKWISE;
	meshInfo.m_indexedMesh = indexed;

	VertTBNs meshVerts;
	std::vector<unsigned int> indexes;
	if (!ParseOBJMeshTextBuffer(meshVerts, indexes, objFileContents, meshInfo, xmlPath.c_str()))
		return false;

	//Find needed texture file paths
	Strings defaultTexturesAttributeNames = { "INVALID_TEXTURE" };
	Strings includedTextureAttributeNames = ParseXmlAttribute(*rootElement, "textureMaps", defaultTexturesAttributeNames, ',');
	Strings textureFilePaths;
	for (int i = 0; i < (int)includedTextureAttributeNames.size(); ++i)
	{
		std::string textureFilePath = ParseXmlAttribute(*rootElement, includedTextureAttributeNames[i].c_str(), "INVALID_TEXTURE");
		if (textureFilePath == "")
		{
			textureFilePath = "INVALID_TEXTURE";
		}
		textureFilePaths.push_back(textureFilePath);
	}

	//Find shader file path
	std::string shaderFilePath = ParseXmlAttribute(*rootElement, "shader", "INVALID_SHADER");
	if (shaderFilePath == "")
	{
		shaderFilePath = "INVALID_SHADER";
	}

	out_mesh->SetBuffers(meshVerts, indexes);
	out_mesh->SetTextures(textureFilePaths);
	out_mesh->SetShader(shaderFilePath);
	out_mesh->SetStaticMeshNameAndFilePath(meshName, meshFolderPath);

	return true;
}


bool LoadOBJMeshFile(StaticMesh* out_staticMesh, std::string const& meshFolderPath, bool indexed)
{
	if (LoadFromXML(out_staticMesh, meshFolderPath, indexed))
	{
		return true;
	}

	if (LoadFromOBJ(out_staticMesh, meshFolderPath, indexed))
	{
		return true;
	}

	return false;
}

bool ParseOBJMeshTextBuffer(VertTBNs& out_verts, std::vector<unsigned int>& out_indexes, std::string const& objFileContents, MeshInfo meshInfo, char const* fileNameForErrorReporting)
{
	const int START_INDEX = (int)out_verts.size();

	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<Vec2> uvs;
	std::vector<Strings> faces;

	Strings lineArgs = SplitStringOnDelimiter(objFileContents, '\n', true);
	const int NUM_LINES = (int)lineArgs.size();

	out_verts.reserve(NUM_LINES / 2);
	positions.reserve(NUM_LINES / 4);
	normals.reserve(NUM_LINES / 4);
	uvs.reserve(NUM_LINES / 4);
	faces.reserve(NUM_LINES / 2);

	//======================== PARSE ========================
	for (int i = 0; i < NUM_LINES; ++i)
	{
		std::string currentLineArg = lineArgs[i];
		if (currentLineArg.size() > 0 && currentLineArg[currentLineArg.size() - 1] == '\r')
		{
			currentLineArg.pop_back();
		}

		Strings lineData = SplitStringOnDelimiter(currentLineArg, ' ', true);
		if (lineData.size() == 0)
			continue;

		std::string argumentType = lineData[0];

		if (argumentType == "v" && lineData.size() >= 4)
		{
			positions.push_back(Vec3(GetStringAsFloat(lineData[1]), GetStringAsFloat(lineData[2]), GetStringAsFloat(lineData[3])));
		}
		else if (argumentType == "vn" && lineData.size() >= 4)
		{
			normals.push_back(Vec3(GetStringAsFloat(lineData[1]), GetStringAsFloat(lineData[2]), GetStringAsFloat(lineData[3])));
		}
		else if (argumentType == "vt" && lineData.size() >= 3)
		{
			uvs.push_back(Vec2(GetStringAsFloat(lineData[1]), GetStringAsFloat(lineData[2])));
		}
		else if (argumentType == "f")
		{
			Strings faceArgs;
			for (int n = 1; n < (int)lineData.size(); ++n)
			{
				faceArgs.push_back(lineData[n]);
			}
			faces.push_back(faceArgs);
		}
	}

	if (positions.empty() || faces.empty())
		return false;

	//======================== DEDUP ========================
	std::unordered_map<VertexKey, unsigned int, VertexKeyHasher> vertexLookup;
	vertexLookup.reserve(NUM_LINES);

	const bool useIndexing = meshInfo.m_indexedMesh;

	//======================== BUILD ========================
	for (int faceIndex = 0; faceIndex < (int)faces.size(); ++faceIndex)
	{
		Strings const& faceArgs = faces[faceIndex];

		std::vector<Vertex_PCUTBN> polygonVerts;
		polygonVerts.reserve(faceArgs.size());

		//----- Build raw verts (NO tangent yet)
		for (int faceVertIndex = 0; faceVertIndex < (int)faceArgs.size(); ++faceVertIndex)
		{
			Vertex_PCUTBN vert;
			vert.m_color = Rgba8::WHITE;
			vert.m_uvTexCoords = Vec2(-1, -1);

			Strings vertexArgs = SplitStringOnDelimiter(faceArgs[faceVertIndex], '/', true);

			int posIndex = (int)GetStringAsInt(vertexArgs[0]) - 1;
			if (posIndex < 0 || posIndex >= (int)positions.size())
			{
				DebuggerPrintf(Stringf("%s: Invalid position index", fileNameForErrorReporting).c_str());
				continue;
			}

			vert.m_position = positions[posIndex];

			if (vertexArgs.size() >= 2)
			{
				int uvIndex = (int)GetStringAsInt(vertexArgs[1]) - 1;
				if (uvIndex >= 0 && uvIndex < (int)uvs.size())
				{
					vert.m_uvTexCoords = uvs[uvIndex];
				}
			}

			if (vertexArgs.size() >= 3)
			{
				int normalIndex = (int)GetStringAsInt(vertexArgs[2]) - 1;
				if (normalIndex >= 0 && normalIndex < (int)normals.size())
				{
					vert.m_normal = normals[normalIndex];
				}
			}

			polygonVerts.push_back(vert);
		}

		//----- Triangulate (fan)
		for (int i = 1; i < (int)polygonVerts.size() - 1; ++i)
		{
			Vertex_PCUTBN triangle[3];
			triangle[0] = polygonVerts[0];
			triangle[1] = polygonVerts[i];
			triangle[2] = polygonVerts[i + 1];

			unsigned int indices[3];
			bool needsTangentBuild = false;

			//----- Dedup check FIRST
			for (int v = 0; v < 3; ++v)
			{
				if (!useIndexing)
				{
					needsTangentBuild = true;
					continue;
				}

				VertexKey key;
				key.position = triangle[v].m_position;
				key.normal = triangle[v].m_normal;
				key.uv = triangle[v].m_uvTexCoords;

				std::unordered_map<VertexKey, unsigned int, VertexKeyHasher>::iterator found = vertexLookup.find(key);

				if (found != vertexLookup.end())
				{
					indices[v] = found->second;
				}
				else
				{
					needsTangentBuild = true;
				}
			}

			//----- Only compute tangent space IF needed
			Vec3 tangent;
			Vec3 bitangent;
			Vec3 normal;

			if (needsTangentBuild)
			{
				Vec3 edge1 = triangle[1].m_position - triangle[0].m_position;
				Vec3 edge2 = triangle[2].m_position - triangle[1].m_position;

				Vec2 deltaUV1 = triangle[1].m_uvTexCoords - triangle[0].m_uvTexCoords;
				Vec2 deltaUV2 = triangle[2].m_uvTexCoords - triangle[1].m_uvTexCoords;

				float det = (deltaUV1.x * deltaUV2.y) - (deltaUV2.x * deltaUV1.y);

				if (fabsf(det) < 0.000001f)
				{
					Vec3 edge3 = triangle[2].m_position - triangle[0].m_position;
					normal = CrossProduct3D(edge1, edge3).GetNormalized();

					tangent = CrossProduct3D(Vec3::UP, normal);
					bitangent = CrossProduct3D(normal, tangent);
				}
				else
				{
					float invDet = 1.f / det;
					tangent = invDet * ((deltaUV2.y * edge1) - (deltaUV1.y * edge2));
					bitangent = invDet * ((deltaUV1.x * edge2) - (deltaUV2.x * edge1));
					normal = CrossProduct3D(tangent, bitangent).GetNormalized();
				}

				Mat44 basis(tangent, bitangent, normal, Vec3::ZERO);
				basis.Orthonormalize_IFwd_JLeft_KUp();

				tangent = basis.GetIBasis3D();
				bitangent = basis.GetJBasis3D();
				normal = basis.GetKBasis3D();
			}

			//----- Emit verts
			for (int v = 0; v < 3; ++v)
			{
				if (useIndexing)
				{
					VertexKey key;
					key.position = triangle[v].m_position;
					key.normal = triangle[v].m_normal;
					key.uv = triangle[v].m_uvTexCoords;

					std::unordered_map<VertexKey, unsigned int, VertexKeyHasher>::iterator found = vertexLookup.find(key);

					if (found != vertexLookup.end())
					{
						out_indexes.push_back(found->second);
						continue;
					}
				}

				if (needsTangentBuild)
				{
					triangle[v].m_tangent = tangent;
					triangle[v].m_biTangent = bitangent;
					triangle[v].m_normal = (triangle[v].m_normal != Vec3::ZERO) ? triangle[v].m_normal : normal;
				}

				unsigned int newIndex = (unsigned int)out_verts.size();
				out_verts.push_back(triangle[v]);

				if (useIndexing)
				{
					VertexKey key;
					key.position = triangle[v].m_position;
					key.normal = triangle[v].m_normal;
					key.uv = triangle[v].m_uvTexCoords;

					vertexLookup[key] = newIndex;
					out_indexes.push_back(newIndex);
				}
				else
				{
					out_indexes.push_back((unsigned int)out_indexes.size());
				}
			}
		}
	}

	TransformVertexArray3D(out_verts, meshInfo.m_transform, IntRange(START_INDEX, (int)out_verts.size() - 1));
	return true;
}

bool GetBasisFromXMLText(Vec3& out_basis, std::string const& text)
{
	std::string textLowercase = GetLowercase(text);
	if (textLowercase == "left")
	{
		out_basis = Vec3::LEFT;
		return true;
	}

	else if (textLowercase == "right")
	{
		out_basis = Vec3::RIGHT;
		return true;
	}

	else if (textLowercase == "forward")
	{
		out_basis = Vec3::FORWARD;
		return true;
	}

	else if (textLowercase == "backward" || textLowercase == "backwards")
	{
		out_basis = Vec3::BACKWARD;
		return true;
	}

	else if (textLowercase == "up")
	{
		out_basis = Vec3::UP;
		return true;
	}

	else if (textLowercase == "down")
	{
		out_basis = Vec3::DOWN;
		return true;
	}

	return false;
}
