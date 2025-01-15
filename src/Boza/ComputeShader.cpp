#include "ComputeShader.hpp"

#include "Logger.hpp"

namespace boza
{
    ComputeShader::ComputeShader(
        const Device&                             device,
        const std::string_view&                   shader_path,
        const std::vector<DescriptorBindingInfo>& descriptor_bindings,
        const std::vector<PushConstantRange>&     push_constant_ranges,
        const vk::PipelineCache                   pipeline_cache)
        : device{ std::cref(device) }, ok{ true }
    {
        if (!create_shader_module(shader_path)) return;
        if (!create_descriptor_set_layout(descriptor_bindings)) return;
        if (!create_pipeline_layout(push_constant_ranges)) return;
        if (!create_pipeline(pipeline_cache)) return;
        if (!create_descriptor_pool_and_set(descriptor_bindings)) return;

        uint32_t total_push_constant_size = 0;
        for (auto& pc : push_constant_ranges)
            total_push_constant_size = std::max(total_push_constant_size, pc.offset + pc.size);

        push_constant_buffer.resize(total_push_constant_size, 0);
    }


    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    {
        shader_module         = std::move(other.shader_module);
        pipeline_layout       = std::move(other.pipeline_layout);
        pipeline              = std::move(other.pipeline);
        descriptor_set_layout = std::move(other.descriptor_set_layout);
        descriptor_pool       = std::move(other.descriptor_pool);
        descriptor_set        = std::move(other.descriptor_set);

        push_constant_buffer = std::exchange(other.push_constant_buffer, {});

        ok = std::exchange(other.ok, false);

        if (other.device) device = std::cref(other.device->get());
        other.device = std::nullopt;
    }

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
    {
        if (this != &other)
        {
            shader_module         = std::move(other.shader_module);
            pipeline_layout       = std::move(other.pipeline_layout);
            pipeline              = std::move(other.pipeline);
            descriptor_set_layout = std::move(other.descriptor_set_layout);
            descriptor_pool       = std::move(other.descriptor_pool);
            descriptor_set        = std::move(other.descriptor_set);

            push_constant_buffer = std::exchange(other.push_constant_buffer, {});

            ok = std::exchange(other.ok, false);

            if (other.device) device = std::cref(other.device->get());
            other.device = std::nullopt;
        }

        return *this;
    }


