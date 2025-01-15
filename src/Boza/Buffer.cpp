#include "Buffer.hpp"

#include "Logger.hpp"

namespace boza
{
    Buffer::Buffer(
        const Device&                 device,
        const vk::DeviceSize          size,
        const vk::BufferUsageFlags    usage,
        const vk::MemoryPropertyFlags properties)
        : device{ std::cref(device) }, ok{ true }
    {
        const vk::BufferCreateInfo buffer_info
        {
            {},
            size,
            usage,
            vk::SharingMode::eExclusive,
            device.get_compute_queue_family_index()
        };

        auto [buf_result, _buffer] = device.get().createBufferUnique(buffer_info);
        if (buf_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create buffer");
            ok = false;
            return;
        }

        buffer = std::move(_buffer);

        const uint32_t memory_type = [&]
        {
            const auto memory_properties = device.get_physical_device().getMemoryProperties();
            for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
            {
                if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }
            return UINT32_MAX;
        }();

        if (memory_type == UINT32_MAX)
        {
            Logger::error("Failed to find suitable memory type");
            ok = false;
            return;
        }

        const auto memory_requirements = device.get().getBufferMemoryRequirements(buffer.get());

        const vk::MemoryAllocateInfo allocate_info
        {
            memory_requirements.size,
            memory_type
        };

        auto [mem_result, _memory] = device.get().allocateMemoryUnique(allocate_info);
        if (mem_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to allocate buffer memory");
            ok = false;
            return;
        }

        memory = std::move(_memory);
        this->size = size;
    }

    Buffer Buffer::uniform_buffer(const Device& device, const vk::DeviceSize size)
    {
        return Buffer {
            device,
            size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        };
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        buffer = std::move(other.buffer);
        memory = std::move(other.memory);
        size   = std::exchange(other.size, 0);
        ok     = std::exchange(other.ok, false);

        if (other.device) device = std::cref(other.device->get());
        other.device = std::nullopt;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            buffer = std::move(other.buffer);
            memory = std::move(other.memory);
            size   = std::exchange(other.size, 0);
            ok     = std::exchange(other.ok, false);

            if (other.device) device = std::cref(other.device->get());
            other.device = std::nullopt;
        }

        return *this;
    }


    bool Buffer::copy_data(const void* data, const vk::DeviceSize size)
    {
        auto [result, dest] = device->get().get().mapMemory(*memory, 0, size, {});
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to map buffer memory");
            ok = false;
            return false;
        }

        std::memcpy(dest, data, size);
        device->get().get().unmapMemory(*memory);

        return true;
    }

    bool Buffer::bind()
    {
        if (device->get().get().bindBufferMemory(*buffer, *memory, 0) != vk::Result::eSuccess)
        {
            Logger::error("Failed to bind buffer memory");
            ok = false;
            return false;
        }

        return true;
    }

    bool Buffer::update_uniform(const void* data, const vk::DeviceSize size)
    {
        auto [result, mapped_memory] = device->get().get().mapMemory(*memory, 0, size, {});
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to map buffer memory");
            ok = false;
            return false;
        }

        std::memcpy(mapped_memory, data, size);
        device->get().get().unmapMemory(*memory);

        return true;
    }
}