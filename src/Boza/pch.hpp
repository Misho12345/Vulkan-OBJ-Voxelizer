#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/ext.hpp>


#include <iostream>
#include <format>

#include <span>
#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <filesystem>

#include <unordered_set>

#include <cassert>
#include <ctime>

#include "check.hpp"

#ifdef NDEBUG
#define DEBUG_ONLY(X) (void)0
#else
#define DEBUG_ONLY(X) X
#endif

#define GETTER(name) [[nodiscard]] auto get_##name() const { return name; }
#define GETTER_CREF(name) [[nodiscard]] const auto& get_##name() const { return name; }
#define STATIC_GETTER(name) [[nodiscard]] static auto get_##name() { return name; }
#define STATIC_GETTER_CREF(name) [[nodiscard]] static const auto& get_##name() { return name; }

#define DELETE_CONSTRUCTORS(class_name)                \
    class_name()                             = delete; \
    class_name(const class_name&)            = delete; \
    class_name(class_name&&)                 = delete; \
    class_name& operator=(const class_name&) = delete; \
    class_name& operator=(class_name&&)      = delete; \
    ~class_name()                            = delete
