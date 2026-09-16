#ifndef OBJ_LOADER
#define OBJ_LOADER

#include <string>
#include <vector>
#include <d3d12.h>
#include "vertex.h"

using namespace DirectX::SimpleMath;

class ObjLoader
{
public:
    ObjLoader() = default;
    ~ObjLoader() = default;

    bool Load(const std::string& filename);

    const std::vector<Vertex>& GetVertices() const;
    const std::vector<UINT>& GetIndices() const;
    UINT GetIndexCount() const;

private:
    UINT IndexCount = 0;

    std::vector<Vector3> Positions;
    std::vector<Vector3> Normals;
    std::vector<Vector2> UVs;

    std::vector<Vertex> Vertices;
    std::vector<UINT> Indices;
};

#endif
