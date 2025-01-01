#pragma once
#include "Boza/pch.hpp"

#include "Boza/App.hpp"
#include "Boza/ComputeShader.hpp"
#include "Boza/Buffer.hpp"
#include "Boza/Image3D.hpp"
#include "Boza/TriangleLoader.hpp"

namespace boza
{
    class ComputeApp final : public App
    {
    public:
        ComputeApp(uint32_t width, uint32_t height, uint32_t depth);
        ~ComputeApp() override;

        bool run() override;

    private:
        struct Params
        {
            uint32_t index_count;
            float    scale;
        };

        ComputeShader compute_shader{ nullptr };
        Image3D       image{ nullptr };
        Buffer        vertex_buffer{ nullptr };
        Buffer        index_buffer{ nullptr };
        Buffer        uniform_buffer{ nullptr };

        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t index_count;

        [[nodiscard]] bool create_compute_shader();
        [[nodiscard]] bool create_output_image();
        [[nodiscard]] bool create_vertex_buffer(const MeshData& mesh_data);
        [[nodiscard]] bool create_index_buffer(const MeshData& mesh_data);
        [[nodiscard]] bool create_uniform_buffer();

        [[nodiscard]]
        bool dispatch();
        void save_image_data(const std::vector<uint8_t>& image_data, const std::string_view& filename);

        [[nodiscard]]
        static std::vector<uint8_t> rearrange_image_data(
            const std::vector<uint8_t>& image_data,
            uint32_t                    width,
            uint32_t                    height,
            uint32_t                    depth);
    };
}
