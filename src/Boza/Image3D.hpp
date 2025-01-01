#pragma once
#include "pch.hpp"

namespace boza
{
    class Image3D final
    {
    public:
        Image3D(std::nullptr_t) : is_null(true) {}

        Image3D(const Image3D&)            = delete;
        Image3D& operator=(const Image3D&) = delete;

        Image3D(Image3D&& other) noexcept;
        Image3D& operator=(Image3D&& other) noexcept;

        static std::optional<Image3D> create(
            vk::Format          format,
            vk::Extent3D        extent,
            vk::ImageUsageFlags usage      = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            bool                createView = true
        );

        void destroy() const;

        std::vector<uint8_t> get_data() const;

        GETTER_CREF(image);
        GETTER_CREF(memory);
        GETTER_CREF(image_view);

    private:
        Image3D() = default;

        bool is_null{ false };

        vk::Image         image{ nullptr };
        vk::DeviceMemory  memory{ nullptr };
        vk::ImageView     image_view{ nullptr };
        vk::CommandBuffer copy_buffer{ nullptr };

        vk::Extent3D extent;
    };
}
