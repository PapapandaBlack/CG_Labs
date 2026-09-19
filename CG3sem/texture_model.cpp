#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <stdexcept>

#include "texture_model.h"


const std::vector<Vertex>& TexturedModel::GetVertices() const
{
    return Vertices;
}

const std::vector<UINT>& TexturedModel::GetIndices() const
{
    return Indices;
}

UINT TexturedModel::GetIndexCount() const
{
    return IndexCount;
}

const std::vector<SurfaceMaterial>& TexturedModel::GetMaterials() const
{
    return Materials;
}

const std::vector<MeshPart>& TexturedModel::GetParts() const
{
    return Parts;
}

std::string ReadText(std::istringstream& ss)
{
    std::string text;
    std::getline(ss >> std::ws, text);

    const size_t end = text.find_last_not_of(" \t\r\n");

    if (end == std::string::npos)
    {
        return "";
    }

    text.erase(end + 1);

    if (text.size() >= 2 &&
        text.front() == '"' &&
        text.back() == '"')
    {
        text = text.substr(1, text.size() - 2);
    }

        return text;
    }

size_t ReadIndex(const std::string& text, size_t count)
{
    size_t parsedCharacters = 0;

    const long long value =
        std::stoll(text, &parsedCharacters);

    if (parsedCharacters != text.size() || value == 0)
    {
        throw std::runtime_error(
            "Invalid OBJ index: " + text);
    }

    const long long index = value > 0
        ? value - 1
        : static_cast<long long>(count) + value;

    if (index < 0 ||
        static_cast<size_t>(index) >= count)
    {
        throw std::runtime_error(
            "OBJ index is outside the array: " + text);
    }

    return static_cast<size_t>(index);
}

UINT TexturedModel::FindOrCreateMaterial(const std::string& name)
{
    for (size_t i = 0; i < Materials.size(); ++i)
    {
        if (Materials[i].Name == name)
        {
            return static_cast<UINT>(i);
        }
    }

    SurfaceMaterial material;
    material.Name = name;

    Materials.push_back(material);

    return static_cast<UINT>(Materials.size() - 1);
}

bool TexturedModel::LoadMaterials(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Failed to open MTL file: "
            << filename << std::endl;

        return false;
    }

    UINT currentMaterial = 0;
    bool hasCurrentMaterial = false;

    std::string line;
    size_t lineNumber = 0;

    try
    {
        while (std::getline(file, line))
        {
            ++lineNumber;

            const size_t comment = line.find('#');

            if (comment != std::string::npos)
            {
                line.erase(comment);
            }

            std::istringstream ss(line);

            std::string type;
            ss >> type;

            if (type == "newmtl")
            {
                const std::string name = ReadText(ss);

                if (name.empty())
                {
                    throw std::runtime_error(
                        "Empty material name");
                }

                currentMaterial = FindOrCreateMaterial(name);
                hasCurrentMaterial = true;
            }
            else if (type == "Kd")
            {
                if (!hasCurrentMaterial)
                {
                    throw std::runtime_error(
                        "Kd before newmtl");
                }

                float r, g, b;

                if (!(ss >> r >> g >> b))
                {
                    throw std::runtime_error(
                        "Invalid Kd color");
                }

                Materials[currentMaterial].Diffuse =
                    Vector4(r, g, b, 1.0f);
            }
            else if (type == "map_Kd")
            {
                if (!hasCurrentMaterial)
                {
                    throw std::runtime_error(
                        "map_Kd before newmtl");
                }

                const std::string textureName = ReadText(ss);

                if (textureName.empty())
                {
                    throw std::runtime_error(
                        "Empty texture filename");
                }

                if (textureName.front() == '-')
                {
                    throw std::runtime_error(
                        "map_Kd options are not supported");
                }

                Materials[currentMaterial].TexturePath =
                    textureName;
            }
        }

        if (file.bad())
        {
            throw std::runtime_error(
                "MTL file read error");
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "MTL error: " << filename
            << ", line " << lineNumber
            << ": " << error.what() << std::endl;

        return false;
    }

    return true;
}

