#pragma once
#include "pch.hpp"

namespace boza
{
    struct MeshData final
    {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;

        operator bool () const { return !vertices.empty() && !indices.empty(); }
    };

    class TriangleLoader final
    {
    public:
        TriangleLoader() = delete;
        static MeshData load_from_obj(const std::string& filename);
    };
}
