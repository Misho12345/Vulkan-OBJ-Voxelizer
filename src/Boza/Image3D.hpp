#pragma once
#include "CommandPool.hpp"
#include "pch.hpp"
#include "Device.hpp"

namespace boza
{
    class Image3D final
    {
    public:
        Image3D(nullptr_t) {}

        Image3D(const Device&       device,
                CommandPool&        command_pool,
                vk::Format          format,
                vk::Extent3D        extent,
                vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
                bool                create_view = true);

        Image3D(const Image3D&)            = delete;
        Image3D& operator=(const Image3D&) = delete;

        Image3D(Image3D&& other) noexcept;
        Image3D& operator=(Image3D&& other) noexcept;

        operator bool () const { return ok; }

        [[nodiscard]]
        std::vector<uint8_t> get_data() const;

        [[nodiscard]] const vk::Image&        get_image() const { return *image; }
        [[nodiscard]] const vk::DeviceMemory& get_memory() const { return *memory; }
        [[nodiscard]] const vk::ImageView&    get_image_view() const { return *image_view; }

    private:
        vk::UniqueImage         image{ nullptr };
        vk::UniqueDeviceMemory  memory{ nullptr };
        vk::UniqueImageView     image_view{ nullptr };
        vk::UniqueCommandBuffer copy_buffer{ nullptr };

        vk::Extent3D extent{};

        std::optional<std::reference_wrapper<const Device>> device{ std::nullopt };

        bool ok = false;
    };
}
