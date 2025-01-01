#include "pch.hpp"
#include "Image3D.hpp"

#include "Buffer.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    Image3D::Image3D(Image3D&& other) noexcept
    {
        image       = other.image;
        memory      = other.memory;
        image_view  = other.image_view;
        copy_buffer = other.copy_buffer;
        extent      = other.extent;
        is_null     = other.is_null;

        other.image       = nullptr;
        other.memory      = nullptr;
        other.image_view  = nullptr;
        other.copy_buffer = nullptr;
        other.is_null     = true;
    }

    Image3D& Image3D::operator=(Image3D&& other) noexcept
    {
        if (this != &other)
        {
            image       = other.image;
            memory      = other.memory;
            image_view  = other.image_view;
            copy_buffer = other.copy_buffer;
            extent      = other.extent;
            is_null     = other.is_null;

            other.image       = nullptr;
            other.memory      = nullptr;
            other.image_view  = nullptr;
            other.copy_buffer = nullptr;
            other.is_null     = true;
        }

        return *this;
    }

    std::optional<Image3D> Image3D::create(
        const vk::Format          format,
        const vk::Extent3D        extent,
        const vk::ImageUsageFlags usage,
        const bool                createView)
    {
        Image3D img;

        img.extent = extent;

        const vk::ImageCreateInfo image_create_info
        {
            {},
            vk::ImageType::e3D,
            format,
            extent,
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            usage
        };

        vk::Result result;
        std::tie(result, img.image) = Device::get_logical_device().createImage(image_create_info);
        VK_CHECK_RET_OPT(result, "Failed to create image");

        const vk::MemoryRequirements mem_reqs = Device::get_logical_device().getImageMemoryRequirements(img.image);

        const auto mem_props = Device::get_physical_device().getMemoryProperties();
        uint32_t   mem_type  = UINT32_MAX;
        for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
        {
            if ((mem_reqs.memoryTypeBits & (1 << i)) &&
                (mem_props.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
            {
                mem_type = i;
                break;
            }
        }

        BOZA_CHECK_RET_OPT(mem_type != UINT32_MAX, "Failed to find suitable memory type for image");

        const vk::MemoryAllocateInfo alloc_info{ mem_reqs.size, mem_type };
        std::tie(result, img.memory) = Device::get_logical_device().allocateMemory(alloc_info);
        VK_CHECK_RET_OPT(result, "Failed to allocate image memory");

        VK_CHECK_RET_OPT(Device::get_logical_device().bindImageMemory(img.image, img.memory, 0),
                         "Failed to bind image memory");

        if (createView)
        {
            const vk::ImageViewCreateInfo view_info
            {
                {},
                img.image,
                vk::ImageViewType::e3D,
                format,
                {},
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
            };

            std::tie(result, img.image_view) = Device::get_logical_device().createImageView(view_info);
            VK_CHECK_RET_OPT(result, "Failed to create image view");
        }

        const auto command_buf_opt = CommandPool::allocate_command_buffer();
        if (!command_buf_opt.has_value()) return std::nullopt;
        img.copy_buffer = command_buf_opt.value();

        return img;
    }

    void Image3D::destroy() const
    {
        if (image_view) Device::get_logical_device().destroyImageView(image_view);
        if (image) Device::get_logical_device().destroyImage(image);
        if (memory) Device::get_logical_device().freeMemory(memory);
    }

    std::vector<uint8_t> Image3D::get_data() const
    {
        BOZA_CHECK_RET_CUSTOM(!is_null, {}, "Failed to get image data: image is null");

        const vk::DeviceSize total_size = Device::get_logical_device().getImageMemoryRequirements(image).size;

        const auto staging_buffer_opt = Buffer::create(
            total_size,
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        BOZA_CHECK_RET_CUSTOM(staging_buffer_opt.has_value(), {}, "Failed to create staging buffer");

        auto&& staging_buffer = *staging_buffer_opt;

        BOZA_CHECK_RET_CUSTOM(staging_buffer.bind(), {}, "Failed to bind staging buffer memory");
        VK_CHECK_RET_CUSTOM(copy_buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit }), {}, "Failed to begin copy command buffer");

        vk::BufferImageCopy region
        {
            0,
            0, 0,
            { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
            { 0, 0, 0 },
            extent
        };

        copy_buffer.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, staging_buffer.get_buffer(), { region });
        VK_CHECK_RET_CUSTOM(copy_buffer.end(), {}, "Failed to end copy command buffer");

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copy_buffer;

        VK_CHECK_RET_CUSTOM(
        Device::get_compute_queue().submit(1, &submitInfo, {}), {}, "Failed to submit copy command buffer");
        VK_CHECK_RET_CUSTOM(Device::get_compute_queue().waitIdle(), {}, "Failed to wait for copy command buffer");

        auto [result, mapped] = Device::get_logical_device().mapMemory(staging_buffer.get_memory(), 0, vk::WholeSize);
        VK_CHECK_RET_CUSTOM(result, {}, "Failed to map memory");

        std::vector<uint8_t> image_data(total_size);
        std::memcpy(image_data.data(), mapped, total_size);

        Device::get_logical_device().unmapMemory(staging_buffer.get_memory());
        staging_buffer.destroy();

        return image_data;
    }
}
