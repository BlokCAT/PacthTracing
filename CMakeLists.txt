cmake_minimum_required(VERSION 3.21)
project(PathTracing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 源文件 ----
add_executable(PathTracing
    PathTracing/main.cpp
    PathTracing/Renderer.cpp
    PathTracing/Scene.cpp
    PathTracing/BVHStruct.cpp
)

# ---- 头文件路径 ----
target_include_directories(PathTracing PRIVATE
    PathTracing          # 项目自身头文件
    include              # OpenCV 头文件 (opencv2/)
)

# ---- OpenCV 4.10.0 (vendored) ----
add_library(opencv_world SHARED IMPORTED)
set_target_properties(opencv_world PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
    IMPORTED_IMPLIB_RELEASE       "${CMAKE_SOURCE_DIR}/lib/opencv_world4100.lib"
    IMPORTED_LOCATION_RELEASE     "${CMAKE_SOURCE_DIR}/lib/opencv_world4100.dll"
    IMPORTED_IMPLIB_DEBUG         "${CMAKE_SOURCE_DIR}/lib/opencv_world4100d.lib"
    IMPORTED_LOCATION_DEBUG       "${CMAKE_SOURCE_DIR}/lib/opencv_world4100d.dll"
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    MAP_IMPORTED_CONFIG_MINSIZEREL     Release
)

# ---- OpenMP ----
find_package(OpenMP REQUIRED)

# ---- 链接 ----
target_link_libraries(PathTracing PRIVATE
    opencv_world
    OpenMP::OpenMP_CXX
)

# ---- MSVC 设置 ----
if(MSVC)
    set_target_properties(PathTracing PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"   # /MD /MDd
    )
    target_compile_options(PathTracing PRIVATE /utf-8)   # 源码为 UTF-8 编码
endif()

# Console 子系统 + Unicode
set_target_properties(PathTracing PROPERTIES WIN32_EXECUTABLE OFF)
target_compile_definitions(PathTracing PRIVATE
    UNICODE
    _UNICODE
    _CONSOLE
    $<$<CONFIG:Release>:NDEBUG>
)

# ---- VS 调试器工作目录：PathTracing/（源码中 ../model/ 路径的要求）----
set_target_properties(PathTracing PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/PathTracing"
)

# ---- 构建后复制 OpenCV DLL 到 exe 旁边（如果有的话）----
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/opencv_world4100.dll")
    add_custom_command(TARGET PathTracing POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/lib/opencv_world4100d.dll,${CMAKE_SOURCE_DIR}/lib/opencv_world4100.dll>"
            $<TARGET_FILE_DIR:PathTracing>
    )
endif()
