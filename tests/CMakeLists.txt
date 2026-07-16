# ---- Catch2 v3 ----
# Prefer system package:  sudo apt install catch2
# Falls back to FetchContent if not found.
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
    )
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(Catch2)
endif()

# ---- Test executable ----
set(TEST_SOURCES
    test_compression.cpp
    test_encryption.cpp
    test_filters.cpp
    test_path_utils.cpp
    test_archive.cpp
    test_registry.cpp
    test_stream_processor.cpp
    test_filter_spec.cpp
    test_cli_parser.cpp
    test_integration.cpp
    test_metadata.cpp
)

add_executable(backup_tests
    ${TEST_SOURCES}
)

target_link_libraries(backup_tests
    PRIVATE
        backup_sources
        OpenSSL::Crypto
        Catch2::Catch2WithMain
)

target_include_directories(backup_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# ---- Register with CTest ----
add_test(NAME backup_tests COMMAND backup_tests)
