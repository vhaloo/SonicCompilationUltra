cmake_minimum_required(VERSION 3.20)
project(GeneratedCompositorChecks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_library(reference_disassembler STATIC ${ROOT}/vendor/musashi-disassembler/m68kdasm.c)
target_compile_definitions(reference_disassembler PRIVATE m68k_disassemble=scu_disassemble m68k_disassemble_raw=scu_disassemble_raw m68k_is_valid_instruction=scu_decode_valid)
add_library(reference_zlib STATIC
    ${ROOT}/vendor/zlib/adler32.c ${ROOT}/vendor/zlib/crc32.c ${ROOT}/vendor/zlib/inffast.c
    ${ROOT}/vendor/zlib/inflate.c ${ROOT}/vendor/zlib/inftrees.c ${ROOT}/vendor/zlib/zutil.c
    ${ROOT}/vendor/zlib/compress.c ${ROOT}/vendor/zlib/deflate.c ${ROOT}/vendor/zlib/trees.c
    ${ROOT}/vendor/zlib/uncompr.c)
add_executable(generated_compositor_checks generated.cpp
    ${ROOT}/src/world.cpp ${ROOT}/src/terrain_atlas.cpp ${ROOT}/src/enemies.cpp
    ${ROOT}/src/spawn_catalogue.cpp ${ROOT}/src/rom_graphics.cpp ${ROOT}/src/rom_code.cpp
    ${ROOT}/src/rom.cpp ${ROOT}/src/performance.cpp)
target_include_directories(generated_compositor_checks PRIVATE ${ROOT}/src ${ROOT}/vendor/zlib)
target_link_libraries(generated_compositor_checks PRIVATE reference_zlib reference_disassembler)
if(MSVC)
    target_compile_options(generated_compositor_checks PRIVATE /utf-8)
endif()
enable_testing()
add_test(NAME generated_compositor_equivalence COMMAND generated_compositor_checks)
set_tests_properties(generated_compositor_equivalence PROPERTIES TIMEOUT 180)
