#include "CommandPool.hpp"

#include "Logger.hpp"

namespace boza
{
    CommandPool::CommandPool(const Device& device) : ok{ true }
    {
        Logger::trace("Creating command pool");

        if (!create_command_pool(device)) return;

        auto cmd_buffs = allocate_command_buffers(device, 1);
        if (cmd_buffs.empty())
        {
            ok = false;
            return;
        }

        command_buffer = std::move(cmd_buffs[0]);
    }

    CommandPool::~CommandPool()
    {
        if (ok) Logger::trace("Destroying command pool");
    }


    CommandPool::CommandPool(CommandPool&& other) noexcept
    {
        command_buffer = std::move(other.command_buffer);
        command_pool = std::move(other.command_pool);
        ok = std::exchange(other.ok, false);
    }

    CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
    {
        if (this != &other)
        {
            command_buffer = std::move(other.command_buffer);
            command_pool = std::move(other.command_pool);
            ok = std::exchange(other.ok, false);
        }

        return *this;
    }


    std::vector<vk::UniqueCommandBuffer> CommandPool::allocate_command_buffers(const Device& device, const uint32_t count)
    {
        const vk::CommandBufferAllocateInfo alloc_info
        {
            *command_pool,
            vk::CommandBufferLevel::ePrimary,
            count
        };

        auto [result, buffers] = device.get().allocateCommandBuffersUnique(alloc_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to allocate {} command buffer{}", count, count > 1 ? "s" : "");
            return {};
        }

        return std::move(buffers);
    }

    bool CommandPool::create_command_pool(const Device& device)
    {
        const vk::CommandPoolCreateInfo pool_info
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            device.get_compute_queue_family_index()
        };

        auto [result, _command_pool] = device.get().createCommandPoolUnique(pool_info);

        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create command pool");
            ok = false;
            return false;
        }

        command_pool = std::move(_command_pool);
        return true;
    }
}
