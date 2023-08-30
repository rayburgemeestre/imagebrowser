cmake_minimum_required(VERSION 3.10.2)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 20)

project(imagebrowser VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CLANGFORMAT_EXECUTABLE clang-format)

include(ClangFormat)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_link_options(-fuse-ld=gold)
    add_compile_options(-stdlib=libstdc++)
else()
    add_link_options(-Wl,-z,notext)
endif()

add_compile_options(-fno-omit-frame-pointer -mssse3)

# allegro4 has a bunch of deprecated stuff that we don't care about
add_compile_options(-Wno-deprecated-declarations)

if (SANITIZER)
    add_compile_options(-fsanitize=address)
endif()

#if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O0 -g)
#else()
#    add_compile_options(-O2)
#endif()

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/crow/include"
)

link_directories("/usr/lib/llvm-10/lib/clang/10.0.0/lib/linux/")
link_directories("/usr/lib/llvm-12/lib/clang/12.0.0/lib/linux/")

# imagebrowser

file(GLOB_RECURSE imagebrowser_SOURCES "src/**.h" "src/**.cpp")

add_executable(imagebrowser ${imagebrowser_SOURCES})
target_link_libraries(imagebrowser PRIVATE -lpthread)
#target_link_libraries(imagebrowser PRIVATE -lalleg)
#target_link_libraries(imagebrowser PRIVATE -lalfont)
#target_link_libraries(imagebrowser PRIVATE -lfreetype)
#target_link_libraries(imagebrowser PRIVATE -lm)

if (SANITIZER)
    target_link_libraries(imagebrowser PRIVATE -lclang_rt.asan-x86_64)
    # target_link_libraries(mapmaker PRIVATE -lclang_rt.asan-x86_64)
    # target_link_libraries(server PRIVATE -lclang_rt.asan-x86_64)
endif()

install (TARGETS imagebrowser DESTINATION bin)

set(all_sources ${imagebrowser_SOURCES})

list(REMOVE_DUPLICATES all_sources)

clangformat_setup(${all_sources})