bool TexturedModel::Load(const std::string& filename)
{
    const auto clearData = [this]()
        {
            Positions.clear();
            Normals.clear();
            UVs.clear();

            Vertices.clear();
            Indices.clear();

            Materials.clear();
            Parts.clear();

            IndexCount = 0;
        };

    clearData();

    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: "
            << filename << std::endl;

        return false;
    }

    Materials.push_back(SurfaceMaterial{});

    UINT currentMaterial = 0;

    std::unordered_map<Vertex, UINT> uniqueVertices;

    std::string line;
    size_t lineNumber = 0;

    try
    {
        while (std::getline(file, line))
        {
            ++lineNumber;

            const size_t comment = line.find('#');

            if (comment != std::string::npos)
            {
                line.erase(comment);
            }

            std::istringstream ss(line);

            std::string type;
            ss >> type;

            if (type == "v")
            {
                float x, y, z;

                if (!(ss >> x >> y >> z))
                {
                    throw std::runtime_error(
                        "Invalid vertex position");
                }

                Positions.push_back(Vector3(x, y, z));
            }
            else if (type == "vn")
            {
                float x, y, z;

                if (!(ss >> x >> y >> z))
                {
                    throw std::runtime_error(
                        "Invalid vertex normal");
                }

                Vector3 normal(x, y, z);

                if (normal.LengthSquared() > 0.000001f)
                {
                    normal.Normalize();
                }

                Normals.push_back(normal);
            }
            else if (type == "vt")
            {
                float u, v;

                if (!(ss >> u >> v))
                {
                    throw std::runtime_error(
                        "Invalid texture coordinates");
                }

                UVs.push_back(Vector2(u, 1.0f - v));
            }
            else if (type == "mtllib")
            {
                const std::string materialFile = ReadText(ss);

                if (materialFile.empty())
                {
                    throw std::runtime_error(
                        "Empty MTL filename");
                }

                if (!LoadMaterials(materialFile))
                {
                    throw std::runtime_error(
                        "Failed to load materials: " + materialFile);
                }
            }
            else if (type == "usemtl")
            {
                const std::string materialName = ReadText(ss);

                if (materialName.empty())
                {
                    throw std::runtime_error(
                        "Empty usemtl name");
                }

                currentMaterial =
                    FindOrCreateMaterial(materialName);
            }
            else if (type == "f")
            {
                std::vector<Vertex> faceVertices;
                std::string vertexText;

                while (ss >> vertexText)
                {
                    std::istringstream vs(vertexText);
                    std::string indexText[3];

                    std::getline(vs, indexText[0], '/');
                    std::getline(vs, indexText[1], '/');
                    std::getline(vs, indexText[2], '/');

                    if (indexText[0].empty())
                    {
                        throw std::runtime_error(
                            "Face vertex has no position index");
                    }

                    Vertex vertex{};
                    vertex.uv = Vector2(0.0f, 0.0f);
                    vertex.normal = Vector3(0.0f, 0.0f, 0.0f);

                    const size_t positionIndex =
                        ReadIndex(indexText[0], Positions.size());

                    vertex.pos = Positions[positionIndex];

                    if (!indexText[1].empty())
                    {
                        const size_t uvIndex =
                            ReadIndex(indexText[1], UVs.size());

                        vertex.uv = UVs[uvIndex];
                    }

                    if (!indexText[2].empty())
                    {
                        const size_t normalIndex =
                            ReadIndex(indexText[2], Normals.size());

                        vertex.normal = Normals[normalIndex];
                    }

                    faceVertices.push_back(vertex);
                }

                if (faceVertices.size() < 3)
                {
                    throw std::runtime_error(
                        "Face has fewer than three vertices");
                }

                if (Parts.empty() ||
                    Parts.back().MaterialIndex != currentMaterial)
                {
                    MeshPart part;

                    part.StartIndex =
                        static_cast<UINT>(Indices.size());

                    part.MaterialIndex = currentMaterial;

                    Parts.push_back(part);
                }

                for (size_t j = 1; j + 1 < faceVertices.size(); ++j)
                {
                    Vertex triangle[3] =
                    {
                        faceVertices[0],
                        faceVertices[j],
                        faceVertices[j + 1]
                    };

                    const Vector3 edge1 =
                        triangle[1].pos - triangle[0].pos;

                    const Vector3 edge2 =
                        triangle[2].pos - triangle[0].pos;

                    Vector3 faceNormal = edge1.Cross(edge2);

                    if (faceNormal.LengthSquared() > 0.000001f)
                    {
                        faceNormal.Normalize();
                    }
                    else
                    {
                        faceNormal = Vector3(0.0f, 1.0f, 0.0f);
                    }

                    for (Vertex& vertex : triangle)
                    {
                        if (vertex.normal.LengthSquared() < 0.000001f)
                        {
                            vertex.normal = faceNormal;
                        }

                        const auto it =
                            uniqueVertices.find(vertex);

                        UINT index = 0;

                        if (it == uniqueVertices.end())
                        {
                            index =
                                static_cast<UINT>(Vertices.size());

                            Vertices.push_back(vertex);
                            uniqueVertices.emplace(vertex, index);
                        }
                        else
                        {
                            index = it->second;
                        }

                        Indices.push_back(index);
                    }

                    Parts.back().IndexCount += 3;
                }
            }
        }

        if (file.bad())
        {
            throw std::runtime_error(
                "OBJ file read error");
        }

        if (Indices.empty())
        {
            throw std::runtime_error(
                "OBJ contains no triangles");
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "OBJ error: " << filename
            << ", line " << lineNumber
            << ": " << error.what() << std::endl;

        clearData();

        return false;
    }

    IndexCount = static_cast<UINT>(Indices.size());

    return true;
}