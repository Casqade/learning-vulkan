cmake_minimum_required(VERSION 3.8)

set(TARGET vulkan_sandbox)
set(${TARGET}_VERSION 1.0.0)
project(${TARGET} VERSION ${${TARGET}_VERSION} LANGUAGES CXX)


option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

option(GLM_DISABLE_SIMD "Disable SIMD instructions usage in GLM" ON)
option(GLM_FORCE_INLINE "Force GLM functions inlining" OFF)


add_executable(${TARGET})
target_sources(${TARGET} PRIVATE
  src/main.cpp

  src/logger.cpp
)

set_target_properties(${TARGET} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
)

if(${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
  set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
else()
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
    set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)
  endif()
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(GLM_BUILD_LIBRARY ON CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_DISABLE_AUTO_DETECTION ON CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
set(ASSIMP_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
else()
  set(ASSIMP_WARNINGS_AS_ERRORS ON CACHE BOOL "" FORCE)
endif()

set(SPDLOG_NO_ATOMIC_LEVELS ON CACHE BOOL "" FORCE)
set(SPDLOG_DISABLE_DEFAULT_LOGGER ON CACHE BOOL "" FORCE)

set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)


include(FetchContent)

FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG        v1.10.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)


FetchContent_Declare(ctpl
  GIT_REPOSITORY https://github.com/Casqade/CTPL
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ctpl)


FetchContent_Declare(TimeUtils
  GIT_REPOSITORY https://github.com/Casqade/TimeUtils
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(TimeUtils)


FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        3.3.8
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)


FetchContent_Declare(vulkan-headers
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
  GIT_TAG        v1.3.276
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vulkan-headers)


FetchContent_Declare(vulkan-loader
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader
  GIT_TAG        v1.3.276
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vulkan-loader)


FetchContent_Declare(vulkan-utility-libraries
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Utility-Libraries
  GIT_TAG        v1.3.276
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vulkan-utility-libraries)


FetchContent_Declare(vulkan-validation-layers
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-ValidationLayers
  GIT_TAG        v1.3.276
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vulkan-validation-layers)


FetchContent_Declare(spirv-headers
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
  GIT_TAG        vulkan-sdk-1.3.275.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spirv-headers)


FetchContent_Declare(spirv-tools
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
  GIT_TAG        vulkan-sdk-1.3.275.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spirv-tools)


FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG        1.0.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)


FetchContent_Declare(assimp
  GIT_REPOSITORY https://github.com/assimp/assimp
  GIT_TAG        v5.2.5
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(assimp)


target_compile_definitions(glm PUBLIC
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

if(${GLM_DISABLE_SIMD})
  target_compile_definitions(glm PUBLIC
    GLM_FORCE_PURE
  )
else()
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(glm PUBLIC
      -msse4.1
    )
  endif()

  target_compile_definitions(glm PUBLIC
    GLM_FORCE_INTRINSICS
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
  )
endif()

if(${GLM_FORCE_INLINE})
  target_compile_definitions(glm PUBLIC
    GLM_FORCE_INLINE
  )
endif()


target_compile_definitions(${TARGET} PRIVATE
  NOMINMAX
)

target_compile_definitions(glfw INTERFACE
  GLFW_INCLUDE_VULKAN
)


find_package(Threads REQUIRED)

target_link_libraries(${TARGET} PUBLIC
  Vulkan::Headers
  Vulkan::Loader
  Vulkan::LayerSettings
  Vulkan::UtilityHeaders
  VkLayer_utils
  Threads::Threads
  glfw
  ctpl::ctpl
  TimeUtils::TimeUtils
  assimp::assimp
  glm::glm
  spdlog::spdlog
)

include(GNUInstallDirs)

if(CMAKE_BUILD_TYPE MATCHES "Release")

  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#    target_compile_options(${TARGET} PUBLIC -O2)
  endif()

  set_target_properties(${TARGET} PROPERTIES
    WIN32_EXECUTABLE TRUE
  )

else()

  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#    target_compile_options(${TARGET} PUBLIC -Og)
    target_compile_options(${TARGET} PUBLIC -O0)
  endif()

  set_target_properties(${TARGET} PROPERTIES
    WIN32_EXECUTABLE FALSE
  )
endif()

if(NOT ${BUILD_SHARED_LIBS})
  target_link_libraries(${TARGET} PUBLIC -static)

  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(${TARGET} PRIVATE -static-libgcc -static-libstdc++)
  endif()
endif()


target_include_directories(${TARGET} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

