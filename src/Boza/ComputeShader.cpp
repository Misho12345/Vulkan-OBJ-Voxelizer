#include "pch.hpp"
#include "ComputeShader.hpp"

#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    std::optional<ComputeShader> ComputeShader::create(
        const std::string&                        shaderPath,
        const std::vector<DescriptorBindingInfo>& descriptorBindings,
        const std::vector<PushConstantRange>&     pushConstantRanges,
        const vk::PipelineCache                         pipelineCache)
    {
        ComputeShader shader;

        BOZA_CHECK_RET_OPT(shader.create_shader_module(shaderPath), "Failed to create shader module");
        BOZA_CHECK_RET_OPT(shader.create_descriptor_set_layout(descriptorBindings), "Failed to create descriptor set layout");
        BOZA_CHECK_RET_OPT(shader.create_pipeline_layout(pushConstantRanges), "Failed to create pipeline layout");
        BOZA_CHECK_RET_OPT(shader.create_pipeline(pipelineCache), "Failed to create pipeline");
        BOZA_CHECK_RET_OPT(shader.create_descriptor_pool_and_set(descriptorBindings), "Failed to create descriptor pool and allocate descriptor set");

        uint32_t totalPushConstantSize = 0;
        for (auto& pc : pushConstantRanges)
            totalPushConstantSize = std::max(totalPushConstantSize, pc.offset + pc.size);

        shader.pushConstantBuffer.resize(totalPushConstantSize, 0);

        return shader;
    }


    ComputeShader::~ComputeShader()
    {
        if (descriptorPool)      Device::get_logical_device().destroyDescriptorPool(descriptorPool);
        if (pipeline)            Device::get_logical_device().destroyPipeline(pipeline);
        if (pipelineLayout)      Device::get_logical_device().destroyPipelineLayout(pipelineLayout);
        if (descriptorSetLayout) Device::get_logical_device().destroyDescriptorSetLayout(descriptorSetLayout);
        if (shaderModule)        Device::get_logical_device().destroyShaderModule(shaderModule);
    }

    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    {
        shaderModule = other.shaderModule;
        pipelineLayout = other.pipelineLayout;
        pipeline = other.pipeline;
        descriptorSetLayout = other.descriptorSetLayout;
        descriptorPool = other.descriptorPool;
        descriptorSet = other.descriptorSet;
        pushConstantBuffer = std::move(other.pushConstantBuffer);
        is_null = other.is_null;

        other.shaderModule = nullptr;
        other.pipelineLayout = nullptr;
        other.pipeline = nullptr;
        other.descriptorSetLayout = nullptr;
        other.descriptorPool = nullptr;
        other.descriptorSet = nullptr;
        other.is_null = true;
    }

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
    {
        if (this != &other)
        {
            shaderModule = other.shaderModule;
            pipelineLayout = other.pipelineLayout;
            pipeline = other.pipeline;
            descriptorSetLayout = other.descriptorSetLayout;
            descriptorPool = other.descriptorPool;
            descriptorSet = other.descriptorSet;
            pushConstantBuffer = std::move(other.pushConstantBuffer);
            is_null = other.is_null;

            other.shaderModule = nullptr;
            other.pipelineLayout = nullptr;
            other.pipeline = nullptr;
            other.descriptorSetLayout = nullptr;
            other.descriptorPool = nullptr;
            other.descriptorSet = nullptr;
            other.is_null = true;
        }

        return *this;
    }

    bool ComputeShader::create_shader_module(const std::string& shaderPath)
    {
        namespace fs = std::filesystem;
        const auto code = load_shader_binary((fs::current_path() / "shaders" / "spv" / (shaderPath + ".spv")).string());
        if (code.empty()) return false;

        const vk::ShaderModuleCreateInfo createInfo
        {
            {},
            code.size() * sizeof(uint32_t),
            code.data()
        };

        vk::Result result;
        std::tie(result, shaderModule) = Device::get_logical_device().createShaderModule(createInfo);
        VK_CHECK(result, "Failed to create shader module");

        return shaderModule != nullptr;
    }

    bool ComputeShader::create_descriptor_set_layout(const std::vector<DescriptorBindingInfo>& descriptorBindings)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.reserve(descriptorBindings.size());

        for (const auto& [binding, descriptorType, stageFlags, descriptorCount] : descriptorBindings)
        {
            vk::DescriptorSetLayoutBinding dsl_binding(binding, descriptorType, descriptorCount, stageFlags);
            bindings.push_back(dsl_binding);
        }

        const vk::DescriptorSetLayoutCreateInfo layoutInfo
        {
            {},
            static_cast<uint32_t>(bindings.size()),
            bindings.data()
        };

        vk::Result result;
        std::tie(result, descriptorSetLayout) = Device::get_logical_device().createDescriptorSetLayout(layoutInfo);
        VK_CHECK(result, "Failed to create descriptor set layout");

        return descriptorSetLayout != nullptr;
    }

    bool ComputeShader::create_pipeline_layout(const std::vector<PushConstantRange>& pushConstantRanges)
    {
        std::vector<vk::PushConstantRange> vkPushRanges;
        vkPushRanges.reserve(pushConstantRanges.size());
        for (const auto& [stageFlags, offset, size] : pushConstantRanges)
        {
            vk::PushConstantRange range(stageFlags, offset, size);
            vkPushRanges.push_back(range);
        }

        const vk::PipelineLayoutCreateInfo pipelineLayoutInfo
        {
            {},
            1, &descriptorSetLayout,
            static_cast<uint32_t>(vkPushRanges.size()), vkPushRanges.data()
        };

        vk::Result result;
        std::tie(result, pipelineLayout) = Device::get_logical_device().createPipelineLayout(pipelineLayoutInfo);
        VK_CHECK(result, "Failed to create pipeline layout");

        return pipelineLayout != nullptr;
    }

    bool ComputeShader::create_pipeline(const vk::PipelineCache pipelineCache)
    {
        const vk::PipelineShaderStageCreateInfo stageInfo
        {
            {},
            vk::ShaderStageFlagBits::eCompute,
            shaderModule,
            "main"
        };

        const vk::ComputePipelineCreateInfo pipelineInfo
        {
            {},
            stageInfo,
            pipelineLayout
        };

        vk::Result result;
        std::tie(result, pipeline) = Device::get_logical_device().createComputePipeline(pipelineCache, pipelineInfo);
        VK_CHECK(result, "Failed to create compute pipeline");

        return true;
    }

    bool ComputeShader::create_descriptor_pool_and_set(const std::vector<DescriptorBindingInfo>& descriptorBindings)
    {
        std::unordered_map<vk::DescriptorType, uint32_t> typeCounts;
        for (auto& b : descriptorBindings)
            typeCounts[b.descriptorType] += b.descriptorCount;

        std::vector<vk::DescriptorPoolSize> poolSizes;
        for (auto& [descriptor, count] : typeCounts)
            poolSizes.emplace_back(descriptor, count);

        const vk::DescriptorPoolCreateInfo poolInfo
        {
            {},
            1,
            static_cast<uint32_t>(poolSizes.size()),
            poolSizes.data()
        };

        vk::Result result;
        std::tie(result, descriptorPool) = Device::get_logical_device().createDescriptorPool(poolInfo);
        VK_CHECK(result, "Failed to create descriptor pool");

        if (!descriptorPool) return false;

        const vk::DescriptorSetAllocateInfo allocInfo
        {
            descriptorPool,
            1, &descriptorSetLayout
        };

        std::vector<vk::DescriptorSet> temp;
        std::tie(result, temp) = Device::get_logical_device().allocateDescriptorSets(allocInfo);

        if (temp.size() != 1) return false;

        descriptorSet = temp[0];
        return descriptorSet != nullptr;
    }

    std::vector<uint32_t> ComputeShader::load_shader_binary(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return {};

        const size_t fileSize = file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
        file.close();

        return buffer;
    }

    void ComputeShader::update_storage_image(const uint32_t binding, const vk::ImageView imageView, const vk::ImageLayout layout) const
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to update storage image: shader is null");

        const vk::DescriptorImageInfo imageInfo
        {
            {},
            imageView, layout
        };

        vk::WriteDescriptorSet write
        {
            descriptorSet,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &imageInfo,
            nullptr,
            nullptr
        };

        Device::get_logical_device().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_storage_buffer(const uint32_t binding, const vk::Buffer buffer, const vk::DeviceSize range, const vk::DeviceSize offset) const
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to update storage buffer: shader is null");

        const vk::DescriptorBufferInfo bufferInfo{ buffer, offset, range };

        vk::WriteDescriptorSet write
        {
            descriptorSet,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &bufferInfo,
            nullptr
        };

        Device::get_logical_device().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_uniform_buffer(const uint32_t binding, const vk::Buffer buffer, const vk::DeviceSize range, const vk::DeviceSize offset) const
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to update uniform buffer: shader is null");

        const vk::DescriptorBufferInfo bufferInfo{ buffer, offset, range };

        vk::WriteDescriptorSet write
        {
            descriptorSet,
            binding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &bufferInfo,
            nullptr
        };

        Device::get_logical_device().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::update_sampled_image(const uint32_t binding, const vk::ImageView imageView, const vk::Sampler sampler, const vk::ImageLayout layout) const
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to update sampled image: shader is null");

        const vk::DescriptorImageInfo imageInfo{ sampler, imageView, layout };

        vk::WriteDescriptorSet write
        {
            descriptorSet,
            binding,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &imageInfo,
            nullptr,
            nullptr
        };

        Device::get_logical_device().updateDescriptorSets({ write }, {});
    }

    void ComputeShader::dispatch(const vk::CommandBuffer commandBuffer, const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to dispatch compute shader: shader is null");

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSet }, {});

        if (!pushConstantBuffer.empty())
            commandBuffer.pushConstants<uint8_t>(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushConstantBuffer);

        commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
    }
}
