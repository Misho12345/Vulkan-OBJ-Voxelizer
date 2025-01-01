#pragma once
#include "pch.hpp"

namespace boza
{
    class Buffer final
    {
    public:
        Buffer(std::nullptr_t) : is_null(true) {}

        Buffer(const Buffer&)            = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        [[nodiscard]]
        static std::optional<Buffer> create(
            vk::DeviceSize          size,
            vk::BufferUsageFlags    usage,
            vk::MemoryPropertyFlags properties);

        void destroy() const;

        [[nodiscard]]
        static std::optional<Buffer> create_uniform_buffer(vk::DeviceSize size);

        [[nodiscard]] bool copy_data(const void* data, vk::DeviceSize size) const;
        [[nodiscard]] bool bind() const;

        bool update_uniform(const void* data, vk::DeviceSize size) const;

        GETTER_CREF(buffer);
        GETTER_CREF(memory);

    private:
        Buffer() = default;

        bool is_null{ false };

        vk::Buffer       buffer{ nullptr };
        vk::DeviceMemory memory{ nullptr };
        vk::DeviceSize   size{};
    };
}
