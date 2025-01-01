#pragma once
#include "pch.hpp"
#include "Logger.hpp"

namespace boza
{
    class ComputeShader
    {
    public:
        struct DescriptorBindingInfo
        {
            uint32_t             binding;
            vk::DescriptorType   descriptorType;
            vk::ShaderStageFlags stageFlags;
            uint32_t             descriptorCount = 1;
        };

        struct PushConstantRange
        {
            vk::ShaderStageFlags stageFlags;
            uint32_t             offset;
            uint32_t             size;
        };

        ComputeShader(std::nullptr_t) : is_null(true) {}

        static std::optional<ComputeShader> create(
            const std::string&                        shaderPath,
            const std::vector<DescriptorBindingInfo>& descriptorBindings,
            const std::vector<PushConstantRange>&     pushConstantRanges = {},
            vk::PipelineCache                         pipelineCache      = nullptr);

        ~ComputeShader();

        ComputeShader(const ComputeShader&)            = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;
        ComputeShader(ComputeShader&&) noexcept;
        ComputeShader& operator=(ComputeShader&& other) noexcept;

        void update_storage_image(uint32_t binding, vk::ImageView imageView, vk::ImageLayout layout) const;
        void update_storage_buffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize range = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;
        void update_uniform_buffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize range = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;
        void update_sampled_image(uint32_t binding, vk::ImageView imageView, vk::Sampler sampler, vk::ImageLayout layout) const;

        template <typename T>
        void set_push_constant(const T& data, uint32_t offset = 0);

        void dispatch(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

        vk::Pipeline       get_pipeline() const { return pipeline; }
        vk::PipelineLayout get_pipeline_layout() const { return pipelineLayout; }
        vk::DescriptorSet  get_descriptor_set() const { return descriptorSet; }

    private:
        ComputeShader() = default;

        bool is_null{ false };

        vk::ShaderModule        shaderModule;
        vk::PipelineLayout      pipelineLayout;
        vk::Pipeline            pipeline;
        vk::DescriptorSetLayout descriptorSetLayout;
        vk::DescriptorPool      descriptorPool;
        vk::DescriptorSet       descriptorSet;

        std::vector<uint8_t> pushConstantBuffer;

        bool create_shader_module(const std::string& shaderPath);
        bool create_descriptor_set_layout(const std::vector<DescriptorBindingInfo>& descriptorBindings);

        bool create_pipeline_layout(const std::vector<PushConstantRange>& pushConstantRanges);
        bool create_pipeline(vk::PipelineCache pipelineCache);
        bool create_descriptor_pool_and_set(const std::vector<DescriptorBindingInfo>& descriptorBindings);

        static std::vector<uint32_t> load_shader_binary(const std::string& path);
    };

    template <typename T>
    void ComputeShader::set_push_constant(const T& data, const uint32_t offset)
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, , "Failed to set push constant: shader is null");

        const auto rawData = reinterpret_cast<const uint8_t*>(&data);
        if (offset + sizeof(T) <= pushConstantBuffer.size())
            std::memcpy(pushConstantBuffer.data() + offset, rawData, sizeof(T));
    }
}
