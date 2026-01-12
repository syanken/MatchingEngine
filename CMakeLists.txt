# CMake 最低版本要求
cmake_minimum_required(VERSION 3.14)

# 项目名称和语言
project(MatchingEngine LANGUAGES CXX)
include_directories(${PROJECT_SOURCE_DIR})
# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项（开启警告、优化）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -Wall -Wextra)
else()
    add_compile_options(-O2 -DNDEBUG)
endif()

# 查找依赖（spdlog 用 FetchContent 自动下载）
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    network/TcpServer.cpp
    network/Connection.cpp
    protocol/MessageCodec.cpp
    protocol/MessageType.h
    core/Order.cpp
    core/OrderBook.cpp
    core/MatchingEngine.cpp
    utils/Logger.h
)

# 链接 spdlog
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)