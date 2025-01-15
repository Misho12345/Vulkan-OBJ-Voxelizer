cmake_minimum_required(VERSION 3.29)
project(vk_test)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

set(SHADER_COMPILE_SCRIPT "${CMAKE_SOURCE_DIR}/shaders/compile.sh")

set(DEBUG_FLAGS "${COMMON_FLAGS} -Wall -Wextra -Wconversion -pedantic -g -flto")
set(RELEASE_FLAGS "${COMMON_FLAGS} -O3 -flto")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${DEBUG_FLAGS}")
    foreach (dir ${vcpkg_INCLUDE_DIRS})
        add_compile_options("-fsanitize-ignorelist=${dir}")
    endforeach ()
    add_compile_options("-fsanitize=address,undefined")
    add_link_options("-fsanitize=address,undefined")
else ()
    set(CMAKE_CXX_FLAGS "${RELEASE_FLAGS}")
endif ()


if (DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
endif ()

find_package(Vulkan REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Boza/pch.hpp src/Boza/pch.cpp
        src/Boza/macros.hpp
        src/Boza/Logger.hpp src/Boza/Logger.cpp
        src/Boza/App.hpp src/Boza/App.cpp
        src/Boza/Instance.hpp src/Boza/Instance.cpp
        src/Boza/Device.hpp src/Boza/Device.cpp
        src/Boza/Buffer.hpp src/Boza/Buffer.cpp
        src/Boza/CommandPool.hpp src/Boza/CommandPool.cpp
        src/Boza/Image3D.hpp src/Boza/Image3D.cpp
        src/Boza/TriangleLoader.hpp src/Boza/TriangleLoader.cpp
        src/Boza/ComputeShader.hpp src/Boza/ComputeShader.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/Boza/pch.hpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${vcpkg_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        tinyobjloader::tinyobjloader
        glm::glm
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wconversion -Werror -pedantic
            -fsanitize=address,undefined
    )
endif ()

target_compile_options(${PROJECT_NAME} PRIVATE
        "$<$<STREQUAL:$<TARGET_PROPERTY:SOURCE_FILE_LANGUAGE>,CXX>:-Wno-error>"
)

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-stringop-overflow")



add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/shaders/compile.sh
        COMMAND ${CMAKE_COMMAND} -E chmod +x ${SHADER_COMPILE_SCRIPT}
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND ${SHADER_COMPILE_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/shaders
        COMMENT "Running shader compilation script..."
        VERBATIM
)