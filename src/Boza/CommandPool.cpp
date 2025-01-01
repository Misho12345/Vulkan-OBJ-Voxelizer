#include "pch.hpp"
#include "CommandPool.hpp"

#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    static bool created = false;

    bool CommandPool::create()
    {
        assert(!created);
        Logger::trace("Creating command pool");
        created = true;

        BOZA_CHECK(create_command_pool(), "Failed to create command pool");
        BOZA_CHECK(create_command_buffer(), "Failed to create command buffer");

        return true;
    }

    void CommandPool::destroy()
    {
        assert(created);
        Logger::trace("Destroying command pool");
        created = false;

        Device::get_logical_device().destroyCommandPool(command_pool);
    }

    std::optional<vk::CommandBuffer> CommandPool::allocate_command_buffer()
    {
        auto res = allocate_command_buffers(1);
        if (res.empty()) return std::nullopt;
        return res[0];
    }

    std::vector<vk::CommandBuffer> CommandPool::allocate_command_buffers(const uint32_t count)
    {
        const vk::CommandBufferAllocateInfo alloc_info
        {
            command_pool,
            vk::CommandBufferLevel::ePrimary,
            count
        };

        auto [result, buffers] = Device::get_logical_device().allocateCommandBuffers(alloc_info);
        VK_CHECK_RET_CUSTOM(result, {}, "Failed to allocate command buffer");

        return buffers;
    }

    bool CommandPool::create_command_pool()
    {
        const vk::CommandPoolCreateInfo pool_info
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            Device::get_compute_queue_family_index()
        };

        vk::Result result;
        std::tie(result, command_pool) = Device::get_logical_device().createCommandPool(pool_info);
        VK_CHECK(result, "Failed to create command pool");

        return true;
    }

    bool CommandPool::create_command_buffer()
    {
        const auto buffer_opt = allocate_command_buffer();
        if (!buffer_opt.has_value()) return false;
        command_buffer = buffer_opt.value();
        return true;
    }
}
