#pragma once
#include "Buffer.hpp"
#include "CommandPool.hpp"
#include "ComputeShader.hpp"
#include "Instance.hpp"
#include "Device.hpp"
#include "Image3D.hpp"
#include "TriangleLoader.hpp"

namespace boza
{
    class App
    {
    public:
        struct Params
        {
            uint32_t index_count;
            float    scale;
        };

        explicit App(const std::string_view& name);
        ~App();

        App(const App&)            = delete;
        App& operator=(const App&) = delete;
        App(App&&)                 = delete;
        App& operator=(App&&)      = delete;

        void run();

        operator bool () const noexcept { return ok; }

    private:
        [[nodiscard]] bool initialize_vulkan_objects();
        [[nodiscard]] bool create_compute_shader(const std::string_view& filename);
        [[nodiscard]] bool create_image3d();

        [[nodiscard]] bool create_vertex_buffer(const MeshData& mesh_data);
        [[nodiscard]] bool create_index_buffer(const MeshData& mesh_data);
        [[nodiscard]] bool create_uniform_buffer();
        [[nodiscard]] bool dispatch();

        void save_image(const std::span<uint8_t>& data, const std::string_view& filename) const;

        const uint32_t width{ 128 };
        const uint32_t height{ 64 };
        const uint32_t depth{ 128 };

        uint32_t index_count{ 0 };

        std::string name;

        Instance      instance{ nullptr };
        Device        device{ nullptr };
        CommandPool   command_pool{ nullptr };
        ComputeShader compute_shader{ nullptr };
        Image3D       image{ nullptr };

        Buffer vertex_buffer{ nullptr };
        Buffer index_buffer{ nullptr };
        Buffer uniform_buffer{ nullptr };

        bool ok = false;
    };
}
