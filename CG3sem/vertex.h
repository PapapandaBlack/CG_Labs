#ifndef VERTEX
#define VERTEX

#include <SimpleMath.h>
#include <d3d12.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

struct Vertex
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && normal == other.normal && uv == other.uv;
    }
};

namespace std
{
    template<>
    struct hash<Vertex>
    {
        size_t operator()(const Vertex& v) const
        {
            size_t h1 = std::hash<float>()(v.pos.x);
            size_t h2 = std::hash<float>()(v.pos.y);
            size_t h3 = std::hash<float>()(v.pos.z);
            size_t h4 = std::hash<float>()(v.normal.x);
            size_t h5 = std::hash<float>()(v.normal.y);
            size_t h6 = std::hash<float>()(v.normal.z);
            size_t h7 = std::hash<float>()(v.uv.x);
            size_t h8 = std::hash<float>()(v.uv.y);

            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^
                (h5 << 4) ^ (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
        }
    };
}

#endif