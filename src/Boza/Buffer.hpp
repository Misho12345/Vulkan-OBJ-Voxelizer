#pragma once
#include "pch.hpp"
#include "Device.hpp"

namespace boza
{
    class Buffer final
    {
    public:
        Buffer(nullptr_t) {}

        Buffer(const Device&           device,
               vk::DeviceSize          size,
               vk::BufferUsageFlags    usage,
               vk::MemoryPropertyFlags properties);

        static Buffer uniform_buffer(const Device& device, vk::DeviceSize size);

        Buffer(const Buffer&)            = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        operator bool () const noexcept { return ok; }


        [[nodiscard]] bool copy_data(const void* data, vk::DeviceSize size);
        [[nodiscard]] bool bind();

        [[nodiscard]]
        bool update_uniform(const void* data, vk::DeviceSize size);

        [[nodiscard]] const vk::Buffer&       get_buffer() const { return *buffer; }
        [[nodiscard]] const vk::DeviceMemory& get_memory() const { return *memory; }

    private:
        vk::UniqueBuffer       buffer{ nullptr };
        vk::UniqueDeviceMemory memory{ nullptr };
        vk::DeviceSize         size{};

        std::optional<std::reference_wrapper<const Device>> device { std::nullopt };

        bool ok = false;
    };
}
