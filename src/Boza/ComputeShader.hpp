#pragma once
#include "Device.hpp"
#include "pch.hpp"

namespace boza
{
    class ComputeShader final
    {
    public:
        struct DescriptorBindingInfo final
        {
            uint32_t             binding;
            vk::DescriptorType   descriptorType;
            vk::ShaderStageFlags stageFlags;
            uint32_t             descriptorCount = 1;
        };

        struct PushConstantRange final
        {
            vk::ShaderStageFlags stageFlags;
            uint32_t             offset;
            uint32_t             size;
        };

        ComputeShader(nullptr_t) {}

        ComputeShader(
            const Device&                             device,
            const std::string_view&                   shader_path,
            const std::vector<DescriptorBindingInfo>& descriptor_bindings,
            const std::vector<PushConstantRange>&     push_constant_ranges = {},
            vk::PipelineCache                         pipeline_cache       = nullptr);

        ComputeShader(const ComputeShader&)            = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;

        ComputeShader(ComputeShader&& other) noexcept;
        ComputeShader& operator=(ComputeShader&& other) noexcept;

        operator bool() const { return ok; }

        void update_storage_image(uint32_t binding, vk::ImageView image_view, vk::ImageLayout layout) const;
        void update_storage_buffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize range = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;
        void update_uniform_buffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize range = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;
        void update_sampled_image(uint32_t binding, vk::ImageView image_view, vk::Sampler sampler, vk::ImageLayout layout) const;

        template <typename T>
        void set_push_constant(const T& data, uint32_t offset = 0);

        void dispatch(vk::CommandBuffer command_buffer, uint32_t group_count_x, uint32_t group_count_y = 1,
                      uint32_t          group_count_z                                                  = 1);

        [[nodiscard]] const vk::Pipeline&       get_pipeline() const { return *pipeline; }
        [[nodiscard]] const vk::PipelineLayout& get_pipeline_layout() const { return *pipeline_layout; }
        [[nodiscard]] const vk::DescriptorSet&  get_descriptor_set() const { return *descriptor_set; }

    private:
        [[nodiscard]] bool create_shader_module(const std::string_view& shader_path);
        [[nodiscard]] bool create_descriptor_set_layout(const std::vector<DescriptorBindingInfo>& descriptor_bindings);

        [[nodiscard]] bool create_pipeline_layout(const std::vector<PushConstantRange>& push_constant_ranges);
        [[nodiscard]] bool create_pipeline(vk::PipelineCache pipeline_cache);
        [[nodiscard]] bool create_descriptor_pool_and_set(const std::vector<DescriptorBindingInfo>& descriptor_bindings);

        [[nodiscard]]
        static std::vector<uint32_t> load_shader_binary(const std::string_view& path);

        vk::UniqueShaderModule        shader_module;
        vk::UniqueDescriptorSetLayout descriptor_set_layout;
        vk::UniquePipelineLayout      pipeline_layout;
        vk::UniquePipeline            pipeline;
        vk::UniqueDescriptorPool      descriptor_pool;
        vk::UniqueDescriptorSet       descriptor_set;

        std::vector<uint8_t> push_constant_buffer;

        std::optional<std::reference_wrapper<const Device>> device;

        bool ok = false;
    };

    template <typename T>
    void ComputeShader::set_push_constant(const T& data, const uint32_t offset)
    {
        const auto raw_data = reinterpret_cast<const uint8_t*>(&data);
        if (offset + sizeof(T) <= push_constant_buffer.size())
            std::memcpy(push_constant_buffer.data() + offset, raw_data, sizeof(T));
    }
}
