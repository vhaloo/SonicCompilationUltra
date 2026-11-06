cmake_minimum_required(VERSION 3.20)
project(GeneratedRewindChecks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_library(check_zlib STATIC
    ${ROOT}/vendor/zlib/adler32.c ${ROOT}/vendor/zlib/crc32.c ${ROOT}/vendor/zlib/inffast.c
    ${ROOT}/vendor/zlib/inflate.c ${ROOT}/vendor/zlib/inftrees.c ${ROOT}/vendor/zlib/zutil.c
    ${ROOT}/vendor/zlib/compress.c ${ROOT}/vendor/zlib/deflate.c ${ROOT}/vendor/zlib/trees.c
    ${ROOT}/vendor/zlib/uncompr.c)
add_executable(rewind_checks checks.cpp ${ROOT}/src/rewind.cpp)
target_include_directories(rewind_checks PRIVATE ${ROOT}/src ${ROOT}/vendor/zlib)
target_link_libraries(rewind_checks PRIVATE check_zlib)
enable_testing()
add_test(NAME generated_rewind_bytes_audio_limits COMMAND rewind_checks)
