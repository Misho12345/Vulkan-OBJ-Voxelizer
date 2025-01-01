#include "pch.hpp"
#include "Buffer.hpp"

#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    std::optional<Buffer> Buffer::create(
        const vk::DeviceSize          size,
        const vk::BufferUsageFlags    usage,
        const vk::MemoryPropertyFlags properties)
    {
        Buffer buffer;

        const vk::BufferCreateInfo buffer_info
        {
            {},
            size,
            usage,
            vk::SharingMode::eExclusive,
            1, &Device::get_compute_queue_family_index()
        };

        vk::Result result;
        std::tie(result, buffer.buffer) = Device::get_logical_device().createBuffer(buffer_info);
        VK_CHECK_RET_OPT(result, "Failed to create buffer");

        const uint32_t memory_type = [&properties]
        {
            const auto memory_properties = Device::get_physical_device().getMemoryProperties();
            for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
            {
                if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }
            return UINT32_MAX;
        }();

        BOZA_CHECK_RET_OPT(memory_type != UINT32_MAX, "Failed to find suitable memory type");

        const auto memory_requirements = Device::get_logical_device().getBufferMemoryRequirements(buffer.buffer);

        const vk::MemoryAllocateInfo allocate_info
        {
            memory_requirements.size,
            memory_type
        };

        std::tie(result, buffer.memory) = Device::get_logical_device().allocateMemory(allocate_info);
        VK_CHECK_RET_OPT(result, "Failed to allocate buffer memory");

        return buffer;
    }

    void Buffer::destroy() const
    {
        if (buffer) Device::get_logical_device().destroyBuffer(buffer);
        if (memory) Device::get_logical_device().freeMemory(memory);
    }


    Buffer::Buffer(Buffer&& other) noexcept
    {
        buffer  = other.buffer;
        memory  = other.memory;
        size    = other.size;
        is_null = other.is_null;

        other.buffer  = nullptr;
        other.memory  = nullptr;
        other.size    = 0;
        other.is_null = true;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            buffer  = other.buffer;
            memory  = other.memory;
            size    = other.size;
            is_null = other.is_null;

            other.buffer  = nullptr;
            other.memory  = nullptr;
            other.size    = 0;
            other.is_null = true;
        }

        return *this;
    }

    std::optional<Buffer> Buffer::create_uniform_buffer(const vk::DeviceSize size)
    {
        return create(size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    bool Buffer::copy_data(const void* data, const vk::DeviceSize size) const
    {
        BOZA_CHECK(!is_null, "Failed to copy data to buffer: buffer is null");

        const vk::Device& device = Device::get_logical_device();

        auto [result, dest] = device.mapMemory(memory, 0, size, {});
        VK_CHECK(result, "Failed to map buffer memory");

        std::memcpy(dest, data, size);
        device.unmapMemory(memory);

        return true;
    }

    bool Buffer::bind() const
    {
        BOZA_CHECK(!is_null, "Failed to bind buffer: buffer is null");
        VK_CHECK(Device::get_logical_device().bindBufferMemory(buffer, memory, 0), "Failed to bind buffer memory");
        return true;
    }

    bool Buffer::update_uniform(const void* data, const vk::DeviceSize size) const
    {
        BOZA_CHECK(!is_null, "Failed to update uniform buffer: buffer is null");

        auto [result, mappedMemory] = Device::get_logical_device().mapMemory(memory, 0, size, {});
        VK_CHECK(result, "Failed to map uniform buffer memory");

        std::memcpy(mappedMemory, data, size);

        Device::get_logical_device().unmapMemory(memory);
        return true;
    }
}
