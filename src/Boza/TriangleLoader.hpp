#pragma once
#include "pch.hpp"

namespace boza
{
    struct MeshData
    {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
    };

    class TriangleLoader
    {
    public:
        static bool loadFromObj(const std::string& filename, MeshData& outMeshData);
    };
}
