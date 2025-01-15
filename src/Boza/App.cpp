#include "App.hpp"

#include "Logger.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace boza
{
    App::App(const std::string_view& name) : name{ name }, ok{ true }
    {
        Logger::trace("Starting...");
        if (!initialize_vulkan_objects()) return;
        if (!create_compute_shader("compute.comp")) return;
        if (!create_image3d()) return;

        const MeshData mesh_data = TriangleLoader::load_from_obj("model.obj");
        if (!mesh_data)
        {
            Logger::error("Failed to load image data from model.obj");
            ok = false;
            return;
        }

        index_count = static_cast<uint32_t>(mesh_data.indices.size());

        if (!create_vertex_buffer(mesh_data)) return;
        if (!create_index_buffer(mesh_data)) return;
        if (!create_uniform_buffer()) return;

        compute_shader.update_storage_image(0, image.get_image_view(), vk::ImageLayout::eGeneral);
        compute_shader.update_storage_buffer(1, vertex_buffer.get_buffer());
        compute_shader.update_storage_buffer(2, index_buffer.get_buffer());
        compute_shader.update_uniform_buffer(3, uniform_buffer.get_buffer());

        const Params params{ index_count, 0.3f };
        if (!uniform_buffer.update_uniform(&params, sizeof(Params)))
        {
            Logger::error("Failed to update uniform buffer");
            ok = false;
            return;
        }
    }

    App::~App()
    {
        if (ok) Logger::trace("Exiting...");
    }


    void App::run()
    {
        if (!dispatch())
        {
            Logger::error("Failed to dispatch compute shader");
            return;
        }

        std::vector<uint8_t> image_data = image.get_data();
        save_image(image_data, "output.png");
    }


    bool App::initialize_vulkan_objects()
    {
        instance = Instance(name);
        if (!instance)
        {
            ok = false;
            return false;
        }

        device = Device(instance);
        if (!device)
        {
            ok = false;
            return false;
        }

        command_pool = CommandPool(device);
        if (!command_pool) ok = false;
        return ok;
    }

    bool App::create_compute_shader(const std::string_view& filename)
    {
        const std::vector<ComputeShader::DescriptorBindingInfo> bindings
        {
            { 0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute },
            { 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute },
            { 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute },
            { 3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute }
        };

        compute_shader = ComputeShader(device, filename, bindings);
        if (!compute_shader) ok = false;
        return ok;
    }

    bool App::create_image3d()
    {
        image = Image3D(device, command_pool, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(width, height, depth));
        if (!image) ok = false;
        return ok;
    }

    bool App::create_vertex_buffer(const MeshData& mesh_data)
    {
        const size_t vertex_buffer_size = sizeof(glm::vec4) * mesh_data.vertices.size();
        vertex_buffer = Buffer{
            device,
            vertex_buffer_size,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        };

        if (!vertex_buffer)
        {
            Logger::error("Failed to create vertex buffer");
            ok = false;
            return false;
        }

        if (!vertex_buffer.bind())
        {
            Logger::error("Failed to bind vertex buffer");
            ok = false;
            return false;
        }

        std::vector<glm::vec4> padded_vertices;
        padded_vertices.reserve(mesh_data.vertices.size());
        for (const auto& v : mesh_data.vertices)
            padded_vertices.emplace_back(v, 0.0f);

        if (!vertex_buffer.copy_data(padded_vertices.data(), vertex_buffer_size))
        {
            Logger::error("Failed to copy data to vertex buffer");
            ok = false;
        }

        return ok;
    }

    bool App::create_index_buffer(const MeshData& mesh_data)
    {
        const vk::DeviceSize index_buffer_size = sizeof(uint32_t) * mesh_data.indices.size();
        index_buffer = Buffer{
            device,
            index_buffer_size,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        };

        if (!index_buffer)
        {
            Logger::error("Failed to create index buffer");
            ok = false;
            return false;
        }

        if (!index_buffer.bind())
        {
            Logger::error("Failed to bind index buffer");
            ok = false;
            return false;
        }

        if (!index_buffer.copy_data(mesh_data.indices.data(), index_buffer_size))
        {
            Logger::error("Failed to copy data to index buffer");
            ok = false;
        }

        return ok;
    }

    bool App::create_uniform_buffer()
    {
        uniform_buffer = Buffer::uniform_buffer(device, sizeof(Params));
        if (!uniform_buffer)
        {
            Logger::error("Failed to create uniform buffer");
            ok = false;
            return false;
        }

        if (!uniform_buffer.bind())
        {
            Logger::error("Failed to bind uniform buffer");
            ok = false;
        }

        return ok;
    }


    bool App::dispatch()
    {
        if (command_pool.get_command_buffer().begin(vk::CommandBufferBeginInfo{}) != vk::Result::eSuccess)
        {
            Logger::error("Failed to begin command buffer");
            ok = false;
            return false;
        }

        vk::ImageMemoryBarrier barrier
        {
            {},
            vk::AccessFlagBits::eShaderWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image.get_image(),
            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        };

        command_pool.get_command_buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eComputeShader,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        compute_shader.dispatch(command_pool.get_command_buffer(),
                                static_cast<uint32_t>(std::ceil(width / 8.0)),
                                static_cast<uint32_t>(std::ceil(height / 8.0)),
                                static_cast<uint32_t>(std::ceil(depth / 8.0)));

        barrier.oldLayout     = vk::ImageLayout::eGeneral;
        barrier.newLayout     = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        command_pool.get_command_buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (command_pool.get_command_buffer().end() != vk::Result::eSuccess)
        {
            Logger::error("Failed to end command buffer");
            ok = false;
            return false;
        }

        const vk::SubmitInfo submit_info
        {
            {},
            {},
            {},
            1, &command_pool.get_command_buffer()
        };

        if (device.get_compute_queue().submit(submit_info, {}) != vk::Result::eSuccess)
        {
            Logger::error("Failed to submit command buffer");
            ok = false;
            return false;
        }

        if (device.get_compute_queue().waitIdle() != vk::Result::eSuccess)
        {
            Logger::error("Failed to wait for compute queue to become idle");
            ok = false;
        }

        return ok;
    }


    void App::save_image(const std::span<uint8_t>& data, const std::string_view& filename) const
    {
        uint32_t grid_side     = static_cast<uint32_t>(std::ceil(std::sqrt(height)));
        uint32_t output_size_x = width * grid_side;
        uint32_t output_size_y = depth * grid_side;

        std::vector<uint8_t> image_data_reordered(output_size_x * output_size_y * 4, 0);

        for (uint32_t slice = 0; slice < height; ++slice)
        {
            uint32_t grid_x = (slice % grid_side) * width;
            uint32_t grid_y = (slice / grid_side) * depth;

            for (uint32_t z = 0; z < depth; ++z)
            {
                for (uint32_t x = 0; x < width; ++x)
                {
                    for (uint32_t c = 0; c < 4; ++c)
                    {
                        const uint32_t src_idx = ((z * height + slice) * width + x) * 4 + c;
                        const uint32_t dst_idx = ((grid_y + z) * output_size_x + (grid_x + x)) * 4 + c;
                        image_data_reordered[dst_idx] = data[src_idx];
                    }
                }
            }
        }

        stbi_flip_vertically_on_write(true);
        stbi_write_png(filename.data(),
                       static_cast<int>(output_size_x),
                       static_cast<int>(output_size_y),
                       4,
                       image_data_reordered.data(),
                       static_cast<int>(output_size_x) * 4);
    }
}
