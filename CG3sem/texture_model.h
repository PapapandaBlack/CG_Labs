#pragma once

#include <string>
#include <vector>

#include "vertex.h"

struct SurfaceMaterial
{
    std::string Name;
    Vector4 Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    std::string TexturePath;
};

struct MeshPart
{
    UINT StartIndex = 0;
    UINT IndexCount = 0;
    UINT MaterialIndex = 0;
};

class TexturedModel
{
public:
    TexturedModel() = default;
    ~TexturedModel() = default;

    bool Load(const std::string& filename);

    const std::vector<Vertex>& GetVertices() const;
    const std::vector<UINT>& GetIndices() const;

    UINT GetIndexCount() const;

    const std::vector<SurfaceMaterial>& GetMaterials() const;
    const std::vector<MeshPart>& GetParts() const;

private:
    bool LoadMaterials(const std::string& filename);

    UINT FindOrCreateMaterial(const std::string& name);

private:
    UINT IndexCount = 0;

    std::vector<Vector3> Positions;
    std::vector<Vector3> Normals;
    std::vector<Vector2> UVs;

    std::vector<Vertex> Vertices;
    std::vector<UINT> Indices;

    std::vector<SurfaceMaterial> Materials;
    std::vector<MeshPart> Parts;
};