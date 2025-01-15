#pragma once
#include "Instance.hpp"
#include "pch.hpp"

namespace boza
{
    class Device final
    {
    public:
        Device(nullptr_t) {}
        explicit Device(const Instance& instance);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        Device(Device&& other) noexcept;
        Device& operator=(Device&& other) noexcept;

        operator bool() const { return ok; }

        [[nodiscard]] const vk::Device&         get() const { return *logical_device; }
        [[nodiscard]] const vk::PhysicalDevice& get_physical_device() const { return physical_device; }
        [[nodiscard]] const vk::Queue&          get_compute_queue() const { return compute_queue; }

        [[nodiscard]] uint32_t get_compute_queue_family_index() const { return compute_queue_family_index; }

    private:
        [[nodiscard]] bool choose_physical_device(const Instance& instance);
        [[nodiscard]] bool create_logical_device();

        vk::PhysicalDevice physical_device;
        vk::UniqueDevice   logical_device;

        vk::Queue compute_queue;
        uint32_t  compute_queue_family_index{};

        bool ok = false;
    };
}
