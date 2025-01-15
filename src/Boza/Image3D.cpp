#include "Image3D.hpp"

#include "Buffer.hpp"
#include "Logger.hpp"

namespace boza
{
    Image3D::Image3D(
        const Device&       device,
        CommandPool&  command_pool,
        vk::Format          format,
        vk::Extent3D        extent,
        vk::ImageUsageFlags usage,
        bool                create_view)
        : extent{ extent }, device{ std::cref(device) }, ok{ true }
    {
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

        auto [img_result, img] = device.get().createImageUnique(image_create_info);
        if (img_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create image");
            ok = false;
            return;
        }

        image = std::move(img);

        const vk::MemoryRequirements mem_reqs = device.get().getImageMemoryRequirements(image.get());

        const auto mem_props = device.get_physical_device().getMemoryProperties();
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

        if (mem_type == UINT32_MAX)
        {
            Logger::error("Failed to find suitable memory type for image");
            ok = false;
            return;
        }

        const vk::MemoryAllocateInfo alloc_info{ mem_reqs.size, mem_type };

        auto [mem_result, _memory] = device.get().allocateMemoryUnique(alloc_info);

        if (mem_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to allocate image memory");
            ok = false;
            return;
        }

        memory = std::move(_memory);

        if (device.get().bindImageMemory(*image, *memory, 0) != vk::Result::eSuccess)
        {
            Logger::error("Failed to bind image memory");
            ok = false;
            return;
        }

        if (create_view)
        {
            const vk::ImageViewCreateInfo view_info
            {
                {},
                *image,
                vk::ImageViewType::e3D,
                format,
                {},
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
            };

            auto [view_result, _image_view] = device.get().createImageViewUnique(view_info);
            if (view_result != vk::Result::eSuccess)
            {
                Logger::error("Failed to create image view");
                ok = false;
                return;
            }
            image_view = std::move(_image_view);
        }

        auto command_buffer_vec = command_pool.allocate_command_buffers(device, 1);

        if (command_buffer_vec.empty())
        {
            Logger::error("Failed to allocate command buffer");
            ok = false;
            return;
        }

        copy_buffer = std::move(command_buffer_vec[0]);
    }

    Image3D::Image3D(Image3D&& other) noexcept
    {
        image = std::move(other.image);
        memory = std::move(other.memory);
        image_view = std::move(other.image_view);
        copy_buffer = std::move(other.copy_buffer);

        extent = std::exchange(other.extent, {});
        ok = std::exchange(other.ok, false);

        if (other.device) device = std::cref(other.device->get());
        other.device = std::nullopt;
    }

    Image3D& Image3D::operator=(Image3D&& other) noexcept
    {
        if (this != &other)
        {
            image = std::move(other.image);
            memory = std::move(other.memory);
            image_view = std::move(other.image_view);
            copy_buffer = std::move(other.copy_buffer);

            extent = std::exchange(other.extent, {});
            ok = std::exchange(other.ok, false);

            if (other.device) device = std::cref(other.device->get());
            other.device = std::nullopt;
        }

        return *this;
    }

    std::vector<uint8_t> Image3D::get_data() const
    {
        const vk::DeviceSize total_size = device->get().get().getImageMemoryRequirements(*image).size;

        Buffer staging_buffer
        {
            device->get(),
            total_size,
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        };

        if (!staging_buffer)
        {
            Logger::error("Failed to create staging buffer");
            return {};
        }

        if (!staging_buffer.bind())
        {
            Logger::error("Failed to bind staging buffer memory");
            return {};
        }

        if (copy_buffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit }) != vk::Result::eSuccess)
        {
            Logger::error("Failed to begin copy command buffer");
            return {};
        }

        vk::BufferImageCopy region
        {
            0,
            0, 0,
            { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
            { 0, 0, 0 },
            extent
        };

        copy_buffer->copyImageToBuffer(
            *image,
            vk::ImageLayout::eTransferSrcOptimal,
            staging_buffer.get_buffer(),
            { region }
        );

        if (copy_buffer->end() != vk::Result::eSuccess)
        {
            Logger::error("Failed to end copy command buffer");
            return {};
        }

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copy_buffer.get();


        if (device->get().get_compute_queue().submit(1, &submitInfo, {}) != vk::Result::eSuccess)
        {
            Logger::error("Failed to submit copy command buffer");
            return {};
        }

        if (device->get().get_compute_queue().waitIdle() != vk::Result::eSuccess)
        {
            Logger::error("Failed to wait for copy command buffer");
            return {};
        }

        auto [result, mapped] = device->get().get().mapMemory(staging_buffer.get_memory(), 0, vk::WholeSize);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to map memory");
            return {};
        }

        std::vector<uint8_t> image_data(total_size);
        std::memcpy(image_data.data(), mapped, total_size);

        device->get().get().unmapMemory(staging_buffer.get_memory());
        return image_data;
    }
}
