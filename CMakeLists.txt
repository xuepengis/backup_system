cmake_minimum_required(VERSION 3.16)

project(backup_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(backup_cli
    main.cpp
    src/core/backup_engine.cpp
    src/strategy/archive_strategy.cpp
    src/utils/logger.cpp
    src/utils/metadata_utils.cpp
    src/utils/path_utils.cpp
)

target_include_directories(backup_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
