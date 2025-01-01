#include "Boza/pch.hpp"
#include "ComputeApp.hpp"

#include "Boza/Logger.hpp"
#include "Boza/CommandPool.hpp"
#include "Boza/Device.hpp"
#include "Boza/TriangleLoader.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace boza
{
    static std::chrono::duration<double> time(const std::function<void()>& lambda)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        lambda();
        const auto end = std::chrono::high_resolution_clock::now();
        return end - start;
    }

    ComputeApp::ComputeApp(const uint32_t width, const uint32_t height, const uint32_t depth)
        : App("Test12345"), width(width), height(height), depth(depth)
    {
        if (![&]
        {
            BOZA_CHECK(create_compute_shader(), "Failed to create compute shader");
            BOZA_CHECK(create_output_image(), "Failed to create output image");

            MeshData mesh_data;
            BOZA_CHECK(TriangleLoader::loadFromObj("model.obj", mesh_data), "Failed to load mesh");
            index_count = static_cast<uint32_t>(mesh_data.indices.size());

            BOZA_CHECK(create_vertex_buffer(mesh_data), "Failed to create vertex buffer");
            BOZA_CHECK(create_index_buffer(mesh_data), "Failed to create index buffer");

            BOZA_CHECK(create_uniform_buffer(), "Failed to create uniform buffer");

            compute_shader.update_storage_image(0, image.get_image_view(), vk::ImageLayout::eGeneral);
            compute_shader.update_storage_buffer(1, vertex_buffer.get_buffer());
            compute_shader.update_storage_buffer(2, index_buffer.get_buffer());
            compute_shader.update_uniform_buffer(3, uniform_buffer.get_buffer());

            return true;
        }())
        {
            hasThrown = true;
            Logger::error("Failed to initialize ComputeApp");
        }
    }

    ComputeApp::~ComputeApp()
    {
        vertex_buffer.destroy();
        index_buffer.destroy();
        uniform_buffer.destroy();
        image.destroy();
    }


    bool ComputeApp::run()
    {
        for (int i = 1; i <= 10; ++i)
        {
            Params params{ index_count, static_cast<float>(i) * 0.1f };
            BOZA_CHECK(uniform_buffer.update_uniform(&params, sizeof(Params)), "Failed to update uniform buffer");

            std::vector<uint8_t> image_data;

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                time([&]
                {
                    BOZA_CHECK_RET_CUSTOM(dispatch(), , "Failed to dispatch compute shader");
                    image_data = image.get_data();
                })
            );

            Logger::info("image {} took {}ms", i, duration.count());

            save_image_data(image_data, std::format("output_{:02}.png", i));
        }

        return true;
    }


    bool ComputeApp::create_compute_shader()
    {
        std::vector<ComputeShader::DescriptorBindingInfo> bindings
        {
            { 0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute },
            { 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute },
            { 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute },
            { 3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute }
        };

        auto compute_shader_opt = ComputeShader::create("compute.comp", bindings);
        BOZA_CHECK(compute_shader_opt.has_value(), "Failed to create compute shader");
        compute_shader = std::move(*compute_shader_opt);

        return true;
    }

    bool ComputeApp::create_output_image()
    {
        auto image_opt = Image3D::create(
            vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D(width, height, depth),
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            true
        );
        BOZA_CHECK(image_opt.has_value(), "Failed to create output image");
        image = std::move(*image_opt);

        return true;
    }


    bool ComputeApp::create_vertex_buffer(const MeshData& mesh_data)
    {
        const size_t vertex_buffer_size = sizeof(glm::vec4) * mesh_data.vertices.size();
        auto vertex_buffer_opt  = Buffer::create(
            vertex_buffer_size,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        BOZA_CHECK(vertex_buffer_opt.has_value(), "Failed to create vertex buffer");
        vertex_buffer = std::move(*vertex_buffer_opt);

        BOZA_CHECK(vertex_buffer.bind(), "Failed to bind vertex buffer memory");
        std::vector<glm::vec4> padded_vertices;
        padded_vertices.reserve(mesh_data.vertices.size());
        for (const auto& v : mesh_data.vertices)
            padded_vertices.emplace_back(v, 0.0f);

        BOZA_CHECK(vertex_buffer.copy_data(padded_vertices.data(), vertex_buffer_size), "Failed to copy vertex data");

        return true;
    }

    bool ComputeApp::create_index_buffer(const MeshData& mesh_data)
    {
        const vk::DeviceSize index_buffer_size = sizeof(uint32_t) * mesh_data.indices.size();
        auto index_buffer_opt = Buffer::create(
            index_buffer_size,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        BOZA_CHECK(index_buffer_opt.has_value(), "Failed to create index buffer");
        index_buffer = std::move(*index_buffer_opt);

        BOZA_CHECK(index_buffer.bind(), "Failed to bind index buffer memory");
        BOZA_CHECK(index_buffer.copy_data(mesh_data.indices.data(), index_buffer_size), "Failed to copy index data");

        return true;
    }


    bool ComputeApp::create_uniform_buffer()
    {
        auto uniform_buffer_opt = Buffer::create_uniform_buffer(sizeof(Params));
        BOZA_CHECK(uniform_buffer_opt.has_value(), "Failed to create uniform buffer");
        uniform_buffer = std::move(*uniform_buffer_opt);

        BOZA_CHECK(uniform_buffer.bind(), "Failed to bind uniform buffer memory");

        return true;
    }


    bool ComputeApp::dispatch()
    {
        VK_CHECK(CommandPool::get_command_buffer().begin(vk::CommandBufferBeginInfo{}), "Failed to begin command buffer");

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

        CommandPool::get_command_buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eComputeShader,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        compute_shader.dispatch(CommandPool::get_command_buffer(),
                                static_cast<uint32_t>(std::ceil(width / 8.0)),
                                static_cast<uint32_t>(std::ceil(height / 8.0)),
                                static_cast<uint32_t>(std::ceil(depth / 8.0)));

        barrier.oldLayout     = vk::ImageLayout::eGeneral;
        barrier.newLayout     = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        CommandPool::get_command_buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VK_CHECK(CommandPool::get_command_buffer().end(), "Failed to end command buffer");

        const vk::SubmitInfo submit_info
        {
            {},
            {},
            {},
            1, &CommandPool::get_command_buffer()
        };

        VK_CHECK(Device::get_compute_queue().submit(submit_info, {}), "Failed to submit compute queue");
        VK_CHECK(Device::get_logical_device().waitIdle(), "Failed to wait for device idle");

        return true;
    }

    void ComputeApp::save_image_data(const std::vector<uint8_t>& image_data, const std::string_view& filename)
    {
        uint32_t grid_side     = static_cast<uint32_t>(std::ceil(std::sqrt(height)));
        uint32_t output_size_x = width * grid_side;
        uint32_t output_size_y = depth * grid_side;

        const std::vector<uint8_t> image_data_reordered = rearrange_image_data(image_data, width, height, depth);

        stbi_flip_vertically_on_write(true);
        stbi_write_png(filename.data(),
                       static_cast<int>(output_size_x),
                       static_cast<int>(output_size_y),
                       4,
                       image_data_reordered.data(),
                       static_cast<int>(output_size_x) * 4);
    }


    std::vector<uint8_t> ComputeApp::rearrange_image_data(
        const std::vector<uint8_t>& image_data,
        const uint32_t              width,
        const uint32_t              height,
        const uint32_t              depth)
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
                        image_data_reordered[dst_idx] = image_data[src_idx];
                    }
                }
            }
        }

        return image_data_reordered;
    }
}