    void ComputeShader::update_storage_image(
        const uint32_t        binding,
        const vk::ImageView   image_view,
        const vk::ImageLayout layout) const
    {
        const vk::DescriptorImageInfo image_info
        {
            {},
            image_view, layout
        };

        vk::WriteDescriptorSet write
        {
            *descriptor_set,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &image_info,
            nullptr,
            nullptr
        };

        device->get().get().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_storage_buffer(
        const uint32_t       binding,
        const vk::Buffer     buffer,
        const vk::DeviceSize range,
        const vk::DeviceSize offset) const
    {
        const vk::DescriptorBufferInfo buffer_info{ buffer, offset, range };

        vk::WriteDescriptorSet write
        {
            *descriptor_set,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &buffer_info,
            nullptr
        };

        device->get().get().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_uniform_buffer(
        const uint32_t       binding,
        const vk::Buffer     buffer,
        const vk::DeviceSize range,
        const vk::DeviceSize offset) const
    {
        const vk::DescriptorBufferInfo buffer_info{ buffer, offset, range };

        vk::WriteDescriptorSet write
        {
            *descriptor_set,
            binding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &buffer_info,
            nullptr
        };

        device->get().get().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_sampled_image(
        const uint32_t        binding,
        const vk::ImageView   image_view,
        const vk::Sampler     sampler,
        const vk::ImageLayout layout) const
    {
        const vk::DescriptorImageInfo image_info{ sampler, image_view, layout };

        vk::WriteDescriptorSet write
        {
            *descriptor_set,
            binding,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &image_info,
            nullptr,
            nullptr
        };

        device->get().get().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::dispatch(
        const vk::CommandBuffer command_buffer,
        const uint32_t          group_count_x,
        const uint32_t          group_count_y,
        const uint32_t          group_count_z)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0,
                                          { *descriptor_set }, {});

        if (!push_constant_buffer.empty())
            command_buffer.pushConstants<uint8_t>(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0,
                                                  push_constant_buffer);

        command_buffer.dispatch(group_count_x, group_count_y, group_count_z);
    }


    bool ComputeShader::create_shader_module(const std::string_view& shader_path)
    {
        namespace fs = std::filesystem;
        const auto code = load_shader_binary(
            (fs::current_path() / "shaders" / "spv" / (std::string(shader_path) + ".spv")).string());

        if (code.empty())
        {
            Logger::error("Failed to load shader binary from {}", shader_path);
            ok = false;
            return false;
        }

        const vk::ShaderModuleCreateInfo create_info
        {
            {},
            code.size() * sizeof(uint32_t),
            code.data()
        };

        auto [result, _shader_module] = device->get().get().createShaderModuleUnique(create_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create shader module from {}", shader_path);
            ok = false;
            return false;
        }

        shader_module = std::move(_shader_module);
        return true;
    }

    bool ComputeShader::create_descriptor_set_layout(const std::vector<DescriptorBindingInfo>& descriptor_bindings)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.reserve(descriptor_bindings.size());

        for (const auto& [binding, descriptorType, stageFlags, descriptorCount] : descriptor_bindings)
        {
            vk::DescriptorSetLayoutBinding dsl_binding(binding, descriptorType, descriptorCount, stageFlags);
            bindings.push_back(dsl_binding);
        }

        const vk::DescriptorSetLayoutCreateInfo layout_info
        {
            {},
            static_cast<uint32_t>(bindings.size()),
            bindings.data()
        };

        auto [result, _descriptor_set_layout] = device->get().get().createDescriptorSetLayoutUnique(layout_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create descriptor set layout");
            ok = false;
            return false;
        }

        descriptor_set_layout = std::move(_descriptor_set_layout);
        return true;
    }

    bool ComputeShader::create_pipeline_layout(const std::vector<PushConstantRange>& push_constant_ranges)
    {
        std::vector<vk::PushConstantRange> push_ranges;
        push_ranges.reserve(push_constant_ranges.size());
        for (const auto& [stageFlags, offset, size] : push_constant_ranges)
        {
            vk::PushConstantRange range(stageFlags, offset, size);
            push_ranges.push_back(range);
        }

        const vk::PipelineLayoutCreateInfo pipeline_layout_info
        {
            {},
            1, &descriptor_set_layout.get(),
            static_cast<uint32_t>(push_ranges.size()), push_ranges.data()
        };

        auto [result, _pipeline_layout] = device->get().get().createPipelineLayoutUnique(pipeline_layout_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create pipeline layout");
            ok = false;
            return false;
        }

        pipeline_layout = std::move(_pipeline_layout);
        return true;
    }

    bool ComputeShader::create_pipeline(const vk::PipelineCache pipeline_cache)
    {
        const vk::PipelineShaderStageCreateInfo stage_info
        {
            {},
            vk::ShaderStageFlagBits::eCompute,
            *shader_module,
            "main"
        };

        const vk::ComputePipelineCreateInfo pipeline_info
        {
            {},
            stage_info,
            *pipeline_layout
        };

        auto [result, _pipeline] = device->get().get().createComputePipelineUnique(pipeline_cache, pipeline_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create compute pipeline");
            ok = false;
            return false;
        }

        pipeline = std::move(_pipeline);
        return true;
    }

    bool ComputeShader::create_descriptor_pool_and_set(const std::vector<DescriptorBindingInfo>& descriptor_bindings)
    {
        std::unordered_map<vk::DescriptorType, uint32_t> type_counts;
        for (auto& b : descriptor_bindings)
            type_counts[b.descriptorType] += b.descriptorCount;

        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (auto& [descriptor, count] : type_counts)
            pool_sizes.emplace_back(descriptor, count);

        const vk::DescriptorPoolCreateInfo pool_info
        {
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1,
            static_cast<uint32_t>(pool_sizes.size()),
            pool_sizes.data()
        };

        auto [pool_result, _descriptor_pool] = device->get().get().createDescriptorPoolUnique(pool_info);
        if (pool_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create descriptor pool");
            ok = false;
            return false;
        }

        descriptor_pool = std::move(_descriptor_pool);

        const vk::DescriptorSetAllocateInfo alloc_info
        {
            *descriptor_pool,
            1, &descriptor_set_layout.get()
        };

        auto [set_result, _descriptor_set] = device->get().get().allocateDescriptorSetsUnique(alloc_info);

        if (set_result != vk::Result::eSuccess || _descriptor_set.size() != 1)
        {
            Logger::error("Failed to allocate descriptor set");
            ok = false;
            return false;
        }

        descriptor_set = std::move(_descriptor_set[0]);

        return true;
    }


    std::vector<uint32_t> ComputeShader::load_shader_binary(const std::string_view& path)
    {
        std::ifstream file(path.data(), std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            Logger::error("Failed to open file {}", path);
            return {};
        }

        const size_t          file_size = file.tellg();
        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));
        file.close();

        return buffer;
    }
}
