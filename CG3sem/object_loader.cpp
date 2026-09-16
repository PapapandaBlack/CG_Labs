#include "object_loader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

bool ObjLoader::Load(const std::string& filename)
{
    Positions.clear();
    Normals.clear();
    UVs.clear();
    Vertices.clear();
    Indices.clear();

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::unordered_map<Vertex, UINT> uniqueVertices;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            float x, y, z; ss >> x >> y >> z;
            Positions.push_back({ x, y, z });
        }
        else if (type == "vn") {
            float x, y, z; ss >> x >> y >> z;
            Normals.push_back({ x, y, z });
        }
        else if (type == "vt") {
            float u, v; ss >> u >> v;
            UVs.push_back({ u, 1.0f - v });
        }
        else if (type == "f") {
            std::vector<UINT> faceIndices;
            std::string vertStr;

            while (ss >> vertStr) {
                std::istringstream vs(vertStr);
                std::string idx[3];
                int i = 0;
                while (i < 3 && std::getline(vs, idx[i], '/')) i++;

                int vi = std::stoi(idx[0]) - 1;
                int uvi = (!idx[1].empty()) ? std::stoi(idx[1]) - 1 : -1;
                int ni = (!idx[2].empty()) ? std::stoi(idx[2]) - 1 : -1;

                Vertex vert;
                vert.pos = (vi >= 0 && vi < (int)Positions.size()) ? Positions[vi] : Vector3(0, 0, 0);
                vert.uv = (uvi >= 0 && uvi < (int)UVs.size()) ? UVs[uvi] : Vector2(0, 0);
                vert.normal = (ni >= 0 && ni < (int)Normals.size()) ? Normals[ni] : Vector3(0, 1, 0);

                auto it = uniqueVertices.find(vert);
                UINT index = 0;

                if (it == uniqueVertices.end()) {
                    index = static_cast<UINT>(Vertices.size());
                    Vertices.push_back(vert);
                    uniqueVertices[vert] = index;
                }
                else {
                    index = it->second;
                }
                faceIndices.push_back(index);
            }

            if (faceIndices.size() >= 3) {
                for (size_t j = 1; j < faceIndices.size() - 1; ++j) {
                    Indices.push_back(faceIndices[0]);
                    Indices.push_back(faceIndices[j]);
                    Indices.push_back(faceIndices[j + 1]);
                }
            }
        }
    }

    IndexCount = static_cast<UINT>(Indices.size());
    return true;
}

const std::vector<Vertex>& ObjLoader::GetVertices() const { return Vertices; }
const std::vector<UINT>& ObjLoader::GetIndices() const { return Indices; }
UINT ObjLoader::GetIndexCount() const { return IndexCount; }