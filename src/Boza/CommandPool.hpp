#pragma once
#include "pch.hpp"
#include "Device.hpp"

namespace boza
{
    class CommandPool final
    {
    public:
        CommandPool(nullptr_t) {}
        explicit CommandPool(const Device& device);
        ~CommandPool();

        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool(CommandPool&& other) noexcept;
        CommandPool& operator=(CommandPool&& other) noexcept;

        operator bool () const noexcept { return ok; }

        [[nodiscard]]
        std::vector<vk::UniqueCommandBuffer> allocate_command_buffers(const Device& device, uint32_t count);

        [[nodiscard]] const vk::CommandPool&   get() const { return *command_pool; }
        [[nodiscard]] const vk::CommandBuffer& get_command_buffer() const { return *command_buffer; }

    private:
        bool create_command_pool(const Device& device);

        vk::UniqueCommandPool command_pool;
        vk::UniqueCommandBuffer command_buffer;

        bool ok = false;
    };
}
